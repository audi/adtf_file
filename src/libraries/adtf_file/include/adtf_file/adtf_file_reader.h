/**
 * @file
 * adtf file reader.
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

#ifndef ADTF_FILE_ADTF_FILE_READER_INCLUDED
#define ADTF_FILE_ADTF_FILE_READER_INCLUDED

#include <chrono>
#include <memory>
#include <vector>
#include <unordered_map>
#include <istream>
#include <string>
#include <ifhd/ifhd.h>
#include "stream_item.h"
#include "object.h"
#include "default_sample.h"
#include "stream_type.h"

namespace adtf_file
{

namespace exceptions
{
    using ifhd::exceptions::EndOfFile;
}

class Extension
{
    public:
        std::string name;
        uint16_t stream_id;
        uint32_t user_id;
        uint32_t type_id;
        uint32_t version_id;
        size_t data_size;
        const void* data;
};

class InputStream
{
    public:
        virtual void read(void* destination, size_t count) = 0;

        template <typename T>
        InputStream& operator >>(T& value)
        {
            read(&value, sizeof(T));
            return *this;
        }
};

template <>
InputStream& InputStream::operator>>(std::string& value);


class StreamTypeDeserializer: public Object
{
    public:
        virtual std::string getId() const = 0;
        virtual void deserialize(InputStream& stream, PropertyStreamType& stream_type) const = 0;
};

class StreamTypeDeserializers: private std::unordered_map<std::string, std::shared_ptr<StreamTypeDeserializer>>
{
    public:
        StreamTypeDeserializers& add(const std::shared_ptr<StreamTypeDeserializer>& deserializer)
        {
            (*this)[deserializer->getId()] = deserializer;
            return *this;
        }

        void Deserialize(const std::string& id, InputStream& stream, PropertyStreamType& stream_type) const
        {
            auto deserializer = find(id);
            if (deserializer == end())
            {
                throw std::runtime_error("no type deserializer for '" + id + "' available");
            }

            deserializer->second->deserialize(stream, stream_type);
        }
};

class StreamTypeFactory
{
    public:
        virtual std::shared_ptr<StreamType> build() const = 0;
};

template <typename STREAM_TYPE_CLASS>
class stream_type_factory: public StreamTypeFactory
{
    public:
        std::shared_ptr<StreamType> build() const override
        {
            return std::make_shared<STREAM_TYPE_CLASS>();
        }
};

class SampleDeserializer
{
    public:
        virtual void setStreamType(const StreamType& type) = 0;
        virtual void deserialize(ReadSample& sample, InputStream& stream) = 0;
};

class SampleDeserializerFactory: public Object
{
    public:
        virtual std::string getId() const = 0;
        virtual std::shared_ptr<SampleDeserializer> build() const = 0;
};

template <typename FACTORY>
class sample_deserializer_factory: public SampleDeserializerFactory
{
    public:
        std::string getId() const override
        {
            return FACTORY::id;
        }

        std::shared_ptr<SampleDeserializer> build() const override
        {
            return std::make_shared<FACTORY>();
        }
};

class SampleDeserializerFactories:
    private std::unordered_map<std::string, std::shared_ptr<SampleDeserializerFactory>>
{
    public:
        SampleDeserializerFactories& add(const std::shared_ptr<SampleDeserializerFactory>& factory)
        {
            (*this)[factory->getId()] = factory;
            return *this;
        }

        std::shared_ptr<SampleDeserializer> build(const std::string& id) const
        {
            try
            {
                return at(id)->build();
            }
            catch (...)
            {
                throw std::runtime_error("no sample deserializer factory for '" + id + "' available");
            }
        }
};

class SampleFactory
{
    public:
        virtual std::shared_ptr<Sample> build() const = 0;
};

template <typename SAMPLE_CLASS>
class sample_factory: public SampleFactory
{
    public:
        std::shared_ptr<Sample> build() const override
        {
            return std::make_shared<SAMPLE_CLASS>();
        }
};

class Stream
{
    public:
        uint16_t stream_id;
        std::string name;
        uint64_t item_count;
        std::chrono::nanoseconds timestamp_of_first_item;
        std::chrono::nanoseconds timestamp_of_last_item;
        std::shared_ptr<const StreamType> initial_type;
};

class FileItem
{
    public:
        uint16_t stream_id;
        std::chrono::nanoseconds time_stamp;
        std::shared_ptr<const StreamItem> stream_item;
};

class Reader
{
    public:
        Reader() = delete;
        Reader(const std::string& file_name,
               StreamTypeDeserializers type_factories,
               SampleDeserializerFactories sample_deserializer_factories,
               std::shared_ptr<SampleFactory> sample_factory = std::make_shared<adtf_file::sample_factory<DefaultSample>>(),
               std::shared_ptr<StreamTypeFactory> stream_type_factory = std::make_shared<adtf_file::stream_type_factory<DefaultStreamType>>(),
               bool ignore_unsupported_streams = false);
        virtual ~Reader();

        Reader(const Reader&) = delete;
        Reader(Reader&&) = default;

        uint32_t getFileVersion() const;
        a_util::datetime::DateTime getDateTime() const;
        std::chrono::nanoseconds getFirstTime() const;
        std::chrono::nanoseconds getLastTime() const;
        std::chrono::nanoseconds getDuration() const;
        std::string getDescription() const;

        const std::vector<Extension>& getExtensions() const;
        const std::vector<Stream>& getStreams() const;
        uint64_t getItemCount() const;

        uint64_t getItemIndexForTimeStamp(std::chrono::nanoseconds time_stamp);
        uint64_t getItemIndexForStreamItemIndex(uint16_t stream_id, uint64_t stream_item_index);
        std::shared_ptr<const StreamType> getStreamTypeBefore(uint64_t item_index, uint16_t stream_id, bool update_sample_deserializer);

        void seekTo(uint64_t item_index);
        FileItem getNextItem();

        /**
         * @return the index of the next item, or -1 if the file is empty.
         */
        int64_t getNextItemIndex();

    private:
        std::shared_ptr<const StreamType> buildType(const std::string& id, InputStream& stream);
        std::pair<std::shared_ptr<const StreamType>, std::shared_ptr<SampleDeserializer>> getInitialTypeAndSampleDeserializer(uint16_t stream_id);
        std::pair<std::shared_ptr<const StreamType>, std::shared_ptr<SampleDeserializer> > getInitialTypeAndSampleFactoryAdtf2(InputStream& stream);
        std::pair<std::shared_ptr<const StreamType>, std::shared_ptr<SampleDeserializer>> getInitialTypeAndSampleFactoryAdtf3(InputStream& stream);
        std::chrono::nanoseconds fromFileTimeStamp(timestamp_t time_stamp);
        timestamp_t toFileTimeStamp(std::chrono::nanoseconds time_stamp);

    private:
        std::unique_ptr<ifhd::v500::IndexedFileReader> _file;
        std::vector<Extension> _extensions;
        std::vector<Stream> _streams;
        StreamTypeDeserializers _type_factories;
        SampleDeserializerFactories _sample_deserializer_factories;
        std::unordered_map<size_t, std::shared_ptr<SampleDeserializer>> _stream_sample_deserializers;
        std::shared_ptr<SampleFactory> _sample_factory;
        std::shared_ptr<StreamTypeFactory> _stream_type_factory;
};

inline std::string getShortDescription(const std::string& description)
{
    return description.substr(0, description.find_first_of("\n"));
}

inline std::string getLongDescription(const std::string& description)
{
    size_t pos_found = description.find_first_of("\n");
    if (pos_found != std::string::npos)
    {
        return description.substr(pos_found);
    }
    else
    {
        return std::string("");
    }
}

}

#endif // _ADTF_FILE_ADTF_FILE_READER_INCLUDED_
