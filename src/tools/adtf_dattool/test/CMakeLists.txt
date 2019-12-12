add_subdirectory(test_processor)

find_package(GTest REQUIRED)
include(GoogleTest)

enable_testing()

add_executable(test_adtf_dattool
    dattool_helper.h
    test_list_streams.cpp
    test_create.cpp
    test_export.cpp
    test_modify_extension.cpp)
target_compile_definitions(test_adtf_dattool PRIVATE
    -DTEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    -DTEST_BUILD_DIR="${CMAKE_CURRENT_BINARY_DIR}"
    -DADTF_DATTOOL_EXECUTABLE="$<TARGET_FILE:adtf_dattool>"
    -DTEST_PROCESSOR_PLUGIN="$<TARGET_FILE:test_processor>")
target_link_libraries(test_adtf_dattool PRIVATE adtf_file GTest::GTest GTest::Main)
if(NOT WIN32)
    target_link_libraries(test_adtf_dattool PRIVATE stdc++fs)
endif()
target_compile_features(test_adtf_dattool PUBLIC cxx_std_14)

gtest_add_tests(TARGET test_adtf_dattool
                TEST_LIST   adtf_dattool_tests)
set_tests_properties(${adtf_dattool_tests} PROPERTIES TIMEOUT 20)

