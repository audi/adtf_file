option(ifhd_cmake_enable_documentation "If enabled, generate the source code documentation\
 (requires doxygen)" ON)
option(ifhd_cmake_enable_post_install_doc_generation "If enabled, doxygen will generate the\
 documentation as post-install event. Disable only for development purposes." OFF)
option(DOXYGEN_SKIP_DOT "If true this module will skip trying to find Dot" OFF)

##continue only if enabled
if(ifhd_cmake_enable_documentation)
    find_package(Doxygen REQUIRED)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "No doxygen executable found. Either set the correct "
                            "DOXYGEN_EXECUTABLE or disable ifhd_cmake_enable_documentation.")
    endif(NOT DOXYGEN_FOUND)
    if(NOT DOXYGEN_SKIP_DOT AND NOT DOXYGEN_DOT_FOUND)
        message(FATAL_ERROR "No dot executable found. Either set the "
                            "correct DOXYGEN_DOT_EXECUTABLE or enable DOXYGEN_SKIP_DOT.")
    elseif(NOT DOXYGEN_SKIP_DOT AND DOXYGEN_DOT_FOUND)
        EXECUTE_PROCESS(COMMAND ${DOXYGEN_DOT_EXECUTABLE} -V OUTPUT_VARIABLE dot_version_info
                                                             ERROR_VARIABLE dot_version_info
                                                             OUTPUT_STRIP_TRAILING_WHITESPACE
                                                             ERROR_STRIP_TRAILING_WHITESPACE)
        set(dot_version_info "(found version: \"${dot_version_info}\")")
        message(STATUS "Found dot: ${DOXYGEN_DOT_EXECUTABLE} ${dot_version_info}")
        unset(dot_version_info)
    endif(NOT DOXYGEN_SKIP_DOT AND NOT DOXYGEN_DOT_FOUND)
else()
    return()
endif(ifhd_cmake_enable_documentation)

configure_file(input/predef.h.in input/predef.h @ONLY)
configure_file(run_doxygen.cmake.in run_doxygen.cmake @ONLY)

##dont know where the run_doxygen will run the doxygen main ?? 
configure_file(input/page_delivered_versions.md.in ${CMAKE_CURRENT_SOURCE_DIR}/input/page_delivered_versions.md @ONLY)
##create seperate target excluded from all
add_custom_target(ifhd_DOC ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/run_doxygen.cmake)
set_target_properties(ifhd_DOC PROPERTIES FOLDER process)

##post install script
if(ifhd_cmake_enable_post_install_doc_generation)
    install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/run_doxygen.cmake)
endif(ifhd_cmake_enable_post_install_doc_generation)