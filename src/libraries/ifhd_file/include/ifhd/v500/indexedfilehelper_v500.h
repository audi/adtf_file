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

#ifndef INDEXEDFILE_HELPER_V500_HEADER
#define INDEXEDFILE_HELPER_V500_HEADER

#include <ifhd/v400/indexedfilehelper_v400.h>

namespace ifhd
{
namespace v500
{

using v400::getDateTime;
using v400::getHeader;
using v400::updateHeader;
using v400::queryFileInfo;
using v400::getExtension;
using v400::writeExtension;
using v400::isIfhdFile;
using v400::stream2FileHeader;
using v400::stream2FileHeaderExtension;
using v400::stream2ChunkHeader;
using v400::stream2ChunkRef;
using v400::stream2StreamRef;
using v400::stream2StreamInfoHeader;
using v400::stream2AdditionalStreamIndexInfo;

} // namespace
} // ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_HELPER_V500_HEADER_
