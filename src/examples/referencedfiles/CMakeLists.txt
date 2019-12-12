if (NOT TARGET adtf_file)
    find_package(adtf_file PATHS ${CMAKE_SOURCE_DIR}/lib/cmake/adtf_file REQUIRED)
endif()

set(EXAMPLE referencedfiles) #to not exceed 260 chars on path under windows...

add_executable(${EXAMPLE} referencedfiles.cpp)
target_link_libraries(${EXAMPLE} adtf_file)
set_target_properties(${EXAMPLE} PROPERTIES FOLDER examples)

set_target_properties(${EXAMPLE} PROPERTIES
    DEBUG_POSTFIX "d"
)

install(TARGETS ${EXAMPLE}
    DESTINATION bin
)

