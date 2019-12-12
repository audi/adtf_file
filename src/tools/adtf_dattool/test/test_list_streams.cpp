#include <gtest/gtest.h>
#include "dattool_helper.h"

GTEST_TEST(dattool, help)
{
    std::string dat_file{TEST_SOURCE_DIR "/example_file.adtfdat"};
    auto output = launchDatTool("--liststreams " + dat_file).first;
    ASSERT_EQ(output,
R"(adtfdat:
    VIDEO:
        type: adtf/image
        processors:
        time range (ns): [405356000, 14805306000]
        items: 874
    NESTED_STRUCT:
        type: adtf2/legacy
        processors:
        time range (ns): [0, 14805306000]
        items: 595
)");
}
