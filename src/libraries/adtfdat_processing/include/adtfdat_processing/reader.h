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
#include <sstream>
#include <algorithm>
#include <adtf_file/adtf_file_reader.h>
#include "configuration.h"

namespace adtf
{
namespace dat
{
namespace ant
{

/**
 * Base class for readers.
 *
 * A reader provides streams of StreamTypes, Samples and Triggers.
 */
class Reader : public Configurable
{
public:
    /**
     * @return The name of reader implementation.
     */
    virtual std::string getReaderIdentifier() const = 0;

    /**
     * This is called to check, whether a reader can handle a given URL.
     * @param [in] url The URL, i.e. a filename.
     * @return The first value describes whether the reader can handle the url or not, the
     *         second is an error message in case that it can't.
     */
    virtual std::pair<bool, std::string> isCompatible(const std::string& url) const = 0;

    /**
     * Called to open the given URL.
     * @param [in] url The URL, what else.
     */
    virtual void open(const std::string& url) = 0;

    /**
     * @return All available streams in the opened source.
     */
    virtual std::vector<adtf_file::Stream> getStreams() const = 0;

    /**
     * @return The current progress in the range [0.0, 1.0].
     */
    virtual double getProgress() const = 0;

    /**
     * @return The next item from the source.
     */
    virtual adtf_file::FileItem getNextItem() = 0;
};

/**
 * Factory that creates readers.
 */
class ReaderFactory : public adtf_file::Object
{
public:
    /**
     * @return A new instance of a reader.
     */
    virtual std::shared_ptr<Reader> make() const = 0;
};

/**
 * Helper template to create reader factories.
 */
template <typename READER_CLASS>
class ReaderFactoryImplementation : public ReaderFactory
{
public:
    std::shared_ptr<Reader> make() const override
    {
        return std::make_shared<READER_CLASS>();
    }
};

/**
 * Method to find a stream of a reader.
 * @param [in] reader The reader.
 * @param [in] stream_name The name of the stream to look for.
 * @return The stream information.
 */
inline adtf_file::Stream findStream(const Reader& reader, const std::string& stream_name)
{
    auto streams = reader.getStreams();
    auto stream =
        std::find_if(streams.begin(), streams.end(), [&](const adtf_file::Stream& stream) {
            return stream.name == stream_name;
        });

    if (stream == streams.end())
    {
        throw std::runtime_error("no such stream: " + stream_name);
    }

    return *stream;
}

/**
 * A container for reader factories.
 */
class ReaderFactories : private std::unordered_map<std::string, std::shared_ptr<const ReaderFactory>>
{
public:
    /**
     * Adds the given factory to the container-
     * @param [in] factory The factory to add.
     */
    void add(const std::shared_ptr<const ReaderFactory>& factory)
    {
        auto reader = factory->make();
        (*this)[reader->getReaderIdentifier()] = factory;
    }

    /**
     * @param reader_id The identifier of the reader implementation
     * @return A new reader instance.
     */
    std::shared_ptr<Reader> make(const std::string& reader_id) const
    {
        auto factory = find(reader_id);
        if (factory == end())
        {
            throw std::runtime_error("no factory for reader " + reader_id);
        }

        return factory->second->make();
    }

    /**
     * @param [in] url The URL.
     * @return Returns all readers capable of reading the given URL.
     * @throws An exception when no capable reader is found.
     */
    std::map<std::string, Configuration> getCapableReaders(const std::string& url) const
    {
        std::map<std::string, Configuration> capable_readers;
        std::map<std::string, std::string> uncapable_readers;
        for (auto& factory : *this)
        {
            auto reader = factory.second->make();
            auto compatible = reader->isCompatible(url);
            if (compatible.first)
            {
                capable_readers[reader->getReaderIdentifier()] = reader->getConfiguration();
            }
            else
            {
                uncapable_readers.emplace(reader->getReaderIdentifier(), compatible.second);
            }
        }

        if (capable_readers.empty())
        {
            std::ostringstream error_message;
            error_message << "no reader capable of reading '" << url << "' available";
            for (auto& uncapable_reader : uncapable_readers)
            {
                error_message << "\nreader '" << uncapable_reader.first
                              << "' said: " << uncapable_reader.second;
            }

            throw std::runtime_error(error_message.str());
        }

        return capable_readers;
    }
};
}

using ant::Reader;
using ant::Reader;
using ant::ReaderFactory;
using ant::ReaderFactoryImplementation;
using ant::findStream;
using ant::ReaderFactories;
}
}
