cmake_minimum_required(VERSION 3.21)
project(MyNeighborTodo LANGUAGES CXX)
include(CTest)

add_subdirectory(printer)
add_subdirectory(database)
add_subdirectory(fachory)
add_subdirectory(rest)
add_subdirectory(receipt)
add_subdirectory(utils)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

