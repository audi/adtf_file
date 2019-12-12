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

#ifndef ADTF_FILE_FACTORY_INCLUDED
#define ADTF_FILE_FACTORY_INCLUDED

#include <vector>
#include <memory>
#include <functional>

namespace adtf_file
{

class Object
{
    public:
        virtual ~Object() = default;
};

class Objects: public std::vector<std::shared_ptr<Object>>
{
    public:
        Objects();

        template <typename OBJECT_TYPE>
        std::vector<std::shared_ptr<OBJECT_TYPE>> getAllOfType() const
        {
            std::vector<std::shared_ptr<OBJECT_TYPE>> all_of_given_type;
            for (auto& object: *this)
            {
                auto object_of_given_type = std::dynamic_pointer_cast<OBJECT_TYPE>(object);
                if (object_of_given_type)
                {
                    all_of_given_type.push_back(object_of_given_type);
                }
            }

            return all_of_given_type;
        }
};

Objects& getObjects();

template <typename FACTORIES, typename FACTORY_TYPE>
FACTORIES getFactories()
{
    FACTORIES factories;

    for (auto& factory: getObjects().getAllOfType<FACTORY_TYPE>())
    {
        factories.add(factory);
    }

    return factories;
}

class PluginInitializer
{
    public:
        PluginInitializer(std::function<void()> initialization_callback);
};

void loadPlugin(const std::string& file_name);

}

extern "C"
{

#ifdef WIN32
__declspec(dllimport)
bool adtfFileIsDebugPlugin();
#endif

#ifdef WIN32
__declspec(dllimport)
#endif
const char* adtfFileGetVersion();

#ifdef WIN32
__declspec(dllimport)
#endif
void adtfFileSetObjects1(adtf_file::Objects& objects);

}


#endif // _ADTF_FILE_ADTF_FILE_READER_INCLUDED_
