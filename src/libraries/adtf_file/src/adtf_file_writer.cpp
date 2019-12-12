/**
 * @file
 * writer.
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

#include <adtf_file/adtf_file_writer.h>
#include <ifhd/ifhd.h>

using namespace ifhd;
using namespace ifhd::v500;

namespace adtf_file
{

template <>
OutputStream& OutputStream::operator<<(const std::string& value)
{
    uint32_t size = static_cast<uint32_t>(value.size() + 1);
    *this << size;
    write(value.data(), size);
    return *this;
}

class CompatIndexedFileWriter: public v500::IndexedFileWriter
{
    public:
        utils5ext::File& GetFileHandle()
        {
            return _file;
        }

        FileExtension& GetLastExtensionHeader()
        {
            return _extensions.back()->file_extension;
        }
};

Writer::Writer(const std::string& file_name,
               std::chrono::nanoseconds history_duration,
               StreamTypeSerializers type_serializers,
               TargetADTFVersion adtf_version,
               size_t cache_size,
               size_t cache_minimum_write_chunk_size,
               size_t cache_maximum_write_chunk_size,
               uint32_t writer_flags):
    _file(new CompatIndexedFileWriter),
    _type_serializers(type_serializers),
    _target_adtf_version(adtf_version),
    _history_active(history_duration.count() > 0),
    _lock_chunk_write(false)
{
    _streams.resize(1);

    std::chrono::seconds index_delay{1};
    timestamp_t file_index_delay = adtf_version < adtf3ns ? std::chrono::duration_cast<std::chrono::microseconds>(index_delay).count() :
                                                            std::chrono::duration_cast<std::chrono::nanoseconds>(index_delay).count();

    _file->create(file_name, cache_size, writer_flags, 0, history_duration.count(), 0,
                  cache_minimum_write_chunk_size, cache_maximum_write_chunk_size, this,
                  file_index_delay);

    FileHeader* header;
    _file->getHeaderRef(&header);
    switch (adtf_version)
    {
        case adtf2:
        {
            if (_history_active)
            {
                header->version_id = ifhd::v201_v301::version_id_with_history_end_offset;
            }
            else
            {
                header->version_id = ifhd::v201_v301::version_id;
            }
            break;
        }
        case adtf3:
        {
            header->version_id = ifhd::v400::version_id;
            break;
        }
        case adtf3ns:
        {
            header->version_id = ifhd::v500::version_id;
            break;
        }
    }
}

size_t Writer::createStream(const std::string& name, const StreamType& initial_type, const std::shared_ptr<SampleSerializer>& serializer)
{
    Stream stream;
    stream.name = name;
    _type_serializers.serialize(initial_type, stream.initial_type);
    stream.adtf2_initial_type_id = _type_serializers.getAdtf2TypeId(initial_type);
    stream.sample_serializer = serializer;
    stream.sample_serializer->setStreamType(initial_type);

    _streams.push_back(std::move(stream));
    return _streams.size() - 1;
}

void Writer::write(size_t stream_id, std::chrono::nanoseconds time_stamp, const StreamType& type)
{
    if (_target_adtf_version == adtf2)
    {
        throw std::invalid_argument("Invalid version for Write call (ADTF 2 does not support StreamType changes within the stream)");
    }

    auto& stream = _streams.at(stream_id);

    if (!stream.has_samples)
    {
        stream.initial_type.clear();
        _type_serializers.serialize(type, stream.initial_type);
    }
    else
    {
        auto chunk = serialize(stream_id, time_stamp, type);
        write(chunk);

        if (_history_active)
        {
            stream.type_queue.emplace_back(std::move(chunk));
        }
    }
}

void Writer::write(size_t stream_id, std::chrono::nanoseconds time_stamp, const WriteSample& sample)
{
    auto chunk = serialize(stream_id, time_stamp, sample);
    write(chunk);
    _streams.at(stream_id).has_samples = true;
}

void Writer::writeTrigger(size_t stream_id, std::chrono::nanoseconds time_stamp)
{
    if (_target_adtf_version == adtf2)
    {
        throw std::invalid_argument("Invalid version for WriteTrigger call (ADTF 2 does not support Triggers)");
    }
    auto chunk = serializeTrigger(stream_id, time_stamp);
    write(chunk);
}

void Writer::quitHistory()
{
    _file->quitHistory();
    _history_active = false;
}

class ExtensionStream: public OutputStream
{
    private:
        utils5ext::File& _file;
        FileExtension& _extension_header;
        bool            _filepos_set;

    public:
        ExtensionStream(utils5ext::File& file,
                         FileExtension& extension_header):
            _file(file),
            _extension_header(extension_header),
            _filepos_set(false)
        {
        }

        void write(const void* data, size_t data_size) override
        {
            if (!_filepos_set)
            {
                _extension_header.data_pos = _file.getFilePos();
                _filepos_set = true;
            }
            _file.writeAll(data, data_size);
            _extension_header.data_size += data_size;
        }
};

std::shared_ptr<OutputStream> Writer::getExtensionStream(const std::string& name, uint32_t user_id, uint32_t type_id, uint32_t version_id)
{
    if (_last_extension_stream.lock())
    {
        throw std::runtime_error("there can only be one extension stream at a time.");
    }

    if (!_lock_chunk_write)
    {
        _file->stopAndFlushCache();
        _lock_chunk_write = true;
    }

    _file->appendExtension(name.c_str(), 0, 0, type_id, version_id, 0, user_id);
    std::shared_ptr<OutputStream> extstream = std::make_shared<ExtensionStream>(_file->GetFileHandle(), _file->GetLastExtensionHeader());
    _last_extension_stream = extstream;
    return extstream;
}

void Writer::onChunkDropped(uint64_t /*index*/, uint16_t stream_id, uint16_t flags, timestamp_t time_stamp)
{    
    if (flags & ChunkType::ct_type)
    {
        auto& stream = _streams.at(stream_id);
        stream.initial_type = std::move(stream.type_queue.front());
        stream.type_queue.pop_front();
    }
}

Writer::Chunk Writer::serialize(size_t stream_id, std::chrono::nanoseconds time_stamp, const StreamType& type)
{
    Chunk chunk(stream_id, time_stamp, ChunkType::ct_type);
    _type_serializers.serialize(type, chunk);
    return chunk;
}

Writer::Chunk Writer::serialize(size_t stream_id, std::chrono::nanoseconds time_stamp, const WriteSample& sample)
{
    auto stream = _streams.at(stream_id);
    Chunk chunk(stream_id, time_stamp, 0);
    stream.sample_serializer->serialize(sample, chunk);
    return chunk;
}

Writer::Chunk Writer::serializeTrigger(size_t stream_id, std::chrono::nanoseconds time_stamp)
{
    return Chunk(stream_id, time_stamp, ChunkType::ct_trigger);
}

void Writer::write(const Writer::Chunk& chunk)
{
    if (_lock_chunk_write)
    {
       throw std::runtime_error("Can not add data after GetExtensionStream was used");
    }
    timestamp_t time_stamp = _target_adtf_version == adtf3ns ? chunk.time_stamp.count() : std::chrono::duration_cast<std::chrono::microseconds>(chunk.time_stamp).count();
    _file->writeChunk(static_cast<uint16_t>(chunk.stream_id), chunk.data(), static_cast<uint32_t>(chunk.size()), time_stamp, chunk.flags);
}

Writer::~Writer()
{
    if (_target_adtf_version >= adtf3)
    {
        closeAdtf3();
    }
    else
    {
        closeAdtf2();
    }
}

#define UCOM_MAX_IDENTIFIER_SIZE    512

std::string strip_compatibility_postfix(const std::string& id)
{
    auto postfix_position = id.find(".adtf2_support.serialization.adtf.cid");
    if (postfix_position != std::string::npos)
    {
        return id.substr(0, postfix_position);
    }
    else
    {
        return id;
    }
}

void Writer::closeAdtf2()
{
    for (size_t stream_id = 1; stream_id < _streams.size(); ++stream_id)
    {
        auto& stream = _streams[stream_id];
        _file->setStreamName(static_cast<uint16_t>(stream_id), stream.name.c_str());

        std::string sample_id = strip_compatibility_postfix(stream.sample_serializer->getId());
        sample_id.resize(UCOM_MAX_IDENTIFIER_SIZE, '\0');

        std::string type_id = strip_compatibility_postfix(stream.adtf2_initial_type_id);
        type_id.resize(UCOM_MAX_IDENTIFIER_SIZE, '\0');

        Buffer additional_data_stream;
        additional_data_stream.write(sample_id.data(), sample_id.size());
        additional_data_stream.write(type_id.data(), type_id.size());
        additional_data_stream.write(stream.initial_type.data(), stream.initial_type.size());

        _file->setAdditionalStreamInfo(static_cast<uint16_t>(stream_id), additional_data_stream.data(), static_cast<uint32_t>(additional_data_stream.size()));
    }

    _file->close();
}

void Writer::closeAdtf3()
{
    for (size_t stream_id = 1; stream_id < _streams.size(); ++stream_id)
    {
        auto& stream = _streams[stream_id];
        _file->setStreamName(stream_id, stream.name.c_str());
        stream.initial_type << stream.sample_serializer->getId();
        _file->setAdditionalStreamInfo(static_cast<uint16_t>(stream_id), stream.initial_type.data(), static_cast<uint32_t>(stream.initial_type.size()));
    }

    _file->close();
}

void Writer::setFileDescription(const std::string& description)
{
    _file->setDescription(description);
}


}
