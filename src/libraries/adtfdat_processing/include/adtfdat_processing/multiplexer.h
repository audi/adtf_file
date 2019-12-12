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
#include <unordered_map>

#include <adtf_file/adtf_file_writer.h>

#include "configuration.h"
#include "reader.h"

namespace adtf
{
namespace dat
{
namespace ant
{

/**
 * Helper wrapper that optionally applies a timestamp offset and limits items to a given time range.
 */
class OffsetReaderWrapper : public Reader
{
public:
    /**
     * Initializes a new wrapper
     * @param [in] original_reader The reader that does the heavy lifting.
     * @param [in] offset The offset that is added to all timestamps.
     * @param [in] start_offset Items before this (without the offset applied) will be skipped.
     * @param [in] end_offset Items after this will be dropped.
     */
    OffsetReaderWrapper(const std::shared_ptr<Reader> original_reader,
                        std::chrono::nanoseconds offset,
                        std::chrono::nanoseconds start_offset,
                        std::chrono::nanoseconds end_offset);

    std::string getReaderIdentifier() const override;
    std::pair<bool, std::string> isCompatible(const std::string& url) const override;
    Configuration getConfiguration() const override;
    void setConfiguration(const Configuration& configuration) override;
    void open(const std::string& url) override;
    std::vector<adtf_file::Stream> getStreams() const override;
    double getProgress() const override;
    adtf_file::FileItem getNextItem() override;

private:
    std::shared_ptr<Reader> _original_reader;
    std::chrono::nanoseconds _offset;
    std::chrono::nanoseconds _start_offset;
    std::chrono::nanoseconds _end_offset;

    adtf_file::FileItem _next_item;
    std::unordered_map<uint16_t, std::shared_ptr<const adtf_file::StreamItem>> _last_stream_types;
};

/**
 * This can multiplex multiple streams from multiple readers into a single ADTFDAT file.
 */
class Multiplexer
{
public:
    /**
     * Initiallizes the multiplexer.
     * @param [in] destination_file_name The output ADTFDAT filename.
     * @param [in] target_adtf_version The target file version.
     * @param [in] skip_stream_types_and_triggers Whether or not to pass on triggers and
     *             stream types to the writer.
     */
    Multiplexer(const std::string& destination_file_name,
                adtf_file::Writer::TargetADTFVersion target_adtf_version =
                    adtf_file::Writer::TargetADTFVersion::adtf3ns,
                bool skip_stream_types_and_triggers = false);

    /**
     * Adds a new stream to the output.
     * @param [in] reader The reader that the stream belongs to.
     * @param [in] stream_name The name of the stream.
     * @param [in] destination_stream_name The name of the stream in the output file.
     * @param [in] serializer The serializer that is used to serialize the samples.
     */
    void addStream(const std::shared_ptr<Reader> reader,
                   const std::string& stream_name,
                   const std::string& destination_stream_name,
                   const std::shared_ptr<adtf_file::SampleSerializer> serializer);

    /**
     * Adds a new extension to the output
     * @param [in] name Name of the extension
     * @param [in] extension_data The extension data.
     * @param [in] extension_size The extension data size.
     * @param [in] user_id The user id of the extension.
     * @param [in] type_id The type id of the extension.
     * @param [in] version_id The version id of the extension.
     */
    void addExtension(const std::string& name,
                      const void* extension_data,
                      size_t extension_size,
                      uint32_t user_id = 0,
                      uint32_t type_id = 0,
                      uint32_t version_id = 0);

    /**
     * Performs the generation of the multiplexed file.
     * @param [inout] progress_handler A callback that is called for each prosessed stream item.
     *                The value passed to thes method is in the range [0.0, 1.0].
     *                Processing will be canceled if this returns false.
     */
    void process(std::function<bool(double)> progress_handler);

private:
    double calculateProgress();

private:
    adtf_file::Writer _writer;
    bool _skip_stream_types_and_triggers;
    std::unordered_map<std::shared_ptr<Reader>, std::unordered_map<uint16_t, size_t>>
        _stream_mapping;
};
}

using ant::OffsetReaderWrapper;
using ant::Multiplexer;
}
}
