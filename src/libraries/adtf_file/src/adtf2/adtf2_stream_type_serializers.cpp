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

#include <adtf_file/adtf2/adtf2_stream_type_serializers.h>
#include <adtf_file/adtf2/legacy_types.h>

namespace adtf_file
{
namespace adtf2
{

std::string AdtfCoreMediaTypeSerializer::getMetaType() const
{
    return "adtf2/legacy";
}

std::string get_property_with_default(const PropertyStreamType& stream_type, const std::string& name, const std::string& default_value)
{
    try
    {
        return stream_type.getProperty(name).second;
    }
    catch (...)
    {
        return default_value;
    }
}

void AdtfCoreMediaTypeSerializer::serialize(const StreamType& stream_type, OutputStream& stream) const
{
    auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);

    uint32_t major_type = std::stoul(get_property_with_default(property_type, "major", "0"), 0, 0);
    uint32_t sub_type = std::stoul(get_property_with_default(property_type, "sub", "0"), 0, 0);
    uint32_t flags = std::stoul(get_property_with_default(property_type, "flags", "0"), 0, 0);
    stream << major_type << sub_type << flags;
}

std::string AdtfCoreMediaTypeSerializer::getTypeId() const
{
    return "adtf.core.media_type.adtf2_support.serialization.adtf.cid";
}

std::string AdtfCoreMediaTypeVideoSerializer::getMetaType() const
{
    return "adtf/image";
}

#define MEDIA_TYPE_VIDEO                            "0x0100"
#define MEDIA_SUBTYPE_VIDEO_UNCOMPRESSED            "0x0000"

#define FORMAT_CONVERSION_CASE(__format, __newformat__, __bits_per_pixel) \
    else if (__newformat__ == format_name)\
    {\
        format.pixel_format = PixelFormats::pf_##__format; \
        format.bits_per_pixel = __bits_per_pixel; \
    }

void AdtfCoreMediaTypeVideoSerializer::serialize(const StreamType& stream_type, OutputStream& stream) const
{
    auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);

    uint32_t major_type = std::stoul(get_property_with_default(property_type, "major", MEDIA_TYPE_VIDEO), 0, 0);
    uint32_t sub_type = std::stoul(get_property_with_default(property_type, "sub", MEDIA_SUBTYPE_VIDEO_UNCOMPRESSED), 0, 0);
    uint32_t flags = std::stoul(get_property_with_default(property_type, "flags", "0"), 0, 0);
    stream << major_type << sub_type << flags;

    BitmapFormat format;

    format.size = std::stoul(get_property_with_default(property_type, "max_byte_size", "0"), 0, 0);
    format.width = std::stoul(get_property_with_default(property_type, "pixel_width", "0"), 0, 0);
    format.height = std::stoul(get_property_with_default(property_type, "pixel_height", "0"), 0, 0);
    format.size = std::stoul(get_property_with_default(property_type, "max_byte_size", "0"), 0, 0);
    format.palette_size = 0;

    // optional properties
    format.bytes_per_line = static_cast<int32_t>(std::stoul(get_property_with_default(property_type, "bytes_per_line", "0"), 0, 0));

    std::string format_name = get_property_with_default(property_type, "format_name", "");

    if(false) {}
    FORMAT_CONVERSION_CASE(greyscale_8, "GREY(8)", 8)
    FORMAT_CONVERSION_CASE(rgb_8, "R(3)G(3)B(2)", 8)
    FORMAT_CONVERSION_CASE(greyscale_10, "GREY(10)", 10)
    FORMAT_CONVERSION_CASE(greyscale_12, "GREY(12)", 12)
    FORMAT_CONVERSION_CASE(greyscale_14, "GREY(14)", 14)
    FORMAT_CONVERSION_CASE(greyscale_16, "GREY(16)", 16)
    FORMAT_CONVERSION_CASE(rgb_444, "R(4)G(4)B(4)", 12)
    FORMAT_CONVERSION_CASE(rgb_555, "R(5)G(5)B(5)(1)", 15)
    FORMAT_CONVERSION_CASE(rgb_565, "R(5)G(6)B(5)", 16)
    FORMAT_CONVERSION_CASE(rgba_4444, "R(4)G(4)B(4)A(4)", 16)
    FORMAT_CONVERSION_CASE(abgr_4444, "A(4)B(4)G(4)R(4)", 16)
    FORMAT_CONVERSION_CASE(bgr_555, "(1)B(5)G(5)R(5)", 16)
    FORMAT_CONVERSION_CASE(bgr_565, "B(5)G(6)R(5)", 16)
    FORMAT_CONVERSION_CASE(greyscale_18, "GREY(18)", 18)
    FORMAT_CONVERSION_CASE(greyscale_20, "GREY(20)", 20)
    FORMAT_CONVERSION_CASE(greyscale_22, "GREY(22)", 22)
    FORMAT_CONVERSION_CASE(greyscale_24, "GREY(24)", 24)
    FORMAT_CONVERSION_CASE(rgb_888, "R(8)G(8)B(8)", 24)
    FORMAT_CONVERSION_CASE(bgr_888, "B(8)G(8)R(8)", 24)
    FORMAT_CONVERSION_CASE(argb_8888, "A(8)R(8)G(8)B(8)", 32)
    FORMAT_CONVERSION_CASE(abgr_8888, "A(8)B(8)G(8)R(8)", 32)
    FORMAT_CONVERSION_CASE(rgba_8888, "R(8)G(8)B(8)A(8)", 32)
    FORMAT_CONVERSION_CASE(bgra_8888, "B(8)G(8)R(8)A(8)", 32)
    FORMAT_CONVERSION_CASE(greyscale_32, "GREY(32)", 32)
    else
    {
        format.pixel_format = static_cast<int16_t>(std::stoi(format_name));
        format.bits_per_pixel = static_cast<int16_t>(std::stoul(get_property_with_default(property_type, "bits_per_pixel", "0"), 0, 0));
    }

    stream << format.bits_per_pixel
           << format.bytes_per_line
           << format.height
           << format.palette_size
           << format.pixel_format
           << format.size
           << format.width;
}

std::string AdtfCoreMediaTypeVideoSerializer::getTypeId() const
{
    return "adtf.type.video.adtf2_support.serialization.adtf.cid";
}

std::string AdtfCoreMediaTypeAudioSerializer::getMetaType() const
{
    return "adtf/audio";
}

void AdtfCoreMediaTypeAudioSerializer::serialize(const StreamType& stream_type, OutputStream& stream) const
{
    auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);

    uint32_t major_type = std::stoul(get_property_with_default(property_type, "major", MEDIA_TYPE_VIDEO), 0, 0);
    uint32_t sub_type = std::stoul(get_property_with_default(property_type, "sub", MEDIA_SUBTYPE_VIDEO_UNCOMPRESSED), 0, 0);
    uint32_t flags = std::stoul(get_property_with_default(property_type, "flags", "0"), 0, 0);
    stream << major_type << sub_type << flags;

    AudioFormat format;
    format.channels = std::stoul(get_property_with_default(property_type, "channel_count", "0"), 0, 0);
    format.format_type = format.channels == 1 ? 1 : 2;
    format.samples_per_sec = std::stoul(get_property_with_default(property_type, "sample_rate_hz", "0"), 0, 0);
    format.bits_per_sample = std::stoul(get_property_with_default(property_type, "bits_per_sample", "0"), 0, 0);
    format.num_samples = std::stoul(get_property_with_default(property_type, "sample_count", "0"), 0, 0);
    format.size = format.num_samples * format.bits_per_sample / 8;

    stream << format.format_type
           << format.channels
           << format.samples_per_sec
           << format.bits_per_sample
           << format.num_samples
           << format.size;
}

std::string AdtfCoreMediaTypeAudioSerializer::getTypeId() const
{
    return "adtf.type.audio.adtf2_support.serialization.adtf.cid";
}

}
}
