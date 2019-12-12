##################################################################
# Information about the package
set(PKG_NAME adtf_file)

add_library(${PKG_NAME} STATIC
    include/adtf_file/adtf2/adtf2_adtf_core_media_sample_deserializer.h
    include/adtf_file/adtf2/adtf2_adtf_core_media_sample_serializer.h
    include/adtf_file/adtf2/adtf2_indexed_data_media_sample_deserializer.h
    include/adtf_file/adtf2/adtf2_sample_info.h
    include/adtf_file/adtf2/adtf2_stream_type_deserializers.h
    include/adtf_file/adtf2/adtf2_stream_type_serializers.h
    include/adtf_file/adtf2/legacy_types.h
    include/adtf_file/adtf3/adtf3_media_description_deserializer.h
    include/adtf_file/adtf3/adtf3_media_description_serializer.h
    include/adtf_file/adtf3/adtf3_sample_copy_deserializer.h
    include/adtf_file/adtf3/adtf3_sample_copy_serializer.h
    include/adtf_file/adtf3/adtf3_sample_info.h
    include/adtf_file/adtf3/adtf3_stream_type_deserializer.h
    include/adtf_file/adtf3/adtf3_stream_type_serializer.h
    include/adtf_file/adtf_file_reader.h
    include/adtf_file/adtf_file_writer.h
    include/adtf_file/default_sample.h
    include/adtf_file/file_extensions.h
    include/adtf_file/legacy_utils4_utils5_types.h
    include/adtf_file/object.h
    include/adtf_file/sample.h
    include/adtf_file/standard_adtf_file_reader.h
    include/adtf_file/standard_factories.h
    include/adtf_file/stream_item.h
    include/adtf_file/stream_type.h

    src/adtf2/adtf2_adtf_core_media_sample_deserializer.cpp
    src/adtf2/adtf2_adtf_core_media_sample_serializer.cpp
    src/adtf2/adtf2_indexed_data_media_sample_deserializer.cpp
    src/adtf2/adtf2_sample_info.cpp
    src/adtf2/adtf2_stream_type_deserializers.cpp
    src/adtf2/adtf2_stream_type_serializers.cpp
    src/adtf3/adtf3_media_description_deserializer.cpp
    src/adtf3/adtf3_media_description_serializer.cpp
    src/adtf3/adtf3_sample_flags.h
    src/adtf3/adtf3_sample_copy_deserializer.cpp
    src/adtf3/adtf3_sample_copy_serializer.cpp
    src/adtf3/adtf3_sample_info.cpp
    src/adtf3/adtf3_stream_type_deserializer.cpp
    src/adtf3/adtf3_stream_type_serializer.cpp
    src/adtf_file_reader.cpp
    src/adtf_file_writer.cpp
    src/default_sample.cpp
    src/file_extensions.cpp
    src/object.cpp
    src/object_plugin.cpp
    src/sample.cpp
    src/stream_type.cpp
    ${CMAKE_SOURCE_DIR}/3rdparty/cityhash/city.cc)

target_compile_definitions(${PKG_NAME} PRIVATE -DADTF_FILE_VERSION="${adtf_file_VERSION}")

target_compile_options(${PKG_NAME} PRIVATE
                       $<$<CXX_COMPILER_ID:GNU>:-pedantic -Wall -fPIC>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>)

target_include_directories(${PKG_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/cityhash)

target_include_directories(${PKG_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)


target_link_libraries(${PKG_NAME} utils5extension ifhd_file ddl)
if(NOT WIN32)
    target_link_libraries(${PKG_NAME} dl)
endif(NOT WIN32)

install(TARGETS ${PKG_NAME}
        EXPORT ${PKG_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION bin
        INCLUDES DESTINATION include)

install(DIRECTORY include/ DESTINATION include)

set_target_properties(${PKG_NAME} PROPERTIES FOLDER libraries)


##Packaging
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}-config-version.cmake
                                 COMPATIBILITY AnyNewerVersion)


install(EXPORT ${PKG_NAME} 
        FILE ${PKG_NAME}-targets.cmake
        DESTINATION lib/cmake/${PKG_NAME})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PKG_NAME}-config-version.cmake
              ${PKG_NAME}-config.cmake
        DESTINATION lib/cmake/${PKG_NAME})

##add tests if enabled
if(ifhd_cmake_enable_integrated_tests)
    ##add all tests to this file
    include(adtf_file_test_projects.cmake)
endif(ifhd_cmake_enable_integrated_tests)

