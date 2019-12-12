find_package(a_util REQUIRED)
find_package(ddl REQUIRED)
find_package(adtf_file PATHS ${CMAKE_CURRENT_LIST_DIR}/../adtf_file REQUIRED)
include(${CMAKE_CURRENT_LIST_DIR}/adtfdat_processing-targets.cmake)
