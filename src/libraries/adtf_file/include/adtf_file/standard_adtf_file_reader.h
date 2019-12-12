/**
 * @file
 * Reader.
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

#ifndef ADTF_FILE_STANDARD_READER
#define ADTF_FILE_STANDARD_READER

#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/standard_factories.h>

namespace adtf_file
{

class StandardReader : public Reader
{
public:
    StandardReader() = delete;
    StandardReader(const Reader&) = delete;
    StandardReader(StandardReader&&) = default;
    ~StandardReader() = default;
    StandardReader(const std::string& file_name,
        bool ignore_unsupported_streams = false) : Reader(file_name,
            StandardTypeDeserializers(),
            StandardSampleDeserializers())
    {};
};

}

#endif  //_ADTF_FILE_STANDARD_READER_
