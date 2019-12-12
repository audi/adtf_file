/**
 * @file
 * ADTF CSV Processor example
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


#include "csv_processor.h"
#include <adtf_file/stream_type.h>
#include <adtf_file/sample.h>
#include <iomanip>
#include <adtfdat_processing/ddl_helpers.h>

static adtf_file::PluginInitializer initializer([] {
    adtf_file::getObjects().push_back(
        std::make_shared<adtf::dat::ProcessorFactoryImplementation<CsvProcessor>>());
});

CsvProcessor::CsvProcessor()
{
    setConfiguration({{"decimal_places", {"4", "uint64"}}, {"separator", {";", "string"}}});
}

CsvProcessor::~CsvProcessor()
{
}

bool CsvProcessor::isCompatible(const adtf_file::Stream& stream) const try
{
    adtf::dat::createCodecFactoryFromStreamType(stream.initial_type);
    return true;
}
catch (...)
{
    return false;
}

void CsvProcessor::open(const adtf_file::Stream& stream, const std::string& destination_url)
{
    std::tie(_codec_factory, _data_is_serialized) = adtf::dat::createCodecFactoryFromStreamType(stream.initial_type);

    _csv_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    try
    {
        _csv_file.open(destination_url);
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error("unable to open output csv file '" +
                                                  destination_url + "' for stream " + stream.name));
    }

    const auto config = getConfiguration();
    _separator = adtf::dat::getPropertyValue<std::string>(config, "separator");
    _csv_file << std::fixed << std::setprecision(adtf::dat::getPropertyValue<uint64_t>(config, "decimal_places"));

    _csv_file << "timestamp";

    for (size_t element_index = 0; element_index < _codec_factory.getStaticElementCount();
         ++element_index)
    {
        const ddl::StructElement* element;
        _codec_factory.getStaticElement(element_index, element);
        _csv_file << config.at("separator").value << element->name;
    }

    _csv_file << "\n";
}

void CsvProcessor::process(const adtf_file::FileItem& item)
{
    using namespace a_util;
    auto sample = std::dynamic_pointer_cast<const adtf_file::WriteSample>(item.stream_item);
    if (sample)
    {
        auto buffer = sample->beginBufferRead();
        auto decoder = _codec_factory.makeStaticDecoderFor(
            buffer.first,
            buffer.second,
            _data_is_serialized ? ddl::DataRepresentation::serialized :
                                  ddl::DataRepresentation::deserialized);

        _csv_file << sample->getTimeStamp().count();
        for (size_t element_index = 0; element_index < decoder.getElementCount(); ++element_index)
        {
            _csv_file << _separator;
            const auto& value = ddl::access_element::get_value(decoder, element_index);
            switch (value.getType())
            {
                case variant::VT_Bool:
                case variant::VT_Int8:
                case variant::VT_Int16:
                case variant::VT_Int32:
                case variant::VT_Int64:
                case variant::VT_UInt8:
                case variant::VT_UInt16:
                case variant::VT_UInt32:
                {
                    _csv_file << value.asInt64();
                    break;
                }
                case variant::VT_UInt64:
                {
                    _csv_file << value.asUInt64();
                    break;
                }
                case variant::VT_Float:
                case variant::VT_Double:
                {
                    _csv_file << value.asDouble();
                    break;
                }
                case variant::VT_String:
                {
                    _csv_file << value.asString();
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        _csv_file << "\n";
    }
}
