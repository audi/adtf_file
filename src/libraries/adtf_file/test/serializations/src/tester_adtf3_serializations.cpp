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
#include <adtf_file/adtf3/adtf3_sample_info.h>

using namespace adtf_file;

static uint32_t test_array[10] = {0,1,2,3,4,5,6,7,8,9};
const char* test_array_desc =
"<?xml version=\"1.0\" encoding=\"iso-8859-1\" standalone=\"no\"?>"
"<structs>"
"<struct alignment=\"4\" name=\"testarray\" version=\"2\">"
"<element alignment=\"1\" arraysize=\"5\" byteorder=\"LE\" bytepos=\"0\" name=\"values1\" type=\"tUInt32\"/>"
"<element alignment=\"1\" arraysize=\"5\" byteorder=\"LE\" bytepos=\"20\" name=\"values2\" type=\"tUInt32\"/>"
"</struct>"
"</structs>";

void check_property(std::shared_ptr<const PropertyStreamType>& stream_type, const std::string& name, const std::string& value)
{
    ASSERT_EQ(stream_type->getProperty(name).second, value);
}

std::shared_ptr<StreamType> test_create_sample_streamtype()
{
    auto stream_type = std::make_shared<DefaultStreamType>();
    stream_type->setProperty("md_struct", "cString", "testarray");
    stream_type->setProperty("md_definitions", "cString", test_array_desc);
    return std::dynamic_pointer_cast<StreamType>(stream_type);
}

void test_create_sample(ReadSample& sample)
{
    sample.setTimeStamp(std::chrono::microseconds(1000));
    memcpy(sample.beginBufferWrite(sizeof(test_array)), static_cast<const void*>(&test_array[0]), sizeof(test_array));
    sample.endBufferWrite();
    sample.setFlags(0x2);
}

size_t test_create_sample_size_expected()
{
     return sizeof(int64_t) + sizeof(uint32_t) + sizeof(test_array) + sizeof(uint64_t);
}

size_t test_create_sample_substream_size_expected()
{
     return sizeof(int64_t) + sizeof(uint32_t) + sizeof(test_array) + sizeof(uint64_t) + sizeof(uint32_t);
}


void test_check_sample(const WriteSample& sample)
{
    ASSERT_EQ(sample.getTimeStamp(), std::chrono::microseconds(1000));
    ASSERT_EQ(sample.getFlags(), 0x2);
    auto buffer = sample.beginBufferRead();
    ASSERT_TRUE(buffer.first);
    ASSERT_EQ(buffer.second, sizeof(test_array));
    ASSERT_EQ(memcmp(static_cast<const void*>(test_array), buffer.first, sizeof(test_array)),0);
}

void test_create_sample_info(ReadSample& sample)
{
    float test_float32 = 4.0;
    sample.addInfo(createSampleInfoHashKey("float32"), DataType::float32, *reinterpret_cast<const uint32_t*>(&test_float32));
    double test_float64 = 200.0;
    sample.addInfo(createSampleInfoHashKey("float64"), DataType::float64, *reinterpret_cast<const uint64_t*>(&test_float64));
    sample.addInfo(createSampleInfoHashKey("uint64"), DataType::uint64, 1234567890123456ull);
    sample.addInfo(createSampleInfoHashKey("timestamp"), DataType::int64, 11111111999999999ll);
}

void test_check_sample_info(WriteSample& sample)
{
    float test_float32 = 0.0;
    double test_float64 = 0.0;
    uint64_t test_uint64 = 0;
    int64_t test_timestamp = 0;
    sample.iterateInfo([&](uint32_t key, DataType type, uint64_t raw_value)
    {
        if (key == createSampleInfoHashKey("float32"))
        {
            test_float32 = *reinterpret_cast<const float*>(&raw_value);
        }
        else if (key == createSampleInfoHashKey("float64"))
        {
            test_float64 = *reinterpret_cast<const double*>(&raw_value);
        }
        else if (key == createSampleInfoHashKey("uint64"))
        {
            test_uint64 = raw_value;
        }
        else if (key == createSampleInfoHashKey("timestamp"))
        {
            test_timestamp = static_cast<int64_t>(raw_value);
        }

    });

    ASSERT_EQ(test_float32, 4.0);
    ASSERT_EQ(test_float64, 200.0);
    ASSERT_EQ(test_uint64, 1234567890123456ull);
    ASSERT_EQ(test_timestamp, 11111111999999999ll);
}

size_t test_create_sample_size_expected_with_info()
{
    //4 values a 16 byte + size information in 32Bit + memorylayout version (uint8)
    // If changing this YOU REALLY HAVE ALSO an action within serialization and raise the version of hash_value_map<> !!!!
    return test_create_sample_size_expected() + (4 * sizeof(adtf3::HashValueStorage)) + sizeof(uint32_t) + sizeof(uint8_t);
}

class SerializationBuffer: public OutputStream, public InputStream, public std::deque<uint8_t>
{
    public:
        void read(void* destination, size_t count) override
        {
            std::copy(begin(), begin() + count, static_cast<uint8_t*>(destination));
            erase(begin(), begin() + count);
        }

        void write(const void* data, size_t data_size) override
        {
            insert(end(), static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + data_size);
        }
};

void test_check_serialisation(const std::string& serialisation_class_id, const std::string& deserialisation_class_id)
{
    auto serialization = StandardSampleSerializers().build(serialisation_class_id);
    auto deserialization = StandardSampleDeserializers().build(deserialisation_class_id);

    auto type = test_create_sample_streamtype();

    serialization->setStreamType(*type);
    deserialization->setStreamType(*type);

    DefaultSample write_sample;
    test_create_sample(write_sample);

    SerializationBuffer buffer;
    serialization->serialize(write_sample, buffer);

    ASSERT_EQ(buffer.size(), test_create_sample_size_expected());

    DefaultSample read_sample;
    deserialization->deserialize(read_sample, buffer);

    test_check_sample(read_sample);
}

void test_check_serialisation_with_sample_info(const std::string& serialisation_class_id, const std::string& deserialisation_class_id)
{
    auto serialization = StandardSampleSerializers().build(serialisation_class_id);
    auto deserialization = StandardSampleDeserializers().build(deserialisation_class_id);

    auto type = test_create_sample_streamtype();

    serialization->setStreamType(*type);
    deserialization->setStreamType(*type);

    DefaultSample write_sample;
    test_create_sample(write_sample);
    test_create_sample_info(write_sample);

    SerializationBuffer buffer;
    serialization->serialize(write_sample, buffer);

    ASSERT_EQ(buffer.size(), test_create_sample_size_expected_with_info());

    DefaultSample read_sample;
    deserialization->deserialize(read_sample, buffer);

    test_check_sample(read_sample);
    test_check_sample_info(read_sample);
}

void test_check_serialisation_substream_id(const std::string& serialisation_class_id, const std::string& deserialisation_class_id)
{
    auto serialization = StandardSampleSerializers().build(serialisation_class_id);
    auto deserialization = StandardSampleDeserializers().build(deserialisation_class_id);

    auto type = test_create_sample_streamtype();

    serialization->setStreamType(*type);
    deserialization->setStreamType(*type);

    DefaultSample write_sample;
    test_create_sample(write_sample);
    write_sample.setSubStreamId(123);

    SerializationBuffer buffer;
    serialization->serialize(write_sample, buffer);

    ASSERT_EQ(buffer.size(), test_create_sample_substream_size_expected());

    DefaultSample read_sample;
    deserialization->deserialize(read_sample, buffer);

    test_check_sample(read_sample);
    ASSERT_EQ(read_sample.getSubStreamId(), 123);
}


/**
****************************************************************************************************************
*/

GTEST_TEST(TestSerialization, ADTF3CopySerialization)
{
    test_check_serialisation(adtf_file::adtf3::SampleCopySerializer::id,
                             adtf_file::adtf3::SampleCopyDeserializer::id);

    test_check_serialisation(adtf_file::adtf3::SampleCopySerializerNs::id,
                             adtf_file::adtf3::SampleCopyDeserializerNs::id);
}

GTEST_TEST(TestSerializationSampleInfo, ADTF3CopySerialization)
{
    test_check_serialisation_with_sample_info(adtf_file::adtf3::SampleCopySerializer::id,
                                              adtf_file::adtf3::SampleCopyDeserializer::id);

    test_check_serialisation_with_sample_info(adtf_file::adtf3::SampleCopySerializerNs::id,
                                              adtf_file::adtf3::SampleCopyDeserializerNs::id);
}

GTEST_TEST(TestSerialization, ADTF3MediaDescriptionSerialization)
{
    test_check_serialisation(adtf_file::adtf3::MediaDescriptionSerializer::id,
                             adtf_file::adtf3::MediaDescriptionDeserializer::id);
}

GTEST_TEST(TestSerializationSampleInfo, ADTF3MediaDescriptionSerialization)
{
    test_check_serialisation_with_sample_info(adtf_file::adtf3::MediaDescriptionSerializer::id,
                                              adtf_file::adtf3::MediaDescriptionDeserializer::id);
}

GTEST_TEST(TestSerializationSubStreamId, ADTF3CopySerialization)
{
    test_check_serialisation_substream_id(adtf_file::adtf3::SampleCopySerializerNs::id,
                                          adtf_file::adtf3::SampleCopyDeserializerNs::id);
}

