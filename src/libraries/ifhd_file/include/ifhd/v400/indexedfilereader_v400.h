/**
 * @file
 * Indexed file reader.
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

#ifndef INDEXEDFILE_READER_V400_CLASS_HEADER
#define INDEXEDFILE_READER_V400_CLASS_HEADER

namespace ifhd
{
namespace v400
{

class IndexedFileReader : public v201_v301::IndexedFileReader
{
public:
    IndexedFileReader() : v201_v301::IndexedFileReader()
    {
    };

    std::set<uint32_t> getSupportedVersions() const
    {
        return {v201_v301::version_id_beta,
                v201_v301::version_id_with_history,
                v201_v301::version_id_with_history_end_offset,
                v201_v301::version_id,
                version_id};
    }
};

} // namespace
} // namespace

//*************************************************************************************************
#endif // _INDEXEDFILE_READER_V400_CLASS_HEADER_
