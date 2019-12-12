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

#pragma once

#include <adtfdat_processing/reader.h>
#include <memory>
#include <fstream>
#include <string>
#include <chrono>
#include <ddl.h>

class CsvReader : public adtf::dat::Reader
{
public:

    CsvReader();

    std::string getReaderIdentifier() const override
    {
        return "csv";
    }

    std::pair<bool, std::string> isCompatible(const std::string& url) const override;
    void open(const std::string& url) override;
    std::vector<adtf_file::Stream> getStreams() const override;

    adtf_file::FileItem getNextItem() override;

    double getProgress() const override;

private:
    std::ifstream _csv_file;
    ddl::CodecFactory _codec_factory;
    std::string _separator;
    std::vector<adtf_file::Stream> _streams;

    size_t _timestamp_column_index = 0;
    double _timestamp_factor = 1.0;
    std::chrono::nanoseconds _timestamp_offset;

    bool _next_item_is_a_trigger = false;
    std::chrono::nanoseconds _last_sample_timestamp;
};
