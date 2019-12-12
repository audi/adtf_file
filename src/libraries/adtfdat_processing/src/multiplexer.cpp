/**
*
* ADTF Dat Exporter.
*
* @file
* Copyright &copy; 2003-2016 Audi Electronics Venture GmbH. All rights reserved
*
* $Author: WROTHFL $
* $Date: 2017-06-06 15:51:27 +0200 (Di, 06 Jun 2017) $
* $Revision: 61312 $
*
* @remarks
*
*/
#include <adtf_file/standard_factories.h>
#include <adtfdat_processing/multiplexer.h>

namespace adtf
{
namespace dat
{
namespace ant
{

OffsetReaderWrapper::OffsetReaderWrapper(const std::shared_ptr<Reader> original_reader,
                                             std::chrono::nanoseconds offset,
                                             std::chrono::nanoseconds start_offset,
                                             std::chrono::nanoseconds end_offset)
    : _original_reader(original_reader),
      _offset(offset),
      _start_offset(start_offset),
      _end_offset(end_offset)
{
}

std::string OffsetReaderWrapper::getReaderIdentifier() const
{
    return _original_reader->getReaderIdentifier();
}

std::pair<bool, std::string> OffsetReaderWrapper::isCompatible(const std::string& url) const
{
    return _original_reader->isCompatible(url);
}

Configuration OffsetReaderWrapper::getConfiguration() const
{
    return _original_reader->getConfiguration();
}

void OffsetReaderWrapper::setConfiguration(const Configuration& configuration)
{
    _original_reader->setConfiguration(configuration);
}

void OffsetReaderWrapper::open(const std::string& url)
{
    return _original_reader->open(url);
}

std::vector<adtf_file::Stream> OffsetReaderWrapper::getStreams() const
{
    return _original_reader->getStreams();
}

double OffsetReaderWrapper::getProgress() const
{
    return _original_reader->getProgress();
}

adtf_file::FileItem OffsetReaderWrapper::getNextItem()
{
    if (!_next_item.stream_item)
    {
        do
        {
            auto stream_type =
                std::dynamic_pointer_cast<const adtf_file::StreamType>(_next_item.stream_item);
            if (stream_type)
            {
                _last_stream_types[_next_item.stream_id] = _next_item.stream_item;
            }
            _next_item = _original_reader->getNextItem();
        } while (_start_offset.count() > 0 && _next_item.time_stamp < _start_offset);
    }

    adtf_file::FileItem item;

    if (!_last_stream_types.empty())
    {
        auto stream_item = _last_stream_types.begin();
        item = {stream_item->first, _next_item.time_stamp, stream_item->second};
        _last_stream_types.erase(stream_item);
    }
    else
    {
        item = std::move(_next_item);
    }

    if (_end_offset.count() > 0 && item.time_stamp > _end_offset)
    {
        throw adtf_file::exceptions::EndOfFile();
    }

    if (_offset.count() != 0)
    {
        auto read_sample = std::dynamic_pointer_cast<const adtf_file::ReadSample>(item.stream_item);
        auto write_sample =
            std::dynamic_pointer_cast<const adtf_file::WriteSample>(item.stream_item);
        if (read_sample && write_sample)
        {
            // ugly, but what shall we do, change the adtf_file API or copy the Sample?
            std::const_pointer_cast<adtf_file::ReadSample>(read_sample)
                ->setTimeStamp(write_sample->getTimeStamp() + _offset);
        }
        item.time_stamp += _offset;
    }

    return item;
}

adtf_file::StreamTypeSerializers get_stream_type_serializers(adtf_file::Writer::TargetADTFVersion target_adtf_version)
{
    switch (target_adtf_version)
    {
        case adtf_file::Writer::TargetADTFVersion::adtf2:
        {
            return adtf_file::getFactories<adtf_file::StreamTypeSerializers,
                    adtf_file::StreamTypeSerializer>();
        }
        default:
        {
            return adtf_file::adtf3::StandardTypeSerializers();
        }
    }
}

Multiplexer::Multiplexer(const std::string& destination_file_name,
                         adtf_file::Writer::TargetADTFVersion target_adtf_version,
                         bool skip_stream_types_and_triggers)
    : _writer(destination_file_name,
              std::chrono::seconds(0),
              get_stream_type_serializers(target_adtf_version),
              target_adtf_version),
      _skip_stream_types_and_triggers(skip_stream_types_and_triggers)
{
}

void Multiplexer::addStream(const std::shared_ptr<Reader> reader,
                            const std::string& stream_name,
                            const std::string& destination_stream_name,
                            const std::shared_ptr<adtf_file::SampleSerializer> serializer)
{
    auto stream = findStream(*reader, stream_name);
    if (!stream.initial_type)
    {
        throw std::runtime_error("there is no stream type deserializer for the stream type of stream '" + stream_name + "'");
    }
    _stream_mapping[reader][stream.stream_id] =
        _writer.createStream(destination_stream_name, *stream.initial_type, serializer);
}

void Multiplexer::addExtension(const std::string& name,
                               const void* extension_data,
                               size_t extension_size,
                               uint32_t user_id,
                               uint32_t type_id,
                               uint32_t version_id)
{
    auto stream = _writer.getExtensionStream(name, user_id, type_id, version_id);
    stream->write(extension_data, extension_size);
}

void Multiplexer::process(std::function<bool(double)> progress_handler)
{
    std::deque<std::pair<std::shared_ptr<Reader>, adtf_file::FileItem>> queue;
    for (auto& reader : _stream_mapping)
    {
        try
        {
            queue.push_back(std::make_pair(reader.first, reader.first->getNextItem()));
        }
        catch (const adtf_file::exceptions::EndOfFile&)
        {
        }
    }

    auto sort_functor = [](const decltype(queue)::value_type& first,
                           const decltype(queue)::value_type& second) {
        return first.second.time_stamp < second.second.time_stamp;
    };

    while (!queue.empty())
    {
        std::sort(queue.begin(), queue.end(), sort_functor);

        auto current_reader = queue.front().first;
        auto item = queue.front().second;
        queue.pop_front();

        auto destination_stream_id = _stream_mapping[current_reader].find(item.stream_id);
        if (destination_stream_id != _stream_mapping[current_reader].end())
        {
            auto write_sample =
                std::dynamic_pointer_cast<const adtf_file::WriteSample>(item.stream_item);
            if (write_sample)
            {
                _writer.write(destination_stream_id->second, item.time_stamp, *write_sample);
            }
            else if (!_skip_stream_types_and_triggers)
            {
                auto trigger =
                    std::dynamic_pointer_cast<const adtf_file::Trigger>(item.stream_item);
                if (trigger)
                {
                    _writer.writeTrigger(destination_stream_id->second, item.time_stamp);
                }
                else
                {
                    auto stream_type =
                        std::dynamic_pointer_cast<const adtf_file::StreamType>(item.stream_item);
                    if (stream_type)
                    {
                        _writer.write(destination_stream_id->second, item.time_stamp, *stream_type);
                    }
                }
            }
        }

        try
        {
            queue.push_back(std::make_pair(current_reader, current_reader->getNextItem()));
        }
        catch (const adtf_file::exceptions::EndOfFile&)
        {
        }

        if (progress_handler)
        {
            if (!progress_handler(calculateProgress()))
            {
                break;
            }
        }
    }
}

double Multiplexer::calculateProgress()
{
    auto reader_count = _stream_mapping.size();
    double progress = 0.0;
    for (auto& reader : _stream_mapping)
    {
        progress += reader.first->getProgress() / reader_count;
    }
    return progress;
}
}
}
}
