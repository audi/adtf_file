/**
 * @file
 * Indexed file writer.
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

#ifndef INDEXEDFILE_WRITER_V201_V301_CLASS_HEADER
#define INDEXEDFILE_WRITER_V201_V301_CLASS_HEADER

namespace ifhd
{
namespace v201_v301
{

/**
 * Class for writing indexed files.
 */
class DOEXPORT IndexedFileWriter : public IndexedFile
{
    private:
        class IndexedFileWriterImpl;
        a_util::memory::unique_ptr<IndexedFileWriterImpl> _d;
    
    public:
        /**
         * Callback interface that informs about dropped chunks in history mode
         */
        class ChunkDroppedCallback
        {
            public:
                  /**
                   * Called whenever a chunk is dropped from the history.
                   * @param[in] index The chunk index.
                   * @param[in] streamId The stream id of the chunk.
                   * @param[in] flags The Flags of the chunk.
                   * @param[in] time The timestamp of the chunk.
                   * @return Will be forwarded by WriteChunk.
                   */
                  virtual void onChunkDropped(uint64_t index,
                                                 uint16_t stream_id,
                                                 uint16_t flags,
                                                 timestamp_t time) = 0;
        };

    protected:
        /*! \cond PRIVATE */
        /// For internal use only (will be moved to a private implementation).
        bool                    _is_open;
        bool                    _catch_first_time;
        timestamp_t             _time_offset;

        int                     _cache_min_store_at_once;
        volatile int            _cache_flush_ptr;
        volatile int            _cache_insert_ptr;
        std::atomic<int>        _cache_usage_count;

        std::mutex              _mutex_freed_event;
        std::mutex              _mutex_cache_used;
        std::condition_variable _cond_freed_event;
        std::condition_variable _cond_cache_used;
        std::atomic<bool>       _cond_cache_used_processed;

        int64_t                 _ref_index;
        timestamp_t             _last_chunk_time;

        bool                    _sync_mode;

        IndexWriteTable         _index_table;
        bool                    _last_write_result;
        uint32_t                _last_write_system_error;

        typedef struct TagStreamInfoAdd
        {
            uint8_t* data;
            int    is_reference;
        } StreamInfoAdd;

        StreamInfoHeader       _stream_info[MAX_INDEXED_STREAMS];
        StreamInfoAdd          _stream_info_add[MAX_INDEXED_STREAMS];

        FilePos                _file_pos_last_chunk;

        std::string            _file_name;
        std::string            _temp_file_name;

        std::string            _prefix_of_temp_save_file_name;
        //! @endcond

        /*
        * if true the tempfilename (savemode) used an prefix
        */
        bool _use_prefix_temp_file_extension;

    public:
        /**
        * Get the Temp Save File Name (Filename during the Save Mode)
        *
        * @return The filename.
        */
        std::string getTempSaveFileName();
        /**
        * Set the mode, if a prefix is set by a temp Save File Name or not
        *
        * @param useMode [in] The filename.
        *
        * @return void
        */
        void setPrefixTempFileExtension(bool use_mode);

        /**
        * Get the mode, if a prefix is set by a temp Save File Name or not
        *
        * @return  bool.
        */
        bool getPrefixTempFileExtensionMode() const;

        /**
        * Returns the prefix
        *
        * @return  String prefix.
        */
        std::string getPrefix() const;

        /**
        * Set a Prefix to the incoming Filename and return the new name
        *
        * @param filename [in] The filename.
        *
        * @return  String.
        */
        std::string getNewFileNameWithPrefix(const std::string& filename) const;
     private:
        /**
        * Return the Name of the file during creation mode.
        * It can be with or without prefix.
        *
        * @param filename [in] The filename.
        * @param saveFilename [out] Filename the save filename 
        */
        void createAFileWithPrefixdAndAFileWithoutPrefix(const std::string& filename,
                                                               std::string& save_filename);
    
        /**
        * Rename the TempFile Name '~$FileName.dat' back to
        * 'FileName.dat' Filename
        */
        void renameTempSaveToFileName();

    public:
        /**
         * Default constructor.
         */
        IndexedFileWriter();
        /**
         * Destructor.
         */
        ~IndexedFileWriter();

        void setDateTime(const a_util::datetime::DateTime& date_time);

        /**
         * Create a new indexed file.
         *
         * @param filename [in] The filename.
         * @param cacheSize  [in] The cache size.
         * @param flags   [in] Creation flags, see @ref OpenMode
         * @param fileTimeOffset [in] unused.
         * @param history [in] Timestamp of history.
         * @param historySize [in] Size of the history.
         * @param index_delay [in] The maximum time difference between index entries.
         * @param dropCallback [in] CallBack implementation  if chunk was dropped
         */
        void create(const std::string& filename,
                    size_t cache_size=0,
                    uint32_t flags=0,
                    uint64_t file_time_offset=0,
                    timestamp_t history = 0,
                    utils5ext::FileSize history_size = 0,
                    size_t cache_minimum_write_chunk_size = 0,
                    size_t cache_maximum_write_chunk_size = 0,
                    ChunkDroppedCallback* drop_callback = nullptr,
                    timestamp_t index_delay = 1000000);

        /**
         * Finishes writing to and closes the file.
         *
         * @return Standard result.
         */
        void close();

        /**
         * Writes a new chunk to the file.
         * This function is not thread safe! (sync must be done outside in caller)!
         *
         * @param streamId [in] The stream id.
         * @param data        [in] The chunk data.
         * @param dataSize    [in] The data size.
         * @param timeStamp   [in] The timestamp of the chunk.
         * @param flags       [in] Chunk flags, see @ref tChunkType
         */
        void writeChunk(uint16_t stream_id,
                           const void* data,
                           uint32_t data_size,
                           timestamp_t time_stamp,
                           uint32_t flags);
        /**
         * Writes a new chunk to the file.
         * This function is not thread safe! (sync must be done outside in caller)!
         *
         * @param streamId [in] The stream id.
         * @param data        [in] The chunk data.
         * @param dataSize    [in] The data size.
         * @param timeStamp   [in] The timestamp of the chunk.
         * @param flags       [in] Chunk flags, see @ref tChunkType
         * @param indexEntryAppended [out] return value Index Entry Added
         */
        void writeChunk(uint16_t stream_id,
                           const void* data,
                           uint32_t data_size,
                           timestamp_t time_stamp,
                           uint32_t flags,
                           bool& index_entry_appended);

        /**
         * Sets additional info for a stream.
         *
         * @param streamId  [in] The stream id.
         * @param infoData     [in] The info data.
         * @param infoDataSize [in] The info data size.
         * @param useAsReference  [in] Whether to store the data in an internal buffer or not (The data is accessed when the file is closed).
         */
        void setAdditionalStreamInfo(uint16_t stream_id,
                                        const void*  info_data,
                                        uint32_t info_data_size,
                                        bool   use_as_reference=false);
        /**
         * Set the name of a stream.
         *
         * @param streamId [in] The stream id.
         * @param streamName [in] The new name.
         * @return Standard result.
         * @rtsafe
         */
        void setStreamName(uint16_t stream_id,
                              const char* stream_name);

        /**
         * Get the amount of cache space used.
         *
         * @return The amount of cache space used.
         * @rtsafe
         */
        int getCacheUsage();

        /**
         * In case there is a history set up, this switches over to permanent storage.
         * @return Standard result.
         */
        void quitHistory();

        /**
         * Retrieves the last system error (errno or GetLastError)
         *
         * @return The amount of cache space used.
         * @rtsafe
         */
        uint32_t getLastSystemErrorFromWriteThread() const;

        /**
         * Stops the cache writing thread and flushes data to disk.
         */
        void stopAndFlushCache();

    protected:
        /**
         * Initializes the writer.
         */
        void initialize();

        /**
         * Write the file header
         *
         * @retval ERR_NOERROR everything is OK
         * @retval ERR_DEVICE_IO couldn't write to file
         */
        void writeFileHeader();

        /**
         * write  the extension file header
         *
         * @retval ERR_NOERROR everything is OK
         * @retval ERR_DEVICE_IO couldn't write to file
         * @retval ERR_MEMORY no memory available to create file extension header
         *
         */
        void writeFileHeaderExt();

        /**
         * write index table
         */
        void writeIndexTable();

        

    protected:
        /**
         * write the given data to local cache
         *
         * @param [in] data data to be written
         * @param [in] dataSize size of data
         * @param [in] isChunkHeader
         *
         * @return standard result code
         */
        void writeToCache(const void* data, int data_size, const bool is_chunk_header = false);

        /**
         * stores the data to disk
         * @param [in] flush flushes the data
         *
         * @return standard result code
         */
        void storeToDisk(bool flush);

        /**
         * @todo
         * @return void
         */
        void writeCacheToDisk();

        /**
         * allocates a given amount of memory for chaching
         *
         * @param size size of memory to be allocated (in bytes)
         *
         * @return standard result
         */
        void allocCache(int size);

        /**
         * internal writing function
         *
         * @param [in] buffer pointer to data to be written
         * @param [in] bufferSize size of data
         * @param [in] useSegmentSize
         *
         * @return written bytes
         *
         */
        void internalWrite(const void* buffer,
                           size_t buffer_size,
                           bool use_segment_size);

    friend class IndexedFileAsyncWriter;
};

} //namespace V201_V301
} // namespace ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_WRITER_V201_V301_CLASS_HEADER_
