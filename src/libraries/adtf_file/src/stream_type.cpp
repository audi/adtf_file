/**
 * @file
 * stream type definition.
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

#include <adtf_file/stream_type.h>
#include <sstream>

namespace adtf_file
{

DefaultStreamType::DefaultStreamType(const std::string& meta_type):
    _meta_type(meta_type)
{
}

void DefaultStreamType::setMetaType(const std::string& meta_type)
{
    this->_meta_type = meta_type;
}

PropertyStreamType& DefaultStreamType::setProperty(const std::string& name, const std::string& type, const std::string& value)
{
    _properties[name] = std::make_pair(type, value);
    return *this;
}

std::string DefaultStreamType::getMetaType() const
{
    return _meta_type;
}

std::pair<std::string, std::string> DefaultStreamType::getProperty(const std::string& name) const
{
    auto property = _properties.find(name);
    if (property == _properties.end())
    {
        throw std::runtime_error("no property with name " + name);
    }

    return property->second;
}

void DefaultStreamType::iterateProperties(std::function<void(const char*, const char*, const char*)> functor) const
{
    for (auto& property: _properties)
    {
        functor(property.first.c_str(), property.second.first.c_str(), property.second.second.c_str());
    }
}

}
