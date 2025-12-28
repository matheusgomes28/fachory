find_package(SQLiteCpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_library(factory_database)
target_sources(factory_database PRIVATE database.cpp)

target_include_directories(factory_database PUBLIC include)
target_link_libraries(factory_database PRIVATE SQLiteCpp spdlog::spdlog fmt::fmt)

add_library(fachory::database ALIAS factory_database)

