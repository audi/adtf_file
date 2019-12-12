add_executable(adtf_dattool adtf_dattool.cpp)

target_link_libraries(adtf_dattool adtfdat_processing)
target_include_directories(adtf_dattool PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/clara/include)

set_target_properties(adtf_dattool PROPERTIES
    DEBUG_POSTFIX "d"
    FOLDER tools
)

install(TARGETS adtf_dattool
    DESTINATION bin
)

if(ifhd_cmake_enable_integrated_tests)
    add_subdirectory(test)
endif()
