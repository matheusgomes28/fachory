#include <fachory/ts_queue.hpp>
#include <fachory/types.hpp>

#include <database/database.hpp>
#include <rest/webserver.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace fachory::app::http_routes {

    void setup_routes(
        fachory::rest::Webserver& webserver, ThreadSafeQueue<std::vector<PrintItem>>& queue,
        fachory::db::Database& database) {


        webserver.add_route("/daily", fachory::rest::Method::GET, [&queue, &database](fachory::rest::Request) {
            auto const tasks = database.pending_tasks();


            std::stringstream text_builder;
            for (auto const& task : tasks) {
                text_builder << fmt::format("- [ ] {}\n", task.name);
                text_builder << fmt::format("      {}", task.description);
            }

            std::vector<PrintItem> const print_items = {{
             PdfPrint{.filename = "./memes/cat.pdf"},
             TextPrint{.text = text_builder.str()},
            }};

            queue.push_back(print_items);
            return fachory::rest::Response{.code = 200, .header = {{"Content-Type", "application/json"}}, .body = "[]"};
        });

        webserver.add_route("/text", fachory::rest::Method::GET, [&queue](fachory::rest::Request) {
            queue.push_back({TextPrint{.text = "Hello, World!"}});
            return fachory::rest::Response{.code = 200, .header = {{"Content-Type", "application/json"}}, .body = "[]"};
        });

        webserver.add_route("/pdf", fachory::rest::Method::GET, [&queue](fachory::rest::Request) {
            queue.push_back({PdfPrint{.filename = "./memes/cat.pdf"}});
            return fachory::rest::Response{.code = 200, .header = {{"Content-Type", "application/json"}}, .body = "[]"};
        });

        webserver.add_route("/image", fachory::rest::Method::GET, [&queue](fachory::rest::Request) {
            queue.push_back({ImagePrint{.filename = "./memes/noice.jpg"}});
            return fachory::rest::Response{.code = 200, .header = {{"Content-Type", "application/json"}}, .body = "[]"};
        });


        // GET    /tasks              # List all tasks (with pagination/filtering)
        webserver.add_route("/tasks", fachory::rest::Method::GET, [&database](fachory::rest::Request) {
            auto const tasks = database.pending_tasks();
            nlohmann::json resp_json{{"tasks", tasks}};
            return fachory::rest::Response{
             .code   = 200,
             .header = {{"Content-Type", "application/json"}},
             .body   = resp_json.dump(),
            };
        });

        // GET    /tasks/<task_id>    # Get a specific task
        webserver.add_route(
            "/tasks/<string>", fachory::rest::Method::GET, [&database](fachory::rest::Request, std::string task_id) {
                auto const maybe_task = database.pending_task(task_id);

                if (!maybe_task) {
                    return fachory::rest::Response{
                     .code   = 404,
                     .header = {{"Content-Type", "application/json"}},
                     .body   = R"({"error": "task not found"})",
                    };
                }

                // PASSS
                nlohmann::json output{*maybe_task};
                return fachory::rest::Response{
                 .code   = 200,
                 .header = {{"Content-Type", "application/json"}},
                 .body   = output.dump(),
                };
            });

        // POST   /tasks              # Create a new task
        webserver.add_route("/tasks", fachory::rest::Method::POST, [&database](fachory::rest::Request request) {
            nlohmann::json body = nlohmann::json::parse(request.body, nullptr, false);

            if (body.is_discarded()) {
                spdlog::error("failed to parse body");

                return fachory::rest::Response{
                 .code   = 400,
                 .header = {{"Content-Type", "application/json"}},
                 .body   = R"({"error": "invalid body json"})",
                };
            }

            auto const respond_error = [](std::uint16_t code, std::string const& error_msg) {
                return fachory::rest::Response{
                 .code   = code,
                 .header = {{"Content-Type", "application/json"}},
                 .body   = fmt::format(R"({{"error_msg": "{}"}})", error_msg),
                };
            };

            if (!body.contains("name")) {
                spdlog::error("body does not contain required field 'name'");
                return respond_error(400, "body does not contain required field 'name'");
            }

            if (!body.contains("description")) {
                spdlog::error("body does not contain required field 'description'");
                return respond_error(400, "body does not contain required field 'description'");
            }

            std::string name;
            body["name"].get_to<std::string>(name);
            std::string description;
            body["description"].get_to<std::string>(description);

            auto const maybe_id = database.add_task(name, description);

            if (!maybe_id) {
                return respond_error(500, "could not add task");
            }

            return fachory::rest::Response{
             .code   = 200,
             .header = {{"Content-Type", "application/json"}},
             .body   = fmt::format(R"({{"task_id": "{}"}})", *maybe_id),
            };
        });
    }
} // namespace fachory::app::http_routes
