include(GoogleTest)
find_package(GTest REQUIRED)

function(create_tests)
  set(oneValueArgs FILENAME TESTNAME)
  set(multiValueArgs LINK_LIBRARIES)
  cmake_parse_arguments(PARSE_ARGV 0 arg "" "${oneValueArgs}" "${multiValueArgs}")


  if(NOT DEFINED arg_FILENAME)
    message(FATAL_ERROR "FILENAME was not given")
  endif()

  if(NOT DEFINED arg_TESTNAME)
    message(FATAL_ERROR "TESTNAME was not given")
  endif()

  # create the test
  set(TEST_TARGET_NAME "${arg_TESTNAME}")
  add_executable("${TEST_TARGET_NAME}")
  target_sources("${TEST_TARGET_NAME}" PRIVATE ${arg_FILENAME})
  target_link_libraries(${TEST_TARGET_NAME} PRIVATE GTest::gtest_main GTest::gtest)

  if(DEFINED arg_LINK_LIBRARIES)
    target_link_libraries("${TEST_TARGET_NAME}" PRIVATE ${arg_LINK_LIBRARIES})
  endif()

  gtest_add_tests(TARGET "${TEST_TARGET_NAME}")
  gtest_discover_tests("${TEST_TARGET_NAME}")
endfunction()

create_tests(
  FILENAME string_utils_tests.cpp
  TESTNAME string_utils_tests
  LINK_LIBRARIES fachory::utils
)


