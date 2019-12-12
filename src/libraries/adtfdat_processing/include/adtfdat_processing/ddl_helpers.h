/**
*
* ADTF Dat Exporter.
*
* @file
* Copyright &copy; 2003-2016 Audi Electronics Venture GmbH. All rights reserved
*
* $Author: WROTHFL $
* $Date: 2017-06-06 15:51:27 +0200 (Di, 06 Jun 2017) $
* $Revision: 61312 $
*
* @remarks
*
*/

#pragma once

#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/stream_type.h>
#include "configuration.h"
#include <ddl.h>

#define A_UTIL_THROW_IF_FAILED(__exp, __msg)\
{\
    auto __result = (__exp);\
    if (a_util::result::isFailed(__result))\
    {\
        throw std::runtime_error(std::string(__msg) + ": " +\
                                 a_util::result::toString(__result));\
    }\
}

namespace adtf
{
namespace dat
{
namespace ant
{

/**
 * creates a ddl codec factory from the given stream type
 * @param [in] stream_type The stream type.
 * @return a codec factory and whether data is serialized or not.
 */
inline std::tuple<ddl::CodecFactory, bool> createCodecFactoryFromStreamType(const std::shared_ptr<const adtf_file::StreamType>& stream_type)
{
    auto property_type = std::dynamic_pointer_cast<const adtf_file::PropertyStreamType>(stream_type);
    if (!property_type)
    {
        throw std::runtime_error("The stream type is not a property stream type");
    }

    auto media_description = property_type->getProperty("md_definitions");
    auto struct_name = property_type->getProperty("md_struct");
    bool data_is_serialized = false;
    try
    {
        data_is_serialized = property_type->getProperty("md_data_serialized").second == "true";
    }
    catch (...)
    {
    }

    ddl::CodecFactory factory(struct_name.second.c_str(), media_description.second.c_str());
    if (a_util::result::isFailed(factory.isValid()))
    {
        throw std::runtime_error("unable to parse media description: " + a_util::result::toString(factory.isValid()));
    }

    return std::tuple<ddl::CodecFactory, bool>(factory, data_is_serialized);
}

/**
 * Creates an adft/default stream type.
 * @param [in] struct_name The name of the ddl struct.
 * @param [in] struct_definition The definition of the ddl struct.
 * @param [in] is_serialized If data is serialized or not.
 * @return A stream type.
 */
inline std::shared_ptr<adtf_file::StreamType> createAdtfDefaultStreamType(const std::string& struct_name,
                                                                              const std::string& struct_definition,
                                                                              bool is_serialized = false)
{
    auto type = std::make_shared<adtf_file::DefaultStreamType>("adtf/default");
    type->setProperty("md_definitions", "cString", struct_definition);
    type->setProperty("md_struct", "cString", struct_name);
    type->setProperty("md_data_serialized", "tBool", is_serialized ? "true" : "false");
    return type;
}

}

using ant::createCodecFactoryFromStreamType;
using ant::createAdtfDefaultStreamType;

}
}
