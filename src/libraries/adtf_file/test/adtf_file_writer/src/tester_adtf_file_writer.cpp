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
#include <adtf_file/adtf_file_writer.h>
#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/standard_factories.h>

using namespace adtf_file;

class TestFile
{
    public:
        TestFile(const std::string& file_name)
        {
            Reader reader(file_name, StandardTypeDeserializers(), StandardSampleDeserializers());
            std::unordered_map<size_t, std::string> stream_map;
            for (auto& stream: reader.getStreams())
            {
                stream_map[stream.stream_id] = stream.name;
                streams[stream.name].initial_type = std::dynamic_pointer_cast<const PropertyStreamType>(stream.initial_type);
            }

            try
            {
                for(;;)
                {
                    auto item = reader.getNextItem();
                    auto& stream = streams[stream_map[item.stream_id]];
                    auto type = std::dynamic_pointer_cast<const PropertyStreamType>(item.stream_item);
                    auto sample = std::dynamic_pointer_cast<const DefaultSample>(item.stream_item);
                    if (type)
                    {
                        stream.types.push_back(type);
                    }
                    else if (sample)
                    {
                        stream.sample_timestamps.push_back(item.time_stamp.count());
                        stream.samples.push_back(sample);
                    }
                }
            }
            catch (const exceptions::EndOfFile&)
            {
            }

        }

    public:
        class Stream
        {
            public:
                std::shared_ptr<const PropertyStreamType> initial_type;
                std::vector<std::shared_ptr<const DefaultSample>> samples;
                std::vector<int64_t> sample_timestamps;
                std::vector<std::shared_ptr<const PropertyStreamType>> types;
        };

        std::unordered_map<std::string, Stream> streams;
};

void check_property(std::shared_ptr<const PropertyStreamType>& stream_type, const std::string& name, const std::string& value)
{
    ASSERT_EQ(stream_type->getProperty(name).second, value);
}

GTEST_TEST(TestWriteADTF3, AdtfFileWriter)
{    
    {
        Writer writer(TEST_FILES_DIR "/test_adtf3.dat", std::chrono::nanoseconds(0), adtf3::StandardTypeSerializers());

        DefaultStreamType stream_type("adtf/anonymous");
        auto stream_id = writer.createStream("test", stream_type, std::make_shared<adtf3::SampleCopySerializerNs>());
        EXPECT_GT(stream_id, 0);
        for (size_t sample_index = 0; sample_index < 10; ++sample_index)
        {
            DefaultSample sample;
            sample.setTimeStamp(std::chrono::nanoseconds(sample_index));
            sample.setContent(sample_index);

            writer.write(stream_id, std::chrono::nanoseconds(sample_index), sample);
            writer.writeTrigger(stream_id, std::chrono::nanoseconds(sample_index));
        }

        writer.write(stream_id, std::chrono::nanoseconds(10), stream_type);
    }

    TestFile file(TEST_FILES_DIR "/test_adtf3.dat");
    ASSERT_EQ(file.streams.size(), 1);
    auto& stream = file.streams["test"];
    ASSERT_EQ(stream.samples.size(), 10);
    std::vector<int64_t> expected_timestamps{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    ASSERT_EQ(stream.sample_timestamps, expected_timestamps);

    ASSERT_EQ(stream.types.size(), 1);
}

GTEST_TEST(TestWriteADTF2, AdtfFileWriter)
{
    {
        Writer writer(TEST_FILES_DIR "/test_adtf2.dat", std::chrono::nanoseconds(0), adtf2::StandardTypeSerializers(), Writer::adtf2);

        DefaultStreamType stream_type("adtf2/legacy");
        stream_type.setProperty("major", "tInt32", "1");
        stream_type.setProperty("sub", "tInt32", "2");
        stream_type.setProperty("flags", "tInt32", "3");

        auto stream_id = writer.createStream("test", stream_type, std::make_shared<adtf2::AdtfCoreMediaSampleSerializer>());
        EXPECT_GT(stream_id, 0);

        for (size_t sample_index = 0; sample_index < 10; ++ sample_index)
        {
            DefaultSample sample;
            sample.setTimeStamp(std::chrono::microseconds(sample_index));
            sample.setContent(sample_index);

            writer.write(stream_id, std::chrono::microseconds(sample_index), sample);
        }
    }

    TestFile file(TEST_FILES_DIR "/test_adtf2.dat");
    ASSERT_EQ(file.streams.size(), 1);
    auto& stream = file.streams["test"];
    ASSERT_EQ(stream.samples.size(), 10);
    std::vector<int64_t> expected_timestamps{0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000};
    ASSERT_EQ(stream.sample_timestamps, expected_timestamps);
    ASSERT_EQ(stream.types.size(), 0);
    check_property(stream.initial_type, "major", "1");
    check_property(stream.initial_type, "sub", "2");
    check_property(stream.initial_type, "flags", "3");
}

void write_samples(Writer& writer, size_t stream_id, std::chrono::nanoseconds from, std::chrono::nanoseconds to, std::chrono::nanoseconds interval)
{
    for (std::chrono::nanoseconds time_stamp = from; time_stamp <= to; time_stamp += interval)
    {
        DefaultSample sample;
        sample.setTimeStamp(time_stamp);
        sample.beginBufferWrite(1024);
        sample.endBufferWrite();

        writer.write(stream_id, time_stamp, sample);
        writer.writeTrigger(stream_id, time_stamp);
    }
}

GTEST_TEST(TestHistoryADTF3, AdtfFileWriter)
{
    {
        Writer writer(TEST_FILES_DIR "/test_history_adtf3.dat", std::chrono::seconds(1), adtf3::StandardTypeSerializers());

        DefaultStreamType stream_type("adtf/anonymous");
        stream_type.setProperty("counter", "unsigned int", "0"); // this one will be dropped completely

        auto stream_id = writer.createStream("test", stream_type, std::make_shared<adtf3::SampleCopySerializerNs>());
        EXPECT_GT(stream_id, 0);

        write_samples(writer, stream_id, std::chrono::milliseconds(0), std::chrono::milliseconds(1000), std::chrono::milliseconds(100));

        stream_type.setProperty("counter", "tUInt", "1"); // this one will become the initial type of the stream
        writer.write(stream_id, std::chrono::milliseconds(1100), stream_type);

        write_samples(writer, stream_id, std::chrono::milliseconds(1100), std::chrono::milliseconds(1500), std::chrono::milliseconds(100));

        stream_type.setProperty("counter", "tUInt", "2"); // this will be the first within the streaming data
        writer.write(stream_id, std::chrono::milliseconds(1600), stream_type);

        write_samples(writer, stream_id, std::chrono::milliseconds(1600), std::chrono::milliseconds(2500), std::chrono::milliseconds(100));

        writer.quitHistory();

        stream_type.setProperty("counter", "tUInt", "3");
        writer.write(stream_id, std::chrono::milliseconds(2600), stream_type);

        write_samples(writer, stream_id, std::chrono::milliseconds(2600), std::chrono::milliseconds(3500), std::chrono::milliseconds(100));
    }

    TestFile file(TEST_FILES_DIR "/test_history_adtf3.dat");
    ASSERT_EQ(file.streams.size(), 1);
    auto& stream = file.streams["test"];

    check_property(stream.initial_type, "counter", "1");

    std::vector<std::string> type_counter_values;
    std::transform(stream.types.begin(), stream.types.end(), std::back_inserter(type_counter_values), [](const std::shared_ptr<const PropertyStreamType>& type)
    {
        return type->getProperty("counter").second;
    });

    std::vector<std::string> expected = {"2", "3"};
    ASSERT_EQ(type_counter_values, expected);

    ASSERT_EQ(stream.sample_timestamps.front(), 1500000000);
    ASSERT_EQ(stream.sample_timestamps.back(), 3500000000);
}

void check_media_description_serialization(const std::string& file_name,
                                           const std::vector<int64_t>& expected_timestamps,
                                           size_t type_count = 0)
{
    TestFile file(file_name);
    ASSERT_EQ(file.streams.size(), 1);
    auto& stream = file.streams["test"];
    ASSERT_EQ(stream.samples.size(), 10);
    ASSERT_EQ(stream.sample_timestamps, expected_timestamps);
    ASSERT_EQ(stream.types.size(), type_count);
    ASSERT_EQ(stream.samples.size(), 10);

    uint32_t sample_index = 0;
    for (auto& sample: stream.samples)
    {
        ASSERT_EQ(sample->getContent<uint32_t>(), sample_index++);
    }
}

GTEST_TEST(TestWriteADTF3MediaDescriptionSerialization, AdtfFileWriter)
{
    {
        Writer writer(TEST_FILES_DIR "/test_adtf3.dat", std::chrono::nanoseconds(0), adtf3::StandardTypeSerializers());

        DefaultStreamType stream_type("adtf/anonymous");
        ASSERT_THROW(writer.createStream("test", stream_type, std::make_shared<adtf3::MediaDescriptionSerializerNs>()), std::runtime_error);

        stream_type.setProperty("md_struct", "cString", "test");
        stream_type.setProperty("md_definitions", "cString", R"(
                                <struct name="test" version="1">
                                <element name="sample_index" type="tUInt32" arraysize="1" alignment="1">
                                    <serialized bytepos="0" byteorder="LE"/>
                                    <deserialized alignment="1"/>
                                </element>
                                </struct>)");
        auto stream_id = writer.createStream("test", stream_type, std::make_shared<adtf3::MediaDescriptionSerializerNs>());

        ASSERT_GT(stream_id, 0);
        for (uint32_t sample_index = 0; sample_index < 10; ++sample_index)
        {
            DefaultSample sample;
            sample.setTimeStamp(std::chrono::nanoseconds(sample_index));
            sample.setContent(sample_index);

            writer.write(stream_id, std::chrono::nanoseconds(sample_index), sample);
            writer.writeTrigger(stream_id, std::chrono::nanoseconds(sample_index));
        }

        writer.write(stream_id, std::chrono::nanoseconds(10), stream_type);
    }

    check_media_description_serialization(TEST_FILES_DIR "/test_adtf3.dat",
                                          {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                                          1);
}

GTEST_TEST(TestWriteADTF2MediaDescriptionSerialization, AdtfFileWriter)
{
    {
        Writer writer(TEST_FILES_DIR "/test_adtf2.dat", std::chrono::nanoseconds(0), adtf3::StandardTypeSerializers(), Writer::adtf2);

        DefaultStreamType stream_type("adtf2/legacy");
        stream_type.setProperty("major", "tInt", "0");
        stream_type.setProperty("sub", "tInt", "0");
        stream_type.setProperty("flags", "tInt", "0");
        stream_type.setProperty("md_struct", "cString", "test");
        stream_type.setProperty("md_definitions", "cString", R"(
                                <struct name="test" version="1">
                                    <element name="sample_index" type="tUInt32" alignment="1" arraysize="1" alignment="1" bytepos="0" byteorder="LE"/>
                                </struct>)");
        auto stream_id = writer.createStream("test", stream_type, std::make_shared<adtf2::AdtfCoreMediaSampleSerializer>());

        ASSERT_GT(stream_id, 0);
        for (uint32_t sample_index = 0; sample_index < 10; ++sample_index)
        {
            DefaultSample sample;
            sample.setTimeStamp(std::chrono::microseconds(sample_index));
            sample.setContent(sample_index);

            writer.write(stream_id, std::chrono::microseconds(sample_index), sample);
        }
    }

    check_media_description_serialization(TEST_FILES_DIR "/test_adtf2.dat",
                                          {0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000});
}

GTEST_TEST(TestInitialType, AdtfFileWriter)
{
    {
        Writer writer(TEST_FILES_DIR "/test_initial_adtf3.dat", std::chrono::seconds(0), adtf3::StandardTypeSerializers());

        DefaultStreamType stream_type("adtf/anonymous");
        stream_type.setProperty("counter", "tUInt", "0"); // this one should be dropped completely

        auto stream_id = writer.createStream("test", stream_type, std::make_shared<adtf3::SampleCopySerializerNs>());
        EXPECT_GT(stream_id, 0);

        stream_type.setProperty("counter", "tUInt", "1"); // this one should also be dropped
        writer.write(stream_id, std::chrono::milliseconds(1), stream_type);

        stream_type.setProperty("counter", "tUInt", "2"); // and this should become the initial type of the stream
        writer.write(stream_id, std::chrono::milliseconds(2), stream_type);

        write_samples(writer, stream_id, std::chrono::milliseconds(1600), std::chrono::milliseconds(2500), std::chrono::milliseconds(100));
    }

    TestFile file(TEST_FILES_DIR "/test_initial_adtf3.dat");
    ASSERT_EQ(file.streams.size(), 1);
    auto& stream = file.streams["test"];

    check_property(stream.initial_type, "counter", "2");
}
