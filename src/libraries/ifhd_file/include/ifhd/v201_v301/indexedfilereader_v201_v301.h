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

#ifndef INDEXEDFILE_READER_V201_V301_CLASS_HEADER
#define INDEXEDFILE_READER_V201_V301_CLASS_HEADER

namespace ifhd
{
namespace v201_v301
{

//*************************************************************************************************
/**
 * Class for reading indexed files.
 */
class DOEXPORT IndexedFileReader : public IndexedFile
{
    private:
        class IndexedFileReaderImpl;
        a_util::memory::unique_ptr<IndexedFileReaderImpl> _d;

    protected:
        /*! \cond PRIVATE */
        // For internal use only (will be moved to a private implementation).
        uint32_t     _flags;
        int64_t      _end_of_data_marker;
        bool         _file_pos_invalid;
        ChunkHeader* _current_chunk;
        void*        _current_chunk_data;
        bool         _header_valid;
        bool         _data_valid;
        bool         _prefetched;
        int64_t      _chunk_index;
        int64_t      _index_table_index;
        FilePos      _file_pos_current_chunk;
        
        IndexReadTable     _index_table;

    protected:
        bool                                         _compatibility_v110;
        void*                                        _delegate;
        mutable std::map<std::string, FileExtension> _extension_info_v110_by_name;
        a_util::filesystem::Path                     _filename;
        //! @endcond

    public:
        /**
         * Default constructor.
         */
        IndexedFileReader();

        /**
         * Destructor.
         */
        ~IndexedFileReader();

        /**
         *
         * This function opens a dat-file for reading.
         *
         * @param filename [in] the file name to be opened
         * @param cacheSize  [in] cache size; <=0: use system file caching (=default)
         * @param flags   [in] a OpenMode value
         *
         * @returns void
         *
         */
        virtual void open(const a_util::filesystem::Path& filename, int cache_size=-1, uint32_t flags=0);

        /**
         *
         * This function closes all.
         *
         * @returns void
         *
         */
        virtual void close();

        /**
         *
         * This function resets the file to the beginning of data.
         *
         * @returns void
         *
         */
        void reset();

        /**
         *
         * This function returns the current file position (Index or TimeStamp)
         *
         * @param timeFormat [in] a format value (tTimeFormat)
         *
         * @returns int64_t -> the current file position (Index or TimeStamp)
         * @rtsafe
         *
         */
        int64_t getCurrentPos(TimeFormat time_format) const;

        /**
         *
         * This function sets the file position.
         *
         * @param position   [in] the new file position
         * @param timeFormat [in] the format of position (tTimeFormat)
         *
         * @returns int64_t -> the current chunk index
         *
         */
        int64_t setCurrentPos(int64_t position, TimeFormat time_format);
               

        /**
         *
         * This function sets the new file position and returns the new chunk index or -1 in case the chunk can not be found (for example stream Id is invalid or file position is out of range).
         *
         * @param streamId [in] the stream Id
         * @param position    [in] the new file position
         * @param timeFormat  [in] the format of position (tTimeFormat)
         * @param flags    [in] a SeekFlags value
         *
         * @returns int64_t -> the current chunk index or -1
         *
         */
        int64_t seek(uint16_t stream_id,
                    int64_t position,
                    TimeFormat time_format,
                    uint32_t flags=0);

        /**
         *
         * This function returns the ChunkInfo of the current chunk.
         *
         * @param  chunkHeader [out] the current chunk info
         *
         * @returns void
         *
         */
        void queryChunkInfo(ChunkHeader** chunk_header);

        /**
         *
         * This function reads and returns the current Chunk and increments
         * the current chunk index
         *
         * @param data    [out] the current chunk data
         * @param flags [in]  a tReadFlags the value
         *
         * @returns void
         *
         */
        void readChunk(void** data, uint32_t flags=0);

        /**
         *
         * This function skips the next Chunk by incrementing the current chunk index
         * and setting the new FilePosition.
         *
         * @returns void
         *
         */
        void skipChunk();

        /**
         *
         * This function reads and returns the current Chunk and ChunkInfo struct
         * and increments the current chunk index. If a stream Id is specified, the
         * next Chunk and ChunkInfo struct of this stream will be returned.
         *
         * @param chunkHeader [out] the current chunk info
         * @param data        [out] the current chunk data
         * @param flags     [in]  a tReadFlagsthe value
         * @param streamId  [in]  the stream Id
         *
         * @returns void
         *
         */
        void readNextChunk(ChunkHeader** chunk_header, void** data, uint32_t flags=0, uint32_t stream_id=0);

        /**
         *
         * This function increments the current chunk index.
         *
         * @return void
         *
         */
        void skipChunkInfo();

        /**
         *
         * This function gets the next chunk information.
         *
         * @param  chunkHeader [out] the ChunkHeader data
         *
         * @return void
         *
         */
        void readNextChunkInfo(ChunkHeader** chunk_header);

        /**
         *
         * This function returns the current chunk index.
         *
         *  @returns int64_t
         *
         */
        int64_t getFilePos() const;

        /**
         *
         * This function returns the number of chunks of the current file.
         *
         * @returns int64_t
         * @rtsafe
         *
         */
        int64_t getChunkCount() const;

        /**
         *
         * This function returns the duration of the current file [microsec].
         *
         * @returns timestamp_t
         * @rtsafe
         *
         */
        timestamp_t getDuration() const;

        /**
         *
         * This function returns the Version Id of the current file.
         *
         * @returns uint32_t
         * @rtsafe
         *
         */
        uint32_t getVersionId() const;

        /**
         *
         * This function returns the Time Zero Point Offset every Time is referring to.
         *
         * @returns timestamp_t the time offset of point zero
         * @rtsafe
         *
         */
        timestamp_t getTimeOffset() const;

        /**
         *
         * This function gets the number of indices of the given stream.
         *
         * @param  streamId [in] the stream Id
         *
         * @return int64_t
         * @rtsafe
         *
         */
        int64_t getStreamTableIndexCount(uint16_t stream_id) const;

        /**
         *
         * This function gets the number of chunks of the given stream.
         *
         * @param   streamId [in] the stream Id
         *
         * @return int64_t
         * @rtsafe
         *
         */
        int64_t getStreamIndexCount(uint16_t stream_id) const;

        /**
         *
         * This function gets the additional stream information data of the given stream.
         *
         * @param streamId [in]  the stream Id
         * @param infoData    [out] the tSampleStreamHeader data
         * @param infoSize   [out] the size of the returned data
         * @rtsafe
         *
         */
        void getAdditionalStreamInfo(uint16_t stream_id,
                                        const void** info_data,
                                        size_t* info_size) const;
        /**
         *
         * This function gets the stream name which is set.
         *
         * @param   streamId [in] the stream Id
         *
         * @return The stream name for the given stream id if set. Returns nullptr if stream id is invalid.
         * @rtsafe
         *
         */
        std::string getStreamName(uint16_t stream_id) const;

        bool streamExists(uint16_t stream_id) const;

        /**
         *
         * This function gets the first time stamp of the given stream.
         *
         * @param  streamId [in] the stream Id
         *
         * @return The first time appears in file for the stream.
         * @rtsafe
         *
         */
        timestamp_t getFirstTime(uint16_t stream_id) const;

        /**
         *
         * This function gets the last time stamp of the given stream.
         *
         * @param  streamId [in] the stream Id
         *
         * @return The last time in file for the stream.
         * @rtsafe
         *
         */
        timestamp_t getLastTime(uint16_t stream_id) const;

        /**
         * Get the amount of extensions in the file.
         * @return The amount of extensions in the file.
         * @rtsafe
         */
        size_t getExtensionCount() const;

        /**
         *
         * This function gets the extension data.
         *
         * @param  identifier    [in]  the extension name
         * @param  extensionInfo [out] the FileExtension struct
         * @param  data           [out] the extension data
         *
         * @return void
         * @rtsafe
         *
         */
        bool findExtension(const char* identifier,
                              FileExtension** extension_info,
                              void** data) const;

        /**
         *
         * This function gets the extension data.
         *
         * @param  index           [in]  the extension index
         * @param  extensionInfo [out] the FileExtension struct
         * @param  data           [out] the extension data
         *
         * @return void
         * @rtsafe
         *
         */
        void getExtension(size_t index,
                             FileExtension** extension_info,
                             void** data) const;

         /**
         *
         * This function gets the chunk index of the given file position.
         *
         * @param streamId [in] the stream Id
         * @param position    [in] the file position
         * @param timeFormat  [in] the format of position (tTimeFormat)
         *
         * @returns int64_t -> the chunk index
         * @rtsafe
         *
         */
        int64_t lookupChunkRef(uint16_t stream_id, int64_t position, TimeFormat time_format) const;

        /**
         * Returns the last chunk of a stream before the given chunk index, that has a given flag
         * @param chunkIndex [in] the chunk index
         * @param streamId [in] the stream Id
         * @param flag [in] the flag
         * @param header [out] the header of the found chunk
         * @param data [out] the data of the found chunk
         * @return true if a chunk with the requested flasg was found, false otherwise.
         */
        bool getLastChunkWithFlagBefore(uint64_t chunk_index, uint16_t stream_id, uint16_t flag,
                                        ChunkHeader& header,
                                        std::vector<uint8_t>& data);

    protected:
        /**
         * Initializes the reader.
         * @return Standard result.
         */
        virtual void initialize();

        /**
         *   Reads and initializes the Header struct
         *
         *   @returns Standard Result
         */
        void readFileHeader();

        /**
         *   Reads and initializes the ExtendedHeader
         *
         *   @returns Standard Result
         */
        void readFileHeaderExt();

        /**
         *   Initializes the IndexTable
         *
         *   @returns Standard Result
         */
        void readIndexTable();

    protected:
        /// For internal use only (will be moved to a private implementation).
        int64_t _cache_offset;
        /// For internal use only (will be moved to a private implementation).
        int64_t _cache_usage;

        /**
         * Reads the next chunk header
         */
        void readCurrentChunkHeader();

        /**
         * Reads the data of the current chunk.
         */
        void readCurrentChunkData(void* buffer);

        /**
         * Set the end of file indication.
         * @return Standard result.
         * @rtsafe
         */
        void setEOF();

        /**
         * Reads a raw data block from the file.
         * @param [out] buffer Here the data will be stored.
         * @param [in]  bufferSize The amount of bytes to read.
         * @return Standard result.
         */
        void readDataBlock(void* buffer, size_t buffer_size);

        /**
         * Resets the cache.
         */
        void clearCache();

        /**
         * Checks whether the current position in the file is valid.
         * @retval ERR_END_OF_FILE if past the end or not opened.
         * @retval ERR_DEVICE_IO if the file position could not be set.
         */
        void checkFilePtr();

        /**
         * Initializes the internal read buffers.
         */
        void allocReadBuffers();

        /**
         * Releases the internal read buffers.
         */
        void freeReadBuffers();
};

} // namespace
} // namespace

//*************************************************************************************************
#endif // _INDEXEDFILE_READER_V201_V301_CLASS_HEADER_
