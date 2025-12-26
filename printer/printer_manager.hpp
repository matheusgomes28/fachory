#ifndef PRINTER_MANAGER_H
#define PRINTER_MANAGER_H

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

extern "C" {
typedef struct cups_dest_s cups_dest_t;
typedef struct cups_option_s cups_option_t;
typedef struct _cups_dinfo_s cups_dinfo_t;
}

struct PrinterDetails {
    std::string name;
    std::string instance;
    bool is_default;
    std::map<std::string, std::string> options;
};


using PrinterOptionBuffer = std::unique_ptr<cups_option_t, std::function<void(cups_option_t*)>>;
using PrinterOptions      = std::pair<PrinterOptionBuffer, std::shared_ptr<int>>;

struct PrinterJob {
    int job_id;
    std::string printer_name;

    PrinterJob(std::string const& printer_name, cups_dest_t* dest, cups_dinfo_t* info, std::string const& job_name,
        PrinterOptions const& options);
    ~PrinterJob();

    void cancel();

private:
    bool _cancelled;
    cups_dest_t* _cups_dest;
    cups_dinfo_t* _cups_info;
};


// TODO : What does this class actually do?
// TODO : do we want to rename it?
class PrinterManager {
public:
    PrinterManager();
    ~PrinterManager();

    void add_printer(std::string const& name, cups_dest_s* instance);
    void remove_printer(std::string const& name, cups_dest_s* instance);

    [[nodiscard]] std::vector<std::string> printers() const;

    [[nodiscard]] bool print_text(std::string const& printer_name, std::string const& text);
    [[nodiscard]] bool print_pdf(std::string const& printer_name, std::string const& pdf_path);
    [[nodiscard]] bool print_jpeg(std::string const& printer_name, std::string const& image_path);

    // bool printer_info(std::string const& name) const;

private:
    std::map<std::string, PrinterDetails> _printer_details;
    std::map<std::string, cups_dinfo_t*> _infos;

    std::map<std::string, int> _cups_dests_indices;
    cups_dest_t* _cups_dests_array;
    int _cups_num_dests;

    void poll_destinations();
    [[nodiscard]] std::optional<std::pair<cups_dest_t*, cups_dinfo_t*>> query_printer(std::string const& printer_name);
    [[nodiscard]] std::optional<PrinterJob> create_printer_job(
        std::string const& printer_name, std::string const& job_name, PrinterOptions const& options);

    [[nodiscard]] bool print_file(
        std::string const& printer_name, std::string const& file_path, std::string const& format);
};


#endif // PRINTER_MANAGER_H
