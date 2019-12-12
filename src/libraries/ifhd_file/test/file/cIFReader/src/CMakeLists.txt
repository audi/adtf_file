set(TEST t_idxfr) #to not exceed 260 chars on path under windows...

set(TEST_FILES_DIR "${CMAKE_CURRENT_LIST_DIR}/../files")
add_definitions(-DTEST_FILES_DIR="${TEST_FILES_DIR}")


add_executable(${TEST} tester_indexedfilereader.cpp)
target_link_libraries(${TEST} gtest gtest_main ifhd_file)
ifhd_test(${TEST} ${TEST})
set_target_properties(${TEST} PROPERTIES FOLDER test/ifhd)