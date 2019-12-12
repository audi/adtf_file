find_package(GTest REQUIRED)
include(GoogleTest)

enable_testing()

add_executable(test_adtfdat_processing
    test_configuration.cpp
    test_reader.h
    test_reader.cpp
    test_multiplexer.cpp
    test_processor.h
    test_processor.cpp
    test_demultiplexer.cpp)
target_compile_definitions(test_adtfdat_processing PRIVATE
    -DTEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    -DTEST_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(test_adtfdat_processing PRIVATE adtfdat_processing GTest::GTest GTest::Main)

gtest_add_tests(TARGET test_adtfdat_processing
                TEST_LIST   ADTF_DATTOOLS_tests)
set_tests_properties(${ADTF_DATTOOLS_tests} PROPERTIES TIMEOUT 20)
