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
#include <algorithm>

namespace ifhd
{
namespace v201_v301
{


void IndexReadTable::create(IndexedFile* indexed_file)
{
    _indexed_file = indexed_file;
    a_util::memory::set(_stream_index_tables, sizeof(StreamIndexTable), 0, sizeof(StreamIndexTable));
    _indexed_file->getHeaderRef(&_file_header);
    // add a dummy (index 0) to our stream table
    addStreamIndexTableEntry(0, nullptr, nullptr, nullptr, 0);
}

void IndexReadTable::free()
{

    _master_index_table.master_chunk_ref_table = nullptr;
    _master_index_table.last_index = 0;
    _master_index_table.index_table_offset = 0;
    _master_index_table.index_count = 0;
    _master_index_table.index_offset = 0;

    _indexed_file = nullptr;
    _file_header = nullptr;
}

int64_t IndexReadTable::getItemCount(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        return -1;
    }

    if (stream_id == 0)
    {
        return _master_index_table.index_count;
    }
    else
    {
        if (_stream_index_tables[stream_id].stream_info_header)
        {
            return _stream_index_tables[stream_id].index_count;
        }
        else
        {
            return -1;
        }
    }
}

void IndexReadTable::setIndexOffsetInfos(uint16_t stream_id,
                                             AdditionalIndexInfo& additional_index_info)
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        throw std::out_of_range("invalid stream id");
    }

    if (0 == stream_id)
    {
        _master_index_table.index_table_offset = additional_index_info.stream_table_index_offset;
        _master_index_table.index_offset = additional_index_info.stream_index_offset;
    }
    else
    {
        _stream_index_tables[stream_id].index_table_offset = additional_index_info.stream_table_index_offset;
        _stream_index_tables[stream_id].index_offset = additional_index_info.stream_index_offset;
    }
}

void IndexReadTable::addStreamIndexTableEntry(uint16_t stream_id,
                                                  StreamRef* stream_ref,
                                                  StreamInfoHeader* stream_info,
                                                  void* additional_stream_info,
                                                  uint64_t count)
{
    StreamIndexTable& stream_idx_tbl = _stream_index_tables[stream_id];

    stream_idx_tbl.stream_ref_table = stream_ref;
    stream_idx_tbl.stream_info_header = stream_info;
    stream_idx_tbl.additional_stream_info = additional_stream_info;
    stream_idx_tbl.index_count = count;
}

void IndexReadTable::addMasterIndexTableEntry(void* ref_tbl, uint64_t count)
{
    if (nullptr == _master_index_table.master_chunk_ref_table)
    {
        _master_index_table.master_chunk_ref_table = (ChunkRef*)ref_tbl;
        _master_index_table.index_count = count;
    }
}

/**
*   Initializes the IndexTable
*
*   @returns Standard Result
*/
void IndexReadTable::readIndexTable()
{
    FileExtension* extension_info = nullptr;
    void* extension_data = nullptr;
    void* additional_stream_info = nullptr;
    ChunkRef* master_index_table = nullptr;
    int64_t master_index_count = 0;
    AdditionalIndexInfo additonal_index_info;

    a_util::memory::set(&additonal_index_info, sizeof(additonal_index_info), 0, sizeof(additonal_index_info));

    // read "index_add_0" extension (offset info for master index table)
    
    if (_indexed_file->findExtension(IDX_EXT_INDEX_ADDITONAL_0, &extension_info, &extension_data))
    {
        additonal_index_info = *static_cast<AdditionalIndexInfo*>(extension_data);
    }
    

    setIndexOffsetInfos(0, additonal_index_info);

    // read "index0" extension (master index table)
    if (!_indexed_file->findExtension(IDX_EXT_INDEX_0, &extension_info, &extension_data))
    {
        throw std::runtime_error("Unable to find " IDX_EXT_INDEX_0 " extension (master index table). File seems to be corrupted.");
    }

    master_index_table = (ChunkRef*) extension_data;
    master_index_count = extension_info->data_size / sizeof(ChunkRef);

    if (_file_header->header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        for (int index = 0; index < master_index_count; ++index)
        {
            stream2ChunkRef(*_file_header, master_index_table[index]);
        }
    }

    // add master index to table
    addMasterIndexTableEntry(extension_data, master_index_count);

    // read "index[1-n]" extensions (stream index tables)
    for (uint16_t idx = 1;
         idx <= MAX_INDEXED_STREAMS;
         ++idx)
    {

        if (!_indexed_file->findExtension(a_util::strings::format("%s%d", IDX_EXT_INDEX, idx).c_str(), &extension_info, &extension_data))
        {
            continue;
        }

        uint64_t stream_table_mem_size = extension_info->data_size;
        uint64_t stream_ref_count = 0;
        StreamInfoHeader* stream_info_hdr = nullptr;
        StreamRef* stream_ref = nullptr;

        // read stream info header
        stream_info_hdr = (StreamInfoHeader*) extension_data;
        stream2StreamInfoHeader(*_file_header, *stream_info_hdr);

        extension_data = (void*) (((uint8_t*) extension_data) + sizeof(StreamInfoHeader));
        stream_table_mem_size -= sizeof(StreamInfoHeader);

        if (stream_info_hdr->info_data_size != 0)
        {
            additional_stream_info = extension_data;
            extension_data = (void*) (((uint8_t*) extension_data) + stream_info_hdr->info_data_size);
            stream_table_mem_size -= stream_info_hdr->info_data_size;
        }

        // read stream ref data
        stream_ref = (StreamRef*) extension_data;
        stream_ref_count = stream_table_mem_size / sizeof (StreamRef);

        if (_file_header->header_byte_order != PLATFORM_BYTEORDER_UINT8)
        {
            for (uint32_t index = 0; index < stream_ref_count; ++index)
            {
                stream2StreamRef(*_file_header, stream_ref[index]);
            }
        }

        // add stream index to table
        addStreamIndexTableEntry(idx, stream_ref, stream_info_hdr, additional_stream_info, stream_ref_count);

        if (_indexed_file->findExtension(a_util::strings::format("%s%d", IDX_EXT_INDEX_ADDITONAL, idx).c_str(), &extension_info, &extension_data))
        {
            additonal_index_info = *static_cast<AdditionalIndexInfo*>(extension_data);
            stream2AdditionalStreamIndexInfo(*_file_header, additonal_index_info);
            setIndexOffsetInfos(idx, additonal_index_info);
        }
    }
}

void IndexReadTable::getStreamRef(uint16_t stream_id,
                                      uint32_t stream_idx,
                                      StreamRef *stream_ref) const
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        throw std::out_of_range("invalid stream id");
    }

    const StreamIndexTable& idx_tbl = _stream_index_tables[stream_id];
    if (idx_tbl.stream_info_header &&
        idx_tbl.stream_info_header->stream_index_count > stream_idx)
    {
        a_util::memory::copy(stream_ref, sizeof(StreamRef),
                                &idx_tbl.stream_ref_table[stream_idx], sizeof(StreamRef));
        stream_ref->ref_master_table_index -= (uint32_t)_master_index_table.index_table_offset;
    }
    else
    {
        throw std::out_of_range("invalid stream id");
    }
}

const StreamInfoHeader* IndexReadTable::getStreamInfo(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        throw std::out_of_range("invalid stream id");
    }

    return _stream_index_tables[stream_id].stream_info_header;
}

timestamp_t IndexReadTable::getFirstTime(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        throw std::out_of_range("invalid stream id");
    }

    if (stream_id == 0)
    {
        if (_master_index_table.index_count > 0)
        {
            return _master_index_table.master_chunk_ref_table[0].time_stamp;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (_stream_index_tables[stream_id].index_count > 0)
        {
            return _stream_index_tables[stream_id].stream_info_header->stream_first_time;
        }
        else
        {
            throw std::out_of_range("invalid stream id");
        }
    }
}

timestamp_t IndexReadTable::getLastTime(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        throw std::out_of_range("invalid stream id");
    }

    if (stream_id == 0)
    {
        if (_master_index_table.index_count > 0)
        {
            return _master_index_table.master_chunk_ref_table[0].time_stamp + _file_header->duration;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (_stream_index_tables[stream_id].index_count > 0)
        {
            return _stream_index_tables[stream_id].stream_info_header->stream_last_time;
        }
        else
        {
            throw std::out_of_range("invalid stream id");
        }
    }
}

bool IndexReadTable::streamExists(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS ||
        !_stream_index_tables[stream_id].stream_info_header)
    {
        return false;
    }
    return true;
}

std::string IndexReadTable::getStreamName(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS ||
        !_stream_index_tables[stream_id].stream_info_header)
    {
        throw std::out_of_range("invalid stream id");
    }

    return std::string(reinterpret_cast<const char*>(_stream_index_tables[stream_id].stream_info_header->stream_name));
}

const void* IndexReadTable::getAdditionalStreamInfo(uint16_t stream_id) const
{
    if (stream_id > MAX_INDEXED_STREAMS)
    {
        throw std::out_of_range("invalid stream id");
    }

    return _stream_index_tables[stream_id].additional_stream_info;
}

bool IndexReadTable::validateRawMasterIndex(int32_t ref_master_table_index)
{
    if (ref_master_table_index - _master_index_table.index_table_offset >=
        _master_index_table.index_count)
    {
        return false;
    }

    return true;
}

void IndexReadTable::adjustChunkHeader(ChunkHeader* header)
{
    if (header->stream_id > MAX_INDEXED_STREAMS ||
        header->stream_id == 0)
    {
        throw std::out_of_range("invalid stream id");
    }

    if (header->ref_master_table_index > _master_index_table.index_table_offset)
    {
        header->ref_master_table_index -= (uint32_t) _master_index_table.index_table_offset;
    }
    else
    {
        header->ref_master_table_index = 0;
    }

    header->stream_index -= _stream_index_tables[header->stream_id].index_offset;
}

void IndexReadTable::fillChunkHeaderFromIndex(uint32_t master_idx,
                                                  ChunkHeader* header,
                                                  int64_t* chunk_index, int64_t* chunk_offset)
{
    ChunkRef& ref = _master_index_table.master_chunk_ref_table[master_idx];
    header->ref_master_table_index = master_idx;
    header->offset_to_last        = 0;
    header->time_stamp           = ref.time_stamp;
    header->size                = ref.size;
    header->flags               = ref.flags;
    header->stream_id            = ref.stream_id;
    header->stream_index         = ref.stream_index;

    *chunk_index = ref.chunk_index - _master_index_table.index_offset;
    *chunk_offset = ref.chunk_offset;
}

/**
 *
 * This function gets the chunk index of the given file position.
 *
 * @param uint32_t stream_id [in] the stream Id
 * @param int64_t position     [in] the file position
 * @param int timeFormat     [in] the format of position (tTimeFormat)
 *
 * @returns int64_t -> the chunk index
 *
 */
int64_t IndexReadTable::lookupChunkRef( uint16_t stream_id, int64_t pos, TimeFormat time_format,
                                        int64_t* chunk_index, int64_t* chunk_offset,
                                        int64_t* end_chunk_index, int64_t* master_index) const
{
    uint64_t index = (uint64_t)pos;
    uint64_t ref_index = 0;
    ChunkRef *chunk_ref = nullptr;
    StreamRef *stream_ref = nullptr;
    const StreamIndexTable *stream_idx_tbl = nullptr;

    if (time_format == tf_chunk_time)
    {
        if (pos < (int64_t)_file_header->time_offset)
        {
            throw std::out_of_range("invalid position before time offset");
        }
    }
    else
    {
        if (pos < 0)
        {
            throw std::invalid_argument("invalid position argument");
        }
    }

    int64_t index_count = getItemCount(stream_id);
    int64_t index_master_count = getItemCount(0);
    if (index_count == -1 || index_master_count == -1)
    {
        throw std::runtime_error("invalid index position");
    }

    if (time_format == tf_chunk_time)
    {
        int64_t duration = _file_header->duration;
        int64_t position_off = pos - _file_header->time_offset;
        if (duration < 1)
        {
            throw std::runtime_error("invalid duration");
        }

        if (position_off == duration)
        {
            index = index_count - 1;
        }
        else
        {
            index = (position_off * index_count) / duration;
        }

        if (index < 0)
        {
            index = 0;
        }

        if (static_cast<int64_t>(index) >= index_count)
        {
            throw exceptions::EndOfFile();
        }

        if (stream_id == 0)
        {
            while (static_cast<int64_t>(index) < (index_count - 1) &&
                _master_index_table.master_chunk_ref_table[index].time_stamp < (uint64_t) pos)
            {
                index++;
            }

            while (index > 0 &&
                _master_index_table.master_chunk_ref_table[index].time_stamp >= (uint64_t) pos)
            {
                index--;
            }
        }
        else
        {
            ref_index = index;

            // fetch ref master table index
            index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                _master_index_table.index_table_offset;

            while (static_cast<int64_t>(ref_index) < (index_count - 1) &&
                   index < (uint64_t)(index_master_count - 1) &&
                   _master_index_table.master_chunk_ref_table[index].time_stamp < (uint64_t) pos)
            {
                ref_index++;
                index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;
            }

            while (ref_index > 0 &&
                   index > 0 &&
                   _master_index_table.master_chunk_ref_table[index].time_stamp > (uint64_t) pos)
            {
                ref_index--;
                index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;
            }
        }
    }
    else if (time_format == tf_chunk_index)
    {
        int64_t num_chunks = _file_header->chunk_count;
        if (num_chunks < 1)
        {
            throw std::runtime_error("file contains no chunks");
        }

        if (pos == num_chunks)
        {
            index = index_count - 1;
        }
        else
        {
            index = (pos * index_count) / num_chunks;
        }

        if (index < 0)
        {
            index = 0;
        }

        if (static_cast<int64_t>(index) >= index_count)
        {
            throw exceptions::EndOfFile();
        }

        //search in the masterindex
        if (stream_id == 0)
        {
            while (static_cast<int64_t>(index) < (index_count - 1) &&
                (_master_index_table.master_chunk_ref_table[index].chunk_index -
                 _master_index_table.index_offset) < (uint64_t) pos)
            {
                index++;
            }

            while (index > 0 &&
                (_master_index_table.master_chunk_ref_table[index].chunk_index -
                 _master_index_table.index_offset) > (uint64_t) pos)
            {
                index--;
            }
        }
        //search in the streamindex
        else
        {
            ref_index = index;

            index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;

            while (static_cast<int64_t>(ref_index) < (index_count - 1) &&
                   index < (uint64_t)(index_master_count - 1) &&
                   (_master_index_table.master_chunk_ref_table[index].chunk_index -
                    _master_index_table.index_offset) < (uint64_t) pos)
            {
                ref_index++;
                index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;
            }

            while (ref_index > 0 &&
                   index > 0 &&
                   (_master_index_table.master_chunk_ref_table[index].chunk_index -
                    _master_index_table.index_offset) > (uint64_t) pos)
            {
                ref_index--;
                index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;
            }
        }
    }
    else if (time_format == tf_stream_index)
    {
        if (stream_id == 0)
        {
            throw std::invalid_argument("stream based chunk lookup only valid for stream ids > 0");
        }
        ref_index = index;

        int64_t num_stream_chunks = _stream_index_tables[stream_id].stream_info_header->stream_index_count;
        if (num_stream_chunks < 1)
        {
            throw std::runtime_error("stream has no chunks");
        }

        if (pos >= num_stream_chunks)
        {
            throw std::out_of_range("stream has not enough chunks");
        }

        if (pos == num_stream_chunks - 1)
        {
            ref_index = index_count - 1;
        }
        else
        {
            ref_index = (pos * index_count) / num_stream_chunks;
        }

        if (ref_index < 0)
        {
            ref_index = 0;
        }

        if (static_cast<int64_t>(ref_index) >= index_count)
        {
            throw exceptions::EndOfFile();
        }

        index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;

        while (static_cast<int64_t>(ref_index) < (index_count - 1) &&
               index < (uint64_t)(index_master_count - 1))
        {
            chunk_ref = &_master_index_table.master_chunk_ref_table[index];
            stream_idx_tbl = &_stream_index_tables[chunk_ref->stream_id];

            if ((chunk_ref->stream_index - stream_idx_tbl->index_offset) > (uint64_t) pos)
            {
                break;
            }

            ref_index++;
            index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;
        }

        while (ref_index > 0 && index > 0)
        {
            chunk_ref = &_master_index_table.master_chunk_ref_table[index];
            stream_idx_tbl = &_stream_index_tables[chunk_ref->stream_id];

            if ((chunk_ref->stream_index - stream_idx_tbl->index_offset) < (uint64_t) pos)
            {
                break;
            }

            ref_index--;
            index = _stream_index_tables[stream_id].stream_ref_table[ref_index].ref_master_table_index -
                     _master_index_table.index_table_offset;
        }
    }

    ref_index = index;
    chunk_ref = &_master_index_table.master_chunk_ref_table[index];
    stream_idx_tbl = &_stream_index_tables[chunk_ref->stream_id];

    if (stream_id != 0)
    {
        ref_index = chunk_ref->ref_stream_table_index - stream_idx_tbl->index_table_offset;
    }

    uint64_t temp_chunk_index = chunk_ref->chunk_index - _master_index_table.index_offset;
    uint64_t stream_index = chunk_ref->stream_index - stream_idx_tbl->index_offset;
    uint64_t time_stamp = chunk_ref->time_stamp;

    if ((time_format == tf_chunk_index && (uint64_t)pos < temp_chunk_index) ||
        (time_format == tf_stream_index && (uint64_t)pos < stream_index) ||
        (time_format == tf_chunk_time && (uint64_t)pos < time_stamp))
    {
        *end_chunk_index = temp_chunk_index;
    }
    else
    {
        if ((int64_t)ref_index < getItemCount(stream_id) - 1)
        {
            if (stream_id != 0)
            {
                stream_ref = &_stream_index_tables[stream_id].stream_ref_table[ref_index + 1];
                chunk_ref = &_master_index_table.master_chunk_ref_table[stream_ref->ref_master_table_index - _master_index_table.index_table_offset];
            }
            else
            {
                chunk_ref = &_master_index_table.master_chunk_ref_table[ref_index + 1];
            }

            *end_chunk_index = (int64_t)chunk_ref->chunk_index - _master_index_table.index_offset + 1;
        }
        else
        {
            *end_chunk_index = (int64_t)_file_header->chunk_count;
        }
    }

    if (index > 0)
    {
        *chunk_index = _master_index_table.master_chunk_ref_table[index].chunk_index -
                       _master_index_table.index_offset;
        *chunk_offset = _master_index_table.master_chunk_ref_table[index].chunk_offset;
    }
    else
    {
        // in case of a history where the some samples are before the first index entry
        // move the search range start to the beginning, otherwise this does nothing
        *chunk_index = 0;
        *chunk_offset = _file_header->first_chunk_offset;
    }

    *master_index = index;
    return index;
}

bool IndexReadTable::findNearestEntryWithFlags(uint16_t stream_id,
                                                uint64_t chunk_index,
                                                uint16_t chunk_flags,
                                                uint64_t* master_index)
{
    if (stream_id == 0)
    {
        throw std::out_of_range("flag based index search only available for stream ids > 0");
    }

    // first perform a guess
    auto& stream_index_table = _stream_index_tables[stream_id];
    if (stream_index_table.index_count == 0)
    {
        return false;
    }

    auto start = stream_index_table.stream_ref_table;
    auto end = stream_index_table.stream_ref_table + stream_index_table.index_count;
    auto current = std::upper_bound(start, end, chunk_index, [&](const uint64_t& chunk_index, const StreamRef& entry)
    {
       return chunk_index < _master_index_table.master_chunk_ref_table[entry.ref_master_table_index].chunk_index;
    });

    --current;

    for (;current >= start; --current)
    {
        if ((_master_index_table.master_chunk_ref_table[current->ref_master_table_index].flags & chunk_flags) == chunk_flags)
        {
            *master_index = current->ref_master_table_index;
            return true;
        }
    }

    return false;
}

} // namespace v400
} // namespace ifhd

