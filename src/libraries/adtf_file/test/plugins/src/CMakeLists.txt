set(TEST t_adtf_file_plugins) #to not exceed 260 chars on path under windows...

set(TEST_FILES_DIR "${CMAKE_CURRENT_LIST_DIR}/../files")
add_definitions(-DTEST_FILES_DIR="${TEST_FILES_DIR}")

add_library(test_plugin MODULE test_plugin.cpp)
target_link_libraries(test_plugin adtf_file)
target_compile_options(test_plugin PRIVATE
                       $<$<CXX_COMPILER_ID:GNU>:-Bsymbolic>)

add_library(test_plugin_old_no_version MODULE test_plugin_old_no_version.cpp)
target_link_libraries(test_plugin_old_no_version adtf_file)
target_compile_options(test_plugin_old_no_version PRIVATE
                      $<$<CXX_COMPILER_ID:GNU>:-Bsymbolic>)

add_library(test_plugin_old MODULE test_plugin_old.cpp)
target_link_libraries(test_plugin_old adtf_file)
target_compile_options(test_plugin_old PRIVATE
                    $<$<CXX_COMPILER_ID:GNU>:-Bsymbolic>)


add_executable(${TEST} tester_plugins.cpp)
target_link_libraries(${TEST} gtest gtest_main adtf_file)

target_compile_options(${TEST} PRIVATE
    -DTEST_PLUGIN="$<TARGET_FILE:test_plugin>"
    -DTEST_PLUGIN_OLD="$<TARGET_FILE:test_plugin_old>"
    -DTEST_PLUGIN_OLD_NO_VERSION="$<TARGET_FILE:test_plugin_old_no_version>")

add_dependencies(${TEST} test_plugin test_plugin_old test_plugin_old_no_version)

ifhd_test(${TEST} ${TEST})
set_target_properties(${TEST} PROPERTIES FOLDER test/adtf_file)
