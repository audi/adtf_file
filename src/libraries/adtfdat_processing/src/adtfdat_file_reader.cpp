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
#include <adtfdat_processing/adtfdat_file_reader.h>
#include <adtf_file/standard_factories.h>

namespace adtf
{
namespace dat
{
namespace ant
{

std::pair<bool, std::string> AdtfDatReader::isCompatible(const std::string& url) const
{
    try
    {
        adtf_file::Reader(
            url,
            adtf_file::getFactories<adtf_file::StreamTypeDeserializers,
                                     adtf_file::StreamTypeDeserializer>(),
            adtf_file::getFactories<adtf_file::SampleDeserializerFactories,
                                     adtf_file::SampleDeserializerFactory>(),
            std::make_shared<adtf_file::sample_factory<adtf_file::DefaultSample>>(),
            std::make_shared<adtf_file::stream_type_factory<adtf_file::DefaultStreamType>>(),
            true);
        return std::make_pair(true, std::string());
    }
    catch (const std::exception& error)
    {
        return std::make_pair(false, error.what());
    }
}

void AdtfDatReader::open(const std::string& url)
{
    _reader.reset(new adtf_file::Reader(
        url,
        adtf_file::getFactories<adtf_file::StreamTypeDeserializers,
                                 adtf_file::StreamTypeDeserializer>(),
        adtf_file::getFactories<adtf_file::SampleDeserializerFactories,
                                 adtf_file::SampleDeserializerFactory>(),
        std::make_shared<adtf_file::sample_factory<adtf_file::DefaultSample>>(),
        std::make_shared<adtf_file::stream_type_factory<adtf_file::DefaultStreamType>>(),
        true));
}

std::vector<adtf_file::Stream> AdtfDatReader::getStreams() const
{
    return _reader->getStreams();
}

std::vector<adtf_file::Extension> AdtfDatReader::getExtensions() const
{
    return _reader->getExtensions();
}

adtf_file::FileItem AdtfDatReader::getNextItem()
{
    auto next_item = _reader->getNextItem();
    ++_processed_items;
    return next_item;
}

double AdtfDatReader::getProgress() const
{
    return static_cast<double>(_processed_items) / _reader->getItemCount();
}
}
}
}
