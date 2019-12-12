/**
 * @file
 * ADTF File Access example
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

#include <adtf_file/standard_adtf_file_reader.h>

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <map>

// initalize ADTF File and Plugin Mechanism
static adtf_file::Objects oObjects;
static adtf_file::PluginInitializer oInitializer([]
{
    adtf_file::add_standard_objects();
});

void query_file_info(adtf_file::Reader& reader)
{
    using namespace adtf_file;

    //setup file version
    uint32_t ifhd_version = reader.getFileVersion();
    std::string adtf_version("ADTF 3 and higher");
    if (ifhd_version < ifhd::v400::version_id)
    {
        adtf_version = "below ADTF 3";
    }
    
    //begin print
    std::cout << std::endl << "File Header" << std::endl;
    std::cout << "------------------------------------------------------------------------------" << std::endl;
    std::cout << "File version      : " << reader.getFileVersion() << " - " << adtf_version << std::endl;
    std::cout << "Date              : " << reader.getDateTime().format("%d.%m.%y - %H:%M:%S") << std::endl;
    std::cout << "Duration          : " << reader.getDuration().count() << std::endl;
    std::cout << "Short description : " << getShortDescription(reader.getDescription()) << std::endl;
    std::cout << "Long description  : " << getLongDescription(reader.getDescription()) << std::endl;
    std::cout << "Chunk count       : " << reader.getItemCount() << std::endl;
    std::cout << "Extension count   : " << reader.getExtensions().size() << std::endl;
    std::cout << "Stream count      : " << reader.getStreams().size() << std::endl;

    std::cout << std::endl << "Streams" << std::endl;
    std::cout << "------------------------------------------------------------------------------" << std::endl;

    auto streams = reader.getStreams();

    for (const auto& current_stream : streams)
    {
        auto property_stream_type = std::dynamic_pointer_cast<const PropertyStreamType>(current_stream.initial_type);
        if (property_stream_type)
        {
            std::string stream_meta_type = property_stream_type->getMetaType();      
            std::cout << "Stream #" << current_stream.stream_id << " : " << current_stream.name << std::endl;
            std::cout << "    MetaType       : " << stream_meta_type << std::endl;

            property_stream_type->iterateProperties(
                [&](const char* name, 
                const char* type,
                const char* value) -> void
                {
                    std::cout << "        " << name << " - " << value << std::endl;
                });
        }
    }
}

class StreamsInfo 
{
    typedef std::map<uint16_t, std::chrono::nanoseconds> LastTimesMap;
    typedef std::map<uint16_t, std::string> StreamNameMap;

    public:
        StreamsInfo(adtf_file::Reader& reader)
        {
            auto streams = reader.getStreams();
            for (auto current_stream : streams)
            {
                _map_stream_name[current_stream.stream_id] = current_stream.name;
                UpdateType(current_stream.stream_id, current_stream.initial_type);
            }
        }
        ~StreamsInfo() = default;
       std::string GetDiffToLastChunkTime(const uint16_t& stream_id, const std::chrono::nanoseconds& current_time)
        {
            return GetLastTimeStamp(_map_last_chunk_time, stream_id, current_time);
        }
        std::string GetDiffToLastSampleStreamTime(const uint16_t& stream_id, const std::chrono::nanoseconds& current_time)
        {
            return GetLastTimeStamp(_map_last_stream_time, stream_id, current_time);
        }
        std::string GetStreamName(const uint16_t& stream_id)
        {
            return _map_stream_name[stream_id];
        }

        void UpdateType(const uint16_t& stream_id, const std::shared_ptr<const adtf_file::StreamType>& type)
        {
            auto property_stream_type = std::dynamic_pointer_cast<const adtf_file::PropertyStreamType>(type);
            if (property_stream_type)
            {
                 _map_stream_meta_type[stream_id] = property_stream_type->getMetaType();
            }
        }
        std::string GetLastStreamMetaType(const uint16_t& stream_id)
        {
            return _map_stream_meta_type[stream_id];
        }
        
    private:
        std::string GetLastTimeStamp(LastTimesMap& map_last_times,
                                                   const uint16_t& stream_id,
                                                   const std::chrono::nanoseconds& current_time)
        {
            std::chrono::nanoseconds result(-1);
            LastTimesMap::iterator it = map_last_times.find(stream_id);
            if (it != map_last_times.end())
            {
                result = current_time - it->second;
                it->second = current_time;
            }
            else
            {
                if (current_time.count() != -1)
                {
                    map_last_times[stream_id] = current_time;
                }
            }
            if (result.count() >= 0)
            {
                return a_util::strings::format("%lld", result.count());
            }
            else
            {
                return "";
            }
        }
        LastTimesMap _map_last_chunk_time;
        LastTimesMap _map_last_stream_time;
        StreamNameMap _map_stream_name;
        StreamNameMap _map_stream_meta_type;
};


void access_file_data(adtf_file::Reader& reader, const std::string& csv_file_path)
{
    using namespace adtf_file;
    
    //load stream information
    StreamsInfo stream_info(reader);

    std::cout << std::endl << "File data" << std::endl;
    std::cout << "------------------------------------------------------------------------------" << std::endl;

    utils5ext::File csv_file;
    csv_file.open(csv_file_path, utils5ext::File::om_append | utils5ext::File::om_write);

    //set the labels
    csv_file.writeLine("stream;stream_name;chunk_type;stream_type;chunk_time;samplestream_time;chunk_time_delta_to_lastofstream;samplestream_time_delta_to_lastofstream");

    size_t item_count = 0;
    for (;; ++item_count)
    {
        try
        {
            auto item = reader.getNextItem();
            auto chunk_time = item.time_stamp;
            
            std::string chunk_type;
            auto type = std::dynamic_pointer_cast<const StreamType>(item.stream_item);
            auto data = std::dynamic_pointer_cast<const Sample>(item.stream_item);
            auto trigger = std::dynamic_pointer_cast<const Trigger>(item.stream_item);
            std::chrono::nanoseconds sample_time(-1);
            std::string sample_time_string("");
            if (type)
            {
                //the type change is part of the 
                chunk_type = "stream_type";
                stream_info.UpdateType(item.stream_id,
                                       type);
            }
            else if (data)
            {
                chunk_type = "sample";
                auto sample_data = std::dynamic_pointer_cast<const DefaultSample>(data);
                if (sample_data)
                {
                    sample_time = sample_data->getTimeStamp();
                    sample_time_string = a_util::strings::format("%lld", sample_time.count());
                }
            }
            else if (trigger)
            {
                chunk_type = "trigger";
            }
            
            csv_file.writeLine(a_util::strings::format("%d;%s;%s;%s;%lld;%s;%s;%s",
                static_cast<int>(item.stream_id),
                stream_info.GetStreamName(item.stream_id).c_str(),
                chunk_type.c_str(),
                stream_info.GetLastStreamMetaType(item.stream_id).c_str(),
                chunk_time.count(),
                sample_time_string.c_str(),
                stream_info.GetDiffToLastChunkTime(item.stream_id, chunk_time).c_str(),
                stream_info.GetDiffToLastSampleStreamTime(item.stream_id, sample_time).c_str()
                ));
        }
        catch (const exceptions::EndOfFile&)
        {
            break;
        }
    }

    csv_file.close();
}

adtf_file::Reader create_reader(const a_util::filesystem::Path& adtfdat_file_path)
{
    //open file -> create reader from former added settings
    adtf_file::Reader reader(adtfdat_file_path,
                             adtf_file::getFactories<adtf_file::StreamTypeDeserializers,
                                                     adtf_file::StreamTypeDeserializer>(),
                             adtf_file::getFactories<adtf_file::SampleDeserializerFactories,
                                                     adtf_file::SampleDeserializerFactory>(),
                             std::make_shared<adtf_file::sample_factory<adtf_file::DefaultSample>>(),
                             std::make_shared<adtf_file::stream_type_factory<adtf_file::DefaultStreamType>>());

    return reader;
}

int main(int argc, char* argv[])
{
    if (argc < 3 || argv[1] == NULL || argv[2] == NULL)
    {
        std::cerr << "usage: " << argv[0] << " <adtfdat> <csv> [<adtffileplugin> ...]" << std::endl;
        return -1;
    }

    //set path for adtfdat|dat and csv file
    a_util::filesystem::Path adtfdat_file = argv[1];
    a_util::filesystem::Path csv_file = argv[2];

    try
    {
        //verify adtf|dat file
        if (("adtfdat" != adtfdat_file.getExtension())
            && ("dat" != adtfdat_file.getExtension()))
        {
            throw std::runtime_error(adtfdat_file + " is not valid, please use .adtfdat (ADTF 3.x) or .dat (ADTF 2.x).");
        }

        //verify csv file
        if ("csv" != csv_file.getExtension())
        {
            throw std::runtime_error(csv_file + " is not valid, please use .csv for sample data export.");
        }

        //check for additional adtffileplugins
        for (int i = 3; i < argc; i++)
        {
            a_util::filesystem::Path adtffileplugin = argv[i];
            if ("adtffileplugin" == adtffileplugin.getExtension())
            {
                adtf_file::loadPlugin(adtffileplugin);
            }
        }

        //setup reader
        auto reader = create_reader(adtfdat_file);
    
        //print information about adtfdat|dat file
        query_file_info(reader);
    
        //export sample data
        access_file_data(reader, csv_file);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return -2;
    }
    return 0;
}
