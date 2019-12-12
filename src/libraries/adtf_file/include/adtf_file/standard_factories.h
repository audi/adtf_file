/**
 * @file
 * standard factories bundle.
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

#ifndef ADTF_FILE_STANDARD_FACTORIES
#define ADTF_FILE_STANDARD_FACTORIES

#include <adtf_file/adtf_file_reader.h>

#include <adtf_file/adtf3/adtf3_sample_copy_deserializer.h>
#include <adtf_file/adtf3/adtf3_media_description_deserializer.h>
#include <adtf_file/adtf3/adtf3_stream_type_deserializer.h>
#include <adtf_file/adtf3/adtf3_stream_type_serializer.h>
#include <adtf_file/adtf3/adtf3_sample_copy_serializer.h>
#include <adtf_file/adtf3/adtf3_media_description_serializer.h>

#include <adtf_file/adtf2/adtf2_adtf_core_media_sample_deserializer.h>
#include <adtf_file/adtf2/adtf2_indexed_data_media_sample_deserializer.h>
#include <adtf_file/adtf2/adtf2_stream_type_deserializers.h>
#include <adtf_file/adtf2/adtf2_adtf_core_media_sample_serializer.h>
#include <adtf_file/adtf2/adtf2_stream_type_serializers.h>

#include <adtf_file/object.h>

namespace adtf_file
{

class StandardSampleSerializers: public SampleSerializerFactories
{
    public:
        StandardSampleSerializers()
        {
            add(std::make_shared<sample_serializer_factory<adtf3::SampleCopySerializer>>());
            add(std::make_shared<sample_serializer_factory<adtf3::SampleCopySerializerNs>>());
            add(std::make_shared<sample_serializer_factory<adtf3::MediaDescriptionSerializer>>());
            add(std::make_shared<sample_serializer_factory<adtf3::MediaDescriptionSerializerNs>>());
            add(std::make_shared<sample_serializer_factory<adtf2::AdtfCoreMediaSampleSerializer>>());
        }
};

class StandardSampleDeserializers: public SampleDeserializerFactories
{
    public:
        StandardSampleDeserializers()
        {
            add(std::make_shared<sample_deserializer_factory<adtf3::SampleCopyDeserializer>>());
            add(std::make_shared<sample_deserializer_factory<adtf3::SampleCopyDeserializerNs>>());
            add(std::make_shared<sample_deserializer_factory<adtf3::MediaDescriptionDeserializer>>());
            add(std::make_shared<sample_deserializer_factory<adtf3::MediaDescriptionDeserializerNs>>());
            add(std::make_shared<sample_deserializer_factory<adtf2::AdtfCoreMediaSampleDeserializer>>());
            add(std::make_shared<sample_deserializer_factory<adtf2::IndexedDataMediaSampleDeserializer>>());
        }
};

class StandardTypeDeserializers: public StreamTypeDeserializers
{
    public:
        StandardTypeDeserializers()
        {
            add(std::make_shared<adtf3::StreamTypeDeserializer>());
            add(std::make_shared<adtf2::AdtfCoreMediaTypeDeserializer>());
            add(std::make_shared<adtf2::AdtfCoreMediaTypeVideoDeserializer>());
            add(std::make_shared<adtf2::AdtfCoreMediaTypeAudioDeserializer>());
            add(std::make_shared<adtf2::IndexedDataDescriptionMediaTypeDeserializer>());
        }
};

namespace adtf3
{

class StandardTypeSerializers: public StreamTypeSerializers
{
    public:
        StandardTypeSerializers()
        {
            add(std::make_shared<adtf3::StreamTypeSerializer>());
        }
};

}

namespace adtf2
{

class StandardTypeSerializers: public StreamTypeSerializers
{
    public:
        StandardTypeSerializers()
        {
            add(std::make_shared<adtf2::AdtfCoreMediaTypeSerializer>());
            add(std::make_shared<adtf2::AdtfCoreMediaTypeVideoSerializer>());
            add(std::make_shared<adtf2::AdtfCoreMediaTypeAudioSerializer>());
        }
};

}

inline void add_standard_objects()
{
    getObjects().push_back(std::make_shared<sample_deserializer_factory<adtf3::SampleCopyDeserializer>>());
    getObjects().push_back(std::make_shared<sample_deserializer_factory<adtf3::SampleCopyDeserializerNs>>());
    getObjects().push_back(std::make_shared<sample_deserializer_factory<adtf3::MediaDescriptionDeserializer>>());
    getObjects().push_back(std::make_shared<sample_deserializer_factory<adtf3::MediaDescriptionDeserializerNs>>());
    getObjects().push_back(std::make_shared<sample_deserializer_factory<adtf2::AdtfCoreMediaSampleDeserializer>>());
    getObjects().push_back(std::make_shared<sample_deserializer_factory<adtf2::IndexedDataMediaSampleDeserializer>>());

    getObjects().push_back(std::make_shared<sample_serializer_factory<adtf_file::adtf3::SampleCopySerializer>>());
    getObjects().push_back(std::make_shared<sample_serializer_factory<adtf_file::adtf3::SampleCopySerializerNs>>());
    getObjects().push_back(std::make_shared<sample_serializer_factory<adtf_file::adtf3::MediaDescriptionSerializer>>());
    getObjects().push_back(std::make_shared<sample_serializer_factory<adtf_file::adtf3::MediaDescriptionSerializerNs>>());
    getObjects().push_back(std::make_shared<sample_serializer_factory<adtf_file::adtf2::AdtfCoreMediaSampleSerializer>>());

    getObjects().push_back(std::make_shared<adtf3::StreamTypeDeserializer>());
    getObjects().push_back(std::make_shared<adtf2::AdtfCoreMediaTypeDeserializer>());
    getObjects().push_back(std::make_shared<adtf2::AdtfCoreMediaTypeVideoDeserializer>());
    getObjects().push_back(std::make_shared<adtf2::AdtfCoreMediaTypeAudioDeserializer>());
    getObjects().push_back(std::make_shared<adtf2::IndexedDataDescriptionMediaTypeDeserializer>());

    getObjects().push_back(std::make_shared<adtf3::StreamTypeSerializer>());
    getObjects().push_back(std::make_shared<adtf2::AdtfCoreMediaTypeSerializer>());
    getObjects().push_back(std::make_shared<adtf2::AdtfCoreMediaTypeVideoSerializer>());
    getObjects().push_back(std::make_shared<adtf2::AdtfCoreMediaTypeAudioSerializer>());
}

}

#endif
