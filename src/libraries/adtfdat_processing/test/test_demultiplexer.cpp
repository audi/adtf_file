#include <gtest/gtest.h>
#include <adtfdat_processing/demultiplexer.h>
#include "test_reader.h"
#include "test_processor.h"

using namespace adtf::dat;

void checkProcessorItems(size_t index)
{
    ASSERT_EQ(processor_items.at(index).size(), 10);
    std::chrono::seconds expected_timestamp(0);
    for (const auto& item: processor_items.at(index))
    {
        ASSERT_EQ(item.time_stamp, expected_timestamp);
        expected_timestamp += std::chrono::seconds(1);
    }
}

GTEST_TEST(Demultiplexer, make)
{
    {
        ProcessorFactories factories;
        factories.add(std::make_shared<ProcessorFactoryImplementation<TestProcessor<1>>>());
        factories.add(std::make_shared<ProcessorFactoryImplementation<TestProcessor<2>>>());

        auto test_reader = std::make_shared<TestReader<0>>();
        test_reader->open("compatible");

        Demultiplexer test_demultiplexer(test_reader, factories);
        test_demultiplexer.addProcessor("stream1", "test_1", "", Configuration());
        test_demultiplexer.addProcessor("stream2", "test_2", "", Configuration());

        test_demultiplexer.process(nullptr);
    }

    ASSERT_EQ(processor_items.size(), 2);

    checkProcessorItems(1);
    checkProcessorItems(2);
}
