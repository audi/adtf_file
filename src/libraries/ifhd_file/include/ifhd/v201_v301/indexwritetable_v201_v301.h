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

#ifndef INDEX_WRITE_TABLE_V201_V301_CLASS_HEADER
#define INDEX_WRITE_TABLE_V201_V301_CLASS_HEADER

namespace ifhd
{
namespace v201_v301
{

/**
 * Class for storing an index table of an indexed file.
 */
class DOEXPORT IndexWriteTable
{
    private:
        struct IndexTable
        {
            IndexTable():index_table_offset(0), index_count(0), last_index(0), index_offset(0) {}
            uint32_t index_table_offset;
            uint32_t index_count;
            //the timestamp of the last index entry
            timestamp_t last_index;
            //the chunk position of the last index entry
            uint64_t index_offset;
        };

        struct MasterIndexTable: public std::deque<ChunkRef>, public IndexTable
        {        
        };

        MasterIndexTable _master_index;

        struct StreamIndexTable: public std::deque<StreamRef>, public IndexTable
        {           
        };

        StreamIndexTable _stream_index_tables[MAX_INDEXED_STREAMS + 1];
        /// The maximum delay betweeen indices (microseconds)
        timestamp_t _index_delay;

    public:

        /**
         * Allocates all neccessary resources.
         * @param index_delay The maximum delay between indices.
         * @return Standard result.
         */
        void create(timestamp_t index_delay = 1000000);

        /**
         * Frees all allocated resources.
         * @return Standard result.
         */
        void free();

        /**
         * Returns the biggest possible stream id.
         * @return The biggest possible stream id.
         * @rtsafe
         */
        uint16_t getMaxStreamId() const;

        /**
         * Returns the item count of a stream.
         * @param streamId [in] The stream id.
         * @return The item count of the stream.
         * @rtsafe
         */
        int64_t getItemCount(uint16_t stream_id) const;

        /**
         * Returns the buffer size of a stream.
         * @param streamId [in] The stream id.
         * @return The buffer size of the stream.
         * @rtsafe
         */
        int64_t getBufferSize(uint16_t stream_id) const;

        /**
         * Copys all stream data into a buffer. No buffer overflow checking is done!
         * @param streamId [in] the stream id.
         * @param buffer [out] The buffer that is to be filled.
         * @rtsafe
         */
        void copyToBuffer(uint16_t stream_id, void* buffer) const;

        /**
         * Appends a new item to the index table.
         *
         * @param streamId [in] The stream id.
         * @param streamIndex [in] The stream index.
         * @param chunkIndex [in] The chunk index.
         * @param filePos [in] The file position.
         * @param size [in] The size of the item.
         * @param timeStamp [in] The timestamp.
         * @param flags [in] append flags, see @ref IndexedFile::tChunkType
         * @param indexEntryAppended [in, out] return true if index enty is appended for this chunk
         *
         * @return  Standard result
         */
        void append(uint16_t stream_id,
                       uint64_t stream_index,
                       uint64_t chunk_index,
                       uint64_t file_pos,
                       uint32_t size,
                       timestamp_t time_stamp,
                       uint32_t flags,
                       bool& index_entry_appended);

        /**
         * The method removes an index entry from the front of the tables.
         * @param [in] chunkIndex the chunk index
         * @param [in] streamId the stream id
         * @return Standard result.
         */
        void remove(uint64_t chunk_index, uint16_t stream_id);

        /**
         * Returns the offset of the first stream index in the chunk headers fro the given stream.
         * @param [in] streamId The stream id.
         * @return the offset of the first stream index in the chunk headers fro the given stream.
         */
        uint64_t getIndexOffset(uint16_t stream_id) const;

        /**
         * Returns the offset of the stream table indices in the chunk header and master/stream references.
         * @param [in] streamId The stream id.
         * @return the offset of the stream table indices in the chunk header and master/stream references..
         */
        uint64_t getIndexTableOffset(uint16_t stream_id) const;
};

} // namspace v400
} // namespace
#endif // _INDEX_WRITE_TABLE_V201_V301_CLASS_HEADER_
