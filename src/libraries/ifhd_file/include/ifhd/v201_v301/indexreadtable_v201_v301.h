/**
 * @file
 * Indexed table.
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

#ifndef INDEX_READ_TABLE_V201_V301_CLASS_HEADER
#define INDEX_READ_TABLE_V201_V301_CLASS_HEADER

namespace ifhd
{
namespace v201_v301
{

/**
 * Class for storing an index table of an indexed file.
 */
class DOEXPORT IndexReadTable
{
    private:
        struct IndexTable
        {
            IndexTable():index_table_offset(0), index_count(0), last_index(0), index_offset(0) {}

            uint64_t index_table_offset;
            uint64_t index_count;
            //the timestamp of the last index entry
            timestamp_t last_index;
            //the chunk position of the last index entry
            uint64_t index_offset;
        };

        struct MasterIndexTable: public IndexTable
        {
            MasterIndexTable() :
                master_chunk_ref_table(nullptr)
            {}

            ChunkRef *master_chunk_ref_table;
        };

        struct StreamIndexTable: public IndexTable
        {
            StreamIndexTable() :
                stream_info_header(nullptr),
                stream_ref_table(nullptr),
                additional_stream_info(nullptr)
            {}

            StreamInfoHeader* stream_info_header;
            StreamRef* stream_ref_table;
            void* additional_stream_info;
        };

        typedef std::vector<StreamIndexTable> StreamIndexTableVector;

        // master table
        MasterIndexTable           _master_index_table;

        // stream tables
        StreamIndexTable  _stream_index_tables[MAX_INDEXED_STREAMS + 1];

        // pointer to indexed source file
        IndexedFile*      _indexed_file;
        FileHeader*       _file_header;

    public:

        /**
         * Allocates all neccessary resources.
         * @param indexedFile [in] the indexed file from where the header is read from
         * @return Standard result.
         */
        void create(IndexedFile *indexed_file);

        /**
         * Build stream index tables
         * @return Standard result
         */
        void readIndexTable();

        /**
         * Frees all allocated resources.
         * @return Standard result.
         */
        void free();

        /**
         *
         * This function gets the name of a stream by its id.
         *
         * @param streamId [in] the stream Id
         *
         * @returns Stream name
         *
         */
        std::string getStreamName(uint16_t stream_id) const;

        bool streamExists(uint16_t stream_id) const;

        /**
         *
         * This function gets the information header of a stream by its id.
         *
         * @param streamId [in] the stream Id
         *
         * @returns Stream information header
         *
         */
        const StreamInfoHeader* getStreamInfo(uint16_t stream_id) const;


        /**
         *
         * This function gets the first time stamp of the given stream.
         *
         * @param streamId [in]  the stream Id
         *
         * @return timestamp_t
         *
         */
        timestamp_t getFirstTime(uint16_t stream_id) const;

        /**
         *
         * This function gets the last time stamp of the given stream.
         *
         * @param streamId [in]  the stream Id
         *
         * @return timestamp_t
         *
         */
        timestamp_t getLastTime(uint16_t stream_id) const;

        /**
         *
         * This function gets the additional stream information by its id.
         *
         * @param streamId [in] the stream Id
         *
         * @return Additional stream information
         *
         */
        const void* getAdditionalStreamInfo(uint16_t stream_id) const;

        /**
         * Returns the item count of a stream.
         * @param streamId [in] The stream id.
         * @return The item count of the stream.
         * @rtsafe
         */
        int64_t getItemCount(uint16_t stream_id) const;

        /**
         * Adjusts the indices in the given chunk header
         * @param header [inout] The header that should be adjusted
         * @return Nothing.
         */
        void adjustChunkHeader(ChunkHeader* header);

        /**
         * Filles a chunk header from a master index entry
         * @param masterIdx [in] The index of the entry.
         * @param header [out] The header that should be filled.
         * @param chunkIndex [out] The index of the chunk.
         * @param chunkOffset [out] The file offset of the chunk.
         */
        void fillChunkHeaderFromIndex(uint32_t master_idx, ChunkHeader* header,
            int64_t* chunk_index, int64_t* chunk_offset);

        /**
         *
         * This function gets the chunk index of the given file position.
         *
         * @param[in]   streamId  the stream Id
         * @param[in]   pos          the file position
         * @param[in]   timeFormat   the format to look for (see IndexedFile::TF_*)
         * @param[out]  chunkIndex    the index of the chunk where the search should start.
         * @param[out]  chunkOffset   the file offset of the chunk where the search should start.
         * @param[out]  endChunkIndex the nearest end chunkindex of pos
         * @param[out]  masterIndex   the index of the nearest master index entry.
         *
         * @returns Standard result
         *
         */
        int64_t lookupChunkRef(uint16_t stream_id, int64_t pos,
                               TimeFormat time_format, int64_t *chunk_index,
                               int64_t* chunk_offset, int64_t* end_chunk_index,
                               int64_t* master_index) const;

        /**
         * Tries to find an entry before or at a given timestamp that has certain flags set.
         * @param[in] streamId The stream id.
         * @param[in] chunkIndex The limiting chunk.
         * @param[in] chunkFlags The flags to look for.
         * @param[out] masterIndex The wil be set to the index in the master index table.
         * @return Standard result
         */
        bool findNearestEntryWithFlags(uint16_t stream_id, uint64_t chunk_index,
                                       uint16_t chunk_flags, uint64_t* master_index);

        /**
         * Checks if the specified index is valid
         * @param [in] refMasterTableIndex The index to be checked
         * @retval ERR_NOERROR Index is fine
         * @retval ERR_OUT_OF_RANGE Index is not within the valid range.
         */
        bool validateRawMasterIndex(int32_t ref_master_table_index);

    private:


        /**
         * Sets additional index offsets
         * @param [in] streamId The stream id.
         * @param [in] additionalIndexInfo Additional stream offsets.
         */
        void setIndexOffsetInfos(uint16_t stream_id,
            AdditionalIndexInfo& additional_index_info);

        /**
         * Adds a new item to the stream index table
         * @param [in] streamId The stream id.
         * @param [in] streamRef Stream reference item.
         * @param [in] streamInfo Stream info header item.
         * @param [in] additionalStreamInfo Additional stream information.
         * @param [in] count Stream reference items count
         */
        void addStreamIndexTableEntry(uint16_t stream_id, StreamRef* stream_ref,
                                         StreamInfoHeader* stream_info,
                                         void* additional_stream_info,
                                         uint64_t count);

        /**
         * Adds a new item to the master index table
         * @param [in] refTbl Master stream reference table
         * @param [in] count Stream reference items count
         */
        void addMasterIndexTableEntry(void* ref_tbl, uint64_t count);

        /**
         *
         * This function gets a stream reference of the given id and index.
         *
         * @param streamId  [in] the stream Id
         * @param streamIdx [in] the stream index
         * @param streamRef    [out] stream reference
         *
         * @returns Standard result
         *
         */
        void getStreamRef(uint16_t stream_id, uint32_t stream_idx,
                             StreamRef *stream_ref) const;
};

} //namespace
} // namespace
#endif // _INDEX_READ_TABLE_V201_V301_CLASS_HEADER_
