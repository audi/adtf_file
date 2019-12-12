/**
 * @file
 * Indexed file compatibility code.
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

#ifndef INDEXEDFILE_V100_CLASS_HEADER
#define INDEXEDFILE_V100_CLASS_HEADER

namespace ifhd
{
namespace v100
{

    /// Version ID
const uint32_t version_id = 0x00000100;

//*************************************************************************************************
/*

    FILE LAYOUT:

    +---------------------------------------------------------------+
    |HDR|                   DATA                |HDR-EXT|    IDX    |
    +---------------------------------------------------------------+

*/

//*************************************************************************************************
/**
 * Compatibility class for reading indexed files in format version 100.
 */
class IndexedFileV100
{
    public:
        enum
        {
            tf_chunk_index   = 1,
            tf_chunk_time    = 2
        };

#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
#else
#pragma pack(push)
#pragma pack(1)
#endif
        typedef struct TagDateTime
        {
            /// The Year - [1900,)
            uint16_t year;
            /// The Month - [1,12]
            uint16_t month;
            /// The Day of the month - [1,31]
            uint16_t day;
            /// The Hours past midnight - [0,23]
            uint16_t hour;
            /// The Minutes after the hour - [0,59]
            uint16_t minute;
            /// The Seconds after the minute - [0,59]
            uint16_t second;
            /// The Microseconds after the second - [0,999999]
            uint32_t microseconds;
        } DateTime;

        typedef struct TagFileHeader
        {
            uint32_t                         file_id;                //!< File identifier
            uint32_t                         version_id;             //!< Version identifier
            FilePos                          extension_offset;       //!< Offset of extensions
            utils5ext::FileSize              extension_size;         //!< Size of extensions
            FilePos                          data_offset;            //!< Offset of data
            utils5ext::FileSize              data_size;              //!< Size of data
            uint64_t                         index_count;            //!< Number of indices
            FilePos                          index_offset;           //!< Offset of indices
            uint64_t                         duration;               //!< Duration
            DateTime                         date_time;              //!< Date and time
            uint64_t                         reserved[7];            //!< Reserved
            int8_t                           description[1912];      //!< File description
        } FileHeader;

        typedef struct TagChunkHeader
        {
            uint64_t                         chunk_offset; //!< Offset of chunk
            uint64_t                         time_stamp;   //!< Timestamp of chunk
            uint32_t                         size;         //!< Size of chunk
            uint32_t                         flags;        //!< Flags for chunk
        } ChunkHeader;

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(pop)
#endif

    protected:
        typedef struct TagIndexBlockItem
        {
            ChunkHeader*                   data;           //!< Data
            int                            data_size;      //!< Size of data
            int                            item_count;     //!< Number of items
            struct TagIndexBlockItem*      next;           //!< Pointer to next IndexBlockItem
        } IndexBlockItem;

        bool                       _attached;                 //!< File attached?

        static int                 _default_block_size;        //!< Default block size
        static int                 _default_cache_size;        //!< Default cache size
        static int                 _index_table_cluster_size;  //!< Size of index table clusters

        int                        _sector_size;              //!< Sector size
        bool                       _system_cache_disabled;    //!< System cache disabled?

        utils5ext::File*           _file;                     //!< File pointer

        int64_t                    _index;                    //!< Current index
        int64_t                    _file_pos;                 //!< Current file position

        uint8_t*                   _buffer;                   //!< Internal buffer
        int                        _buffer_size;              //!< Buffer size

        IndexBlockItem*            _index_blocks;             //!< Pointer to index blocks
        IndexBlockItem*            _active_index_block;       //!< Pointer to current index block

        FileHeader*                _file_header;              //!< File header

        void*                      _header_extension;         //!< Header extension

        void*                      _cache;                    //!< Pointer to cache
        int                        _cache_size;               //!< Cache size

        bool                       _write_mode;               //!< Write mode enabled?

    public:
        /**
         * Default constructor.
         */
        IndexedFileV100();
        /**
         * Destructor.
         */
        ~IndexedFileV100();

        /**
         * Closes the file.
         */
        void close();

        /**
         * Sets the description of the file
         * @param description [in] The new description.
         */
        void setDescription(const std::string& description);

        /**
         * Get the description of the file.
         * @return The description of the file.
         */
        std::string getDescription() const;

        /**
         * Set the timestamp of the file.
         * @param dateTime [in] The new timestamp.
         */
        void setDateTime(const DateTime& date_time);

        /**
         * Get the timestamp of the file.
         * @param dateTime [out] The timestamp of the file.
         * @return Standard result code.
         */
        DateTime getDateTime();

        /**
         * Allocate memory for the header extension.
         * @param size [in] The amount of memory to allocate.
         */
        void  allocHeaderExtension(int size);
        /**
         * Frees memory allocated for the header extension.
         */
        void freeHeaderExtension();
        /**
         * Returns the header extension.
         * @param data [out] This will point to the header extension.
         * @param dataSize [out] The size of the header extension.
         */
        void getHeaderExtension(void** data, int* data_size);
        /**
         * Set the header extension.
         * @param data [in] The new extension data.
         * @param dataSize [in] The size of the extension data.
         */
        void setHeaderExtension(void* data, int data_size);

    protected:
        /**
         * Internal initialization method.
         */
        void initialize();

        /**
         * Allocate internal buffer.
         * @param size [in] Size of buffer.
         */
        void allocBuffer(int size);
        /**
         * Free internal buffer.
         */
        void freeBuffer();

        /**
         * Allocate index table.
         * @param size [in] Size of table.
         */
        void allocIndexTable(int size);
        /**
         * Free index table.
         */
        void freeIndexTable();
        /**
         * Append index.
         * @param pos [in] Position.
         * @param timeStamp [in] Timestamp.
         */
        void appendIndex(uint64_t pos, timestamp_t time_stamp);
        /**
         * Allocate index block.
         * @param count [in] Number of entries.
         */
        void allocIndexBlock(int count=-1);

        /**
         * Allocate header.
         */
        void allocHeader();
        /**
         * Free header.
         */
        void freeHeader();

    protected:
        /**
         * Allocate cache.
         * @param size [in] Size of cache.
         */
        void allocCache(int size);
        /**
         * Free cache.
         */
        void freeCache();
        /**
         * Retrieve cache address.
         * @return Pointer to cache.
         */
        void*  getCacheAddr();

        /**
         * Get sector size of specified file.
         * @param filename [in] File to retrieve sector size of.
         * @return Sector size of file.
         */
        int getSectorSize(const std::string &filename) const;

        /**
         * Internal allocation method.
         * @param size [in] Size of Allocation.
         * @param useSegmentSize [in] Use segment size?.
         * @return Allocated memory.
         */
        void* internalMalloc(int size, bool use_segment_size);
        /**
         * Internal deallocation method.
         * @param memory [in] Pointer to memory.
         * @return void
         */
        void internalFree(void* memory);
};

} //namespace v100
} // namespace ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_V100_CLASS_HEADER_
