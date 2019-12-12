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

#include <adtfdat_processing/demultiplexer.h>

namespace adtf
{
namespace dat
{
namespace ant
{

Demultiplexer::Demultiplexer(std::shared_ptr<Reader> reader,
                             const ProcessorFactories& processor_factories)
    : _reader(reader), _processor_factories(processor_factories)
{
}

void Demultiplexer::addProcessor(const std::string& stream_name,
                                  const std::string& processor_id,
                                  const std::string& destination_url,
                                  const Configuration& configuration)
{
    auto stream = findStream(*_reader, stream_name);
    auto processor = _processor_factories.make(processor_id);
    processor->setConfiguration(configuration);
    processor->open(stream, destination_url);
    _processors[stream.stream_id] = processor;
}

void Demultiplexer::process(std::function<bool(double)> progress_handler)
{
    for (;;)
    {
        adtf_file::FileItem item;

        try
        {
            item = _reader->getNextItem();
        }
        catch (const adtf_file::exceptions::EndOfFile&)
        {
            break;
        }

        auto processor = _processors.find(item.stream_id);
        if (processor != _processors.end())
        {
            processor->second->process(item);
        }

        if (progress_handler)
        {
            if (!progress_handler(_reader->getProgress()))
            {
                break;
            }
        }
    }
}
}
}
}
