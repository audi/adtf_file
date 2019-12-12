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

#ifndef INDEXEDFILE_V400_TYPE_HEADER
#define INDEXEDFILE_V400_TYPE_HEADER

namespace ifhd
{
namespace v400
{

static constexpr uint32_t version_id = 0x00000400;

static inline uint32_t getFileId()
{
    return v201_v301::getFileId();
}

/**
 * Time format (for seek etc.)
 */
using TimeFormat = v201_v301::TimeFormat;

/**
 * Read options.
 */
using ReadFlags = v201_v301::ReadFlags;

using FieldMask = v201_v301::FieldMask;

using FileHeader = v201_v301::FileHeader;

/**
 * \struct FileExtension
 * @brief Header for a file extensions.
 * 
 */
using FileExtension = v201_v301::FileExtension;

/**
 * \struct ChunkHeader
 * @brief header for chunks
 * Each Chunk header 16 Byte aligned within the file.
 */
using ChunkHeader = v201_v301::ChunkHeader;

/**
 * \struct chunkRef
 * @brief header for a chunk reference
 */
using ChunkRef = v201_v301::ChunkRef;

/**
 * \struct StreamRef
 * header for a stream reference elements
 */
using StreamRef = v201_v301::StreamRef;

/** 
 * \struct StreamInfoHeader
 *  Stream info header
 */
using StreamInfoHeader = v201_v301::StreamInfoHeader;

/// Additional index table information
using AdditionalIndexInfo = v201_v301::AdditionalIndexInfo;

/**
 * Seek flags.
 */
using SeekFlags = v201_v301::SeekFlags;

/**
 * The chunk types.
 */
using ChunkType = v201_v301::ChunkType;

/**
 * File open modes.
 */
using OpenMode = v201_v301::OpenMode;

}  // namespace v400
} // namespace  ifhd

#endif // _INDEXEDFILE_V400_TYPE_HEADER_
