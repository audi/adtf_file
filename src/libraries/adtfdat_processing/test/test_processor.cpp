#include <gtest/gtest.h>
#include <adtfdat_processing/processor.h>
#include "test_processor.h"

using namespace adtf::dat;

std::map<size_t, std::vector<adtf_file::FileItem>> processor_items;

GTEST_TEST(ProcessorFactory, make)
{
    ProcessorFactoryImplementation<TestProcessor<0>> factory;
    auto processor = factory.make();
    ASSERT_EQ(processor->getProcessorIdentifier(), "test_0");
}

GTEST_TEST(ProcessorFactories, make)
{
    ProcessorFactories factories;
    factories.add(std::make_shared<ProcessorFactoryImplementation<TestProcessor<0>>>());
    factories.add(std::make_shared<ProcessorFactoryImplementation<TestProcessor<1>>>());

    ASSERT_EQ(factories.make("test_0")->getProcessorIdentifier(), "test_0");
    ASSERT_EQ(factories.make("test_1")->getProcessorIdentifier(), "test_1");

    {
        adtf_file::Stream stream{0, "stream0"};
        const auto capable_processors = factories.getCapableProcessors(stream);
        ASSERT_EQ(capable_processors.size(), 1);
        ASSERT_EQ(capable_processors.count("test_0"), 1);
    }

    {
        adtf_file::Stream stream{0, "stream1"};
        const auto capable_processors = factories.getCapableProcessors(stream);
        ASSERT_EQ(capable_processors.size(), 1);
        ASSERT_EQ(capable_processors.count("test_1"), 1);
    }

    {
        adtf_file::Stream incompatible_stream{0, "stream10"};
        ASSERT_TRUE(factories.getCapableProcessors(incompatible_stream).empty());
    }
}
