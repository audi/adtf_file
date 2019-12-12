/**
 * @file
 * Indexed file reader.
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

#ifndef INDEXEDFILE_V110_READER_CLASS_HEADER
#define INDEXEDFILE_V110_READER_CLASS_HEADER

namespace ifhd
{
namespace v110
{

//*************************************************************************************************
/**
 * Class for reading indexed file in version 110
 */
class IndexedFileReaderV110 : public IndexedFileV110
{
    protected:
        uint32_t                   _flags;              //!< @todo
        IndexedFileV110::ChunkRef* _index_table;        //!< @todo
        int                        _index_table_size;   //!< @todo
        int64_t                    _end_of_data_marker; //!< @todo
        bool                       _file_pos_invalid;   //!< @todo

        IndexedFileV110::ChunkHeader* _current_chunk;      //!< @todo
        void*                         _current_chunk_data; //!< @todo
        bool                          _header_valid;       //!< @todo
        bool                          _data_valid;         //!< @todo
        bool                          _prefetched;         //!< @todo
        int64_t                       _chunk_index;        //!< @todo
        int64_t                       _index_table_index;  //!< @todo

    protected:
        bool                                   _compatibility_v100;  //!< @todo
        void*                                  _delegate;            //!< @todo
        mutable IndexedFileV110::FileExtension _extension_info_v100; //!< @todo

    public:
        /**
         * Default constructor.
         */
        IndexedFileReaderV110();

        /**
         * Destructor.
         */
        ~IndexedFileReaderV110();

        /**
         * This function attaches a dat-file for reading
         *
         * @param file      [in] the file where
         * @param fileName  [in] the filename of the file to be attached
         * @param cacheSize [in] cache size; <=0: use system file caching (=default)
         * @param flags     [in] additional flags
         */
        void attach(utils5ext::File* file,
                       const a_util::filesystem::Path& file_name,
                       int cache_size=-1, 
                       uint32_t flags=0);

        /**
         *   Closes all
         *
         */
        void close();

        /**
         *   Resets the file to the beginning of data
         *
         */
        void reset();

        /**
         *   Returns the current file position (Index or TimeStamp)
         *
         *   @param   timeFormat    [in] Format of position (TF_ChunkTime / TF_ChunkIndex);
         *
         *   @returns Current Position
         */
        int64_t getCurrentPos(int time_format) const;

        /**
         *   Sets the file position by Index or by TimeStamp
         *
         *   @param   position      [in] New file position
         *   @param   timeFormat    [in] Format of position (TF_ChunkTime / TF_ChunkIndex);
         *
         *   @returns New Index
         */
        int64_t setCurrentPos(int64_t position, int time_format);
        
        /**
         * Returns the chunk index of an entry in the index table.
         *
         * @param indexPos [in] The index of the entry in the index table.
         *
         * @return The chunk index.
         */
        int64_t getChunkIndexForIndexPos(int64_t index_pos) const;
        /**
         * Get the current index position.
         * @return The current index position.
         */
        int64_t getChunkIndexForIndexPos() const;

        /**
         * Sets the file position to the Chunk of index and returns the respective ChunkInfo
         *
         * @param   position              [in] the new file position
         * @param   timeFormat            [in] the format of position (tTimeFormat)
         * @param   flags              [in] a SeekFlags value
         *
         * @returns int64_t -> the current chunk index
         */
        int64_t seek(int64_t position, int time_format, uint32_t flags=0);

        /**
         *   Returns the current ChunkInfo from IndexTable
         *
         *   @param  chunkHeader [out] Current chunk info struct
         */
        void queryChunkInfo(ChunkHeader** chunk_header);

        /**
         *   Reads and returns the next Chunk and increments the IndexTable index
         *
         *   @param  data [out] Current chunk data
         */
        void readChunk(void** data);

        /**
         *   Skips the next Chunk by incrementing the IndexTable index and setting the new FilePosition
         *
         */
        void skipChunk();

        /**
         *   Reads and returns the next Chunk and ChunkInfo struct and increments the IndexTable index
         *
         *   @param  chunkHeader [out] Current chunk info
         *   @param  data [out] Current chunk data
         */
        void readNextChunk(ChunkHeader** chunk_header, void** data);

        /**
         *  Skips the next chunk info
         */
        void skipChunkInfo();
        /**
         * Reads the next chunk info
         * @param chunkHeader [out] Will point to the chunk info. 
         */
        void readNextChunkInfo(ChunkHeader** chunk_header);

        /**
         *   Returns the current index of the IndexTable
         *
         *   @returns Index
         */
        int64_t getFilePos() const;

        /**
         *   Returns the number of chunks of the current file
         *
         *   @returns Number of chunks
         */
        int64_t getChunkCount() const;

        /**
         * Get the amount of indexes in the file.
         * @return The amount of indexes in the file.
         */
        int64_t getIndexCount() const;

        /**
         *   Returns the duration of the current file [microsec]
         *
         *   @returns File duration
         */
        timestamp_t getDuration() const;

        /**
         *   Returns the Version ID of the current file
         *
         *   @returns Version ID
         */
        uint32_t getVersionId() const;

        /**
         * Finds an extension with a specific identifier.
         * @param identifier [in] The identifier of the extension
         * @param extensionInfo [out] The extension info data.
         * @param data [out] The extension data.
         */
        bool findExtension(const std::string& identifier, FileExtension** extension_info, void** data) const;

        /**
         * Get an extension with a specific index.
         * @param index [in] The index of the extension.
         * @param extensionInfo [out] The extension info data.
         * @param data [out] The extension data.
         */
        void getExtension(int index, FileExtension** extension_info, void** data) const;

    protected:
        /**
         * @todo
         */
        void initialize();

        /**
         * @todo
         */
        void readFileHeader();

        /**
         * @todo
         */
        void readFileHeaderExt();

        /**
         * @todo
         */
        void readIndexTable();

        /**
         * @todo
         * @param   position   [in]    Postion
         * @param   timeFormat [in]    Time format
         */
        int64_t  lookupChunkRef(int64_t position, int time_format);

    protected:
        int    _cache_offset;     //!< @todo
        int    _cache_usage;      //!< @todo

        /**
         * @todo
         */
        void readCurrentChunkHeader();

        /**
         * @todo
         */
        void readCurrentChunkData();

        /**
         * @todo
         */
        void setEOF();

        /**
         * @todo
         * @param buffer      [in]    Pointer to buffer
         * @param bufferSize   [in]    Size of buffer
         */
        void readDataBlock(void* buffer, long buffer_size);

        /**
         * @todo
         */
        void clearCache();

        /**
         * @todo
         */
        void checkFilePtr();

        /**
         * @todo
         */
        void allocReadBuffers();

        /**
         * @todo
         */
        void freeReadBuffers();
};

} //namespace v100
} // namespace ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_V110_READER_CLASS_HEADER_
