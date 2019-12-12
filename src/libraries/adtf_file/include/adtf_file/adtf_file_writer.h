/**
 * @file
 * adtf file writer.
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

#ifndef ADTF_FILE_ADTF_FILE_WRITER_INCLUDED
#define ADTF_FILE_ADTF_FILE_WRITER_INCLUDED

#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <queue>
#include <list>

#include <ifhd/ifhd.h>
#include "sample.h"
#include "stream_type.h"
#include "object.h"

namespace adtf_file
{

class OutputStream
{
    public:
        virtual void write(const void* data, size_t data_size) = 0;

        template <typename T>
        OutputStream& operator<<(const T& value)
        {
            write(&value, sizeof(T));
            return *this;
        }
};

template <>
OutputStream& OutputStream::operator<<(const std::string& value);

class StreamTypeSerializer: public Object
{
    public:
        virtual std::string getMetaType() const = 0;
        virtual void serialize(const StreamType& stream_type, OutputStream& stream) const = 0;
};

class ADTF2StreamTypeSerializer: public StreamTypeSerializer
{
    public:
        virtual std::string getTypeId() const = 0;
};

class StreamTypeSerializers: private std::unordered_map<std::string, std::shared_ptr<StreamTypeSerializer>>
{
    public:
        void add(const std::shared_ptr<StreamTypeSerializer>& serializer)
        {
            emplace(serializer->getMetaType(), serializer);
        }

        void serialize(const StreamType& stream_type, OutputStream& stream) const
        {
            auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);
            auto serializer = find(property_type.getMetaType());
            if (serializer == end())
            {
                // try to find a default serializer
                serializer = find("");
                if (serializer == end())
                {
                    throw std::runtime_error("No serializer for stream type " + property_type.getMetaType());
                }
            }

            serializer->second->serialize(stream_type, stream);
        }

        std::string getAdtf2TypeId(const StreamType& stream_type)
        {
            auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);
            auto serializer = find(property_type.getMetaType());
            if (serializer != end())
            {
                auto adtf2_serializer = std::dynamic_pointer_cast<ADTF2StreamTypeSerializer>(serializer->second);
                if (adtf2_serializer)
                {
                    return adtf2_serializer->getTypeId();
                }
            }

            return "";
        }
};

class SampleSerializer
{
    public:
        virtual std::string getId() const = 0;
        virtual void setStreamType(const StreamType& stream_type) = 0;
        virtual void serialize(const WriteSample& sample, OutputStream& stream) = 0;
};

class SampleSerializerFactory: public Object
{
    public:
        virtual std::string getId() const = 0;
        virtual std::shared_ptr<SampleSerializer> build() const = 0;
};

template <typename SERIALIZER>
class sample_serializer_factory: public SampleSerializerFactory
{
    public:
        std::string getId() const override
        {
            return SERIALIZER::id;
        }

        std::shared_ptr<SampleSerializer> build() const override
        {
            return std::make_shared<SERIALIZER>();
        }
};

class SampleSerializerFactories: private std::unordered_map<std::string, std::shared_ptr<SampleSerializerFactory>>
{
    public:
        void add(const std::shared_ptr<SampleSerializerFactory>& serializer_factory)
        {
            emplace(serializer_factory->getId(), serializer_factory);
        }

        std::shared_ptr<SampleSerializer> build(const std::string& id) const
        {
            try
            {
                return at(id)->build();
            }
            catch (...)
            {
                throw std::runtime_error("no sample serializer factory with id '" + id + "' available");
            }
        }
};

class CompatIndexedFileWriter;

class Writer: private ifhd::v400::IndexedFileWriter::ChunkDroppedCallback
{
    private:
        class Buffer: public std::vector<uint8_t>, public OutputStream
        {

            public:
                void write(const void* data, size_t data_size) override
                {
                    this->insert(end(), static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + data_size);
                }
        };

        class Chunk: public Buffer
        {
            public:
                size_t stream_id;
                std::chrono::nanoseconds time_stamp;
                uint32_t flags;

            public:
                Chunk(size_t stream_id,
                      std::chrono::nanoseconds time_stamp,
                      uint32_t flags):
                    stream_id(stream_id),
                    time_stamp(time_stamp),
                    flags(flags)
                {
                }
        };

    public:
        enum TargetADTFVersion
        {
            adtf2 = 2,
            adtf3 = 3,
            adtf3ns = 4
        };

    public:
        Writer() = delete;
        Writer(const std::string& file_name,
               std::chrono::nanoseconds history_duration,
               StreamTypeSerializers type_serializers,
               TargetADTFVersion target_adtf_version = adtf3ns,
               size_t cache_size = 0,
               size_t cache_minimum_write_chunk_size = 0,
               size_t cache_maximum_write_chunk_size = 0,
               uint32_t writer_flags = 0);
        ~Writer();

        Writer(const Writer&) = delete;
        Writer(Writer&&) = default;

        size_t createStream(const std::string& name,
                            const StreamType& initial_type,
                            const std::shared_ptr<SampleSerializer>& serializer);

        void write(size_t stream_id, std::chrono::nanoseconds time_stamp, const StreamType& type);
        void write(size_t stream_id, std::chrono::nanoseconds time_stamp, const WriteSample& sample);
        void writeTrigger(size_t stream_id, std::chrono::nanoseconds time_stamp);

        void quitHistory();

        std::shared_ptr<OutputStream> getExtensionStream(const std::string& name,
                                                         uint32_t user_id,
                                                         uint32_t type_id,
                                                         uint32_t version_id);
        
        void setFileDescription(const std::string& description);
    private:
        void onChunkDropped(uint64_t index, uint16_t stream_id, uint16_t flags, timestamp_t time_stamp) override;

        Chunk serialize(size_t stream_id, std::chrono::nanoseconds time_stamp, const StreamType& type);
        Chunk serialize(size_t stream_id, std::chrono::nanoseconds time_stamp, const WriteSample& sample);
        Chunk serializeTrigger(size_t stream_id, std::chrono::nanoseconds time_stamp);

        void write(const Chunk& chunk);

        void closeAdtf2();
        void closeAdtf3();

    private:
        std::unique_ptr<CompatIndexedFileWriter> _file;
        StreamTypeSerializers _type_serializers;
        TargetADTFVersion _target_adtf_version;
        size_t _stream_id_counter = 0;
        bool _history_active;
        bool _lock_chunk_write;

        struct Stream
        {
            std::string name;
            Buffer initial_type;
            std::string adtf2_initial_type_id;
            std::shared_ptr<SampleSerializer> sample_serializer;
            std::list<Buffer> type_queue;
            bool has_samples = false;
        };

        std::vector<Stream> _streams;
        std::weak_ptr<OutputStream> _last_extension_stream;
};

}

#endif
