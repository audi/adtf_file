################################################################################
# Set CMake requirements and policy
################################################################################
cmake_minimum_required(VERSION 3.5.1 FATAL_ERROR)

cmake_policy(SET CMP0002 NEW)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0011 NEW)
cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0028 NEW)

project(adtffile VERSION 0.6.0)

if (CONAN_COMPILER)
    message(STATUS "Including Conan build info")
    include(${CMAKE_BINARY_DIR}/../conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SCRIPTS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/scripts/cmake)

option(ifhd_cmake_enable_integrated_tests "Enable tests as integrated build (requires googletest)" OFF)
option(ifhd_build_and_install_examples "Enable to build and install examples" ON)

add_subdirectory(3rdparty)
#######################################################################################
# Now we start to build internal Libraries
#######################################################################################
find_package(a_util REQUIRED)
find_package(ddl REQUIRED)

if (ifhd_cmake_enable_integrated_tests)
    enable_testing()
endif()

add_subdirectory(src)
add_subdirectory(doc)

# License Information must be delivered anyway!
install(FILES README.md DESTINATION ./)
install(FILES doc/license/MPL2.0.txt DESTINATION ./)
install(FILES doc/license/MPL2.0.txt DESTINATION doc/license)
install(FILES doc/input/used_licenses.md DESTINATION doc/license)

if (${ifhd_build_and_install_examples})
install(
    FILES CMakeLists.txt.install
    DESTINATION ./
    RENAME CMakeLists.txt
)
install(
    DIRECTORY src/examples
    DESTINATION ./src
)
endif (${ifhd_build_and_install_examples})
