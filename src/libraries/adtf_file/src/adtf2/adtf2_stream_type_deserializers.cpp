/**
 * @file
 * adtf2 mediatype deserializer.
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

#include <adtf_file/adtf2/adtf2_stream_type_deserializers.h>
#include <adtf_file/adtf2/legacy_types.h>

namespace adtf_file
{

namespace adtf2
{

enum LegacyTypes
{
    media_type_structured_data                 = 0x0300
};

enum LegacyStructTypes
{
    media_subtype_struct_float_64               = 0x0001,
    media_subtype_struct_float_32               = 0x0002,
    media_subtype_struct_uint_32                = 0x0003,
    media_subtype_struct_uint_16                = 0x0004,
    media_subtype_struct_uint_8                 = 0x0005,
    media_subtype_struct_uint_64                = 0x0007
};

std::string AdtfCoreMediaTypeDeserializer::getId() const
{
    return "adtf.core.media_type.adtf2_support.serialization.adtf.cid";
}

void create_plain_stream_type(PropertyStreamType& stream_type, const std::string& type)
{
    stream_type.setMetaType("adtf/plaintype");
    stream_type.setProperty("c-type", "cString", type);
}

void AdtfCoreMediaTypeDeserializer::deserialize(InputStream& stream, PropertyStreamType& stream_type) const
{
    uint32_t major_type;
    uint32_t sub_type;
    uint32_t flags;
    stream >> major_type >> sub_type >> flags;

    switch (major_type)
    {
        case media_type_structured_data:
        {
            switch (sub_type)
            {
                case media_subtype_struct_uint_8:
                {
                    return create_plain_stream_type(stream_type, "tUInt8");
                }
                case media_subtype_struct_uint_16:
                {
                    return create_plain_stream_type(stream_type,"tUInt16");
                }
                case media_subtype_struct_uint_32:
                {
                    return create_plain_stream_type(stream_type,"tUInt32");
                }
                case media_subtype_struct_uint_64:
                {
                    return create_plain_stream_type(stream_type,"tUInt64");
                }
                case media_subtype_struct_float_32:
                {
                    return create_plain_stream_type(stream_type,"tFloat32");
                }
                case media_subtype_struct_float_64:
                {
                    return create_plain_stream_type(stream_type,"tFloat64");
                }
            }
        }

        default:
        {
            stream_type.setMetaType("adtf2/legacy");
            stream_type.setProperty("major", "tUInt32", std::to_string(major_type));
            stream_type.setProperty("sub", "tUInt32", std::to_string(sub_type));
            stream_type.setProperty("flags", "tUInt32", std::to_string(flags));
        }
    }
}

std::string AdtfCoreMediaTypeVideoDeserializer::getId() const
{
    return "adtf.type.video.adtf2_support.serialization.adtf.cid";
}


#define FORMAT_CONVERSION_CASE(__format, __newformat__) \
    case PixelFormats::pf_##__format:\
    {\
        if (__newformat__ != nullptr) \
        { \
            format_name = __newformat__; \
        } \
        else \
        { \
            format_name = #__format; \
        }\
        break;\
    }

void AdtfCoreMediaTypeVideoDeserializer::deserialize(InputStream& stream, PropertyStreamType& stream_type) const
{
    uint32_t major_type;
    uint32_t sub_type;
    uint32_t flags;
    stream >> major_type >> sub_type >> flags;

    BitmapFormat format;
    stream >> format.bits_per_pixel
           >> format.bytes_per_line
           >> format.height
           >> format.palette_size
           >> format.pixel_format
           >> format.size
           >> format.width;

    std::vector<PaletteEntry> palette;
    for (int32_t entry_index = 0; entry_index < format.palette_size; ++entry_index)
    {
        PaletteEntry entry;
        stream >> entry.red
               >> entry.green
               >> entry.blue
               >> entry.alpha;
        palette.push_back(entry);
    }

    stream_type.setMetaType("adtf/image");
    stream_type.setProperty("max_byte_size", "tUInt32", std::to_string(format.size));
    stream_type.setProperty("pixel_width", "tUInt", std::to_string(format.width));
    stream_type.setProperty("pixel_height", "tUInt", std::to_string(format.height));
    stream_type.setProperty("data_endianess", "tUInt", std::to_string(static_cast<uint8_t>(ifhd::Endianess::platform_little_endian)));

    // optional properties
    stream_type.setProperty("bits_per_pixel", "tUInt", std::to_string(format.bits_per_pixel));
    stream_type.setProperty("bytes_per_line", "tUInt", std::to_string(format.bytes_per_line));

    std::string format_name;
    switch (format.pixel_format)
    {
        FORMAT_CONVERSION_CASE(unknown, nullptr)
        FORMAT_CONVERSION_CASE(8_bit, nullptr)
        FORMAT_CONVERSION_CASE(greyscale_8, "GREY(8)")
        FORMAT_CONVERSION_CASE(rgb_8, "R(3)G(3)B(2)")
        FORMAT_CONVERSION_CASE(16_bit, nullptr)
        FORMAT_CONVERSION_CASE(greyscale_10, "GREY(10)")
        FORMAT_CONVERSION_CASE(greyscale_12, "GREY(12)")
        FORMAT_CONVERSION_CASE(greyscale_14, "GREY(14)")
        FORMAT_CONVERSION_CASE(greyscale_16, "GREY(16)")
        FORMAT_CONVERSION_CASE(rgb_444, "R(4)G(4)B(4)")
        FORMAT_CONVERSION_CASE(rgb_555, "R(5)G(5)B(5)(1)")
        FORMAT_CONVERSION_CASE(rgb_565, "R(5)G(6)B(5)")
        FORMAT_CONVERSION_CASE(rgba_4444, "R(4)G(4)B(4)A(4)")
        FORMAT_CONVERSION_CASE(abgr_4444, "A(4)B(4)G(4)R(4)")
        FORMAT_CONVERSION_CASE(riii_10, nullptr)
        FORMAT_CONVERSION_CASE(riii_12, nullptr)
        FORMAT_CONVERSION_CASE(riii_14, nullptr)
        FORMAT_CONVERSION_CASE(riii_16, nullptr)
        FORMAT_CONVERSION_CASE(bgr_555, "(1)B(5)G(5)R(5)")
        FORMAT_CONVERSION_CASE(bgr_565, "B(5)G(6)R(5)")
        FORMAT_CONVERSION_CASE(24_bit, nullptr)
        FORMAT_CONVERSION_CASE(greyscale_18, "GREY(18)")
        FORMAT_CONVERSION_CASE(greyscale_20, "GREY(20)")
        FORMAT_CONVERSION_CASE(greyscale_22, "GREY(22)")
        FORMAT_CONVERSION_CASE(greyscale_24, "GREY(24)")
        FORMAT_CONVERSION_CASE(rgb_888, "R(8)G(8)B(8)")
        FORMAT_CONVERSION_CASE(bgr_888, "B(8)G(8)R(8)")
        FORMAT_CONVERSION_CASE(32_bit, nullptr)
        FORMAT_CONVERSION_CASE(argb_8888, "A(8)R(8)G(8)B(8)")
        FORMAT_CONVERSION_CASE(abgr_8888, "A(8)B(8)G(8)R(8)")
        FORMAT_CONVERSION_CASE(rgba_8888, "R(8)G(8)B(8)A(8)")
        FORMAT_CONVERSION_CASE(bgra_8888, "B(8)G(8)R(8)A(8)")
        FORMAT_CONVERSION_CASE(greyscale_32, "GREY(32)")
        FORMAT_CONVERSION_CASE(greyscale_float_32, nullptr)
        FORMAT_CONVERSION_CASE(yuv420p_888, nullptr)
        default:
        {
            format_name = std::to_string(format.pixel_format);
            break;
        }
    }

    stream_type.setProperty("format_name", "cString", format_name);

    stream_type.setProperty("major", "tUInt32", std::to_string(major_type));
    stream_type.setProperty("sub", "tUInt32", std::to_string(sub_type));
    stream_type.setProperty("flags", "tUInt32", std::to_string(flags));
}

std::string AdtfCoreMediaTypeAudioDeserializer::getId() const
{
    return "adtf.type.audio.adtf2_support.serialization.adtf.cid";
}

void AdtfCoreMediaTypeAudioDeserializer::deserialize(InputStream& stream, PropertyStreamType& stream_type) const
{
    uint32_t major_type;
    uint32_t sub_type;
    uint32_t flags;
    stream >> major_type >> sub_type >> flags;

    AudioFormat format;
    stream >> format.format_type
           >> format.channels
           >> format.samples_per_sec
           >> format.bits_per_sample
           >> format.num_samples
           >> format.size;

    stream_type.setMetaType("adtf/audio");
    stream_type.setProperty("format_name", "cString", "PCM");
    stream_type.setProperty("channel_count", "tUInt", std::to_string(format.channels));
    stream_type.setProperty("sample_rate_hz", "tUInt", std::to_string(format.samples_per_sec));
    stream_type.setProperty("bits_per_sample", "tUInt", std::to_string(format.bits_per_sample));
    stream_type.setProperty("sample_count", "tUInt", std::to_string(format.num_samples));
}

std::string IndexedDataDescriptionMediaTypeDeserializer::getId() const
{
    return "indexed_data_toolbox.indexed_data_description_media_type.adtf2_support.serialization.adtf.cid";
}

void IndexedDataDescriptionMediaTypeDeserializer::deserialize(adtf_file::InputStream& stream, adtf_file::PropertyStreamType& stream_type) const
{
    uint32_t major_type;
    uint32_t sub_type;
    uint32_t flags;
    stream >> major_type >> sub_type >> flags;

    uint32_t count = 0;
    stream >> count;

    stream_type.setMetaType("adtf/substreams");
    stream_type.setProperty("substreams", "tBool", "true");

    for (uint32_t data_index = 0; data_index < count; ++data_index)
    {
        uint32_t data_id = 0;
        stream >> data_id;
        uint32_t string_buffer_size;
        stream >> string_buffer_size;
        std::string name(string_buffer_size, '\0');
        stream.read(&name.front(), string_buffer_size);
        name.pop_back();

        stream >> string_buffer_size;
        std::string media_description(string_buffer_size, '\0');
        stream.read(&media_description.front(), string_buffer_size);
        media_description.pop_back();

        auto prefix = "substreams/" + name;
        stream_type.setProperty(prefix, "tUInt32", std::to_string(data_id));
        prefix += "/type";
        stream_type.setProperty(prefix, "cString", "adft/default");
        stream_type.setProperty(prefix + "/md_struct", "cString", name);
        stream_type.setProperty(prefix + "/md_definitions", "cString", media_description);
    }
}


}

}
