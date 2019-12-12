/**
 * @file
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
#include <string.h>
#include <assert.h>

#define MAX_CACHE_WRITE_SIZE 512*1024
#define EXT_STORAGE_INFO "storage_info"

static uint8_t chunk_fill_buffer[16] = {0};  // chunks are filled up to 16 byte boundaries

namespace ifhd
{
namespace v110
{

#define DELEGATE_PTR(pObj) ((ifhd::v100::IndexedFileReaderV100*) (pObj))

IndexedFileReaderV110::IndexedFileReaderV110()
{
    initialize();
}

IndexedFileReaderV110::~IndexedFileReaderV110()
{
    close();
}

/**
*   Initializes the values
*
*   @returns Standard Result 
*/
void IndexedFileReaderV110::initialize()
{
    IndexedFileV110::initialize();

    _delegate                     = nullptr;

    _flags                     = 0;
    _write_mode                    = false;
    _end_of_data_marker              = 0;
    _index_table                  = nullptr;
    _index_table_size               = 0;
    _header_valid                  = false;
    _prefetched                   = false;
    _chunk_index                   = 0;
    _index_table_index              = 0;
    _current_chunk_data            = nullptr;
    _file_pos_invalid               = true;
    _compatibility_v100            = false;
    _current_chunk                = nullptr;

    _file                        = nullptr;    
}

//*************************************************************************************************
/**
*   attaches a dat-file for reading
*   
*   @param cacheSize  [in] cache size; <=0: use system file caching (=default)
*
*/
void IndexedFileReaderV110::attach(utils5ext::File* file,
                                        const a_util::filesystem::Path& file_name,
                                        int cache_size, 
                                        uint32_t flags)
{
    close();

    _file = file;

    _flags = flags;

    _system_cache_disabled = false;

    if ((flags & IndexedFileV110::om_disable_file_system_cache) != 0)
    {
        _system_cache_disabled = true;
    }
       
    allocReadBuffers();

    _sector_size = getSectorSize(file_name);
    if (_sector_size == 0)
    {
        _sector_size = default_block_size;
    }

    if (cache_size < 0)
    {
        cache_size = 16 * _sector_size; // 8 kilobytes default
    }
    
    _file->setReadCache(cache_size);

    readFileHeader();

    if (_delegate)
    {
        return;
    }

    /*
    if (!_systemCacheDisabled)
    {
        IFHD_RETURN_IF_FAILED(AllocCache(cacheSize));
    }
    */

   readFileHeaderExt();

    if ((flags & IndexedFileV110::om_query_info) == 0)
    {
        readIndexTable();
        allocBuffer((int) _file_header->max_chunk_size);

        _index_table                  = _index_blocks->data;
        _index_table_size               = _index_blocks->item_count;
        _current_chunk_data            = nullptr;
        _header_valid                  = false;

        reset();
        readCurrentChunkHeader();
    }
    else    // just open for header and extension info
    {
        _index_table                  = nullptr;
        _index_table_size               = 0;
        _current_chunk_data            = nullptr;
        _header_valid                  = false;
    }
}

//*************************************************************************************************
/**
*   Closes all
*
*   @returns Standard Result 
*/
void IndexedFileReaderV110::close()
{
    _compatibility_v100 = false;

    if (_delegate != nullptr)
    {
        delete (DELEGATE_PTR(_delegate));
        _delegate = nullptr;
    }

    freeReadBuffers();

    return IndexedFileV110::close();
}

//*************************************************************************************************
/**
*   Reads and initializes the Header struct
*
*   @returns Standard Result 
*/
void IndexedFileReaderV110::readFileHeader()
{
    allocHeader();
    clearCache();

    _file->setFilePos(0, utils5ext::File::fp_begin);

    _file->readAll(_file_header, sizeof(IndexedFileV110::FileHeader));
    if (_file_header->file_id != *(uint32_t*)("IFHD"))
    {
        throw std::runtime_error("unsupported file");
    }

    _compatibility_v100 = false;
    if (_file_header->version_id != version_id)
    {
        if (_file_header->version_id == v100::version_id)
        {
            _compatibility_v100 = true;

            v100::IndexedFileV100::FileHeader file_header_v100;
            a_util::memory::copy(&file_header_v100, sizeof(file_header_v100), _file_header, sizeof(file_header_v100));
            _file_header->extension_count  = (file_header_v100.extension_size != 0) ? 1 : 0;
            _file_header->reserved1          = 0;
            _file_header->chunk_count         = file_header_v100.index_count;
            _file_header->max_chunk_size       = 0;

            _delegate = new v100::IndexedFileReaderV100();

            DELEGATE_PTR(_delegate)->attach(&file_header_v100, _file);
        }
        else
        {
            throw std::runtime_error("unsupported file");
        }
    }

    _chunk_index                   = 0;
    _header_valid                  = false;
}

//*************************************************************************************************
/**
*   Reads and initializes the ExtendedHeader
*
*   @returns Standard Result 
*/
void IndexedFileReaderV110::readFileHeaderExt()
{

    // layout on disk: [...|ext1|ext2|...|extN]ext-table]...]

    freeExtensions();

    int num_extensions = (int)_file_header->extension_count;
    if (num_extensions == 0)
    {
        return;
    }

    clearCache();

    _file->setFilePos(_file_header->extension_offset, utils5ext::File::fp_begin);

    void* extension_table_page_temp = internalMalloc(sizeof(FileExtension) * num_extensions, true);
    FileExtension* extension_tab = (FileExtension*)extension_table_page_temp;

    try
    {
        _file->readAll(extension_tab, sizeof(FileExtension) * num_extensions);
    }
    catch(...)
    {
        internalFree(extension_table_page_temp);
        throw;
    }

    FileExtension* extension_info = extension_tab;
    void* extension_data_page;
    for (int extension_idx = 0; extension_idx < num_extensions; extension_idx++)
    {
        try
        {
            _file->setFilePos(extension_info->data_pos, utils5ext::File::fp_begin);
        }
        catch (...)
        {
            internalFree(extension_table_page_temp);
            throw;
        }

        allocExtensionPage(extension_info->data_size, &extension_data_page);
        FileExtensionStruct* extension_struct = nullptr;
        try
        {
            extension_struct = new FileExtensionStruct;
        }
        catch (...)
        {
            internalFree(extension_data_page);
            internalFree(extension_table_page_temp);
            throw;
        }

        a_util::memory::copy(&extension_struct->file_extension, sizeof(FileExtension), extension_info, sizeof(FileExtension));

        try
        {
            _file->readAll(extension_data_page, (int)extension_info->data_size);
        }
        catch(...)
        {
            delete extension_struct;
            internalFree(extension_data_page);
            internalFree(extension_table_page_temp);
            throw;
        }

        extension_struct->extension_page = extension_data_page;

        extension_info++;

        _extensions.push_back(extension_struct);
    }

    internalFree(extension_table_page_temp);
}

//*************************************************************************************************
/**
*   Initializes the IndexTable
*
*   @returns Standard Result
*/
void IndexedFileReaderV110::readIndexTable()
{
    allocIndexBlock((int)_file_header->index_count);

    _index_table = _active_index_block->data;
    _index_table_size = _active_index_block->data_size;

    clearCache();

    _file->setFilePos(_file_header->index_offset, utils5ext::File::fp_begin);
    _file->readAll(_index_table, sizeof(ChunkRef) * _index_table_size);

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
void IndexedFileReaderV110::reset()
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->reset();
    }

    clearCache();

    _file_pos                      = _file_header->data_offset;
    _file_pos_invalid               = true;

    _chunk_index                   = 0;
    _index_table_size               = 0;
    _current_chunk_data            = nullptr;
    _header_valid                  = false;
}


//*************************************************************************************************
/**
*   Returns the current file position (Index or TimeStamp)
*
*   @param   timeFormat    [in] Format of position (TF_ChunkTime / TF_ChunkIndex); 
*
*   @returns Current Position
*/
int64_t IndexedFileReaderV110::getCurrentPos(int time_format) const
{                                                               
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getCurrentPos(time_format);
    }

    if (_chunk_index < 0)
    {
        return -1;
    }

    if (time_format == tf_chunk_time)
    {
        return _current_chunk->time_stamp;
    }

    
    
    return _chunk_index;
}

int64_t IndexedFileReaderV110::lookupChunkRef(int64_t position, int time_format)
{
    if (nullptr != _delegate)
    {
        return 0;
    }

    if (position < 0)
    {
        return -1;  // invalid argument
    }

    uint64_t index = 0;//(uint64_t) position;

    if (time_format == tf_chunk_index || time_format == 0)
    {
        uint64_t index_count = _file_header->index_count;
        int64_t chunk_count = _file_header->chunk_count;
        if (chunk_count < 1)
        {
            throw std::out_of_range("file contains no chunks");
        }
        if (position == chunk_count)
        {
            index = _file_header->index_count - 1;
        }
        else
        {
            index = (position * _file_header->index_count) / chunk_count;
        }
        if (index < 0)
        {
            index = 0;
        }
        
        if (index >= index_count)
        {
            return -1;  // eof
        }

        while (index < (index_count - 1) && _index_table[index].chunk_index < (uint64_t) position)
        {
            index++;
        }

        while (index > 0 && _index_table[index].chunk_index > (uint64_t) position)
        {
            index--;
        }
    }
    else if (time_format == tf_chunk_time)
    {
        uint64_t index_count = _file_header->index_count;
        int64_t duration    = _file_header->duration;
        if (duration < 1)
        {
            throw std::out_of_range("file contains no chunks");
        }

        if (position == duration)
        {
            index = _file_header->index_count - 1;
        }
        else
        {
            index = (position * _file_header->index_count) / duration;
        }

        if (index < 0)
        {
            index = 0;
        }
        
        if (index >= index_count)
        {
            throw std::out_of_range("file contains not enough chunks");
        }

        while (index < (index_count - 1) && _index_table[index].time_stamp < (uint64_t) position)
        {
            index++;
        }

        while (index > 0 && _index_table[index].time_stamp > (uint64_t) position)
        {
            index--;
        }
    }

    return (int64_t) index;
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
int64_t IndexedFileReaderV110::setCurrentPos(int64_t position, int time_format)
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->setCurrentPos(position, time_format);
    }

    return seek(position, time_format);
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
int64_t IndexedFileReaderV110::seek(int64_t position, int time_format, uint32_t flags)
try
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->seek(position, time_format, flags);
    }

    _current_chunk_data            = nullptr;
    _header_valid                  = false;
    _prefetched                   = false;

    clearCache();

    int64_t index = lookupChunkRef(position, time_format);

    ChunkRef* ref = &_index_table[index];
    _chunk_index = ref->chunk_index;

    // just seek in index table
    if ((flags & sf_keydata) != 0)
    {
        _current_chunk->ref_index   = index;
        _current_chunk->time_stamp  = ref->time_stamp;
        _current_chunk->size       = ref->size;
        _current_chunk->flags      = ref->flags;

        _file_pos                  = ref->chunk_offset;
        _file_pos_invalid           = true; // need to call SetFilePos
        return index;
    }

    int64_t pos = ref->chunk_offset;
    if (pos + ref->size > _end_of_data_marker)
    {
        throw std::runtime_error("inconsistent file");
    }

    _file_pos = pos;
    _file_pos_invalid = true;
    checkFilePtr();

    uint64_t seek_end_index;
    if (index < (int64_t) _file_header->index_count - 1)
    {
        seek_end_index = _index_table[index+1].chunk_index + 1;
    }
    else
    {
        seek_end_index = _file_header->chunk_count;
    }

    // seek in range between two border elements in index table
    int64_t current_index = _chunk_index;
    while (true)
    {
        if (current_index == (int64_t)seek_end_index)
        {
            throw std::runtime_error("position not found");
        }

        readCurrentChunkHeader();
        readCurrentChunkData();

        if (time_format == tf_chunk_index)
        {
            if (current_index == position)
            {
                break;
            }
        }
        else if (time_format == tf_chunk_time)
        {
            if (_current_chunk->time_stamp >= (uint64_t) position)
            {
                break;
            }
        }

        current_index++;
    }

    _prefetched    = true;
    _chunk_index    = (uint64_t) current_index;

    return current_index;
}
catch(...)
{
    setEOF();
    throw;
}

//*************************************************************************************************
/**
*   Returns the duration of the current file [microsec]
*
*   @returns File duration
*/
timestamp_t IndexedFileReaderV110::getDuration() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getDuration();
    }

    return _file_header->duration;
}

//*************************************************************************************************
/**
*   Returns the current index of the IndexTable
*
*   @returns Index
*/
int64_t IndexedFileReaderV110::getFilePos() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getFilePos();
    }

    return _chunk_index;
}

//*************************************************************************************************
/**
*   Returns the number of chunks of the current file
*
*   @returns Number of chunks
*/
int64_t IndexedFileReaderV110::getChunkCount() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getChunkCount();
    }

    return _file_header->chunk_count;
}

int64_t IndexedFileReaderV110::getIndexCount() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getIndexCount();
    }

    return _file_header->index_count;
}

//*************************************************************************************************
/**
*   Returns the Version ID of the current file
*
*   @returns Version ID
*/
uint32_t IndexedFileReaderV110::getVersionId() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getVersionId();
    }

    return _file_header->version_id;
}

void IndexedFileReaderV110::checkFilePtr()
{
    if (_file_pos < 0 || _chunk_index < 0)
    {
        throw std::runtime_error("invalid file position");
    }

    IFHD_ASSERT((_file_pos & 0xF) == 0);

    if (_file_pos_invalid == true)
    {
        _file->setFilePos(_file_pos, utils5ext::File::fp_begin);
        _file_pos_invalid = false;
    }
}

void IndexedFileReaderV110::readCurrentChunkHeader()
try
{
    if (_chunk_index < 0 || (uint64_t) _chunk_index >= _file_header->chunk_count)
    {
        throw exceptions::EndOfFile();
    }

    _current_chunk_data = nullptr;
    
    if (static_cast<int64_t>(_file_pos + sizeof(ChunkHeader)) > _end_of_data_marker)
    {
        throw exceptions::EndOfFile();
    }

    checkFilePtr();

    readDataBlock(_current_chunk, sizeof(ChunkHeader));

    _file_pos    += sizeof(ChunkHeader);
    _header_valid = true;
}
catch (...)
{
    setEOF();
    throw;
}

void IndexedFileReaderV110::readCurrentChunkData()
try
{
    _current_chunk_data = nullptr;

    if (_chunk_index < 0 || (uint64_t) _chunk_index >= _file_header->chunk_count)
    {
        throw exceptions::EndOfFile();
    }

    uint32_t data_size = _current_chunk->size - sizeof(ChunkHeader);

    if (_file_pos + data_size > _end_of_data_marker)
    {
        throw exceptions::EndOfFile();    // EOF if chunk size exceeds data region
    }

    checkFilePtr();

    readDataBlock(_buffer, data_size);

    _file_pos += data_size;

    if ((data_size & 0xF) != 0)
    {
        int skip_bytes = 16-(data_size & 0xF);
        if (_cache_size > 0)
        {
            readDataBlock(chunk_fill_buffer, skip_bytes);
        }
        else
        {
            _file->skip(skip_bytes);
        }

        _file_pos += skip_bytes;
    }

    IFHD_ASSERT((_file_pos & 0xF) == 0);

    _current_chunk_data = _buffer;
}
catch (...)
{
    setEOF();
    throw;
}

//*************************************************************************************************
/**
*   Returns the current ChunkInfo from IndexTable
*
*   @param  chunk [out] Current chunk info struct 
*
*   @returns Standard result code
*/
void IndexedFileReaderV110::queryChunkInfo(ChunkHeader** chunk_header)
{
    *chunk_header = nullptr;

    if (nullptr != _delegate)
    {
        v100::IndexedFileV100::ChunkHeader* chunk_header_v100 = nullptr;

        DELEGATE_PTR(_delegate)->queryChunkInfo(&chunk_header_v100);

        _current_chunk->time_stamp  = chunk_header_v100->time_stamp;
        _current_chunk->size       = chunk_header_v100->size + (sizeof(IndexedFileV110::ChunkHeader) - sizeof(v100::IndexedFileV100::ChunkHeader));;
        _current_chunk->flags      = chunk_header_v100->flags;
        _current_chunk->ref_index   = DELEGATE_PTR(_delegate)->internalGetIndex();

        *chunk_header = _current_chunk;
    }

    if (!_header_valid)
    {
        readCurrentChunkHeader();
    }

    *chunk_header = _current_chunk;
}

//*************************************************************************************************
/**
*   Reads and returns the next Chunk and increments the IndexTable index
*
*   @param  data [out] Current chunk data
*
*   @returns Standard result code
*/
void IndexedFileReaderV110::readChunk(void** data)
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->readChunk(data);
    }

    *data = nullptr;

    if (!_header_valid)
    {
        readCurrentChunkHeader();
    }

    if (!_prefetched)
    {
        readCurrentChunkData();
    }
    else
    {
        // reset prefetch flag
        _prefetched = false;
    }

    _header_valid = false;

    *data = _buffer;

    _chunk_index++;
}

//*************************************************************************************************
/**
*   Skips the next Chunk by incrementing the IndexTable index and setting the new FilePosition
*
*   @param  data [out] Current chunk data
*
*   @returns Standard result code
*/
void IndexedFileReaderV110::skipChunk()
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->skipChunk();
    }

    void* unused_data;
    return readChunk(&unused_data);
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
void IndexedFileReaderV110::readNextChunk(ChunkHeader** chunk_header, void** data)
{
    if (nullptr != _delegate)
    {
        v100::IndexedFileV100::ChunkHeader* chunk_header_v100 = nullptr;

        DELEGATE_PTR(_delegate)->readNextChunk(&chunk_header_v100, data);

        _current_chunk->time_stamp  = chunk_header_v100->time_stamp;
        _current_chunk->size       = chunk_header_v100->size + (sizeof(IndexedFileV110::ChunkHeader) - sizeof(v100::IndexedFileV100::ChunkHeader));
        _current_chunk->flags      = chunk_header_v100->flags;
        _current_chunk->ref_index   = DELEGATE_PTR(_delegate)->internalGetIndex();

        *chunk_header = _current_chunk;
    }

    queryChunkInfo(chunk_header);

    readChunk(data);
}

void IndexedFileReaderV110::skipChunkInfo()
{    
    _index_table_index++;
    setCurrentPos(_index_table_index, IndexedFileV110::tf_chunk_index);
}
void IndexedFileReaderV110::readNextChunkInfo(ChunkHeader** chunk_header)
{
    skipChunkInfo();
    queryChunkInfo(chunk_header);
}



//*********************************************************************************
void IndexedFileReaderV110::readDataBlock(void* buffer, long buffer_size)
{
    uint8_t* dest       = (uint8_t*) buffer;
    long read_size      = buffer_size;
    uint8_t* cache      = (uint8_t*) getCacheAddr();

    IFHD_ASSERT(_file_pos_invalid == false);

    if (_cache_size > 0)
    {
        if (read_size <= _cache_size)
        {
            if (read_size > _cache_usage)  // data has to be read from disk
            {
                if (_cache_usage > 0)
                {
                    a_util::memory::copy(dest, _cache_usage, cache + _cache_offset, _cache_usage);
                    dest     += _cache_usage;
                    read_size -= _cache_usage;
                }

                // refresh cache
                _file->readAll(cache, _cache_size);

                _cache_offset = 0;
                _cache_usage  = _cache_size;
            }

            // now there's enough data in the cache
            a_util::memory::copy(dest, read_size, cache + _cache_offset, read_size);
            _cache_usage -= read_size;
            _cache_offset += read_size;
        }
        else // requested block is larger than cache - directly read from disk
        {
            if (_cache_usage > 0)  // don't forget remaining cached bytes
            {
                a_util::memory::copy(dest, _cache_usage, cache + _cache_offset, _cache_usage);
                dest += _cache_usage;
                read_size -= _cache_usage;
                _cache_offset = 0;
                _cache_usage  = 0;
            }

            _file->readAll(dest, read_size);
        }
    }
    else // don't use any cache
    {
        _file->readAll(buffer, buffer_size);
    }    
}

void IndexedFileReaderV110::clearCache()
{
    _cache_offset = 0;
    _cache_usage  = 0;
}

void IndexedFileReaderV110::setEOF()
{
    _chunk_index           = -1;
    _current_chunk_data    = nullptr;
    _header_valid          = false;
    _file_pos              = -1;
    _file_pos_invalid       = true;
}

bool IndexedFileReaderV110::findExtension(const std::string& identifier, FileExtension** extension_info, void** data) const
{
    if (nullptr != _delegate)
    {
        if (identifier == (EXT_STORAGE_INFO))
        {
            return false;
        }

        getExtension(0, extension_info, data);
    }

    return IndexedFileV110::findExtension(identifier, extension_info, data);
}

void IndexedFileReaderV110::getExtension(int index, FileExtension** extension_info, void** data) const
{
    if (nullptr != _delegate)
    {
        if (index < 0 || index >= (int) _file_header->extension_count)
        {
            throw std::out_of_range("invalid extension index");
        }

        int  header_extension;
        DELEGATE_PTR(_delegate)->getHeaderExtension(data, &header_extension);
        if (extension_info != nullptr)
        {
            v100::IndexedFileV100::FileHeader* file_header_v100 = DELEGATE_PTR(_delegate)->internalGetFileHeader();

            a_util::memory::set(&_extension_info_v100, sizeof(_extension_info_v100), 0, sizeof(_extension_info_v100));
            strncpy((char*) _extension_info_v100.identifier, EXT_STORAGE_INFO, 32);
            _extension_info_v100.identifier[31] = 0;
            _extension_info_v100.data_pos       = file_header_v100->extension_offset;
            _extension_info_v100.data_size      = header_extension;
            _extension_info_v100.type_id     = 0x0;
            _extension_info_v100.version_id  = 0x0;

            *extension_info = &_extension_info_v100;
        }
    }

    IndexedFileV110::getExtension(index, extension_info, data);
}

void IndexedFileReaderV110::allocReadBuffers()
{
    _current_chunk = (ChunkHeader*) utils5ext::allocPageAlignedMemory(sizeof(ChunkHeader), utils5ext::getDefaultSectorSize());

    utils5ext::memZero(_current_chunk, sizeof(ChunkHeader));
}

void IndexedFileReaderV110::freeReadBuffers()
{
    if (_current_chunk != nullptr)
    {
        utils5ext::freePageAlignedMemory(_current_chunk);

        _current_chunk = nullptr;
    }
}

int64_t IndexedFileReaderV110::getChunkIndexForIndexPos() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->internalGetIndex();
    }
    if (_current_chunk != nullptr)
    {   
        return getChunkIndexForIndexPos(_current_chunk->ref_index);
    }
    return -1;
}

int64_t IndexedFileReaderV110::getChunkIndexForIndexPos(int64_t ref_idx) const
{
    if (nullptr != _delegate)
    {
        return -1;
    }
    if (ref_idx >= 0 && ref_idx < int64_t(_file_header->index_count))
    {
        return _index_table[ref_idx].chunk_index;
    } 
    return -1; 
}

}
} // namespace ifhd
