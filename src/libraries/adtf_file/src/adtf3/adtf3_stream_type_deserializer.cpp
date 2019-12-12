/**
 * @file
 * adtf3 stream type deserializer.
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

#include <adtf_file/adtf3/adtf3_stream_type_deserializer.h>
#include <a_util/xml.h>

namespace adtf_file
{

namespace adtf3
{

std::string StreamTypeDeserializer::getId() const
{
    return "";
}

void StreamTypeDeserializer::deserialize(InputStream& stream, PropertyStreamType& stream_type) const
{
    using namespace a_util::xml;

    std::string xml;
    stream >> xml;

    DOM dom;
    if (!dom.fromString(xml))
    {
        throw std::runtime_error("unabel to parse stream type: " + dom.getLastError());
    }

    DOMElement stream_node;
    if (!dom.findNode("/stream", stream_node))
    {
        throw std::runtime_error("unable to parse stream type");
    }

    if (!stream_node.hasAttribute("meta_type"))
    {
        throw std::runtime_error("no meta type specification in stream type");
    }
    stream_type.setMetaType(stream_node.getAttribute("meta_type"));

    DOMElementList property_nodes;
    if (stream_node.findNodes("property", property_nodes))
    {
        for (auto& property_node: property_nodes)
        {
            if (!property_node.hasAttribute("name"))
            {
                throw std::runtime_error("no name attribute for property");
            }

            if (!property_node.hasAttribute("type"))
            {
                throw std::runtime_error("no type attribute for property");
            }

            stream_type.setProperty(property_node.getAttribute("name"), property_node.getAttribute("type"), property_node.getData());
        }
    }
}

}

}
