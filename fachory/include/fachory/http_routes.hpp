#ifndef FACHORY_HTTP_ROUTES_H
#define FACHORY_HTTP_ROUTES_H

#include <fachory/ts_queue.hpp>
#include <fachory/types.hpp>

#include <database/database.hpp>
#include <rest/webserver.hpp>

namespace fachory::app::http_routes {

    void setup_routes(
        fachory::rest::Webserver& webserver, ThreadSafeQueue<std::vector<PrintItem>>& queue,
        fachory::db::Database& database);
} // namespace fachory::app::http_routes
#endif //FACHORY_HTTP_ROUTES_H
