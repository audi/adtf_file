#include <gtest/gtest.h>
#include <adtfdat_processing/multiplexer.h>
#include <adtf_file/adtf3/adtf3_sample_copy_serializer.h>
#include <adtf_file/standard_adtf_file_reader.h>
#include "test_reader.h"

using namespace adtf::dat;

namespace adtf_file
{

bool operator==(const adtf_file::Stream& first, const adtf_file::Stream& second)
{
    return first.name == second.name;
}

}

GTEST_TEST(OffsetReaderWrapper, check)
{
    auto wrapped_reader = std::make_shared<TestReader<0>>();
    OffsetReaderWrapper wrapper(wrapped_reader,
                                std::chrono::seconds(-3),
                                std::chrono::seconds(3),
                                std::chrono::seconds(6));

    ASSERT_EQ(wrapper.getReaderIdentifier(), wrapped_reader->getReaderIdentifier());
    ASSERT_EQ(wrapper.isCompatible("compatible"), wrapped_reader->isCompatible("compatible"));
    ASSERT_EQ(wrapper.isCompatible("not compatible"), wrapped_reader->isCompatible("not compatible"));

    wrapper.open("compatible");
    ASSERT_EQ(wrapper.getStreams(), wrapped_reader->getStreams());

    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(0));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(0));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(1));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(1));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(2));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(2));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(3));
    ASSERT_EQ(wrapper.getNextItem().time_stamp, std::chrono::seconds(3));
    ASSERT_ANY_THROW(wrapper.getNextItem());
}

void checkStream(const adtf_file::Stream& stream,
                 const std::string& name,
                 size_t count,
                 std::chrono::nanoseconds timestamp_of_first_item,
                 std::chrono::nanoseconds timestamp_of_last_item,
                 const std::string& stream_meta_name)
{
    ASSERT_EQ(stream.name, name);
    ASSERT_EQ(stream.item_count, count);
    ASSERT_EQ(stream.timestamp_of_first_item, timestamp_of_first_item);
    ASSERT_EQ(stream.timestamp_of_last_item, timestamp_of_last_item);
    auto type = std::dynamic_pointer_cast<const adtf_file::PropertyStreamType>(stream.initial_type);
    ASSERT_EQ(type->getMetaType(), stream_meta_name);
}

GTEST_TEST(Multiplexer, process)
{
    std::string file_name = TEST_BUILD_DIR "/test_multiplex.adtfdat";

    {
        Multiplexer test_multiplexer(file_name);

        auto reader1 = std::make_shared<TestReader<0>>();
        reader1->open("compatible");
        auto reader2 = std::make_shared<TestReader<10>>();
        reader2->open("compatible");

        test_multiplexer.addStream(reader1, "stream1", "outstream11", std::make_shared<adtf_file::adtf3::SampleCopySerializer>());
        test_multiplexer.addStream(reader1, "stream2", "outstream12", std::make_shared<adtf_file::adtf3::SampleCopySerializer>());
        test_multiplexer.addStream(reader2, "stream2", "outstream21", std::make_shared<adtf_file::adtf3::SampleCopySerializer>());

        test_multiplexer.process(nullptr);
    }

    adtf_file::StandardReader adtf_reader(file_name);
    auto streams = adtf_reader.getStreams();
    ASSERT_EQ(streams.size(), 3);
    checkStream(streams[0], "outstream11", 10, std::chrono::seconds(0), std::chrono::seconds(9), "adtf/anonymous");
    checkStream(streams[1], "outstream12", 10, std::chrono::seconds(0), std::chrono::seconds(9), "adtf/anonymous");
    checkStream(streams[2], "outstream21", 10, std::chrono::seconds(10), std::chrono::seconds(19), "adtf/anonymous");
}
