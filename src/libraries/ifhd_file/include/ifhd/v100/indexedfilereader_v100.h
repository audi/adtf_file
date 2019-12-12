/**
 * @file
 * Indexed file reader compatibility code.
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

#ifndef INDEXEDFILE_READER_V100_CLASS_HEADER
#define INDEXEDFILE_READER_V100_CLASS_HEADER

namespace ifhd
{
namespace v100
{

//*************************************************************************************************
/**
 * Class for reading indexed file in version 100
 */
class IndexedFileReaderV100 : public IndexedFileV100
{
    public:
        enum
        {
            flag_none                       = 0x0,
            flag_disable_file_system_cache     = 0x1
        };
 
    protected:
        ///flag value to attach
        uint32_t                     _flags;

    protected:
        ///index table
        ChunkHeader*               _index_table;
        ///size of index table
        int                        _index_table_size;
        ///endmarker for data
        int64_t                    _end_of_data_marker;

    public:
        /**
         * Default constructor.
         */
        IndexedFileReaderV100();

        /**
         * Destructor.
         */
        ~IndexedFileReaderV100();

        /**
         * Attaches the reader to an already opened file.
         *
         * @param fileHeader [in] The file header.
         * @param file      [in] The file.
         * @param flags   [in] Unused.
         *
         * @return Standard result.
         */
        void attach(FileHeader* file_header, utils5ext::File* file, uint32_t flags=0);

        /**
         * Detaches the reader from a file.
         *
         * @return Standard result.
         */
        void detach();

        /**
         *   Resets the file to the beginning of data
         *
         *   @returns Standard Result
         */
        void reset();

        /**
         *   Sets the file position to the Chunk of index and returns the respective ChunkInfo
         *
         *   @param   position    [in] New file position
         *   @param   timeFormat  [in] ???
         *   @param   flags    [in] ???
         *
         *   @returns Standard Result
         */
        int64_t seek(int64_t position, int time_format, uint32_t flags=0);

        /**
         *   Increments the file index until seeked chunk flags found
         *
         *   @param   flags         [in] Flags seeked for
         *   @param   chunk        [out] Returns the ChunkInfo of the next Chunk with the flags
         *
         *   @returns Current Index
         */
        int64_t seekFlags(uint32_t flags, ChunkHeader** chunk);

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
         *   Returns the current ChunkInfo from IndexTable
         *
         *   @param  chunkInfo [out] Current chunk info struct
         *
         *   @returns Standard result code
         */
        void queryChunkInfo(ChunkHeader** chunk_info);

        /**
         *   Reads and returns the next Chunk and increments the IndexTable index
         *
         *   @param  data [out] Current chunk data
         *
         *   @returns Standard result code
         */
        void readChunk(void** data);

        /**
         *   Skips the next Chunk by incrementing the IndexTable index and setting the new FilePosition
         *
         *   @returns Standard result code
         */
        void skipChunk();

        /**
         *   Reads and returns the next Chunk and ChunkInfo struct and increments the IndexTable index
         *
         *   @param  chunkInfo [out] Current chunk info
         *   @param  data [out] Current chunk data
         *
         *   @returns Standard result code
         */
        void readNextChunk(ChunkHeader** chunk_info, void** data);

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
         *   Returns the number of indexes of the current file
         *
         *   @returns Number of indexes.
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

    protected:
        /**
         * @todo
         * @returns Standard result code
         */
        void initialize();

        /**
         * @todo
         * @returns Standard result code
         */
        void readFileHeader();

        /**
         * @todo
         * @returns Standard result code
         */
        void readFileHeaderExt();

        /**
         * @todo
         * @returns Standard result code
         */
        void readIndexTable();

    public:
        /**
         * @todo
         * @returns Index
         */
        int64_t internalGetIndex() const;

        /**
         * @todo
         * @returns Pointer to fileheader
         */
        FileHeader* internalGetFileHeader() const;
};

} //namespace v100
} // namespace

//*************************************************************************************************
#endif // _INDEXEDFILE_READER_V100_CLASS_HEADER_
