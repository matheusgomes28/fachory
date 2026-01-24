find_package(Crow REQUIRED)
find_package(Microsoft.GSL REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

add_library(fachory_rest)
target_sources(fachory_rest PRIVATE webserver.cpp PUBLIC include/rest/webserver.hpp)
target_include_directories(fachory_rest PUBLIC include)

target_link_libraries(fachory_rest PRIVATE Crow::Crow spdlog::spdlog fmt::fmt Microsoft.GSL::GSL)

add_library(fachory::rest ALIAS fachory_rest)

