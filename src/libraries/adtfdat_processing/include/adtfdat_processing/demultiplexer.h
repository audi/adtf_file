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

#pragma once

#include <string>
#include <functional>
#include <map>

#include "configuration.h"
#include "reader.h"
#include "processor.h"

namespace adtf
{
namespace dat
{
namespace ant
{

/**
 * This one demultiplexes the streams of an ADTFDAT file (or any other @ref Reader), and passes the
 * stream items to @ref Processor implementations.
 */
class Demultiplexer
{
public:
    /**
     * Initializes the demultiplexer
     * @param [in] reader The source reader.
     * @param [in] processor_factories Factories for processors, these are used by @ref
     * addProcessor.
     */
    Demultiplexer(std::shared_ptr<Reader> reader, const ProcessorFactories& processor_factories);

    /**
     * Adds a processor for a given stream.
     * @param [in] stream_name The name of the stream.
     * @param [in] processor_id The identifier of the processor implementation.
     * @param [in] destination_url The output URL.
     * @param [in] configuration The configuration of the processor.
     */
    void addProcessor(const std::string& stream_name,
                      const std::string& processor_id,
                      const std::string& destination_url,
                      const Configuration& configuration);

    /**
     * Processes all selected streams.
     * @param [inout] progress_handler A callback that is called for each prosessed stream item.
     *                The value passed to thes method is in the range [0.0, 1.0].
     *                Processing will be canceled if this returns false.
     */
    void process(std::function<bool(double)> progress_handler);

private:
    std::shared_ptr<Reader> _reader;
    const ProcessorFactories& _processor_factories;
    std::unordered_map<uint16_t, std::shared_ptr<Processor>> _processors;
};
}

using ant::Demultiplexer;
}
}
