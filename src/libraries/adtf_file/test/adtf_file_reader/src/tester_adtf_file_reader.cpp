/**
 * @file
 * Tester init.
 *
 * @copyright
 * @verbatim
   Copyright @ 2017 Audi Electronics Venture GmbH. All rights reserved.

       This Source Code Form is subject to the terms of the Mozilla
       Public License, v. 2.0. If a copy of the MPL was not distributed
       with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

   If it is not possible or desirable to put the notice in a particular file, then
   You may include the notice in a location (such as a LICENSE file in a
   relevant directory) where a recipient would be likely to look for such a notice.

   You may add additional accurate notices of copyright ownership.
   @endverbatim
 */

#include "gtest/gtest.h"
#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/standard_factories.h>
#include <ddl.h>

using namespace adtf_file;

GTEST_TEST(TestReadADTF3, AdtfFileReader)
{    
    Reader reader(TEST_FILES_DIR "/test_stop_signal.dat", StandardTypeDeserializers(), StandardSampleDeserializers());

    ASSERT_EQ(reader.getStreams().size(), 3);
    ASSERT_EQ(reader.getDuration(), std::chrono::microseconds(2906356));
    ASSERT_EQ(reader.getFirstTime(), std::chrono::microseconds(4015135));
    ASSERT_EQ(reader.getLastTime(), std::chrono::microseconds(6921491));

    {
        auto& stream = reader.getStreams()[0];
        ASSERT_EQ(stream.name, "in1");
        auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
        ASSERT_TRUE(property_stream_type);
        ASSERT_EQ(property_stream_type->getMetaType(), "adtf/anonymous");
    }
    {
        auto& stream = reader.getStreams()[1];
        ASSERT_EQ(stream.name, "in2");
        auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
        ASSERT_TRUE(property_stream_type);
        ASSERT_EQ(property_stream_type->getMetaType(), "adtf/anonymous");
    }
    {
        auto& stream = reader.getStreams()[2];
        ASSERT_EQ(stream.name, "in3");
        auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
        ASSERT_TRUE(property_stream_type);
        ASSERT_EQ(property_stream_type->getMetaType(), "adtf/anonymous");
    }

    size_t item_count = 0;
    size_t sample_count = 0;
    size_t trigger_count = 0;
    size_t type_count = 0;
    for (;; ++item_count)
    {
        try
        {
            auto item = reader.getNextItem();
            if (std::dynamic_pointer_cast<const StreamType>(item.stream_item))
            {
                ++type_count;
            }
            else if (std::dynamic_pointer_cast<const Sample>(item.stream_item))
            {
                ++sample_count;
            }
            else if (std::dynamic_pointer_cast<const Trigger>(item.stream_item))
            {
                ++trigger_count;
            }
        }
        catch (const exceptions::EndOfFile&)
        {
            break;
        }
    }

    ASSERT_EQ(item_count, reader.getItemCount());
    ASSERT_EQ(sample_count, 30);
    ASSERT_EQ(trigger_count, 30);
    ASSERT_EQ(type_count, 2);
}

GTEST_TEST(TestReadADTF2, AdtfFileReader)
{
    Reader reader(TEST_FILES_DIR "/example_test_file.dat", StandardTypeDeserializers(), StandardSampleDeserializers());

    ASSERT_EQ(reader.getStreams().size(), 2);
    ASSERT_EQ(reader.getDuration(), std::chrono::microseconds(14805306));

    {
        auto& stream = reader.getStreams()[0];
        ASSERT_EQ(stream.name, "video");
        auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
        ASSERT_TRUE(property_stream_type);
        ASSERT_EQ(property_stream_type->getMetaType(), "adtf/image");
    }
    {
        auto& stream = reader.getStreams()[1];
        ASSERT_EQ(stream.name, "NESTED_STRUCT");
        auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
        ASSERT_TRUE(property_stream_type);
        ASSERT_EQ(property_stream_type->getMetaType(), "adtf2/legacy");
    }

    size_t item_count = 0;
    size_t sample_count = 0;
    size_t trigger_count = 0;
    size_t type_count = 0;
    for (;; ++item_count)
    {
        try
        {
            auto item = reader.getNextItem();
            if (std::dynamic_pointer_cast<const StreamType>(item.stream_item))
            {
                ++type_count;
            }
            else if (std::dynamic_pointer_cast<const Sample>(item.stream_item))
            {
                ++sample_count;
            }
            else if (std::dynamic_pointer_cast<const Trigger>(item.stream_item))
            {
                ++trigger_count;
            }
        }
        catch (const exceptions::EndOfFile&)
        {
            break;
        }
    }

    ASSERT_EQ(item_count, reader.getItemCount());
    ASSERT_EQ(sample_count, 446);
    ASSERT_EQ(trigger_count, 0);
    ASSERT_EQ(type_count, 0);
}

void check_type_counter(const std::shared_ptr<const StreamType> stream_type, uint32_t counter)
{
    auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream_type);
    ASSERT_TRUE(property_stream_type);
    ASSERT_EQ(property_stream_type->getProperty("counter").second, std::to_string(counter));
}

GTEST_TEST(TestRetrieveTypes, AdtfFileReader)
{
    Reader reader(TEST_FILES_DIR "/test_type_seek.dat", StandardTypeDeserializers(), StandardSampleDeserializers());

    auto stream1 = reader.getStreams()[0];
    auto stream2 = reader.getStreams()[1];

    check_type_counter(stream1.initial_type, 0);
    check_type_counter(stream2.initial_type, 10);

    auto position = reader.getItemIndexForTimeStamp(std::chrono::microseconds(3000));
    check_type_counter(reader.getStreamTypeBefore(position, stream1.stream_id, true), 1);
    check_type_counter(reader.getStreamTypeBefore(position, stream2.stream_id, true), 10);

    position = reader.getItemIndexForTimeStamp(std::chrono::microseconds(5000));
    check_type_counter(reader.getStreamTypeBefore(position, stream1.stream_id, true), 1);
    check_type_counter(reader.getStreamTypeBefore(position, stream2.stream_id, true), 11);

    position = reader.getItemIndexForTimeStamp(std::chrono::microseconds(7000));
    check_type_counter(reader.getStreamTypeBefore(position, stream1.stream_id, true), 2);
    check_type_counter(reader.getStreamTypeBefore(position, stream2.stream_id, true), 11);

    position = reader.getItemIndexForTimeStamp(std::chrono::microseconds(9000));
    check_type_counter(reader.getStreamTypeBefore(position, stream1.stream_id, true), 2);
    check_type_counter(reader.getStreamTypeBefore(position, stream2.stream_id, true), 12);

    position = reader.getItemIndexForTimeStamp(std::chrono::microseconds(11000));
    check_type_counter(reader.getStreamTypeBefore(position, stream1.stream_id, true), 3);
    check_type_counter(reader.getStreamTypeBefore(position, stream2.stream_id, true), 12);

    position = reader.getItemIndexForTimeStamp(std::chrono::microseconds(13000));
    check_type_counter(reader.getStreamTypeBefore(position, stream1.stream_id, true), 3);
    check_type_counter(reader.getStreamTypeBefore(position, stream2.stream_id, true), 13);
}

GTEST_TEST(TestADTF2SampleInfo, AdtfFileReader)
{
    Reader reader(TEST_FILES_DIR "/adtf2/test_sample_info.dat", StandardTypeDeserializers(), StandardSampleDeserializers());
    for (;;)
    {
        try
        {
            auto sample = std::dynamic_pointer_cast<const WriteSample>(reader.getNextItem().stream_item);
            if (sample)
            {
                bool time_stamp_found = false;
                bool counter_found = false;
                bool custom_info_found = false;
                sample->iterateInfo([&](uint32_t key, DataType type, uint64_t value)
                {
                    if (key == SampleInfoKeys::sai_device_original_time)
                    {
                        time_stamp_found = true;
                    }
                    else if (key == SampleInfoKeys::sai_counter)
                    {
                        counter_found = true;
                    }
                    else if (key == createSampleInfoHashKey("adtf2_index_201"))
                    {
                        custom_info_found = true;
                    }
                });

                ASSERT_TRUE(time_stamp_found);
                ASSERT_TRUE(counter_found);
                ASSERT_TRUE(custom_info_found);
            }
        }
        catch (const exceptions::EndOfFile&)
        {
            break;
        }
    }
}

GTEST_TEST(TestADTF2ExternalDescription, AdtfFileReader)
{
    Reader reader(TEST_FILES_DIR "/adtf2/test_file_rec_newfile.dat", StandardTypeDeserializers(), StandardSampleDeserializers());
    std::unordered_map<uint16_t, std::unique_ptr<ddl::CodecFactory>> codec_factories;
    for (auto& stream: reader.getStreams())
    {
        auto property_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
        ASSERT_TRUE(property_type);
        codec_factories[stream.stream_id].reset(new ddl::CodecFactory(property_type->getProperty("md_struct").second.c_str(),
                                                                       property_type->getProperty("md_definitions").second.c_str()));
        ASSERT_TRUE(a_util::result::isOk(codec_factories[stream.stream_id]->isValid()));
    }

    for (;;)
    {
        try
        {
            auto next_item = reader.getNextItem();
            auto sample = std::dynamic_pointer_cast<const WriteSample>(next_item.stream_item);
            if (sample)
            {
                auto buffer = sample->beginBufferRead();

                auto decoder = codec_factories[next_item.stream_id]->makeDecoderFor(buffer.first, buffer.second);
                ASSERT_TRUE(a_util::result::isOk(decoder.isValid()));
                ASSERT_GT(decoder.getElementCount(), 0);
                for (size_t element_index = 0; element_index < decoder.getElementCount(); ++element_index)
                {
                    ASSERT_NE(ddl::access_element::get_value_as_string(decoder, element_index), "");
                }
                sample->endBufferRead();
            }
        }
        catch (const exceptions::EndOfFile&)
        {
            break;
        }
    }
}

void create_test_files()
{
    {
        Writer writer(TEST_FILES_DIR "/two_empty_streams.dat", std::chrono::seconds(0), adtf3::StandardTypeSerializers());
        DefaultStreamType stream_type("adtf/anonymous");
        writer.createStream("stream1", stream_type, std::make_shared<adtf3::SampleCopySerializer>());
        writer.createStream("stream2", stream_type, std::make_shared<adtf3::SampleCopySerializer>());
    }

    {
        Writer writer(TEST_FILES_DIR "/one_empty_stream.dat", std::chrono::seconds(0), adtf3::StandardTypeSerializers());
        DefaultStreamType stream_type("adtf/anonymous");
        writer.createStream("stream1", stream_type, std::make_shared<adtf3::SampleCopySerializer>());
        auto stream2 = writer.createStream("stream2", stream_type, std::make_shared<adtf3::SampleCopySerializer>());

        std::chrono::seconds time_stamp(1);
        DefaultSample sample;
        sample.setTimeStamp(time_stamp);
        writer.write(stream2, time_stamp, sample);
        writer.writeTrigger(stream2, time_stamp);

        time_stamp = std::chrono::seconds(2);
        sample.setTimeStamp(time_stamp);
        writer.write(stream2, time_stamp, sample);
    }
}

GTEST_TEST(TestGetDuration, AdtfFileReader)
{
#ifdef CREATE_TEST_FILES
    create_test_files();
#endif

    {
        Reader reader(TEST_FILES_DIR "/two_empty_streams.dat", StandardTypeDeserializers(), StandardSampleDeserializers());
        ASSERT_EQ(reader.getDuration(), std::chrono::microseconds(0));
        ASSERT_EQ(reader.getFirstTime(), std::chrono::microseconds(0));
        ASSERT_EQ(reader.getLastTime(), std::chrono::microseconds(0));
    }

    {
        Reader reader(TEST_FILES_DIR "/one_empty_stream.dat", StandardTypeDeserializers(), StandardSampleDeserializers());
        ASSERT_EQ(reader.getDuration(), std::chrono::seconds(1));
        ASSERT_EQ(reader.getFirstTime(), std::chrono::seconds(1));
        ASSERT_EQ(reader.getLastTime(), std::chrono::seconds(2));
    }
}

GTEST_TEST(TestEmptyADTF2Streams, AdtfFileReader)
{
    {
        Reader reader(TEST_FILES_DIR "/adtf2_one_empty_stream.dat", StandardTypeDeserializers(), StandardSampleDeserializers());
        ASSERT_EQ(reader.getStreams().size(), 2);
        ASSERT_EQ(reader.getStreams()[0].item_count, 0);
        ASSERT_EQ(reader.getStreams()[1].item_count, 7);
    }
}

GTEST_TEST(TestSeekAndGetIndex, AdtfFileReader)
{
    Reader reader(TEST_FILES_DIR "/test_stop_signal.dat", StandardTypeDeserializers(), StandardSampleDeserializers());

    ASSERT_EQ(reader.getNextItemIndex(), 0);
    reader.getNextItem();
    ASSERT_EQ(reader.getNextItemIndex(), 1);
    auto time_stamp_1 = reader.getNextItem().time_stamp;
    ASSERT_EQ(reader.getNextItemIndex(), 2);

    reader.seekTo(10);

    ASSERT_EQ(reader.getNextItemIndex(), 10);
    reader.getNextItem();
    ASSERT_EQ(reader.getNextItemIndex(), 11);

    reader.seekTo(1);
    ASSERT_EQ(reader.getNextItemIndex(), 1);
    ASSERT_EQ(reader.getNextItem().time_stamp, time_stamp_1);
}

static constexpr const char* adtf2_core_media_type_cid = "adtf.core.media_type.adtf2_support.serialization.adtf.cid";
static constexpr const char* test_meta_type = "test_meta_type";

class TestTypeDeserializer: public adtf_file::StreamTypeDeserializer
{
    public:
        std::string getId() const override
        {
            return adtf2_core_media_type_cid;
        }

        void deserialize(InputStream& stream, PropertyStreamType& stream_type) const override
        {
            stream_type.setMetaType(test_meta_type);
        }
};

static constexpr const char* stream_error_message = "test stream empty";

class EmptyStream: public adtf_file::InputStream
{
    public:
        void read(void* destination, size_t count) override
        {
            throw std::runtime_error(stream_error_message);
        }
};

GTEST_TEST(TestOverrideTypeDeserializers, AdtfFileReader)
{
    StandardTypeDeserializers stream_type_deserializers;
    EmptyStream empty_stream;
    DefaultStreamType stream_type;

    std::string error_message;
    try
    {
        stream_type_deserializers.Deserialize(adtf2_core_media_type_cid, empty_stream, stream_type);
    }
    catch (const std::exception& error)
    {
        error_message = error.what();
    }

    ASSERT_EQ(error_message, stream_error_message);

    stream_type_deserializers.add(std::make_shared<TestTypeDeserializer>());
    stream_type_deserializers.Deserialize(adtf2_core_media_type_cid, empty_stream, stream_type);
    ASSERT_EQ(stream_type.getMetaType(), test_meta_type);
}

class TestSampleDeserializer: public SampleDeserializer
{
    public:
        static constexpr const char* id = "sample_copy_serialization.serialization.adtf.cid";

    public:
        void setStreamType(const StreamType& type) override
        {
        }
        void deserialize(ReadSample& sample, InputStream& stream) override
        {
        }
};

GTEST_TEST(TestOverrideSampleDeserializers, AdtfFileReader)
{
    StandardSampleDeserializers sample_deserializers;
    auto deserializer = sample_deserializers.build(adtf_file::adtf3::SampleCopyDeserializer::id);
    ASSERT_TRUE(std::dynamic_pointer_cast<adtf_file::adtf3::SampleCopyDeserializer>(deserializer));

    sample_deserializers.add(std::make_shared<adtf_file::sample_deserializer_factory<TestSampleDeserializer>>());
    deserializer = sample_deserializers.build(adtf_file::adtf3::SampleCopyDeserializer::id);
    ASSERT_TRUE(std::dynamic_pointer_cast<TestSampleDeserializer>(deserializer));
}
