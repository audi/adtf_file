if (NOT TARGET adtfdat_processing)
    find_package(adtfdat_processing PATHS ${CMAKE_SOURCE_DIR}/lib/cmake/adtfdat_processing REQUIRED)
endif()

add_library(csv_reader MODULE
    csv_reader.h
    csv_reader.cpp
)

target_link_libraries(csv_reader adtfdat_processing)

set_target_properties(csv_reader PROPERTIES
    PREFIX ""
    SUFFIX ".adtffileplugin"
    DEFINE_SYMBOL ""
    DEBUG_POSTFIX "d"
    FOLDER examples
)

install(TARGETS csv_reader
    DESTINATION bin
)

if(ifhd_cmake_enable_integrated_tests)
    add_subdirectory(test)
endif()
