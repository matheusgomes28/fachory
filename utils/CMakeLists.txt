add_library(utils)

target_sources(utils PRIVATE string.cpp PUBLIC include/utils/string.hpp)
target_include_directories(utils PUBLIC include)

add_library(fachory::utils ALIAS utils)

