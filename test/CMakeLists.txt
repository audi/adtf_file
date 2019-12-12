cmake_minimum_required(VERSION 3.0.0)   #minimum for generated find scripts

project(ifhd_tests)

if(CONAN_COMPILER)
    message(STATUS "Including Conan build info")
    include(${CMAKE_BINARY_DIR}/../conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS)
endif()

find_package(ifhd REQUIRED)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

##single point of include, the same as for project 'a_util'
include_directories(standalone)

##googletest integration
add_subdirectory(../3rdparty/googletest ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/googletest)

##functional test integration
add_subdirectory(function)
