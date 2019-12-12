add_library(test_processor MODULE    
    test_processor.cpp)

target_link_libraries(test_processor adtfdat_processing)

set_target_properties(test_processor PROPERTIES
    PREFIX ""
    SUFFIX ".adtffileplugin"
    DEFINE_SYMBOL ""
)
