/**
 * @file
 * Indexed file.
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

#ifndef INDEXEDFILE_V110_CLASS_HEADER
#define INDEXEDFILE_V110_CLASS_HEADER


namespace ifhd
{
namespace v110
{

//*************************************************************************************************
/*

    FILE LAYOUT:

    +---------------------------------------------------------------+
    |HDR|                   DATA                |HDR-EXT|    IDX    |
    +---------------------------------------------------------------+

*/

static constexpr uint32_t version_id = 0x00000110;



/**
 *
 * @todo
 *
 */
class IndexedFileV110
{
    public:
        enum
        {
            tf_chunk_index      = 1,
            tf_chunk_time       = 2
        };

#ifdef WIN32
#pragma pack(push)
#pragma pack(1)
#else
#pragma pack(push)
#pragma pack(1)
#endif

        using DateTime = v100::IndexedFileV100::DateTime;
        /// @todo
        typedef struct TagFileHeader
        {
            /// file identifier 
            uint32_t                         file_id;   
            /// format version of dat files. current version is 0x0201            
            uint32_t                         version_id;
            /// file-offset to the begin of extension table block (absolute)
            FilePos                          extension_offset;
            /// amount of extension blocks
            uint32_t                         extension_count;
            /// reserved for further use
            uint32_t                         reserved1;
            /// file-offset to the begin of data block (absolute)
            FilePos                          data_offset;
            /// size of the data block (in bytes)
            utils5ext::FileSize              data_size;  
            /// @todo             
            uint64_t                         index_count;
            /// @todo 
            FilePos                          index_offset;
            /// @todo
            uint64_t                         duration;
            /// @todo 
            DateTime                         date_time;
            /// amount of chunks
            uint64_t                         chunk_count;            
            /// greatest user data size of chunk
            uint64_t                         max_chunk_size;          
            /// reserved bytes. currently not in use
            uint64_t                         reserved2[5];        
            /// description. The short and detailed description are seperated by '\n'
            int8_t                           description[1912]; 
        } FileHeader;   // filled up to 2048 Bytes

        /// @todo
        typedef struct TagFileExtension
        {
            /// identifier
            int8_t                           identifier[32];
            /// related Stream. 0 for every stream 1> id >= Max streams)
            uint32_t                         type_id;
            /// optional version id
            uint32_t                         version_id;
            /// file offset of the extension data (absolute)
            FilePos                          data_pos;
            /// size of the extension-data in bytes
            utils5ext::FileSize              data_size;
            /// reserved bytes. currently not in use
            uint8_t                          reserved[8];
        } FileExtension;

        /**
         *  header for chunks
         * @remarks the header is filled up to 32 bytes. To be 16 byte alligned
         */
        typedef struct TagChunkHeader
        {
            /// timestamp of the chunk @see tTimestamp
            uint64_t                         time_stamp;
            /// refering to the master index table
            uint64_t                         ref_index;
            /// size of the chunks (in bytes)
            uint32_t                         size;
            /// key data / flags
            uint32_t                         flags;
            /// reserved bytes. currently not in use
            uint64_t                         reserved;
        } ChunkHeader;

        /**
         * header for a chunk reference
         */
        typedef struct TagChunkRef
        {
            /// timestamp of the chunk it refers to @see tTimestamp
            uint64_t                         time_stamp;
            /// size of the chunk it refers to 
            uint32_t                         size;
            /// key data / flags of the chunk it refers to
            uint32_t                         flags;
            /// file offset position of the chunk it refers to (in byte)
            uint64_t                         chunk_offset;
            /// number of chunk
            uint64_t                         chunk_index;
        } ChunkRef;

#ifdef WIN32
#pragma pack(pop)
#else
#pragma pack(pop)
#endif

        /// @todo
        typedef struct TagIndexBlockItem
        {
            ChunkRef*                      data;             //!< @todo
            int                            data_size;         //!< @todo
            int                            item_count;        //!< @todo
            struct TagIndexBlockItem*      next;             //!< @todo
        } IndexBlockItem;

        /// @todo
        typedef enum
        {
            sf_default                      = 0x0,         //!< @todo
            sf_keydata                      = 0x1          //!< @todo
        } SeekFlags;

        /// @todo
        typedef enum
        {
            om_none                          = 0x0,          //!< @todo
            om_disable_file_system_cache     = 0x1,          //!< @todo
            om_sync_write                    = 0x2,          //!< @todo
            om_query_info                    = 0x4           //!< @todo
        } OpenMode;

    protected:

        static int                         default_block_size;       //!< @todo
        static int                         default_cache_size;       //!< @todo
        static int                         index_table_cluster_size; //!< @todo

        int                                _sector_size;             //!< @todo
        bool                               _system_cache_disabled;   //!< @todo

        //File                               _file;
        utils5ext::File*                   _file;                    //!< @todo

        FilePos                            _file_pos;                //!< @todo

        uint8_t*                           _buffer;                  //!< @todo
        int                                _buffer_size;             //!< @todo

        IndexBlockItem*                    _index_blocks;            //!< @todo
        IndexBlockItem*                    _active_index_block;      //!< @todo

        FileHeader*                        _file_header;             //!< @todo

        /// @todo
        typedef struct
        {
            FileExtension  file_extension;  //!< 64Byte
            void*          extension_page;  //!< see in pFileExtension->dataSize
        } FileExtensionStruct;

        typedef std::list<FileExtensionStruct*> FileExtensionList;   //!< @todo
        FileExtensionList                   _extensions;             //!< @todo

        void*                               _cache;                  //!< @todo
        int                                 _cache_size;             //!< @todo

        bool                                _write_mode;             //!< @todo

    public:
        /**
         * Default constructor.
         */
        IndexedFileV110();

        /**
         * Destructor.
         */
        ~IndexedFileV110();

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
        void setDateTime(const a_util::datetime::DateTime* date_time);

        /**
         * Get the timestamp of the file.
         * @param dateTime [out] The timestamp of the file.
         */
        void getDateTime(a_util::datetime::DateTime* date_time) const;

        /**
         * Returns the amount of extensions in the file.
         * @return The amount of extensions in the file.
         */
        int getExtensionCount() const;

        /**
         * Finds an extension with a specific identifier.
         * @param identifier [in] The identifier of the extension
         * @param extensionInfo [out] The extension info data.
         * @param data [out] The extension data.
         * @return Standard result.
         */
        bool findExtension(const std::string& identifier, FileExtension** extension_info, void** data) const;

        /**
         * Get an extension with a specific index.
         * @param index [in] The index of the extension.
         * @param extensionInfo [out] The extension info data.
         * @param data [out] The extension data.
         */
        void getExtension(int index, FileExtension** extension_info, void** data) const;

        /**
         * Adds a new extension to the file.
         * @param identifier [in] The identifier of the extension.
         * @param data         [in] The extension data.
         * @param dataSize     [in] The data size.
         * @param typeId    [in] An optional type id
         * @param versionId [in] An optional version id
         */
        void appendExtension(const std::string& identifier,
                                const void* data,
                                int data_size,
                                uint32_t type_id=0,
                                uint32_t version_id=0);

        /**
         * Adds a new extension to the file.
         * @param data [in] The extension data.
         * @param extensionInfo [in] The extension info.
         */
        void appendExtension(const void* data, const FileExtension* extension_info);

        /**
         * Frees all extensions.
         */
        void freeExtensions();

        /**
         * Returns the file header.
         * @param fileHeader [out] Will point to the file header.
         */
        void getHeader(FileHeader** file_header) const;

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
         * Free index table.
         */
        void freeIndexTable();

        /**
         * Allocate header.
         */
        void allocHeader();
        /**
         * Free header.
         */
        void freeHeader();
        /**
         * Allocate extension page.
         * @param size [in] Size of extension page.
         * @param data [in] Pointer to data.
         */
        void allocExtensionPage(utils5ext::FileSize size, void** data);

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
        int getSectorSize(const a_util::filesystem::Path& filename) const;

        /**
         * Retrieve size of file on disk.
         * @param size [in] Size of file.
         * @param useSegmentSize [in] Use segment size?.
         * @return Size of file on disk.
         */
        utils5ext::FileSize getSizeOnDisk(utils5ext::FileSize size, bool use_segment_size) const;
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

} // namespace v110
} // namespace ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_V110_CLASS_HEADER_
