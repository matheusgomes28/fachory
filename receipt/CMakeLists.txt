find_package(podofo REQUIRED)

add_library(receipt)
target_sources(receipt PRIVATE receipt.cpp PUBLIC include/receipt/receipt.hpp)

target_include_directories(receipt PUBLIC include)
target_link_libraries(receipt PRIVATE podofo::podofo)
add_library(fachory::receipt ALIAS receipt)
