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

#pragma once

#include <adtfdat_processing/processor.h>
#include <memory>
#include <fstream>
#include <ddl.h>

class CsvProcessor : public adtf::dat::Processor
{
public:
    CsvProcessor();
    ~CsvProcessor();

    std::string getProcessorIdentifier() const override
    {
        return "csv";
    }

    bool isCompatible(const adtf_file::Stream& stream) const override;
    void open(const adtf_file::Stream& stream, const std::string& destination_url) override;
    void process(const adtf_file::FileItem& item) override;

private:
    std::ofstream _csv_file;
    ddl::CodecFactory _codec_factory;
    bool _data_is_serialized;
    std::string _separator;
};
