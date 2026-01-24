#include <rest/webserver.hpp>

#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <gsl/assert>

#include <cstdint>
#include <functional>

namespace {
    template <typename... Args>
    void add_route_helper(
        crow::Crow<>& app, std::string const& path, fachory::rest::Method method,
        std::function<fachory::rest::Response(fachory::rest::Request, Args...)> handler) {

        auto& route = app.route_dynamic(path);

        // This is badically here to wrap the crow responses
        auto const handler_wrapper = [handler](crow::request const& request, Args... args) {
            auto const response = handler(fachory::rest::Request{.body = request.body}, std::forward<Args>(args)...);

            crow::response crow_resp;

            for (auto const& [k, v] : response.header) {
                crow_resp.add_header(k, v);
            }

            crow_resp.body = response.body;
            crow_resp.code = response.code;
            return crow_resp;
        };

        switch (method) {
        case fachory::rest::Method::GET:
            route.methods(crow::HTTPMethod::GET)(handler_wrapper);
            break;
        case fachory::rest::Method::POST:
            route.methods(crow::HTTPMethod::POST)(handler_wrapper);
            break;
        case fachory::rest::Method::PUT:
            route.methods(crow::HTTPMethod::PUT)(handler_wrapper);
        }
    }
} // namespace

namespace fachory::rest {

    Webserver::Webserver(std::uint16_t port)
        : _crow_app{std::make_unique<crow::SimpleApp>()}, _port{port} {}

    Webserver::~Webserver() {}

    void Webserver::start() {
        Expects(_crow_app);

        // compile-time routes
        CROW_ROUTE((*_crow_app), "/").methods("GET"_method)([]() { return "healthy"; });

        _crow_app->port(_port).multithreaded().run();
    }

    void Webserver::add_route(std::string const& path, Method method, std::function<Response(Request)> handler) {
        add_route_helper(*_crow_app, path, method, handler);
    }

    void Webserver::add_route(
        std::string const& path, Method method, std::function<Response(Request, std::string)> handler) {
        add_route_helper(*_crow_app, path, method, handler);
    }
} // namespace fachory::rest
