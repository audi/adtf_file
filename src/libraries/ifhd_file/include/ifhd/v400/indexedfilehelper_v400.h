/**
 * @file
 * Indexed file helper.
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

#ifndef INDEXEDFILE_HELPER_V400_HEADER
#define INDEXEDFILE_HELPER_V400_HEADER

namespace ifhd
{
namespace v400
{

using v201_v301::getDateTime;
using v201_v301::getHeader;
using v201_v301::updateHeader;
using v201_v301::queryFileInfo;
using v201_v301::getExtension;
using v201_v301::writeExtension;
using v201_v301::isIfhdFile;
using v201_v301::stream2FileHeader; 
using v201_v301::stream2FileHeaderExtension;
using v201_v301::stream2ChunkHeader;
using v201_v301::stream2ChunkRef; 
using v201_v301::stream2StreamRef;
using v201_v301::stream2StreamInfoHeader;
using v201_v301::stream2AdditionalStreamIndexInfo;

} // namespace
} // ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_HELPER_V400_HEADER_
