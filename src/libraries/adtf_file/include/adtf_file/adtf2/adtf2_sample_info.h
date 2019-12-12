/**
 * @file
 * adtf2 sample info serialization.
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

#ifndef ADTF_FILE_ADTF2_SAMPLE_INFO
#define ADTF_FILE_ADTF2_SAMPLE_INFO

#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/adtf_file_writer.h>

namespace adtf_file
{

namespace adtf2
{

void deserializeMediaSampleInfo(ReadSample& sample, InputStream& stream);
void deserializeMediaSampleLogTrace(ReadSample& sample, InputStream& stream);

enum VariantType
{
    vt_empty   = 0x0000,
    vt_null    = 0x0001,
    vt_bool    = 0x0002,
    vt_int8    = 0x0004,
    vt_u_int8   = 0x0008,
    vt_int16   = 0x0010,
    vt_u_int16  = 0x0020,
    vt_int32   = 0x0040,
    vt_u_int32  = 0x0080,
    vt_float32 = 0x0100,
    vt_float64 = 0x0200,
    vt_string  = 0x0400,
    vt_int64   = 0x0800,
    vt_u_int64  = 0x1000,

    vt_array   = 0x10000000
};

}

}

#endif
