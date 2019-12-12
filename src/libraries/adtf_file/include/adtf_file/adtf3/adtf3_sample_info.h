/**
 * @file
 * adtf3 sample info serialization.
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

#ifndef ADTF_FILE_ADTF3_SAMPLE_INFO
#define ADTF_FILE_ADTF3_SAMPLE_INFO

#include <adtf_file/adtf_file_writer.h>
#include <adtf_file/adtf_file_reader.h>

namespace adtf_file
{
namespace adtf3
{

enum class HashedValueType: uint8_t
{
    hvt_invalid = 0,
    hvt_bool    = 2,
    hvt_int8    = 3,
    hvt_uint8   = 4,
    hvt_int16   = 5,
    hvt_uint16  = 6,
    hvt_int32   = 7,
    hvt_uint32  = 8,
    hvt_float32 = 9,
    hvt_float64 = 10,
    hvt_int64   = 12,
    hvt_uint64  = 13
};

#pragma pack(push)
#pragma pack(1)
struct HashValueStorage   //16 Byte per key value pair
{
    static uint8_t getVersion()
    {
        return 1;
    }

    HashValueStorage():
        storage_version(getVersion()),
        byte_size(0),
        type(HashedValueType::hvt_invalid)
    {
    }

    uint8_t storage_version; //we using 8 bit for versioning
    uint8_t byte_size; // we using 8 bit to restrict the value size
    HashedValueType type;
    uint8_t reserved[1];
    uint32_t key;      // for accessing its important to have aligned value
    uint8_t storage[8]; // for accessing its important to have aligned value
};
#pragma pack(pop)

bool hasSampleInfo(const WriteSample& sample);
void serializeSampleInfo(const WriteSample& sample, OutputStream& stream);

void deserializeSampleInfo(ReadSample& sample, InputStream& stream);



}
}

#endif
