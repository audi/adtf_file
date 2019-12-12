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

#ifndef INDEXEDFILE_V201_V301_TYPE_HEADER
#define INDEXEDFILE_V201_V301_TYPE_HEADER

namespace ifhd
{
namespace v201_v301
{

static inline uint32_t getFileId()
{
    static uint32_t file_id = *((uint32_t*) "IFHD");
    return file_id;
}

static constexpr uint32_t version_id_beta = 0x0200;
/// supported Version of Indexed File within ADTF 2.0 until 2.12 and >= 2.13 
/// if NO History is used while writing file (currently 0x00000201)
static constexpr uint32_t version_id = 0x00000201;
/// supported Version of Indexed File within >= ADTF 2.13 if a file buffered history 
/// is used while writing file (currently 0x00000300)
static constexpr uint32_t version_id_with_history = 0x00000300;
/// supported Version of Indexed File within >= ADTF 2.13.1 if a file buffered history
/// is used while writing file (currently 0x00000301)
static const uint32_t version_id_with_history_end_offset = 0x00000301;


/**
 * Time format (for seek etc.)
 */
enum TimeFormat
{
    /// The chunk at this index counter within the whole file. 
    /// The stream Id is ignored in this case.
    tf_chunk_index   = 1,
    /// The chunk with this timestamp within the stream which is 
    /// specified by the stream Id.
    tf_chunk_time    = 2,
    /// The chunk at this index counter within the stream which is 
    /// specified by the stream Id.
    tf_stream_index  = 3
};

/**
* Mask which fields of the file header should be updated, see
* \link update_header \c update_header
* \endlink.
*/
enum FieldMask
{
    fm_none = 0x00,
    fm_description = 0x01,
    fm_date_time = 0x02
};

/**
 * Read options.
 */
enum ReadFlags
{
    rf_none              = 0x0,
    rf_use_external_buffer = 0x1,
    rf_backwards         = 0x2
};

#pragma pack(push)
#pragma pack(1)

/**
    * \struct FileHeader
    * @brief The File header for dat-files
    *
    * @warning !!! IMPORTANT !!!
    *   When changing header please change the
    *   Stream2FileHeader in IndexedFile!
    */
struct FileHeader
{
    /// identifier for every dat-File. intel cpu implementations uses 
    /// "IFHD" motorola uses "DHFI" see also \c headerByteOrder
    uint32_t                         file_id;
    /// format version of dat files, current version is 
    /// IndexedFile::m_nVersionId
    /// or IndexedFile::m_nVersionIdWithHistory
    uint32_t                         version_id;
    /// flags ... not in use yet
    uint32_t                         flags;
    /// amount of extension blocks
    uint32_t                         extension_count;
    /// file-offset to the begin of extension table block (absolute)
    uint64_t                         extension_offset;
    /// file-offset to the begin of data block (absolute)
    uint64_t                         data_offset;
    /// size of the data area block (in bytes)
    uint64_t                         data_size;
    /// amount of chunks
    uint64_t                         chunk_count;
    /// greatest user data size of chunk
    uint64_t                         max_chunk_size;
    /// timestamp of the last chunk
    uint64_t                         duration;
    /// creation time of file
    uint64_t                         file_time;
    /// endianess of management structures (little or big endian), see 
    /// also @ref PLATFORM_BYTEORDER_UINT8.
    /// every single value within the structures
    /// (FileHeader, FileExtension, ChunkHeader ... ) are stored in 
    /// this corresponding byteorder!
    uint8_t                          header_byte_order;
    /// time offset for every time within the file is referred to 
    /// (timestamp zero)
    uint64_t                         time_offset;
    /// patch number (not in use yet)
    uint8_t                          patch_number;
    /// the file offset of the first chunk 
    /// (only needed for IndexedFile::m_nVersionIdWithHistory)
    uint64_t                         first_chunk_offset;
    /// the offset of the first chunk of the continuous section of the file
    /// (only needed for IndexedFile::m_nVersionIdWithHistory)
    uint64_t                         continuous_offset;
    /// the end position withing the ring buffer section
    /// (only needed for IndexedFile::m_nVersionIdWithHistory)
    uint64_t                         ring_buffer_end_offset;
    /// reserved bytes. currently not in use
    int8_t                           reserved[30];
    /// common string description.
    /// This value is separated into a short and detailed description 
    /// separated by '\n'.
    int8_t                           description[1912];
};   // size is 2048 Bytes

/**
 * \struct FileExtension
 * @brief Header for a file extensions.
 * 
 */
struct FileExtension
{
    /// Identifier
    int8_t                           identifier[MAX_FILEEXTENSIONIDENTIFIER_LENGTH];
    /// related Stream identifier. 0 for every stream 1> id >= Max streams)
    uint16_t                         stream_id;
    /// reserved. currently not in use
    uint8_t                          reserved1[2];
    /// optional user id
    uint32_t                         user_id;
    /// optional type id
    uint32_t                         type_id;
    /// optional version id
    uint32_t                         version_id;
    /// file offset of the extension data (absolute),
    /// will be changed to int64_t in a future version
    uint64_t                         data_pos;
    /// size of the extension-data in bytes
    uint64_t                         data_size;
    /// reserved. currently not in use
    uint8_t                          reserved[96];
};  // size is 512 Bytes

/**
 * \struct ChunkHeader
 * @brief header for chunks
 * Each Chunk header 16 Byte aligned within the file.
 */
struct ChunkHeader
{
    /// timestamp of the chunk @see tTimestamp
    uint64_t                         time_stamp;
    /// referring to the master index table
    uint32_t                         ref_master_table_index;
    /// relative byte offset to the previous chunk header (in bytes)
    /// @remark this is NOT necessarily the size of the previous chunk. 
    /// Chunk headers are 16 Byte aligned! 
    uint32_t                         offset_to_last;
    /// size of the chunks (in bytes)
    /// @remark This value includes the size of the chunk data AND the 
    /// chunk header size! 
    uint32_t                         size;
    /// stream identifier the chunk belongs to
    uint16_t                         stream_id;
    /// key data / flags 
    /// see also IndexedFile::tChunkType
    uint16_t                         flags;
    /// number of the chunk within stream it belongs to
    uint64_t                         stream_index;
};  // size is 32 Bytes

/**
 * \struct chunkRef
 * @brief header for a chunk reference
 */
struct ChunkRef
{
    /// timestamp of the chunk it refers to @see tTimestamp
    uint64_t                         time_stamp;
    /// size of the chunk it refers to 
    /// @remark This value includes the size of the Chunk Data size 
    /// \b AND the Chunk Header size! 
    uint32_t                         size;
    /// stream identifier of the chunk it refers to
    uint16_t                         stream_id;
    /// key data / flags of the chunk it refers to
    /// see also IIndexFile::tChunkType
    uint16_t                         flags;
    /// file offset position of the chunk it refers to (in byte)
    uint64_t                         chunk_offset;
    /// number of chunk
    uint64_t                         chunk_index;
    /// number of chunk within the stream it belongs to
    uint64_t                         stream_index;
    /**
        * number of stream index table entry this master index entry belongs to
        */
    uint32_t                         ref_stream_table_index;
};    // size is 44 Bytes

/**
 * \struct StreamRef
 * header for a stream reference elements
 */
struct StreamRef
{
    /**
        * number of master index entry it belongs to
        */
    uint32_t ref_master_table_index;
};    // size is 4 Bytes

/** 
 * \struct StreamInfoHeader
 *  Stream info header
 */
struct StreamInfoHeader
{
    /// Amount of stream indexes
    uint64_t                         stream_index_count;
    /// First timestamp of stream
    uint64_t                         stream_first_time;
    /// Last timestamp of stream
    uint64_t                         stream_last_time;
    /// Info data size
    uint32_t                         info_data_size;
    /// Stream name
    int8_t                           stream_name[MAX_STREAMNAME_LENGTH]; //use Ascii 7
}; // size is 256 Byte

/// Additional index table information
struct AdditionalIndexInfo
{
    /// stream index count offset (the offset is > 0 if data was dropped 
    /// within the ring buffer while recording)
    uint64_t stream_index_offset;
    /// Offset of the Index table entry position within master table 
    /// (dropped indextable entries while recording with history)
    uint32_t stream_table_index_offset;
    /// for later use 
    uint8_t  reserved[20];
};

#pragma pack(pop)




/**
 * Seek flags.
 */
enum SeekFlags
{
    ///The default seek flag
    sf_default = 0x0,
    ///Just seek in index table
    sf_keydata = 0x1,
    ///Seek to the last chunk before the seek time
    sf_before = 0x02
};

/**
 * The chunk types.
 */
enum ChunkType
{
    /// marks the chunk as data
    ct_data    = 0x00,
    /// marks the chunk as KEY data (results to an index table entry) 
    ct_keydata = 0x01,
    /// marks the chunk as info data 
    ct_info    = 0x02,
    /// marks the chunk as marker data
    ct_marker  = 0x04,
    /// marks the chunk as type data
    ct_type = 0x08,
    /// marks the chunk as trigger data
    ct_trigger = 0x10
};

/**
 * File open modes.
 */
enum OpenMode
{
    /** 
        * No Flag set.
        */
    om_none                   = 0x00,
    /** 
        * This will disable the file system cache provided by operating 
        * system for any file operations. The disabling of file system 
        * cache results in a direct writing operation to the HD controller 
        * to ensure sector aligned writing etc. Only valid for writing 
        * operations.
        */
    om_disable_file_system_cache = 0x01,
    /** 
        * This will disable the asynchronous internal cache. Any Write 
        * operation results in a direct file operation call. Only valid for 
        * writing operations.
        */
    om_sync_write              = 0x02,
    /** 
        * Only valid for reading file operations.
        * Will only open the extension table.
        */
    om_query_info              = 0x04,
    /** 
        * Only valid for writing file.
        * An additional check of chunk validity is made! 
        * This Flag is available as performance reasons.
        */
    om_validate_chunk_header    = 0x08,
    /** 
        * Only valid for modify file operations.
        */
    om_file_change_mode         = 0x10
};

}  // namespace v201_301
} // namespace  ifhd

#endif // _INDEXEDFILE_V201_v301_TYPE_HEADER_
