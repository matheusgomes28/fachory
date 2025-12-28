find_package(fmt REQUIRED)
find_package(cxxopts REQUIRED)
find_package(spdlog REQUIRED)
find_package(Microsoft.GSL REQUIRED)

add_library(fachory_printer)
target_sources(fachory_printer
  PRIVATE printer_manager.cpp
  PUBLIC include/printer/printer_manager.hpp)

target_include_directories(fachory_printer PUBLIC include)

target_link_libraries(fachory_printer PRIVATE fmt::fmt spdlog::spdlog Microsoft.GSL::GSL cups)

add_library(fachory::printer ALIAS fachory_printer)

