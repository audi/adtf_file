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


#include <ifhd/ifhd.h>

namespace ifhd
{
namespace v201_v301
{

void IndexWriteTable::create(timestamp_t index_delay)
{     
    _index_delay = index_delay;
}

void IndexWriteTable::free()
{
    _master_index.clear();
    _master_index.last_index = 0;
    _master_index.index_table_offset = 0;
    _master_index.index_count = 0;
    _master_index.index_offset = 0;

    for (uint16_t stream_id = 1; stream_id <= MAX_INDEXED_STREAMS; ++stream_id)
    {
        _stream_index_tables[stream_id].clear();
        _stream_index_tables[stream_id].last_index = 0;
        _stream_index_tables[stream_id].index_table_offset = 0;
        _stream_index_tables[stream_id].index_count = 0;
        _stream_index_tables[stream_id].index_offset = 0;
    }
}

uint16_t IndexWriteTable::getMaxStreamId() const
{
    return MAX_INDEXED_STREAMS; // this is off by one, but existing code depends on it,
                                // method name is wrong
}

int64_t IndexWriteTable::getItemCount(uint16_t stream_id) const
{
    if (stream_id == 0)
    {
        return _master_index.size();
    }
    else
    {
        return _stream_index_tables[stream_id].size();
    }
}

int64_t IndexWriteTable::getBufferSize(uint16_t stream_id) const
{
    if (stream_id == 0)
    {
        return _master_index.size() * sizeof(ChunkRef);
    }
    else
    {
        return _stream_index_tables[stream_id].size() * sizeof(StreamRef);
    }
}

void IndexWriteTable::copyToBuffer(uint16_t stream_id, void* buffer) const
{
    if (MAX_INDEXED_STREAMS < stream_id)
    {
        throw std::out_of_range("invalid stream id");
    }

    // unfortunately std::deque does not guarantee a continuous memory layout
    // so we have to copy the items one by one.

    if (stream_id == 0) //get the master index
    {
        ChunkRef* output = static_cast<ChunkRef*>(buffer);
        for (MasterIndexTable::const_iterator it_entry = _master_index.cbegin();
             it_entry != _master_index.end(); ++it_entry, ++output)
        {
            a_util::memory::copy(output, sizeof(ChunkRef), &(*it_entry), sizeof(ChunkRef));
        }
    }
    else // get the StreamTables
    {
        StreamRef* output = static_cast<StreamRef*>(buffer);
        for (StreamIndexTable::const_iterator it_entry = _stream_index_tables[stream_id].cbegin();
             it_entry != _stream_index_tables[stream_id].end(); ++it_entry, ++output)
        {
            // adjust the offset from dropped chunks
            output->ref_master_table_index = it_entry->ref_master_table_index;
        }
    }
}

void IndexWriteTable::append(uint16_t stream_id,
                            uint64_t stream_index,
                            uint64_t chunk_index,
                            uint64_t file_pos,
                            uint32_t size,
                            timestamp_t time_stamp,
                            uint32_t flags,
                            bool& index_entry_appended)
{
    if ((file_pos & 0xF) != 0x00) // check for correct alignment
    {
        throw std::runtime_error("unexpected");
    }
    index_entry_appended = false;
    //StreamId 0 is never valid
    if (stream_id == 0)
    {
        throw std::out_of_range("invalid stream index");
    }

    StreamIndexTable* stream_table = &_stream_index_tables[stream_id];

    if ((flags == 0) &&
        (stream_table->last_index > 0 &&
        (time_stamp - stream_table->last_index) < _index_delay))
    {
        return;
    }

    ChunkRef  index_entry;
    StreamRef stream_index_entry;

    //index entry
    index_entry.chunk_offset          = file_pos;
    index_entry.size                 = size;
    index_entry.time_stamp            = time_stamp;
    index_entry.flags                = (uint16_t) flags;
    index_entry.stream_id             = stream_id;
    index_entry.chunk_index           = chunk_index;
    index_entry.stream_index          = stream_index;
    index_entry.ref_stream_table_index  = stream_table->index_count;

    //Stream Index entry is only a counter ref to the master table
    stream_index_entry.ref_master_table_index = _master_index.index_count;

    _master_index.push_back(index_entry);
    ++_master_index.index_count;

    stream_table->push_back(stream_index_entry);
    stream_table->last_index = time_stamp;
    ++stream_table->index_count;

    index_entry_appended = true;
}

void IndexWriteTable::remove(uint64_t chunk_index, uint16_t stream_id)
{
    ++_master_index.index_offset;
    ++_stream_index_tables[stream_id].index_offset;

    if (_master_index.empty())
    {
        throw std::runtime_error("index empty");
    }

    ChunkRef& chunk = _master_index.front();
    if (chunk.chunk_index != chunk_index)
    {
        // there is no index entry for this chunk
        // we support removing the first index entry only
        return;
    }

    _stream_index_tables[chunk.stream_id].pop_front();
    ++_stream_index_tables[chunk.stream_id].index_table_offset;

    _master_index.pop_front();
    ++_master_index.index_table_offset;
}

uint64_t IndexWriteTable::getIndexOffset(uint16_t stream_id) const
{
    if (stream_id == 0)
    {
        return _master_index.index_offset;
    }
    else
    {
        return _stream_index_tables[stream_id].index_offset;
    }
}

uint64_t IndexWriteTable::getIndexTableOffset(uint16_t stream_id) const
{
    if (stream_id == 0)
    {
        return _master_index.index_table_offset;
    }
    else
    {
        return _stream_index_tables[stream_id].index_table_offset;
    }
}

} // namspace v400
} // namespace
