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

#include "reader.h"
#include <adtf_file/adtf_file_reader.h>

namespace adtf
{
namespace dat
{
namespace ant
{

/**
 * An implementation of a reader that can read ADTFDAT files.
 */
class AdtfDatReader : public adtf::dat::ant::Reader
{
public:
    std::string getReaderIdentifier() const override
    {
        return "adtfdat";
    }

    std::pair<bool, std::string> isCompatible(const std::string& url) const override;
    void open(const std::string& url) override;
    std::vector<adtf_file::Stream> getStreams() const override;

    std::vector<adtf_file::Extension> getExtensions() const;

    adtf_file::FileItem getNextItem() override;

    double getProgress() const override;

private:
    std::unique_ptr<adtf_file::Reader> _reader;
    size_t _processed_items = 0;
};
}

using ant::AdtfDatReader;
}
}
