/**
 * @file
 * adtf2 mediasample deserializer implementation.
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

#include <adtf_file/adtf2/adtf2_adtf_core_media_sample_deserializer.h>
#include <adtf_file/adtf2/adtf2_sample_info.h>
#include <adtf_file/stream_type.h>

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

ddl::CodecFactory create_codec_factory_from_stream_type(const StreamType& stream_type)
{
    try
    {
        auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);
        if (property_type.getMetaType() == "adtf2/legacy")
        {
            if (property_type.getProperty("major").second == "0" &&
                property_type.getProperty("sub").second == "0")
            {
                return ddl::CodecFactory(property_type.getProperty("md_struct").second.c_str(),
                                          property_type.getProperty("md_definitions").second.c_str());
            }
        }
    }
    catch(const std::exception&)
    {
    }

    return ddl::CodecFactory();
}

class AdtfCoreMediaSampleDeserializer::Implementation
{
    public:
        ddl::CodecFactory codec_factory;
};

AdtfCoreMediaSampleDeserializer::AdtfCoreMediaSampleDeserializer():
    _implementation(new Implementation)
{
}

AdtfCoreMediaSampleDeserializer::~AdtfCoreMediaSampleDeserializer()
{
}

void AdtfCoreMediaSampleDeserializer::setStreamType(const StreamType& type)
{
    _implementation->codec_factory = create_codec_factory_from_stream_type(type);
}

void AdtfCoreMediaSampleDeserializer::deserializeData(ReadSample& sample, InputStream& stream, size_t buffer_size)
{
    if (a_util::result::isOk(_implementation->codec_factory.isValid()))
    {
        // unfortunately we have to copy the data again.
        std::vector<uint8_t> serialized_buffer(buffer_size);
        stream.read(serialized_buffer.data(), buffer_size);

        auto decoder = _implementation->codec_factory.makeDecoderFor(serialized_buffer.data(),
                                                                    buffer_size,
                                                                    ddl::DataRepresentation::serialized);
        A_UTIL5_RESULT_TO_EXCEPTION(decoder.isValid());
        auto deserialized_size = decoder.getBufferSize(ddl::DataRepresentation::deserialized);

        auto codec = decoder.makeCodecFor(sample.beginBufferWrite(deserialized_size), deserialized_size,
                                          ddl::DataRepresentation::deserialized);
        A_UTIL5_RESULT_TO_EXCEPTION(ddl::serialization::transform(decoder, codec));
        sample.endBufferWrite();
    }
    else
    {
        auto buffer = sample.beginBufferWrite(buffer_size);
        stream.read(buffer, buffer_size);
        sample.endBufferWrite();
    }
}

#define ADTF_MEDIASAMPLE_CLASS_VERSION_2    0x02
#define ADTF_MEDIASAMPLE_CLASS_VERSION_4    0x04

void AdtfCoreMediaSampleDeserializer::deserialize(ReadSample& sample, InputStream& stream)
{
    uint8_t version;
    uint32_t buffer_size = 0;
    uint64_t time_stamp;
    uint32_t flags;
    stream >> version >> buffer_size >> time_stamp >> flags;

    sample.setTimeStamp(std::chrono::microseconds(time_stamp));

    // flags are designed to be compatible
    sample.setFlags(flags);

    if (buffer_size)
    {
        deserializeData(sample, stream, buffer_size);
    }

    // check if there should be media sample info data
    if (version >= ADTF_MEDIASAMPLE_CLASS_VERSION_2)
    {
        if (version == ADTF_MEDIASAMPLE_CLASS_VERSION_2)
        {
            return;
            // we cannot detect Bug #4203 here.
            // this has been fixed in ADTF 2.2 so we simply ignore this case.
        }

        deserializeMediaSampleInfo(sample, stream);

        if (version >= ADTF_MEDIASAMPLE_CLASS_VERSION_4)
        {
            deserializeMediaSampleLogTrace(sample, stream);
        }
    }

}

}

}
