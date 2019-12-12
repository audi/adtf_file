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

#include <ifhd/ifhd.h>

#define MAX_CACHE_WRITE_SIZE 65536 * 4


namespace ifhd
{
namespace v100
{

IndexedFileReaderV100::IndexedFileReaderV100()
{
    initialize();
}

IndexedFileReaderV100::~IndexedFileReaderV100()
{
    close();
}

/**
*   Initializes the values
*
*   @returns Standard Result
*/
void IndexedFileReaderV100::initialize()
{
    IndexedFileV100::initialize();

    _flags                     = 0;

    _write_mode                    = false;
    _end_of_data_marker              = 0;
    _index_table                  = nullptr;
    _index_table_size               = 0;
}

void IndexedFileReaderV100::attach(FileHeader* file_header, utils5ext::File* file, uint32_t flags)
{
    close();

    _file = file;

    _system_cache_disabled = ((flags & flag_disable_file_system_cache) != 0);

    _flags = flags;

    allocHeader();
    a_util::memory::copy(_file_header, sizeof(FileHeader), file_header, sizeof(FileHeader));

    readFileHeaderExt();
    readIndexTable();

    _index_table      = _index_blocks->data;
    _index_table_size   = _index_blocks->item_count;

    _attached = true;
}

void IndexedFileReaderV100::detach()
{
    if (_attached)
    {
        freeHeader();
        _attached = false;
    }

    IndexedFileV100::close();
}

//*************************************************************************************************
/**
*   Reads and initializes the Header struct
*
*   @returns Standard Result
*/
void IndexedFileReaderV100::readFileHeader()
{
    allocHeader();

    _file->setFilePos(0, utils5ext::File::fp_begin);
    _file->readAll(_file_header, sizeof(FileHeader));

    if (_file_header->file_id != *(uint32_t*)("IFHD"))
    {
        throw std::runtime_error("unsupported file");
    }

    if (_file_header->version_id > version_id)
    {
        throw std::runtime_error("unsupported file");
    }

    _index = 0;
}

//*************************************************************************************************
/**
*   Reads and initializes the ExtendedHeader
*
*   @returns Standard Result
*/
void IndexedFileReaderV100::readFileHeaderExt()
{
    if (_file_header->extension_size > 0)
    {
        allocHeaderExtension((int)_file_header->extension_size);
        _file->setFilePos(_file_header->extension_offset, utils5ext::File::fp_begin);
        _file->readAll(_header_extension, (long)_file_header->extension_size);
    }
}

//*************************************************************************************************
/**
*   Initializes the IndexTable
*
*   @returns Standard Result
*/
void IndexedFileReaderV100::readIndexTable()
{
    allocIndexBlock((int)_file_header->index_count);

    _index_table = _active_index_block->data;
    _index_table_size = _active_index_block->data_size;

    _file->setFilePos(_file_header->index_offset, utils5ext::File::fp_begin);
    _file->readAll(_index_table, sizeof(ChunkHeader) * _index_table_size);

    if (_index_table_size > 0)
    {
        _end_of_data_marker = _file_header->data_offset + _file_header->data_size;
    }
    else
    {
        _end_of_data_marker = 0;
    }

    reset();
}

//*************************************************************************************************
/**
*   Resets the file to the beginning of data
*
*   @returns Standard Result
*/
void IndexedFileReaderV100::reset()
{
    _file->setFilePos(_file_header->data_offset, utils5ext::File::fp_begin);
    _file_pos = _file_header->data_offset;
    _index   = 0;
}

//*************************************************************************************************
/**
*   Sets the file position to the Chunk of index and returns the respective ChunkInfo
*
*   @param   index         [in] New file position
*   @param   chunk        [out] Returns the respective ChunkInfo
*
*   @returns Standard Result
*/
int64_t IndexedFileReaderV100::seek(int64_t position, int time_format, uint32_t flags)
{
    int64_t index = (int64_t) position;

    if (time_format == tf_chunk_time)
    {
        uint64_t index_count = _file_header->index_count;
        int64_t duration    = _file_header->duration;
        if (duration < 1)
        {
            throw std::runtime_error("invalid file duration");
        }

        if (position == duration)
        {
            index = _file_header->index_count - 1;
            if (index < 0)
            {
                throw std::runtime_error("file contains no chunks");
            }
        }
        else
        {
            index = (position * _file_header->index_count) / duration;
            if (index < 0)
            {
                index = 0;
            }

            if (index >= (int64_t) index_count)
            {
                throw std::runtime_error("file contains not enough chunks");
            }
        }

        while (index > 0 && _index_table[index].time_stamp > (uint64_t) position)
        {
            index--;
        }

        while (index < (int64_t) (index_count - 1) && _index_table[index].time_stamp < (uint64_t) position)
        {
            index++;
        }
    }

    if (index < 0 || (uint64_t) index >= _file_header->index_count)
    {
        _index = -1;
        throw std::out_of_range("invalid index");
    }

    _index = (uint64_t) index;
    if (flags != 0)
    {
        int64_t flags_index = seekFlags(flags, nullptr);
        if (flags_index >= 0)
        {
            index = flags_index;
        }
    }

    int64_t pos = _index_table[index].chunk_offset;
    if (pos + _index_table[index].size > _end_of_data_marker)
    {
        _index = -1;
        throw std::out_of_range("end of file");
    }

    _file_pos = pos;
    try
    {
        _file->setFilePos(_file_pos, utils5ext::File::fp_begin);
    }
    catch(...)
    {
        _index = -1;
        throw;
    }

    _index = (uint64_t) index;
    return _index;
}

//*************************************************************************************************
/**
*   Increments the file index until seeked chunk flags found
*
*   @param   flags         [in] Flags seeked for
*   @param   chunk        [out] Returns the ChunkInfo of the next Chunk with the flags
*
*   @returns Current Index
*/
int64_t IndexedFileReaderV100::seekFlags(uint32_t flags, ChunkHeader** chunk)
{
    if (chunk != nullptr)
    {
        *chunk = nullptr;
    }

    if (_index < 0)
    {
        return -1;
    }

    int64_t index = _index;
    ChunkHeader* temp_chunk = &_index_table[index];
    while ((uint64_t) index < _file_header->index_count)
    {
        if ((temp_chunk->flags & flags) != 0)
        {
            if (chunk != nullptr)
            {
                *chunk = temp_chunk;
            }

            return index;
        }

        chunk++;
        index++;
    }

    return -1;
}

//*************************************************************************************************
/**
*   Returns the current file position (Index or TimeStamp)
*
*   @param   timeFormat    [in] Format of position (TF_ChunkTime / TF_ChunkIndex);
*
*   @returns Current Position
*/
int64_t IndexedFileReaderV100::getCurrentPos(int time_format) const
{

    if (_index < 0 || (uint64_t) _index >= _file_header->index_count)
    {
        return -1;
    }

    if (time_format == tf_chunk_time)
    {
        return _index_table[_index].time_stamp;
    }

    return _index;
}
//*************************************************************************************************
/**
*   Sets the file position by Index or by TimeStamp
*
*   @param   position      [in] New file position
*   @param   timeFormat    [in] Format of position (TF_ChunkTime / TF_ChunkIndex);
*
*   @returns New Index
*/
int64_t IndexedFileReaderV100::setCurrentPos(int64_t position, int time_format)
{
    return seek(position, time_format);
}

//*************************************************************************************************
/**
*   Returns the duration of the current file [microsec]
*
*   @returns File duration
*/
timestamp_t IndexedFileReaderV100::getDuration() const
{
    return _file_header->duration;
}

//*************************************************************************************************
/**
*   Returns the current index of the IndexTable
*
*   @returns Index
*/
int64_t IndexedFileReaderV100::getFilePos() const
{
    return _index;
}

//*************************************************************************************************
/**
*   Returns the number of chunks of the current file
*
*   @returns Number of chunks
*/
int64_t IndexedFileReaderV100::getChunkCount() const
{
    return _file_header->index_count;
}

int64_t IndexedFileReaderV100::getIndexCount() const
{
    return _file_header->index_count;
}

//*************************************************************************************************
/**
*   Returns the Version ID of the current file
*
*   @returns Version ID
*/
uint32_t IndexedFileReaderV100::getVersionId() const
{
    return _file_header->version_id;
}

//*************************************************************************************************
/**
*   Returns the current ChunkInfo from IndexTable
*
*   @param  chunk [out] Current chunk info struct
*
*   @returns Standard result code
*/
void IndexedFileReaderV100::queryChunkInfo(ChunkHeader** chunk)
{
    if (_index < 0 || (uint64_t) _index >= _file_header->index_count)
    {
        _index = -1;
        throw exceptions::EndOfFile();
    }

    *chunk = &_index_table[_index];
}
//*************************************************************************************************
/**
*   Reads and returns the next Chunk and increments the IndexTable index
*
*   @param  data [out] Current chunk data
*
*   @returns Standard result code
*/
void IndexedFileReaderV100::readChunk(void** data)
{
    *data = nullptr;

    if (_index < 0 || (uint64_t) _index >= _file_header->index_count)
    {
        _index = -1;
        throw exceptions::EndOfFile();
    }

    ChunkHeader* current_chunk = &_index_table[_index];

    if (_file_pos + current_chunk->size > _end_of_data_marker)
    {
        _index = -1;
        throw exceptions::EndOfFile();    // EOF if chunk size exceeds data region
    }

    allocBuffer(current_chunk->size);

    try
    {
        _file->readAll(_buffer, current_chunk->size);
    }
    catch(...)
    {
        _index = -1;
        throw;
    }

    _file_pos += current_chunk->size;

    _index++;

    *data = _buffer;
}

//*************************************************************************************************
/**
*   Skips the next Chunk by incrementing the IndexTable index and setting the new FilePosition
*
*   @param  data [out] Current chunk data
*
*   @returns Standard result code
*/
void IndexedFileReaderV100::skipChunk()
{
    if (_index < 0 || (uint64_t) _index >= _file_header->index_count)
    {
        _index = -1;
        throw exceptions::EndOfFile();
    }

    ChunkHeader* current_chunk = &_index_table[_index];

    if (_file_pos + current_chunk->size > _end_of_data_marker)
    {
        _index = -1;
        throw exceptions::EndOfFile();    // EOF if chunk size exceeds data region
    }

    try
    {
        _file->setFilePos(current_chunk->chunk_offset + current_chunk->size, utils5ext::File::fp_begin);
    }
    catch(...)
    {
        _index = -1;
        throw;
    }

    _file_pos += current_chunk->size;
    _index++;
}

//*************************************************************************************************
/**
*   Reads and returns the next Chunk and ChunkInfo struct and increments the IndexTable index
*
*   @param  chunkInfo [out] Current chunk info
*   @param  data [out] Current chunk data
*
*   @returns Standard result code
*/
void IndexedFileReaderV100::readNextChunk(ChunkHeader** chunk_info, void** data)
{
    queryChunkInfo(chunk_info);
    return readChunk(data);
}

int64_t IndexedFileReaderV100::internalGetIndex() const
{
    return _index;
}

IndexedFileV100::FileHeader* IndexedFileReaderV100::internalGetFileHeader() const
{
    return _file_header;
}

} //namespace v100
} // namespace
