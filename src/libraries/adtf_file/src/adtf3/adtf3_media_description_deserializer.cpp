/**
 * @file
 * adtf3 media description deserializer.
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
#include <adtf_file/adtf3/adtf3_media_description_deserializer.h>
#include <adtf_file/adtf3/adtf3_sample_info.h>
#include <adtf_file/stream_type.h>
#include <ddl.h>

namespace adtf_file
{

namespace adtf3
{

namespace
{

class MediaDescriptionDeserializerImplementation
{
    public:
        void setStreamType(const StreamType& type)
        {
            auto property_type = dynamic_cast<const PropertyStreamType*>(&type);
            if (!property_type)
            {
                throw std::runtime_error("error no media description in stream type");
            }

            codec_factory = ddl::CodecFactory(property_type->getProperty("md_struct").second.c_str(),
                                              property_type->getProperty("md_definitions").second.c_str());

            if (a_util::result::isFailed(codec_factory.isValid()))
            {
                throw std::runtime_error("error parsing ddl: " + a_util::result::toString(codec_factory.isValid()));
            }
        }

        int64_t deserialize(ReadSample& sample, InputStream& stream)
        {
            int64_t time;
            int32_t flags;
            uint64_t buffer_size = 0;
            stream >> time >> flags >> buffer_size;

            if (buffer_size)
            {
                std::vector<uint8_t> serialized_buffer(buffer_size);
                stream.read(serialized_buffer.data(), buffer_size);
                auto decoder = codec_factory.makeDecoderFor(serialized_buffer.data(),
                                                            buffer_size,
                                                            ddl::DataRepresentation::serialized);

                auto deserialized_size = decoder.getBufferSize(ddl::DataRepresentation::deserialized);

                auto codec = decoder.makeCodecFor(sample.beginBufferWrite(deserialized_size),
                                                  deserialized_size,
                                                  ddl::DataRepresentation::deserialized);
                ddl::serialization::transform(decoder, codec);
                sample.endBufferWrite();
            }

            if (flags & InternalSampleFlags::sf_sample_info_present)
            {
                deserializeSampleInfo(sample, stream);
                flags ^= InternalSampleFlags::sf_sample_info_present;
            }

            sample.setFlags(flags);

            return time;
        }

    private:
        ddl::CodecFactory codec_factory;
};

}

class MediaDescriptionDeserializer::Implementation: public MediaDescriptionDeserializerImplementation
{
};

MediaDescriptionDeserializer::MediaDescriptionDeserializer():
    _implementation(new Implementation)
{
}

MediaDescriptionDeserializer::~MediaDescriptionDeserializer()
{
}

void MediaDescriptionDeserializer::setStreamType(const StreamType& type)
{
    _implementation->setStreamType(type);
}

void MediaDescriptionDeserializer::deserialize(ReadSample& sample, InputStream& stream)
{
    sample.setTimeStamp(std::chrono::microseconds(_implementation->deserialize(sample, stream)));
}

class MediaDescriptionDeserializerNs::Implementation: public MediaDescriptionDeserializerImplementation
{
};

MediaDescriptionDeserializerNs::MediaDescriptionDeserializerNs():
    _implementation(new Implementation)
{
}

MediaDescriptionDeserializerNs::~MediaDescriptionDeserializerNs()
{
}

void MediaDescriptionDeserializerNs::setStreamType(const StreamType& type)
{
    _implementation->setStreamType(type);
}

void MediaDescriptionDeserializerNs::deserialize(ReadSample& sample, InputStream& stream)
{
    sample.setTimeStamp(std::chrono::nanoseconds(_implementation->deserialize(sample, stream)));
}

}

}
