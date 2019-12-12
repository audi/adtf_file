/**
 * @file
 * sample impl.
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

#include <adtf_file/sample.h>
#include <city.h>

namespace adtf_file
{

uint32_t createSampleInfoHashKey(const std::string& name)
{
    return CityHash32(name.c_str(), name.length());
}

enum ADTF2AdditonalDataInfoIndex
{
    /// The Hardware Time ./ Device Time.
    /// While using this a microseconds-value-resolution should be used.
    msai_device_original_time       = 0,

    /// A Counter.
    msai_counter                  = 1,

    /**
     * This is a Info Offset
     */
    msai_info_user_offset           = 200
};

uint32_t createAdtf2SampleInfoHashKey(uint32_t info_index)
{
    switch (info_index)
    {
        case ADTF2AdditonalDataInfoIndex::msai_device_original_time:
        {
            return SampleInfoKeys::sai_device_original_time;
        }
        case ADTF2AdditonalDataInfoIndex::msai_counter:
        {
            return SampleInfoKeys::sai_counter;
        }
        default:
        {
            std::string string_to_hash = "adtf2_index_" + std::to_string(info_index);
            return createSampleInfoHashKey(string_to_hash);
        }
    }
}

}
