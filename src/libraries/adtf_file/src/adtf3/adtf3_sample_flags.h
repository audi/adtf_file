/**
 * @file
 * adtf3 sample flags.
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

#include <adtf_file/adtf3/adtf3_sample_copy_deserializer.h>
#include <adtf_file/adtf3/adtf3_sample_info.h>

namespace adtf_file
{

namespace adtf3
{

enum InternalSampleFlags: uint32_t
{
    sf_sample_info_present = 0x100,
    sf_substream_id_present = 0x200
};

}
}
