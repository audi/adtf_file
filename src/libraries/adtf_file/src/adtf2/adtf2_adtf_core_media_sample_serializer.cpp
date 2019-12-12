/**
 * @file
 * adtf2 (core) mediasample serializer implementation
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

#include <ddl.h>

#include <adtf_file/adtf2/adtf2_adtf_core_media_sample_serializer.h>
#include <adtf_file/adtf2/adtf2_adtf_core_media_sample_deserializer.h>

#define A_UTIL5_RESULT_TO_EXCEPTION(__exp)\
{\
    a_util::result::Result __result = (__exp);\
    if (a_util::result::isFailed(__result))\
    {\
        throw std::runtime_error(a_util::result::toString(__result));\
    }\
}


namespace adtf_file
{

namespace adtf2
{

ddl::CodecFactory create_codec_factory_from_stream_type(const StreamType& stream_type);

class AdtfCoreMediaSampleSerializer::Implementation
{
    public:
        ddl::CodecFactory codec_factory;
};


AdtfCoreMediaSampleSerializer::AdtfCoreMediaSampleSerializer():
    _implementation(new Implementation)
{
}

AdtfCoreMediaSampleSerializer::~AdtfCoreMediaSampleSerializer()
{
}

std::string AdtfCoreMediaSampleSerializer::getId() const
{
    return id;
}

void AdtfCoreMediaSampleSerializer::setStreamType(const StreamType& stream_type)
{
    _implementation->codec_factory = create_codec_factory_from_stream_type(stream_type);
}

#define ADTF_MEDIASAMPLE_CLASS_VERSION_4    0x04

void AdtfCoreMediaSampleSerializer::serialize(const WriteSample& sample, OutputStream& stream)
{
    auto buffer = sample.beginBufferRead();

    stream << uint8_t(ADTF_MEDIASAMPLE_CLASS_VERSION_4);

    if (a_util::result::isOk(_implementation->codec_factory.isValid()))
    {
        auto decoder = _implementation->codec_factory.makeDecoderFor(buffer.first,
                                                                    buffer.second,
                                                                    ddl::DataRepresentation::deserialized);
        A_UTIL5_RESULT_TO_EXCEPTION(decoder.isValid());
        auto serialized_size = decoder.getBufferSize(ddl::DataRepresentation::serialized);

        std::vector<uint8_t> serialized_buffer(serialized_size);
        auto codec = decoder.makeCodecFor(serialized_buffer.data(), serialized_size,
                                          ddl::DataRepresentation::serialized);
        A_UTIL5_RESULT_TO_EXCEPTION(ddl::serialization::transform(decoder, codec));
        stream << static_cast<uint32_t>(serialized_size)
               << static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(sample.getTimeStamp()).count())
               << static_cast<uint32_t>(sample.getFlags());
        stream.write(serialized_buffer.data(), serialized_size);
    }
    else
    {
        stream << static_cast<uint32_t>(buffer.second)
               << static_cast<int64_t>(sample.getTimeStamp().count())
               << static_cast<uint32_t>(sample.getFlags());

        stream.write(buffer.first, buffer.second);
    }

    //@todo sample_info will be hard to create info indexes for hash keys.
    uint32_t map_size = 0;
    stream << map_size;

    uint8_t sample_log_trace_present = 0;
    stream << sample_log_trace_present;
}


}
}
