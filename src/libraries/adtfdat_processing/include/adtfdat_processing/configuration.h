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

#include <string>
#include <utility>
#include <unordered_map>

namespace adtf
{
namespace dat
{
namespace ant
{

struct PropertyValue
{
    std::string value;
    std::string type;
};

inline bool operator==(const PropertyValue& first, const PropertyValue& second)
{
    return first.type == second.type &&
           first.value == second.value;
}

typedef std::unordered_map<std::string, PropertyValue> Configuration;

/**
 * Base class that implements configurable properties.
 */
class Configurable
{
public:
    /**
     * @return The configuration (a set of properties) of the object.
     */
    virtual Configuration getConfiguration() const
    {
        return _current_configuration;
    }

    /**
     * Sets the configuration to be used by the object during processing.
     * @param [in] configuration The configuration.
     */
    virtual void setConfiguration(const Configuration& new_configuration)
    {
        for (const auto& property : new_configuration)
        {
            _current_configuration[property.first] = property.second;
        }
    }

private:
    Configuration _current_configuration;
};

namespace detail
{

template <typename T>
T propertyValueAs(const std::string& property_value)
{
    static_assert(sizeof(T) == -1,
                  "please use the specializations for int64_t, uint64_t, double and std::string.");
}

template <>
inline uint64_t propertyValueAs<uint64_t>(const std::string& property_value)
{
    return std::stoull(property_value, 0, 0);
}

template <>
inline int64_t propertyValueAs<int64_t>(const std::string& property_value)
{
    return std::stoll(property_value, 0, 0);
}

template <>
inline double propertyValueAs<double>(const std::string& property_value)
{
    return std::stod(property_value);
}

template <>
inline std::string propertyValueAs<std::string>(const std::string& property_value)
{
    return property_value;
}
}

/**
 * Use this to access property values and convert them to numeric types.
 * @param [in] configuration The configuration.
 * @param [in] property_name The name of the property.
 * @return The property value.
 */
template <typename T>
T getPropertyValue(const Configuration& configuration, const std::string& property_name)
{
    try
    {
        return detail::propertyValueAs<T>(configuration.at(property_name).value);
    }
    catch (const std::exception& error)
    {
        throw std::runtime_error("cannot read the property value of '" + property_name +
                                 "':" + error.what());
    }
}
}

using ant::PropertyValue;
using ant::Configuration;
using ant::Configurable;
using ant::getPropertyValue;
}
}
