/**
 * @file
 * Indexed file type descriptions.
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

#pragma once

 /// Maximum of indexed streams
#define MAX_INDEXED_STREAMS                      512

 /// maximum length of a file extension
#define MAX_FILEEXTENSIONIDENTIFIER_LENGTH       384

 /// maximum length of the stream name
#define MAX_STREAMNAME_LENGTH                    228

 /// Prefix for stream index names
#define IDX_EXT_INDEX "index"
 /// Name of master index
#define IDX_EXT_INDEX_0 IDX_EXT_INDEX "0"
 /// Prefix for stream index addional extension names
#define IDX_EXT_INDEX_ADDITONAL "index_add"
 /// Name of master index additional extension
#define IDX_EXT_INDEX_ADDITONAL_0 IDX_EXT_INDEX_ADDITONAL "0"


namespace ifhd
{

using FilePos = utils5ext::FilePos;

enum class Endianess : uint8_t
{
    platform_not_supported = 0x00,
    platform_little_endian = 0x01,
    platform_big_endian    = 0x02
};

#define PLATFORM_BYTEORDER_UINT8 static_cast<uint8_t>(ifhd::getCurrentPlatformByteorder())

/**
* This function retrieves the platform dependent byte order.
* @return @ref PLATFORM_NOT_SUPPORTED, @ref PLATFORM_LITTLE_ENDIAN_8 or @ref PLATFORM_BIG_ENDIAN_8.
*/
static inline Endianess getCurrentPlatformByteorder()
{
    uint32_t value = 0x01020304;
    if (((unsigned char*)&value)[0] == 0x04 &&
        ((unsigned char*)&value)[2] == 0x02)
    {
        return Endianess::platform_little_endian;
    }
    else if (((unsigned char*)&value)[0] == 0x01 &&
        ((unsigned char*)&value)[2] == 0x03)
    {
        return Endianess::platform_big_endian;
    }
    else
    {
        return Endianess::platform_not_supported;
    }
}

namespace exceptions
{

class EndOfFile: public std::runtime_error
{
    public:
        EndOfFile(): runtime_error("end of file")
        {
        }

        EndOfFile(const std::string& what_arg): runtime_error(what_arg)
        {
        }
};

}

/**
*  \def A_UTILS_ASSERT
*  This macro is used for platform independent assertion expressions.
*/
#ifndef IFHD_ASSERT
#ifdef WIN32
#ifdef _ASSERTE
#define IFHD_ASSERT          _ASSERTE
#else
#define IFHD_ASSERT          assert
#endif
#else
#define IFHD_ASSERT          assert
#endif
#endif

} // namespace
