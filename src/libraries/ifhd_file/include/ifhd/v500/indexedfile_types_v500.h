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

#ifndef INDEXEDFILE_V500_TYPE_HEADER
#define INDEXEDFILE_V500_TYPE_HEADER

#include <ifhd/v400/indexedfile_types_v400.h>
#include <cstdint>

namespace ifhd
{
namespace v500
{

static constexpr uint32_t version_id = 0x00000500;

static inline uint32_t getFileId()
{
    return v400::getFileId();
}

/**
 * Time format (for seek etc.)
 */
using TimeFormat = v400::TimeFormat;

/**
 * Read options.
 */
using ReadFlags = v400::ReadFlags;

using FieldMask = v400::FieldMask;

using FileHeader = v400::FileHeader;

/**
 * \struct FileExtension
 * @brief Header for a file extensions.
 * 
 */
using FileExtension = v400::FileExtension;

/**
 * \struct ChunkHeader
 * @brief header for chunks
 * Each Chunk header 16 Byte aligned within the file.
 */
using ChunkHeader = v400::ChunkHeader;

/**
 * \struct chunkRef
 * @brief header for a chunk reference
 */
using ChunkRef = v400::ChunkRef;

/**
 * \struct StreamRef
 * header for a stream reference elements
 */
using StreamRef = v400::StreamRef;

/** 
 * \struct StreamInfoHeader
 *  Stream info header
 */
using StreamInfoHeader = v400::StreamInfoHeader;

/// Additional index table information
using AdditionalIndexInfo = v400::AdditionalIndexInfo;

/**
 * Seek flags.
 */
using SeekFlags = v400::SeekFlags;

/**
 * The chunk types.
 */
using ChunkType = v400::ChunkType;

/**
 * File open modes.
 */
using OpenMode = v400::OpenMode;

}  // namespace v500
} // namespace  ifhd

#endif // _INDEXEDFILE_V500_TYPE_HEADER_
