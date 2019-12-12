/**
 * @file
 * Tester init.
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

#include <adtf_file/object.h>

using namespace adtf_file;

#ifdef WIN32
__declspec(dllexport)
bool adtfFileIsDebugPlugin()
{
#ifdef _DEBUG
    return true;
#else
    return false;
#endif
}
#endif

#ifdef WIN32
__declspec(dllexport)
#endif
const char* adtfFileGetVersion()
{
    return "very old";
}


#ifdef WIN32
__declspec(dllexport)
#endif
void adtfFileSetObjectsFoo(Objects& objects)
{
}
