/**
 * @file
 * adtf3 stream type serializer.
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

#include <adtf_file/adtf3/adtf3_stream_type_serializer.h>
#include <adtf_file/stream_type.h>
#include <sstream>

namespace adtf_file
{
namespace adtf3
{

std::string StreamTypeSerializer::getMetaType() const
{
    return "";
}

void StreamTypeSerializer::serialize(const StreamType& stream_type, OutputStream& stream) const
{
    auto& property_type = dynamic_cast<const PropertyStreamType&>(stream_type);

    a_util::xml::DOM dom;
    dom.fromString("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?><stream xmlns=\"adtf/xsd/stream_description\"/>");
    a_util::xml::DOMElement stream_node = dom.getRoot();
    stream_node.setAttribute("name", "");
    stream_node.setAttribute("meta_type", property_type.getMetaType());

    property_type.iterateProperties([&](const char* name, const char* type, const char* value)
    {
        a_util::xml::DOMElement property_node = stream_node.createChild("property");
        property_node.setAttribute("name", name);
        property_node.setAttribute("type", type);
        property_node.setData(value);
    });

    stream << dom.toString();
}

}
}
