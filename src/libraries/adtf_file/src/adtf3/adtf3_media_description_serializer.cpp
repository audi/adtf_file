/**
 * @file
 * adtf3 media description serializer.
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

#include "adtf3_sample_flags.h"
#include <adtf_file/adtf3/adtf3_media_description_serializer.h>
#include <adtf_file/adtf3/adtf3_sample_info.h>
#include <ddl.h>
#include <cstring>


namespace adtf_file
{

namespace adtf3
{

namespace
{

class MediaDescriptionSerializerImplementation
{
    public:
        void setStreamType(const StreamType& stream_type)
        {
            auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);

            try
            {
                if (property_type.getProperty("md_data_serialized").second == "true")
                {
                    throw std::runtime_error("data is already serialized, use sample copy serialization instead");
                }
            }
            catch(const std::exception&)
            {
            }

            codec_factory = ddl::CodecFactory(property_type.getProperty("md_struct").second.c_str(),
                                              property_type.getProperty("md_definitions").second.c_str());
            if (a_util::result::isFailed(codec_factory.isValid()))
            {
                throw std::runtime_error("error parsing ddl: " + a_util::result::toString(codec_factory.isValid()));
            }
        }

        void serialize(const WriteSample& sample, OutputStream& stream, int64_t time_stamp)
        {
            bool has_info = hasSampleInfo(sample);

            auto flags = sample.getFlags() & ~InternalSampleFlags::sf_sample_info_present;
            if (has_info)
            {
                flags |= InternalSampleFlags::sf_sample_info_present;
            }

            stream << time_stamp
                   << flags;

            auto buffer = sample.beginBufferRead();

            {
                auto decoder = codec_factory.makeDecoderFor(buffer.first,
                                                            buffer.second,
                                                            ddl::DataRepresentation::deserialized);
                auto serialized_size = decoder.getBufferSize(ddl::DataRepresentation::serialized);

                std::vector<uint8_t> serialized_buffer(serialized_size);
                auto codec = decoder.makeCodecFor(serialized_buffer.data(), serialized_size,
                                                  ddl::DataRepresentation::serialized);
                ddl::serialization::transform(decoder, codec);
                stream << static_cast<uint64_t>(serialized_size);
                stream.write(serialized_buffer.data(), serialized_size);
            }

            sample.endBufferRead();

            if (has_info)
            {
                serializeSampleInfo(sample, stream);
            }
        }

    private:
        ddl::CodecFactory codec_factory;
};

}

class MediaDescriptionSerializer::Implementation: public MediaDescriptionSerializerImplementation
{
};

MediaDescriptionSerializer::MediaDescriptionSerializer():
    _implementation(new Implementation)
{
}

MediaDescriptionSerializer::~MediaDescriptionSerializer()
{
}

std::string MediaDescriptionSerializer::getId() const
{
    return id;
}

void MediaDescriptionSerializer::setStreamType(const StreamType& stream_type)
{
    _implementation->setStreamType(stream_type);
}

void MediaDescriptionSerializer::serialize(const WriteSample& sample, OutputStream& stream)
{
    _implementation->serialize(sample, stream, static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(sample.getTimeStamp()).count()));
}

class MediaDescriptionSerializerNs::Implementation: public MediaDescriptionSerializerImplementation
{
};

MediaDescriptionSerializerNs::MediaDescriptionSerializerNs():
    _implementation(new Implementation)
{
}

MediaDescriptionSerializerNs::~MediaDescriptionSerializerNs()
{
}

std::string MediaDescriptionSerializerNs::getId() const
{
    return id;
}

void MediaDescriptionSerializerNs::setStreamType(const StreamType& stream_type)
{
    _implementation->setStreamType(stream_type);
}

void MediaDescriptionSerializerNs::serialize(const WriteSample& sample, OutputStream& stream)
{
    _implementation->serialize(sample, stream, sample.getTimeStamp().count());
}


}
}
