/**
 * @file
 * adtf3 sample info serialization.
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
#include <adtf_file/adtf3/adtf3_sample_info.h>
#include <cstring>

namespace adtf_file
{

namespace adtf3
{

struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

static std::unordered_map<adtf_file::DataType, std::pair<HashedValueType, size_t>, EnumClassHash> serialization_type_map =
{
    {DataType::uint8, {HashedValueType::hvt_uint8, 1}},
    {DataType::int8, {HashedValueType::hvt_int8, 1}},
    {DataType::uint16, {HashedValueType::hvt_uint16, 2}},
    {DataType::int16, {HashedValueType::hvt_int16, 2}},
    {DataType::uint32, {HashedValueType::hvt_uint32, 4}},
    {DataType::int32, {HashedValueType::hvt_int32, 4}},
    {DataType::uint64, {HashedValueType::hvt_uint64, 8}},
    {DataType::int64, {HashedValueType::hvt_int64, 8}},
    {DataType::float32, {HashedValueType::hvt_float32, 4}},
    {DataType::float64, {HashedValueType::hvt_float64, 8}}
};

bool hasSampleInfo(const WriteSample& sample)
{
    bool has_info = false;
    auto raw_sample_info = dynamic_cast<const WriteRawSampleInfo*>(&sample);
    if (raw_sample_info)
    {
        raw_sample_info->getRawSampleInfo([&](const void* data, size_t data_size, uint8_t layout_version)
        {
            if (data_size)
            {
                has_info = true;
            }
        });
    }
    else
    {
        sample.iterateInfo([&](uint32_t key, DataType type, uint64_t raw_bytes)
        {
            has_info = true;
        });
    }

    return has_info;
}

void serializeSampleInfo(const WriteSample& sample, OutputStream& stream)
{
    auto raw_sample_info = dynamic_cast<const WriteRawSampleInfo*>(&sample);
    if (raw_sample_info)
    {
        raw_sample_info->getRawSampleInfo([&](const void* data, size_t data_size, uint8_t layout_version)
        {
            stream << layout_version << static_cast<uint32_t>(data_size);
            stream.write(data, data_size);
        });
    }
    else
    {
        stream << HashValueStorage::getVersion();
        std::vector<HashValueStorage> buffer;

        sample.iterateInfo([&](uint32_t key, DataType type, uint64_t raw_bytes)
        {
            HashValueStorage value;
            value.key = key;
            memcpy(value.storage, &raw_bytes, 8);
            value.type = serialization_type_map[type].first;
            value.byte_size = serialization_type_map[type].second;
            buffer.push_back(value);
        });

        uint32_t data_size = buffer.size() * sizeof(HashValueStorage);
        stream << data_size;
        stream.write(buffer.data(), data_size);
    }
}

static std::unordered_map<HashedValueType, DataType, EnumClassHash> deserialization_type_map =
{
    {HashedValueType::hvt_bool,  adtf_file::DataType::uint8},
    {HashedValueType::hvt_uint8,  adtf_file::DataType::uint8},
    {HashedValueType::hvt_int8,   adtf_file::DataType::int8},
    {HashedValueType::hvt_uint16, adtf_file::DataType::uint16},
    {HashedValueType::hvt_int16,  adtf_file::DataType::int16},
    {HashedValueType::hvt_uint32, adtf_file::DataType::uint32},
    {HashedValueType::hvt_int32,  adtf_file::DataType::int32},
    {HashedValueType::hvt_uint64, adtf_file::DataType::uint64},
    {HashedValueType::hvt_int64,  adtf_file::DataType::int64},
    {HashedValueType::hvt_float32, adtf_file::DataType::float32},
    {HashedValueType::hvt_float64, adtf_file::DataType::float64}
};

void deserializeSampleInfo(ReadSample& sample, InputStream& stream)
{
    uint8_t memory_layout_version = 0;
    uint32_t size_of_sample_info = 0;
    stream >> memory_layout_version >> size_of_sample_info;

    std::vector<uint8_t> buffer(size_of_sample_info);
    stream.read(buffer.data(), size_of_sample_info);

    auto raw_sample_info = dynamic_cast<ReadRawSampleInfo*>(&sample);
    if (raw_sample_info)
    {
        raw_sample_info->setRawSampleInfo(buffer.data(), buffer.size(), memory_layout_version);
    }
    else
    {
        if (memory_layout_version == HashValueStorage::getVersion())
        {
            for (auto current_value = reinterpret_cast<const HashValueStorage*>(buffer.data());
                 size_of_sample_info > 0; size_of_sample_info -= sizeof(HashValueStorage), ++current_value)
            {
                if (current_value->storage_version == HashValueStorage::getVersion())
                {
                    sample.addInfo(current_value->key,
                                   deserialization_type_map[current_value->type],
                                   *reinterpret_cast<const uint64_t*>(current_value->storage));
                }
            }
        }
    }
}

}
}
