option(gtest_disable_installation
       "Enable/Disable installation of gtest/gmock libraries and headers"
       ON)

##For cmake policy CMP00048
unset(PROJECT_VERSION)
unset(PROJECT_VERSION_MAJOR)
unset(PROJECT_VERSION_MINOR)
unset(PROJECT_VERSION_PATCH)
# policy end

##force gtest to be build with shared c runtime library (otherwise we have a mismatch)
# pre-set 'option' in upcoming gtest dir
SET(gtest_force_shared_crt ON CACHE BOOL
    "Use shared (DLL) run-time lib even when Google Test is built as static lib.")

add_subdirectory(googletest-release-1.8.0)
set_target_properties(gmock gmock_main gtest gtest_main PROPERTIES FOLDER 3rdparty)

# /FC causes the Microsoft compilers to make __FILE__ standard compliant
# Without it, the macro will contain a relative path which is bad for tests
target_compile_options(gtest INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/FC>)

if(gtest_disable_installation)
    configure_file(remove_gtest_from_installation.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/remove_gtest_from_installation.cmake
                   @ONLY)
    add_custom_target(gtest_disable_installation
                      ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/remove_gtest_from_installation.cmake"
                      SOURCES ${CMAKE_CURRENT_BINARY_DIR}/remove_gtest_from_installation.cmake)
    add_dependencies(gtest gtest_disable_installation)
    set_target_properties(gtest_disable_installation PROPERTIES FOLDER 3rdparty)
endif(gtest_disable_installation)
