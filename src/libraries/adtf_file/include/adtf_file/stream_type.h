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

#ifndef ADTF_FILE_STREAM_TYPE
#define ADTF_FILE_STREAM_TYPE

#include <string>
#include <unordered_map>
#include <functional>
#include "stream_item.h"

namespace adtf_file
{

class PropertyStreamType
{
    public:
        virtual void setMetaType(const std::string& meta_type) = 0;
        virtual PropertyStreamType& setProperty(const std::string& name, const std::string& type, const std::string& value) = 0;

        virtual std::string getMetaType() const = 0;
        virtual std::pair<std::string, std::string> getProperty(const std::string& name) const = 0;
        virtual void iterateProperties(std::function<void(const char*, const char*, const char*)> functor) const = 0;
};

class DefaultStreamType: public PropertyStreamType, public StreamType
{
    public:
        DefaultStreamType() = default;
        DefaultStreamType(const std::string& meta_type);

        void setMetaType(const std::string& meta_type) override;
        PropertyStreamType& setProperty(const std::string& name, const std::string& type, const std::string& value) override;

        std::string getMetaType() const override;
        std::pair<std::string, std::string> getProperty(const std::string& name) const override;
        void iterateProperties(std::function<void(const char*, const char*, const char*)> functor) const override;

    private:
        std::string _meta_type;
        std::unordered_map<std::string, std::pair<std::string, std::string>> _properties;
};


}

#endif
