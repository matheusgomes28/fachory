#include <fachory/http_routes.hpp>
#include <fachory/ts_queue.hpp>
#include <fachory/types.hpp>

#include <database/database.hpp>
#include <printer/printer_manager.hpp>
#include <receipt/receipt.hpp>
#include <rest/webserver.hpp>
#include <utils/string.hpp>

#include <fmt/format.h>
#include <netinet/in_systm.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

namespace {
    template <class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    void print_image(PrinterManager& manager, std::string const& printer_name, std::string const& filename) {
        if (!manager.print_jpeg(printer_name, filename)) {
            spdlog::error("could not print jpeg {}", filename);
        }
        spdlog::info("printer jpeg {}", filename);
    }

    void print_pdf(PrinterManager& manager, std::string const& printer_name, std::string const& filename) {
        if (!manager.print_pdf(printer_name, filename)) {
            spdlog::error("could not print pdf {}", filename);
        }
        spdlog::info("printed pdf {}", filename);
    }

    void print_text(PrinterManager& manager, std::string const& printer_name, std::string const& text) {
        if (!manager.print_text(printer_name, text)) {
            spdlog::error("could not print text");
        }
        spdlog::info("printed text");
    }

    void queue_processor(
        fachory::app::ThreadSafeQueue<std::vector<fachory::app::PrintItem>>& queue,
        std::chrono::milliseconds sleep_duration) {

        using namespace std::chrono_literals;
        PrinterManager manager{};


        while (true) {
            auto const v = queue.pop_front();

            if (v) {
                for (auto const print_job : *v) {
                    // clang-format off
                    std::visit(overloaded{
                        [&manager](fachory::app::ImagePrint const& p) { print_image(manager, "terow", p.filename); },
                        [&manager](fachory::app::PdfPrint const& p) { print_pdf(manager, "terow", p.filename); },
                        [&manager](fachory::app::TextPrint const& p) { print_text(manager, "terow", p.text); },
                        [&manager](std::monostate) {}
                    },
                      print_job);
                    // clang-format on
                }
            }

            if (sleep_duration > 0ms) {
                std::this_thread::sleep_for(sleep_duration);
            }
        }
    }

    // Maybe the combination of the queue, database, webserver should be global state / context
    // PUT    /tasks/<task_id>    # Update entire task (full replacement)
    // PATCH  /tasks/<task_id>    # Partial update (modify specific fields)
    // DELETE /tasks/<task_id>    # Delete a task
} // namespace

int main(int arc, char** arv) {

    fachory::app::ThreadSafeQueue<std::vector<fachory::app::PrintItem>> queue;

    // Create Database
    //
    //
    // Try catch should be there for the all main things that may throw:
    //
    // - Database connections
    // - Printer failures
    // - Webserver failures
    std::string const fonts = "Roboto, Arial";
    fachory::receipt::PdfConfig const pdf_config{
     .preferred_fonts = fachory::utils::string::split(fonts, ','),
    };


    fachory::receipt::Receipt receipt{pdf_config};
    receipt.add_item("Title", "Hello World", 1);
    receipt.to_pdf("pdf_test_arial.pdf");

    try {
        fachory::db::Database database{"test.db", "password"};
        std::jthread queue_consumer{queue_processor, std::ref(queue), std::chrono::milliseconds{100}};
        fachory::rest::Webserver webserver{8080};
        fachory::app::http_routes::setup_routes(webserver, queue, database);
        webserver.start();
    } catch (fachory::db::DatabaseException const& e) {
        spdlog::error("could not connect to datbaase: {}", e.what());
        return -1;
    }


    return 0;
}
