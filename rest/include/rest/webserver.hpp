#ifndef REST_WEBSERVER_H
#define REST_WEBSERVER_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace crow {
    template <typename... Middlewares>
    class Crow;
};

namespace fachory::rest {

    enum class Method {
        GET,
        POST,
        PUT,
    };

    struct Response {
        std::uint16_t code;
        std::map<std::string, std::string> header;
        std::string body;
    };

    struct Request {
        std::string body;
    };

    class Webserver {
    public:
        explicit Webserver(std::uint16_t port);
        ~Webserver();

        void start();

        void add_route(std::string const& path, Method method, std::function<Response(Request)> handler);

        // TODO : Can we make these templateable in the future?
        void add_route(std::string const& path, Method method, std::function<Response(Request, std::string)> handler);


    private:
        std::unique_ptr<crow::Crow<>> _crow_app;
        std::uint16_t _port;
    };

} // namespace fachory::rest
#endif // REST_WEBSERVER_H
