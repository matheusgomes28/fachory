find_package(fmt REQUIRED)
find_package(cxxopts REQUIRED)
find_package(spdlog REQUIRED)
find_package(Microsoft.GSL REQUIRED)

add_executable(printer)
target_sources(printer
  PRIVATE main.cpp printer_manager.cpp
  PUBLIC printer_manager.hpp)

target_link_libraries(printer PRIVATE fmt::fmt spdlog::spdlog Microsoft.GSL::GSL cups)
