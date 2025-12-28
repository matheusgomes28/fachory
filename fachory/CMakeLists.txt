find_package(fmt REQUIRED)

add_executable(fachory)
target_sources(fachory PRIVATE main.cpp)

target_link_libraries(fachory PRIVATE fachory::database fachory::printer fmt::fmt)
