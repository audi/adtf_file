/**
 * @file
 * ADTF CSV Reader example
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

#include "csv_reader.h"
#include <adtf_file/stream_type.h>
#include <adtf_file/sample.h>
#include <sstream>
#include <cerrno>
#include <string.h>
#include <adtfdat_processing/ddl_helpers.h>

static adtf_file::PluginInitializer initializer([] {
    adtf_file::getObjects().push_back(
        std::make_shared<adtf::dat::ReaderFactoryImplementation<CsvReader>>());
});

CsvReader::CsvReader()
{
    setConfiguration({{"stream_name", {"csv", "string"}},
                      {"separator", {";", "string"}},
                      {"timestamp_column_index", {"0", "uint64"}},
                      {"timestamp_factor", {"1.0", "double"}},
                      {"timestamp_offset", {"0", "int64"}},
                      {"ddl_data_type", {"tFloat64", "string"}},});
}

std::pair<bool, std::string> CsvReader::isCompatible(const std::string &url) const
{
    if (url.rfind("csv") == url.size() - 3)
    {
        return {true, ""};
    }

    return {false, "not a csv file."};
}

std::vector<std::string> split_line(std::string line, const std::string& separator)
{
    std::vector<std::string> columns;
    auto separator_position = std::string::npos;
    while ((separator_position = line.find(separator)) != std::string::npos)
    {
        columns.push_back(line.substr(0, separator_position));
        line = line.substr(separator_position + 1);
    }

    if (!line.empty())
    {
        columns.push_back(line);
    }

    return columns;
}

std::string build_ddl(const std::vector<std::string>& columns,
                      const std::string& stream_name,
                      const std::string& ddl_data_type,
                      size_t timestamp_column_index)
{
    std::unique_ptr<ddl::DDLDescription> default_description(ddl::DDLDescription::createDefault());
    auto data_type = default_description->getDataTypeByName(ddl_data_type);
    if (!data_type)
    {
        throw std::runtime_error("unable to resolve data type: " + ddl_data_type);
    }
    auto type_byte_size = data_type->getNumBits() / 8;
    if (data_type->getNumBits() % 8)
    {
        ++type_byte_size;
    }

    // I hate to do it manually, but the ddl:: implementation is sooo cumbersome,
    // that it is barely usable (especially the memory handling)
    std::ostringstream struct_definition;
    struct_definition << "<struct name=\"" << stream_name << "\" alignment=\"1\" version=\"1\">";
    size_t byte_position = 0;
    for (size_t column_index = 0; column_index < columns.size(); ++column_index)
    {
        if (column_index != timestamp_column_index)
        {
            struct_definition << "<element name=\"" << columns[column_index] << "\" alignment=\"1\" type=\"" << ddl_data_type << "\" bytepos=\"" << byte_position << "\" arraysize=\"1\" byteorder=\"LE\"/>";
            byte_position += type_byte_size;
        }
    }
    struct_definition << "</struct>";

    return struct_definition.str();
}

void check_error_bits(const std::ifstream& stream)
{
    if (stream.fail() ||
        stream.bad())
    {
        throw std::runtime_error(std::string("unable to read from file: ") + strerror(errno));
    }
}

void CsvReader::open(const std::string &url)
{
    try
    {
        _csv_file.open(url);
        check_error_bits(_csv_file);
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error("unable to open input csv file '" + url + "'"));
    }

    const auto config = getConfiguration();
    auto stream_name = adtf::dat::getPropertyValue<std::string>(config, "stream_name");
    _separator = adtf::dat::getPropertyValue<std::string>(config, "separator");
    auto ddl_data_type = adtf::dat::getPropertyValue<std::string>(config, "ddl_data_type");
    _timestamp_column_index = adtf::dat::getPropertyValue<uint64_t>(config, "timestamp_column_index");
    _timestamp_factor = adtf::dat::getPropertyValue<double>(config, "timestamp_factor");
    _timestamp_offset = std::chrono::microseconds(adtf::dat::getPropertyValue<int64_t>(config, "timestamp_offset"));

    std::string first_line;
    std::getline(_csv_file, first_line);
    if (first_line.empty())
    {
        throw std::runtime_error("unable to read header line from '" + url + "'");
    }

    auto columns = split_line(first_line, _separator);
    auto struct_definition = build_ddl(columns, stream_name, ddl_data_type, _timestamp_column_index);

    _codec_factory = ddl::CodecFactory(stream_name.c_str(), struct_definition.c_str());
    A_UTIL_THROW_IF_FAILED(_codec_factory.isValid(),"unable to create codec factory for '" + url + "'");

    auto type = std::make_shared<adtf_file::DefaultStreamType>("adtf/default");
    type->setProperty("md_definitions", "cString", struct_definition);
    type->setProperty("md_struct", "cString", stream_name);
    type->setProperty("md_data_serialized", "tBool", "false");
    _streams.push_back({0,
                        stream_name,
                        0,
                        std::chrono::seconds(0),
                        std::chrono::seconds(0),
                        adtf::dat::createAdtfDefaultStreamType(stream_name, struct_definition, false)});
}

std::vector<adtf_file::Stream> CsvReader::getStreams() const
{
    return _streams;
}

adtf_file::FileItem CsvReader::getNextItem()
{
    if (_next_item_is_a_trigger)
    {
        _next_item_is_a_trigger = false;
        return {0, _last_sample_timestamp, std::make_shared<adtf_file::Trigger>()};
    }

    std::string line;
    if (!std::getline(_csv_file, line))
    {
        if (_csv_file.eof())
        {
            throw adtf_file::exceptions::EndOfFile();
        }

        check_error_bits(_csv_file);
    }

    auto items = split_line(line, _separator);

    auto sample = std::make_shared<adtf_file::DefaultSample>();
    {
        auto codec = _codec_factory.makeStaticCodecFor(sample->beginBufferWrite(_codec_factory.getStaticBufferSize()),
                                                       _codec_factory.getStaticBufferSize());

        for (size_t item_index = 0; item_index < items.size(); ++item_index)
        {
            auto value = std::stod(items[item_index]);

            if (item_index == _timestamp_column_index)
            {
                sample->setTimeStamp(std::chrono::microseconds(static_cast<int64_t>(value * _timestamp_factor)) + _timestamp_offset);
            }
            else
            {
                auto corrected_index = item_index < _timestamp_column_index ? item_index : item_index - 1;
                A_UTIL_THROW_IF_FAILED(codec.setElementValue(corrected_index, value),
                                       "unable to update element with index " + std::to_string(corrected_index));
            }
        }

        sample->endBufferWrite();
    }

    _next_item_is_a_trigger = true;
    _last_sample_timestamp = sample->getTimeStamp();
    return {0, _last_sample_timestamp, sample};
}

double CsvReader::getProgress() const
{
    return 0.0;
}

