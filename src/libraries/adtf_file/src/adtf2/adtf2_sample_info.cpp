/**
 * @file
 * adtf2 sample info serialization.
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

#include <adtf_file/adtf2/adtf2_sample_info.h>
#include <cstring>

namespace adtf_file
{

namespace adtf2
{

static const int adtf2_variant_version = 0x01;

template <typename T>
T get_value(const uint8_t* buffer, uint8_t byte_order)
{
    T value = *reinterpret_cast<const T*>(buffer);
    if (byte_order != static_cast<uint8_t>(ifhd::getCurrentPlatformByteorder()))
    {
        value = a_util::memory::swapEndianess(value);
    }
    return value;
}

std::pair<uint64_t, DataType> deserialize_adtf2_variant(const void* buffer,
                                                        size_t buffer_size)
{
    const uint8_t* read_position = static_cast<const uint8_t*>(buffer);

    uint8_t version = 0;
    if (sizeof(version) > buffer_size)
    {
        throw std::runtime_error("invalid sample info data");
    }
    version = *read_position;
    read_position += sizeof(version);
    buffer_size -= sizeof(version);

    if (version != adtf2_variant_version)
    {
        throw std::runtime_error("unsupported sample info data format");
    }

    uint8_t byte_order = 0;
    if (sizeof(byte_order) > buffer_size)
    {
        throw std::runtime_error("invalid sample info data");
    }
    byte_order = *read_position;
    read_position += sizeof(byte_order);
    buffer_size -= sizeof(byte_order);

    uint64_t data_type = 0;
    if (sizeof(data_type) > buffer_size)
    {
        throw std::runtime_error("invalid sample info data");
    }
    data_type = *reinterpret_cast<const uint64_t*>(read_position);
    if (byte_order != static_cast<uint8_t>(ifhd::getCurrentPlatformByteorder()))
    {
        data_type = a_util::memory::swapEndianess(data_type);
    }
    read_position += sizeof(data_type);
    buffer_size -= sizeof(data_type);

    //Read String
    // string is added with size in the beginning
    if ((data_type & VariantType::vt_string) != 0)
    {
        throw std::runtime_error("variant strings are not supported");
    }

    if (data_type & VariantType::vt_array)
    {
        throw std::runtime_error("variant arrays are not supported");
    }

    switch (data_type)
    {
        case VariantType::vt_bool:
        {
            return {get_value<uint8_t>(read_position, byte_order), DataType::uint8};
        }
        case VariantType::vt_int8:
        {
            return {get_value<int8_t>(read_position, byte_order), DataType::int8};
        }
        case VariantType::vt_u_int8:
        {
            return {get_value<uint8_t>(read_position, byte_order), DataType::uint8};
        }
        case VariantType::vt_int16:
        {
            return {get_value<int16_t>(read_position, byte_order), DataType::int16};
        }
        case VariantType::vt_u_int16:
        {
            return {get_value<uint16_t>(read_position, byte_order), DataType::uint16};
        }
        case VariantType::vt_int32:
        {
            return {get_value<int32_t>(read_position, byte_order), DataType::int32};
        }
        case VariantType::vt_u_int32:
        {
            return {get_value<uint32_t>(read_position, byte_order), DataType::uint32};
        }
        case VariantType::vt_int64:
        {
            return {get_value<int64_t>(read_position, byte_order), DataType::int64};
        }
        case VariantType::vt_u_int64:
        {
            return {get_value<uint64_t>(read_position, byte_order), DataType::uint64};
        }
        case VariantType::vt_float32:
        {
            float value = *reinterpret_cast<const float*>(read_position);
            if (byte_order != static_cast<uint8_t>(ifhd::getCurrentPlatformByteorder()))
            {
                uint32_t helper = a_util::memory::swapEndianess(*reinterpret_cast<uint32_t*>(&value));
                value = *reinterpret_cast<float*>(&helper);
            }
            return {value, DataType::float32};
        }
        case VariantType::vt_float64:
        {
            double value = *reinterpret_cast<const float*>(read_position);
            if (byte_order != static_cast<uint8_t>(ifhd::getCurrentPlatformByteorder()))
            {
                uint32_t helper = a_util::memory::swapEndianess(*reinterpret_cast<uint32_t*>(&value));
                value = *reinterpret_cast<double*>(&helper);
            }
            return {value, DataType::float64};
        }
        default:
        {
            throw std::runtime_error("unsupported variant type");
        }
    }
}

void deserializeMediaSampleInfo(ReadSample& sample, InputStream& stream)
{
    uint32_t map_size = 0;
    stream >> map_size;
    if (map_size > 0)
    {
        std::vector<uint8_t> buffer;
        for (;map_size > 0; --map_size)
        {
            uint32_t value_index = 0;
            stream >> value_index;
            uint32_t variant_size = 0;
            stream >> variant_size;
            buffer.resize(variant_size);
            stream.read(buffer.data(), variant_size);

            try
            {
                auto value = deserialize_adtf2_variant(buffer.data(), buffer.size());
                sample.addInfo(createAdtf2SampleInfoHashKey(value_index), value.second, value.first);
            }
            catch (...)
            {
            }
        }
    }
}

void deserializeMediaSampleLogTrace(ReadSample& sample, InputStream& stream)
{
    uint8_t sample_log_trace_present = 0;
    stream >> sample_log_trace_present;
    if (sample_log_trace_present > 0)
    {
        throw std::runtime_error("Sample Log Traces from ADTF 2 are not supported");
    }
}

}

}
