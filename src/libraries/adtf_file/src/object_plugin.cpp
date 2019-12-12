/**
 * @file
 * Factory.
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

#ifdef WIN32
    #include <Windows.h>
#else
    #include <dlfcn.h>
    #include <string.h>
#endif

namespace adtf_file
{

namespace detail
{

Objects*& get_objects_pointer();
std::function<void()>& get_initialization_callback()
{
    static std::function<void()> initialization_callback;
    return initialization_callback;
}

}

PluginInitializer::PluginInitializer(std::function<void()> initialization_callback)
{
    detail::get_initialization_callback() = initialization_callback;
    if (detail::get_objects_pointer())
    {
        detail::get_initialization_callback()();
    }
}

extern "C"
{

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
    return ADTF_FILE_VERSION;
}


#ifdef WIN32
__declspec(dllexport)
#endif
void adtfFileSetObjects1(Objects& objects)
{
    detail::get_objects_pointer() = &objects;
    if (detail::get_initialization_callback())
    {
        detail::get_initialization_callback()();
    }
}

}

}
