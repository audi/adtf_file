##################################################################
# Information about the package
set(PKG_NAME adtfdat_processing)

add_library(${PKG_NAME} STATIC
    include/adtfdat_processing/adtfdat_file_reader.h
    include/adtfdat_processing/adtfdat_processing.h
    include/adtfdat_processing/configuration.h
    include/adtfdat_processing/ddl_helpers.h
    include/adtfdat_processing/ddl_helpers.h
    include/adtfdat_processing/demultiplexer.h
    include/adtfdat_processing/multiplexer.h
    include/adtfdat_processing/processor.h
    include/adtfdat_processing/reader.h
    src/adtfdat_file_reader.cpp
    src/demultiplexer.cpp
    src/multiplexer.cpp)

target_compile_options(${PKG_NAME} PRIVATE
                       $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall -fPIC>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)

target_include_directories(${PKG_NAME}
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${PKG_NAME} adtf_file)

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
    include(adtfdat_processing_test_projects.cmake)
endif(ifhd_cmake_enable_integrated_tests)

