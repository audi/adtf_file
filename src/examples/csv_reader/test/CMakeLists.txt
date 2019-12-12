find_package(GTest REQUIRED)
include(GoogleTest)

enable_testing()

add_executable(test_csv_reader
    test_csv_reader.cpp)
target_compile_definitions(test_csv_reader PRIVATE
    -DTEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    -DTEST_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    -DCSV_READER_PLUGIN="$<TARGET_FILE:csv_reader>")
target_link_libraries(test_csv_reader PRIVATE adtfdat_processing GTest::GTest GTest::Main)
if(NOT WIN32)
    target_link_libraries(test_csv_reader PRIVATE stdc++fs)
endif()

gtest_add_tests(TARGET test_csv_reader
                TEST_LIST   csv_reader_tests)
set_tests_properties(${csv_reader_tests} PROPERTIES TIMEOUT 20)
