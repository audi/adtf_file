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
#include <adtf_file/adtf_file_reader.h>
#include "configuration.h"
#include "reader.h"

namespace adtf
{
namespace dat
{
namespace ant
{

/**
 * A processer handles incoming StreamTypes, Samples and Triggers.
 */
class Processor : public Configurable
{
public:
    /**
     * @return The name of processor implementation.
     */
    virtual std::string getProcessorIdentifier() const = 0;

    /**
     * This is called to check, whether a processor can handle a given stream.
     */
    virtual bool isCompatible(const adtf_file::Stream& stream) const = 0;

    /**
     * Opens the processor for handling subsequent items from the given stream.
     * @param [in] stream The stream.
     * @param [in] destination_file_name The output url, i.e. a filename.
     */
    virtual void open(const adtf_file::Stream& stream, const std::string& destination_url) = 0;

    /**
     * This is called for every item of the stream.
     * @param [in] item The next item of the stream.
     */
    virtual void process(const adtf_file::FileItem& item) = 0;
};

/**
 * A Factory for processors.
 */
class ProcessorFactory : public adtf_file::Object
{
public:
    /**
     * @return A new instance of a processor.
     */
    virtual std::shared_ptr<Processor> make() const = 0;
};

/**
 * Helper template to create processor factories.
 */
template <typename PROCESSOR_CLASS>
class ProcessorFactoryImplementation : public ProcessorFactory
{
public:
    std::shared_ptr<Processor> make() const override
    {
        return std::make_shared<PROCESSOR_CLASS>();
    }
};

/**
 * A container for processor factories.
 */
class ProcessorFactories
    : private std::unordered_map<std::string, std::shared_ptr<const ProcessorFactory>>
{
public:
    /**
     * Adds the given factory to the container-
     * @param [in] factory The factory to add.
     */
    void add(const std::shared_ptr<const ProcessorFactory>& factory)
    {
        auto processor = factory->make();
        (*this)[processor->getProcessorIdentifier()] = factory;
    }

    /**
     * @param processor_id The identifier of the processor implementation
     * @return A new processor instance.
     */
    std::shared_ptr<Processor> make(const std::string& processor_id) const
    {
        auto factory = find(processor_id);
        if (factory == end())
        {
            throw std::runtime_error("no factory for processor " + processor_id);
        }

        return factory->second->make();
    }

    /**
     * @param [in] stream The stream.
     * @return Returns all processors capable of processing the given stream.
     */
    std::map<std::string, Configuration>
        getCapableProcessors(const adtf_file::Stream& stream) const
    {
        std::map<std::string, Configuration> capable_processors;
        for (auto& factory : *this)
        {
            auto processor = factory.second->make();
            if (processor->isCompatible(stream))
            {
                capable_processors[processor->getProcessorIdentifier()] =
                    processor->getConfiguration();
            }
        }

        return capable_processors;
    }
};
}

using ant::Processor;
using ant::ProcessorFactory;
using ant::ProcessorFactoryImplementation;
using ant::ProcessorFactories;
}
}
