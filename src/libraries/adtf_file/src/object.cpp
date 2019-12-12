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
#include <sstream>

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

Objects*& get_objects_pointer()
{
    static Objects* objects = 0;
    return objects;
}

}

Objects::Objects()
{
    detail::get_objects_pointer() = this;
}

Objects& getObjects()
{
    return *detail::get_objects_pointer();
}

void loadPlugin(const std::string& file_name)
{
#ifdef WIN32
    auto library_handle = ::LoadLibrary(file_name.c_str());
    if (!library_handle)
    {
        throw std::runtime_error("unable to load shared library '" + file_name + "'");
    }
#else
    auto library_handle = ::dlopen(file_name.c_str(), RTLD_LAZY);
    if (!library_handle)
    {
        throw std::runtime_error("unable to load shared library '" + file_name + "': " + dlerror());
    }
#endif

#ifdef WIN32
    std::function<bool()> is_debug_plugin = reinterpret_cast<bool(*)()>(::GetProcAddress(library_handle, "adtfFileIsDebugPlugin"));
    if (!is_debug_plugin)
    {
        throw std::runtime_error("the shared library '" + file_name + "' does not provide the required adtfFileIsDebugPlugin method");
    }

#ifdef _DEBUG
    if (!is_debug_plugin())
    {
        throw std::runtime_error("the shared library '" + file_name + "' is not compiled in debug mode which this executable is");
    }
#else
    if (is_debug_plugin())
    {
        throw std::runtime_error("the shared library '" + file_name + "' is compiled in debug mode which this executable is not");
    }
#endif
#endif

#ifdef WIN32
    std::function<void(Objects&)> set_objects_method = reinterpret_cast<void(*)(Objects&)>(::GetProcAddress(library_handle, "adtfFileSetObjects1"));
#else
    std::function<void(Objects&)> set_objects_method = reinterpret_cast<void(*)(Objects&)>(dlsym(library_handle, "adtfFileSetObjects1"));
#endif
    if (!set_objects_method)
    {
        const char* plugin_version = "0.5.0 or less";
        #ifdef WIN32
            std::function<const char*()> get_version_method = reinterpret_cast<const char*(*)()>(::GetProcAddress(library_handle, "adtfFileGetVersion"));
        #else
            std::function<const char*()> get_version_method = reinterpret_cast<const char*(*)()>(dlsym(library_handle, "adtfFileGetVersion"));
        #endif

        if (get_version_method)
        {
            plugin_version = get_version_method();
        }

        std::ostringstream error;
        error << "The shared library '" << file_name << "' does not provide the required version of the adtfFileSetObjects method."
              << "This tool uses version " << ADTF_FILE_VERSION << " of the adtf_file library, the plugin was built with version " << plugin_version << ".";
        throw std::runtime_error(error.str());
    }

    set_objects_method(getObjects());
}

}
