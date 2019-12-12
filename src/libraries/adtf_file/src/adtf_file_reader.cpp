/**
 * @file
 * Class for path handling
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

#include <ifhd/ifhd.h>
#include <istream>
#include <string.h>
#include <algorithm>
#include <ddl.h>

#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/sample.h>
#include <adtf_file/stream_type.h>

#define A_UTIL5_RESULT_TO_EXCEPTION(__exp)\
{\
    a_util::result::Result __result = (__exp);\
    if (a_util::result::isFailed(__result))\
    {\
        throw std::runtime_error(a_util::result::toString(__result));\
    }\
}

using namespace ifhd;
using namespace ifhd::v500;

namespace adtf_file
{


template<>
InputStream& adtf_file::InputStream::operator>>(std::string& value)
{
    uint32_t size;
    *this >> size;
    value.resize(size - 1);
    read(&value[0], size - 1);
    char dummy;
    *this >> dummy;
    return *this;
}


class BufferInputStream: public InputStream
{
    public:
        BufferInputStream(const void* buffer, size_t size):
            _buffer(static_cast<const char*>(buffer)),
            _data_left(size)
        {

        }

        void read(void* destination, size_t count) override
        {
            if (count > _data_left)
            {
                throw std::runtime_error("not enough data");
            }

            memcpy(destination, _buffer, count);

            _buffer +=count;
            _data_left -=count;
        }

    private:
        const char* _buffer;
        size_t _data_left;

};

static void add_external_media_description_to_stream_type(const std::string& stream_name, const std::shared_ptr<const StreamType>& type, const ddl::DDLImporter& importer)
{
    ///@todo create copy instead of const cast
    auto property_type = std::const_pointer_cast<PropertyStreamType>(std::dynamic_pointer_cast<const PropertyStreamType>(type));
    if (property_type)
    {
        auto ddl_stream = importer.getDDL()->getStreamByName(stream_name.c_str());
        if (ddl_stream)
        {
            if (!ddl_stream->getStructs().empty())
            {
                auto struct_name = ddl_stream->getStructs().front()->getType();
                ddl::DDLResolver resolver;
                resolver.setTargetName(struct_name);
                resolver.visitDDL(importer.getDDL());

                property_type->setProperty("md_struct", "cString", struct_name);
                property_type->setProperty("md_definitions", "cString", resolver.getResolvedXML());
                property_type->setProperty("md_data_serialized", "tBool", "false");
            }
        }
    }
}

Reader::Reader(const std::string& file_name,
               StreamTypeDeserializers type_factories,
               SampleDeserializerFactories sample_deserializer_factories,
               std::shared_ptr<SampleFactory> sample_factory,
               std::shared_ptr<StreamTypeFactory> stream_type_factory,
               bool ignore_unsupported_streams):
    _file(new v500::IndexedFileReader),
    _type_factories(type_factories),
    _sample_deserializer_factories(sample_deserializer_factories),
    _sample_factory(sample_factory),
    _stream_type_factory(stream_type_factory)
{
    _file->open(file_name);

    ddl::DDLImporter importer;
    bool external_media_description = _file->getVersionId() < ifhd::v400::version_id;

    if (external_media_description)
    {
        auto description_file_name = file_name + ".description";
        if (a_util::result::isOk(importer.setFile(description_file_name.c_str())))
        {
            A_UTIL5_RESULT_TO_EXCEPTION(importer.createNew());
        }
        else
        {
            external_media_description = false;
        }
    }

    for (uint16_t stream_id = 1; stream_id < MAX_INDEXED_STREAMS; ++stream_id)
    {
        Stream stream;

        if (!_file->streamExists(stream_id))
        {
            continue;
        }
        try
        {
            stream.name = _file->getStreamName(stream_id);
        }
        catch (...)
        {
            continue;
        }

        try
        {
            auto type_and_factory = getInitialTypeAndSampleDeserializer(stream_id);

            stream.initial_type = type_and_factory.first;
            _stream_sample_deserializers[stream_id] = type_and_factory.second;

            if (external_media_description)
            {
                add_external_media_description_to_stream_type(stream.name, stream.initial_type, importer);
            }

            type_and_factory.second->setStreamType(*stream.initial_type);
        }
        catch (...)
        {
            if (!ignore_unsupported_streams)
            {
                throw;
            }
        }

        stream.stream_id = stream_id;
        stream.item_count = _file->getStreamIndexCount(stream_id);
        if (stream.item_count)
        {
            stream.timestamp_of_first_item = fromFileTimeStamp(_file->getFirstTime(stream_id));
            stream.timestamp_of_last_item = fromFileTimeStamp(_file->getLastTime(stream_id));
        }
        else
        {
            stream.timestamp_of_first_item = std::chrono::nanoseconds(0);
            stream.timestamp_of_last_item = std::chrono::nanoseconds(0);
        }
        _streams.push_back(stream);
    }

    for (size_t extension_index = 0; extension_index < _file->getExtensionCount(); ++extension_index)
    {
        FileExtension* file_extension;
        void* data_ptr;
        _file->getExtension(extension_index, &file_extension, &data_ptr);
        _extensions.push_back({reinterpret_cast<const char*>(file_extension->identifier),
                              file_extension->stream_id,
                              file_extension->type_id,
                              file_extension->user_id,
                              file_extension->version_id,
                              file_extension->data_size,
                              data_ptr});
    }
}

Reader::~Reader()
{
}


std::pair<std::shared_ptr<const StreamType>, std::shared_ptr<SampleDeserializer> > Reader::getInitialTypeAndSampleDeserializer(uint16_t stream_id)
{
    const void* additional_data;
    size_t data_size;
    _file->getAdditionalStreamInfo(stream_id, &additional_data, &data_size);

    BufferInputStream stream(additional_data, data_size);

    if (getFileVersion() < ifhd::v400::version_id)
    {
        return getInitialTypeAndSampleFactoryAdtf2(stream);
    }
    else
    {
        return getInitialTypeAndSampleFactoryAdtf3(stream);
    }
}

#define UCOM_MAX_IDENTIFIER_SIZE    512
#define OID_ADTF_MEDIA_TYPE         "adtf.core.media_type"
#define OID_ADTF_MEDIA_SAMPLE       "adtf.core.media_sample"

std::pair<std::shared_ptr<const StreamType>, std::shared_ptr<SampleDeserializer>> Reader::getInitialTypeAndSampleFactoryAdtf2(InputStream& stream)
{
    char type_id_buffer[UCOM_MAX_IDENTIFIER_SIZE] = "";
    char sample_id_buffer[UCOM_MAX_IDENTIFIER_SIZE] = "";

    stream.read(sample_id_buffer, UCOM_MAX_IDENTIFIER_SIZE);
    stream.read(type_id_buffer, UCOM_MAX_IDENTIFIER_SIZE);

    std::string type_class_id;
    if (type_id_buffer[0] == '\0')
    {
        type_class_id = OID_ADTF_MEDIA_TYPE;
    }
    else
    {
        type_id_buffer[UCOM_MAX_IDENTIFIER_SIZE - 1] = '\0';
        type_class_id = type_id_buffer;
    }

    std::string sample_class_id;
    if (sample_id_buffer[0] == '\0')
    {
        sample_class_id = OID_ADTF_MEDIA_SAMPLE;
    }
    else
    {
        sample_id_buffer[UCOM_MAX_IDENTIFIER_SIZE - 1] = '\0';
        sample_class_id = sample_id_buffer;
    }

    auto type = buildType(type_class_id + ".adtf2_support.serialization.adtf.cid", stream);
    auto deserializer = _sample_deserializer_factories.build(sample_class_id + ".adtf2_support.serialization.adtf.cid");

    return std::make_pair(type, deserializer);
}

std::pair<std::shared_ptr<const StreamType>, std::shared_ptr<SampleDeserializer>> Reader::getInitialTypeAndSampleFactoryAdtf3(InputStream& stream)
{
    auto type = buildType("", stream);

    std::string serialization_class_id;
    stream >> serialization_class_id;

    auto deserializer = _sample_deserializer_factories.build(serialization_class_id);

    return std::make_pair(type, deserializer);
}

std::chrono::nanoseconds Reader::fromFileTimeStamp(timestamp_t time_stamp)
{
    if (_file->getVersionId() == v500::version_id)
    {
        return std::chrono::nanoseconds{time_stamp};
    }
    else
    {
        return std::chrono::microseconds{time_stamp};
    }
}

timestamp_t Reader::toFileTimeStamp(std::chrono::nanoseconds time_stamp)
{
    if (_file->getVersionId() == v500::version_id)
    {
        return time_stamp.count();
    }
    else
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(time_stamp).count();
    }
}

uint32_t Reader::getFileVersion() const
{
    return _file->getVersionId();
}

a_util::datetime::DateTime Reader::getDateTime() const
{
    return _file->getDateTime();
}

std::chrono::nanoseconds Reader::getFirstTime() const
{
    std::vector<Stream> streams = getStreams();
    std::chrono::nanoseconds first_time = std::chrono::nanoseconds::max();
    for (const auto& current_stream : streams)
    {
        if (current_stream.item_count == 0)
        {
            continue;
        }

        if (current_stream.timestamp_of_first_item < first_time)
        {
            first_time = current_stream.timestamp_of_first_item;
        }
    }

    if (first_time == std::chrono::nanoseconds::max())
    {
        return std::chrono::nanoseconds{0};
    }

    return first_time;
}
 
std::chrono::nanoseconds Reader::getLastTime() const
{
    std::vector<Stream> streams = getStreams();
    std::chrono::nanoseconds last_time = std::chrono::nanoseconds::min();
    for (const auto& current_stream : streams)
    {
        if (current_stream.item_count == 0)
        {
            continue;
        }

        if (current_stream.timestamp_of_last_item > last_time)
        {
            last_time = current_stream.timestamp_of_last_item;
        }
    }

    if (last_time == std::chrono::nanoseconds::min())
    {
        return std::chrono::nanoseconds{0};
    }

    return last_time;
}

std::chrono::nanoseconds Reader::getDuration() const
{
    std::vector<Stream> streams = getStreams();
    std::chrono::nanoseconds first_time = std::chrono::nanoseconds::max();
    std::chrono::nanoseconds last_time = std::chrono::nanoseconds::min();

    for (const auto& current_stream : streams)
    {
        if (current_stream.item_count == 0)
        {
            continue;
        }

        if (current_stream.timestamp_of_first_item < first_time)
        {
            first_time = current_stream.timestamp_of_first_item;
        }
        if (current_stream.timestamp_of_last_item > last_time)
        {
            last_time = current_stream.timestamp_of_last_item;
        }
    }

    if (last_time < first_time)
    {
        return std::chrono::nanoseconds{0};
    }

    return std::chrono::nanoseconds(last_time - first_time);
}

std::string Reader::getDescription() const
{
   return _file->getDescription();
}

const std::vector<Extension>&Reader::getExtensions() const
{
    return _extensions;
}

const std::vector<Stream>&Reader::getStreams() const
{
    return _streams;
}

uint64_t Reader::getItemCount() const
{
    return _file->getChunkCount();
}

uint64_t Reader::getItemIndexForTimeStamp(std::chrono::nanoseconds time_stamp)
{
    return _file->seek(0, toFileTimeStamp(time_stamp), TimeFormat::tf_chunk_time);
}

uint64_t Reader::getItemIndexForStreamItemIndex(uint16_t stream_id, uint64_t stream_item_index)
{
    return _file->seek(stream_id, stream_item_index, TimeFormat::tf_stream_index);
}

std::shared_ptr<const StreamType> Reader::getStreamTypeBefore(uint64_t item_index, uint16_t stream_id, bool update_sample_deserializer)
{
    std::shared_ptr<const StreamType> stream_type;

    ChunkHeader header;
    std::vector<uint8_t> data;
    if (_file->getLastChunkWithFlagBefore(item_index, stream_id, ChunkType::ct_type, header, data))
    {
        BufferInputStream stream(data.data(), data.size());
        stream_type = buildType("", stream);
    }
    else
    {
        auto stream = std::find_if(_streams.begin(), _streams.end(), [&](const Stream& stream)
        {
            return stream.stream_id == stream_id;
        });
        stream_type = stream->initial_type;
    }

    if (update_sample_deserializer)
    {
        auto sample_deserializer = _stream_sample_deserializers.find(stream_id);
        if (sample_deserializer != _stream_sample_deserializers.end())
        {
            sample_deserializer->second->setStreamType(*stream_type);
        }
    }

    return stream_type;
}

void Reader::seekTo(uint64_t item_index)
{
    if (_file->getCurrentPos(TimeFormat::tf_chunk_index) != static_cast<int64_t>(item_index))
    {
        if (item_index < static_cast<uint64_t>(_file->getChunkCount()))
        {
            _file->seek(0, item_index, TimeFormat::tf_chunk_index);
        }
        else
        {
            throw std::out_of_range("index to large");
        }
    }
}

FileItem Reader::getNextItem()
{
    for (;;)
    {
        std::shared_ptr<const StreamItem> stream_item;

        ChunkHeader* chunk_header;
        const void* chunk_data;
        _file->readNextChunk(&chunk_header, const_cast<void**>(&chunk_data));

        if (chunk_header->flags & ChunkType::ct_trigger)
        {
            stream_item = std::make_shared<Trigger>();
        }
        else
        {
            auto sample_deserializer = _stream_sample_deserializers.find(chunk_header->stream_id);
            if (sample_deserializer == _stream_sample_deserializers.end())
            {
                continue;
            }

            BufferInputStream stream(chunk_data, chunk_header->size  - sizeof(ChunkHeader));

            if (chunk_header->flags & ChunkType::ct_type)
            {
                auto type = buildType("", stream);
                sample_deserializer->second->setStreamType(*type);
                stream_item = type;
            }
            else
            {
                auto sample = _sample_factory->build();
                auto read_sample = std::dynamic_pointer_cast<ReadSample>(sample);
                if (!read_sample)
                {
                    throw std::runtime_error("sample factory builds samples that do not implement the ReadSample interface");
                }

                sample_deserializer->second->deserialize(*read_sample, stream);
                stream_item = sample;
            }
        }

        return {chunk_header->stream_id, fromFileTimeStamp(chunk_header->time_stamp), stream_item};
    }
}

int64_t Reader::getNextItemIndex()
{
    return _file->getCurrentPos(TimeFormat::tf_chunk_index);
}

std::shared_ptr<const StreamType> Reader::buildType(const std::string& id, InputStream& stream)
{
    auto type = _stream_type_factory->build();
    auto property_type = std::dynamic_pointer_cast<PropertyStreamType>(type);
    if (!property_type)
    {
        throw std::runtime_error("stream type factory does not build PropertyStreamTypes");
    }
    _type_factories.Deserialize(id, stream, *property_type);
    return type;
}

}
