#include <gtest/gtest.h>
#include <adtfdat_processing/configuration.h>

using namespace adtf::dat;
GTEST_TEST(Configuration, empty)
{
    Configurable empty;
    auto configuration = empty.getConfiguration();
    ASSERT_TRUE(configuration.empty());
}

GTEST_TEST(Configuration, setAndGet)
{
    Configuration input_configuration{{"prop1", {"value1", "string"}},
                                      {"prop2", {"value2", "string"}}};
    Configurable configurable_object;
    configurable_object.setConfiguration(input_configuration);
    auto output_configuration = configurable_object.getConfiguration();
    ASSERT_EQ(output_configuration, input_configuration);
}

GTEST_TEST(Configuration, getAsType)
{
    Configuration test_configuration{{"prop1", {"3.1415", "tFloat64"}},
                                     {"prop2", {"-1", "tInt64"}}};

    ASSERT_EQ(getPropertyValue<std::string>(test_configuration, "prop1"), "3.1415");
    ASSERT_EQ(getPropertyValue<std::string>(test_configuration, "prop2"), "-1");

    ASSERT_EQ(getPropertyValue<int64_t>(test_configuration, "prop1"), 3);
    ASSERT_EQ(getPropertyValue<int64_t>(test_configuration, "prop2"), -1);

    ASSERT_EQ(getPropertyValue<uint64_t>(test_configuration, "prop1"), 3);
    ASSERT_EQ(getPropertyValue<uint64_t>(test_configuration, "prop2"), std::numeric_limits<uint64_t>::max());

    ASSERT_EQ(getPropertyValue<double>(test_configuration, "prop1"), 3.1415);
    ASSERT_EQ(getPropertyValue<double>(test_configuration, "prop2"), -1.0);
}
