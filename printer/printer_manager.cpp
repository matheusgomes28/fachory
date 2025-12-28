#include <printer/printer_manager.hpp>

#include <cups/cups.h>
#include <cups/http.h>
#include <fmt/format.h>
#include <gsl/assert>
#include <iterator>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>

namespace {


    int printer_register_cp(void* user_data, unsigned flags, cups_dest_t* dest) {
        if (!dest) {
            return 1;
        }

        auto manager = static_cast<PrinterManager*>(user_data);

        if (flags & CUPS_DEST_FLAGS_REMOVED) {
            spdlog::info("deregistering printer {}", dest->name);
            manager->remove_printer(dest->name, dest);
            return 1;
        }

        spdlog::info("registering printer {}", dest->name);
        manager->add_printer(dest->name, dest);
        // spdlog::info("calling it on dest item{}", dest->name);
        return 1;
    }

    PrinterOptions make_default_options() {
        int job_id             = 0;
        int num_options        = 0;
        cups_option_t* options = NULL;

        // Gets all options (thermal printer related)
        num_options = cupsAddOption(CUPS_MEDIA, CUPS_MEDIA_LETTER, num_options, &options);
        num_options = cupsAddOption(CUPS_SIDES, CUPS_SIDES_ONE_SIDED, num_options, &options);
        num_options = cupsAddOption("media", "Roll80mm", num_options, &options);
        num_options = cupsAddOption("orientation-requested", "3", num_options, &options);

        auto const options_size = std::make_shared<int>(num_options);
        PrinterOptionBuffer options_buffer{
         options, [&options_size](auto* ptr) { cupsFreeOptions(*options_size, ptr); }};

        return {std::move(options_buffer), options_size};
    }

    void reset_printer(cups_dest_t* dest, cups_dinfo_t* info, int job_id, PrinterOptions const& options) {
        const char init_sequence[] = "\x1B\x40"; // Reset
        auto const init_doc        = cupsStartDestDocument(
            CUPS_HTTP_DEFAULT, dest, info, job_id, "init", CUPS_FORMAT_RAW, *options.second, options.first.get(), 0);

        if (init_doc == HTTP_STATUS_CONTINUE) {
            cupsWriteRequestData(CUPS_HTTP_DEFAULT, init_sequence, sizeof(init_sequence) - 1);
            cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, info);
        }
    }

    std::optional<std::vector<char>> read_file_contents(std::string const& file_name) {
        std::ifstream file_stream{file_name};
        std::vector<char> const all_contents{std::istreambuf_iterator<char>{file_stream}, {}};
        return std::make_optional(std::move(all_contents));
    }

    bool send_blob_to_printer(std::vector<char> const& blob) {
        auto const write_res = cupsWriteRequestData(CUPS_HTTP_DEFAULT, blob.data(), blob.size());
        if (write_res != HTTP_STATUS_CONTINUE) {
            return false;
        }

        return true;
    }

    struct TempFile {
        TempFile(std::string filename)
            : filename{filename} {}

        ~TempFile() {
            std::filesystem::remove(filename);
        }

        std::string filename;
    };

    TempFile create_temp_file(std::string const& contents) {
        namespace fs = std::filesystem;

        fs::path temp_dir  = fs::temp_directory_path();
        fs::path temp_file = temp_dir / (std::tmpnam(nullptr));

        std::ofstream ofs(temp_file);
        ofs << contents;
        ofs.close();

        return TempFile{temp_file};
    }
} // namespace

PrinterJob::PrinterJob(std::string const& printer_name, cups_dest_t* dest, cups_dinfo_t* info,
    std::string const& job_name, PrinterOptions const& options)
    : job_id{0}, printer_name{printer_name}, _cancelled(false), _cups_dest{dest}, _cups_info{info} {


    auto const job_res = cupsCreateDestJob(
        CUPS_HTTP_DEFAULT, _cups_dest, info, &job_id, job_name.c_str(), *options.second, options.first.get());

    if (job_res != IPP_STATUS_OK) {
        spdlog::error("unable to create job for printer {}: {}", printer_name, cupsLastErrorString());
        job_id = 0;
    }
}

PrinterJob::~PrinterJob() {
    if (_cancelled) {
        cupsCancelJob2(CUPS_HTTP_DEFAULT, printer_name.c_str(), job_id, 0);
        return;
    }

    if (cupsFinishDestDocument(CUPS_HTTP_DEFAULT, _cups_dest, _cups_info) == IPP_STATUS_OK) {
        spdlog::info("job succeeded for printer {}", printer_name);
    } else {
        spdlog::error("job failed for printer {}", printer_name);
    }
}

void PrinterJob::cancel() {
    _cancelled = true;
}

PrinterManager::PrinterManager()
    : _printer_details{}, _cups_dests_indices{}, _cups_dests_array{nullptr}, _cups_num_dests(0) {
    poll_destinations();
}

PrinterManager::~PrinterManager() {
    _printer_details.clear();
    cupsFreeDests(_cups_num_dests, _cups_dests_array);

    for (auto [name, info] : _infos) {
        cupsFreeDestInfo(info);
    }
}

void PrinterManager::poll_destinations() {
    cupsEnumDests(CUPS_DEST_FLAGS_NONE, 1000, NULL, 0, 0, printer_register_cp, this);

    std::map<std::string, int> new_map_indices{};
    for (int i = 0; i < _cups_num_dests; ++i) {
        auto dest                  = _cups_dests_array[i];
        new_map_indices[dest.name] = i;
    }
    _cups_dests_indices = std::move(new_map_indices);
}

void PrinterManager::add_printer(std::string const& name, cups_dest_t* dest) {
    if (!dest) {
        spdlog::error("invalid destination for added printer, skipping");
        return;
    }

    int const prev_num_dests = _cups_num_dests;

    _cups_num_dests = cupsCopyDest(dest, _cups_num_dests, &_cups_dests_array);

    if (prev_num_dests == _cups_num_dests) {
        spdlog::error("destination was not added by cups");
        return;
    }

    _infos[name] = cupsCopyDestInfo(CUPS_HTTP_DEFAULT, dest);

    _cups_dests_indices[name] = _cups_num_dests - 1;

    _printer_details[name] = PrinterDetails{.name = dest->name,
     .instance                                    = dest->instance != nullptr ? dest->instance : "",
     .is_default                                  = static_cast<bool>(dest->is_default),
     .options                                     = {}};
}

void PrinterManager::remove_printer(std::string const& name, cups_dest_t* dest) {
    if (!dest) {
        spdlog::error("invalid destination for removed printer, skippint");
        return;
    }

    auto const found = _printer_details.find(name);
    if (found != end(_printer_details)) {
        _printer_details.erase(found);
    }

    _cups_num_dests = cupsRemoveDest(dest->name, dest->instance, _cups_num_dests, &_cups_dests_array);
    _printer_details.erase(dest->name);
    _infos.erase(dest->name);
}

std::optional<std::pair<cups_dest_t*, cups_dinfo_t*>> PrinterManager::query_printer(std::string const& printer_name) {

    auto const dest_index_found = _cups_dests_indices.find(printer_name);
    if (dest_index_found == end(_cups_dests_indices)) {
        spdlog::error("printer {} is not registered", printer_name);
        return std::nullopt;
    }

    auto const dest = _cups_dests_array + dest_index_found->second;
    Expects(dest);

    // Find the printer info
    auto const info_found = _infos.find(printer_name);
    if (info_found == end(_infos)) {
        spdlog::error("could not find info for printer {}", printer_name);
        return std::nullopt;
    }

    return std::make_optional<std::pair<cups_dest_t*, cups_dinfo_t*>>(dest, info_found->second);
}

std::optional<PrinterJob> PrinterManager::create_printer_job(
    std::string const& printer_name, std::string const& job_name, PrinterOptions const& options) {
    auto const maybe_printer = query_printer(printer_name);
    if (!maybe_printer) {
        return std::nullopt;
    }
    auto [dest, info] = *maybe_printer;

    int job_id = 0;

    auto const job_res =
        cupsCreateDestJob(CUPS_HTTP_DEFAULT, dest, info, &job_id, "My Job", *options.second, options.first.get());

    if (job_res != IPP_STATUS_OK) {
        return std::nullopt;
    }

    return std::make_optional<PrinterJob>(printer_name, dest, info, job_name.c_str(), options);
}

bool PrinterManager::print_file(
    std::string const& printer_name, std::string const& file_path, std::string const& format) {

    auto const maybe_file_contents = read_file_contents(file_path);
    if (!maybe_file_contents) {
        spdlog::error("could not open file {} for printing", file_path);
        return false;
    }

    auto const maybe_printer = query_printer(printer_name);
    if (!maybe_printer) {
        return false;
    }
    auto [dest, info]  = *maybe_printer;
    auto const options = make_default_options();

    auto maybe_job = create_printer_job(printer_name, "My Job", options);
    if (!maybe_job) {
        spdlog::error("could not create job for printer {}: {}", printer_name, cupsLastErrorString());
        return false;
    }

    reset_printer(dest, info, maybe_job->job_id, options);

    auto const start_doc_res = cupsStartDestDocument(CUPS_HTTP_DEFAULT, dest, info, maybe_job->job_id,
        file_path.c_str(), format.c_str(), *options.second, options.first.get(), 1);

    if (HTTP_STATUS_CONTINUE != start_doc_res) {
        spdlog::error("unable to start the document for printer {}: {}", printer_name, cupsLastErrorString());
        maybe_job->cancel();
        return false;
    }

    if (!send_blob_to_printer(*maybe_file_contents)) {
        spdlog::error("could not write blob to printer: {}", cupsLastErrorString());
        return false;
    }

    return true;
}

bool PrinterManager::print_pdf(std::string const& printer_name, std::string const& pdf_path) {
    if (print_file(printer_name, pdf_path, CUPS_FORMAT_PDF)) {
        spdlog::error("failed to print pdf file {}", pdf_path);
        return false;
    }

    return true;
}

bool PrinterManager::print_jpeg(std::string const& printer_name, std::string const& image_path) {
    if (print_file(printer_name, image_path, CUPS_FORMAT_JPEG)) {
        spdlog::error("failed to print jpeg file {}", image_path);
        return false;
    }

    return true;
}

bool PrinterManager::print_text(std::string const& printer_name, std::string const& text) {

    auto const file = create_temp_file(text);

    if (print_file(printer_name, file.filename, CUPS_FORMAT_RAW)) {
        spdlog::error("failed to print text");
        return false;
    }

    return true;
}


std::vector<std::string> PrinterManager::printers() const {
    std::vector<std::string> all_printers;
    for (auto [name, details] : _printer_details) {
        spdlog::info("calling it on map item ({}, {})", name, details.instance);
        all_printers.push_back(name);
    }

    return all_printers;
}
