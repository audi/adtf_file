##################################################################
# Information about the package
set(PKG_NAME ifhd_file)

add_library(${PKG_NAME} STATIC
    include/ifhd/ifhd.h
    include/ifhd/indexedfile_pkg.h
    include/ifhd/indexedfile_types.h
    include/ifhd/v100/indexedfilereader_v100.h
    include/ifhd/v100/indexedfile_v100.h
    include/ifhd/v100/indexedfile_v100_pkg.h
    include/ifhd/v110/indexedfilereader_v110.h
    include/ifhd/v110/indexedfile_v110.h
    include/ifhd/v110/indexedfile_v110_pkg.h
    include/ifhd/v201_v301/indexedfilehelper_v201_v301.h
    include/ifhd/v201_v301/indexedfilereader_v201_v301.h
    include/ifhd/v201_v301/indexedfile_types_v201_v301.h
    include/ifhd/v201_v301/indexedfile_v201_v301.h
    include/ifhd/v201_v301/indexedfile_v201_v301_pkg.h
    include/ifhd/v201_v301/indexedfilewriter_v201_v301.h
    include/ifhd/v201_v301/indexreadtable_v201_v301.h
    include/ifhd/v201_v301/indexwritetable_v201_v301.h
    include/ifhd/v400/indexedfilehelper_v400.h
    include/ifhd/v400/indexedfilereader_v400.h
    include/ifhd/v400/indexedfile_types_v400.h
    include/ifhd/v400/indexedfile_v400.h
    include/ifhd/v400/indexedfile_v400_pkg.h
    include/ifhd/v400/indexedfilewriter_v400.h
    include/ifhd/v400/indexreadtable_v400.h
    include/ifhd/v400/indexwritetable_v400.h
    include/ifhd/v500/indexedfilehelper_v500.h
    include/ifhd/v500/indexedfilereader_v500.h
    include/ifhd/v500/indexedfile_types_v500.h
    include/ifhd/v500/indexedfile_v500.h
    include/ifhd/v500/indexedfile_v500_pkg.h
    include/ifhd/v500/indexedfilewriter_v500.h
    include/ifhd/v500/indexreadtable_v500.h
    include/ifhd/v500/indexwritetable_v500.h

    src/indexedfilehelper_v201_v301.cpp
    src/indexedfilehelper_v400.cpp
    src/indexedfilereader_v100.cpp
    src/indexedfilereader_v110.cpp
    src/indexedfilereader_v201_v301.cpp
    src/indexedfilereader_v400.cpp
    src/indexedfile_v100.cpp
    src/indexedfile_v110.cpp
    src/indexedfile_v201_v301.cpp
    src/indexedfile_v400.cpp
    src/indexedfilewriter_v201_v301.cpp
    src/indexedfilewriter_v400.cpp
    src/indexreadtable_v201_v301.cpp
    src/indexreadtable_v400.cpp
    src/indexwritetable_v201_v301.cpp
    src/indexwritetable_v400.cpp)

target_compile_options(${PKG_NAME} PRIVATE
                       $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall -fPIC>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)

target_include_directories(${PKG_NAME}
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${PKG_NAME} utils5extension)

install(TARGETS ${PKG_NAME}
        EXPORT ${PKG_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION bin
        INCLUDES DESTINATION include)

install(DIRECTORY include/ DESTINATION include)


##Packaging
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}-config-version.cmake
                                 COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}-config-version.cmake
              ${PKG_NAME}-config.cmake
        DESTINATION lib/cmake/${PKG_NAME})
install(EXPORT ${PKG_NAME} 
        FILE ${PKG_NAME}-targets.cmake
        DESTINATION lib/cmake/${PKG_NAME})

set_target_properties(${PKG_NAME} PROPERTIES FOLDER libraries)

##add tests if enabled
if(ifhd_cmake_enable_integrated_tests)
    ##add all tests to this file
    include(ifhd_test_projects.cmake)
endif(ifhd_cmake_enable_integrated_tests)

