set(TEST t_a_utils_idxwtable) #to not exceed 260 chars on path under windows...

aev_cmake_add_function_test(${TEST}
    60
    ""
    ""
    "${CMAKE_CURRENT_SOURCE_DIR}/../"

    stdafx.h
    stdafx.cpp
    main.cpp

    tester_indexwritetable.cpp
    tester_indexwritetable.h
)
target_link_libraries(${TEST} ${A_UTILS_LIBRARIES})
aev_cmake_project_set_folder(${TEST} extern/test/library/a_utils/utils_file)