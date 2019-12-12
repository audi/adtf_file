set(PKG_NAME utils5extension)

add_library(${PKG_NAME} STATIC
    include/utils5extension/file.h
    include/utils5extension/fileringbuffer.h
    include/utils5extension/utils5extension.h
    include/utils5extension/utils5ext_pkg.h

    src/file.cpp)
            
target_compile_options(${PKG_NAME} PRIVATE
                       $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall -fPIC>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)

target_include_directories(${PKG_NAME}
                          PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${PKG_NAME} a_util_concurrency 
                                  a_util_filesystem 
                                  a_util_datetime 
                                  a_util_memory
                                  a_util_xml)

if(NOT WIN32)
    target_link_libraries(${PKG_NAME} pthread)
endif(NOT WIN32)

set_target_properties(${PKG_NAME} PROPERTIES FOLDER libraries)

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
install(EXPORT ${PKG_NAME} FILE ${PKG_NAME}-targets.cmake DESTINATION lib/cmake/${PKG_NAME})




