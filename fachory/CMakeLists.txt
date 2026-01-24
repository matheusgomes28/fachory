find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

add_executable(fachory)
target_sources(fachory PRIVATE main.cpp http_routes.cpp)

target_link_libraries(fachory PRIVATE
  fachory::database
  fachory::printer
  fachory::receipt
  fachory::rest
  fachory::utils
  fmt::fmt
  spdlog::spdlog
  nlohmann_json::nlohmann_json
)

target_include_directories(fachory PUBLIC include)
