find_package(utils5extension PATHS ${CMAKE_CURRENT_LIST_DIR}/../utils5extension REQUIRED)
find_package(ifhd_file PATHS ${CMAKE_CURRENT_LIST_DIR}/../ifhd_file REQUIRED)
find_package(ddl REQUIRED)
include(${CMAKE_CURRENT_LIST_DIR}/adtf_file-targets.cmake)
