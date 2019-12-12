/**
 * @file
 * Indexed file compatibility code.
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

namespace ifhd
{
namespace v100
{



/// Default Blocksize
int IndexedFileV100::_default_block_size = 512;
/// Default Cache Size
int IndexedFileV100::_default_cache_size = 1024 * 1024 * 8;   // x MBytes
/// Index Table Cluster Size
int IndexedFileV100::_index_table_cluster_size = 4096;         // always alloc N indices at once

IndexedFileV100::IndexedFileV100()
{
    initialize();
}

IndexedFileV100::~IndexedFileV100()
{
}

void IndexedFileV100::initialize()
{
    _attached                     = false;

    _file                        = nullptr;

    _sector_size                   = _default_block_size;

    _cache                        = nullptr;
    _cache_size                    = 0;

    _buffer                       = nullptr;
    _buffer_size                   = 0;
    _file_pos                      = 0;

    _index_blocks                  = nullptr;
    _active_index_block               = nullptr;

    _index                        = 0;

    _file_header                  = nullptr;
    _header_extension              = nullptr;

    _system_cache_disabled          = false;
}

//*************************************************************************************************
void IndexedFileV100::close()
{
    freeHeaderExtension();
    freeBuffer();
    freeIndexTable();
    freeHeader();

    _file_pos          = 0;

    freeCache();

    if (_file != nullptr)
    {
        _file->close();
        _file = nullptr;
    }
}

//*************************************************************************************************
void IndexedFileV100::allocHeader()
{
    freeHeader();

    _file_header = (FileHeader*) internalMalloc(sizeof(FileHeader), true);
}


//*************************************************************************************************
void IndexedFileV100::allocHeaderExtension(int size)
{
    if (size == 0)
    {
        freeHeaderExtension();
    }

    if (size == _file_header->extension_size && _header_extension != nullptr)
    {
        return;
    }
 
    _header_extension = internalMalloc(size, true);
    a_util::memory::set(_header_extension, size, 0, size);
    _file_header->extension_size = size;
}

//*************************************************************************************************
void IndexedFileV100::freeHeader()
{
    if (_file_header != nullptr)
    {
        internalFree(_file_header);
        _file_header = nullptr;
    }
}

//*************************************************************************************************
void IndexedFileV100::freeHeaderExtension()
{
    if (_header_extension != nullptr)
    {
        internalFree(_header_extension);
        _header_extension = nullptr;
    }

    if (_file_header != nullptr)
    {
        _file_header->extension_size = 0;
    }
}

//*************************************************************************************************
void IndexedFileV100::allocBuffer(int size)
{
    if (size == 0)
    {
        freeBuffer();
        return;
    }

    if (size <= _buffer_size && _buffer != nullptr)
    {
        return;
    }

    freeBuffer();

    _buffer = (uint8_t*) internalMalloc(size, false);
    _buffer_size = size;

}

//*************************************************************************************************
void IndexedFileV100::freeBuffer()
{
    if (_buffer != nullptr)
    {
        internalFree(_buffer);
        _buffer = nullptr;
    }

    _buffer_size = 0;
}

//*************************************************************************************************
void IndexedFileV100::allocIndexTable(int size)
{ 
    size; // currently unused
}

//*************************************************************************************************
void IndexedFileV100::allocIndexBlock(int count)
{
    if (count == 0)
    {
        count = _sector_size;    // to make sure we're working according to the sector size
    }
    else if (count < 0)
    {
        count = -count * _sector_size;
    }

    IndexBlockItem* new_item = new IndexBlockItem;
    try
    {
        new_item->data = new ChunkHeader[count];
    }
    catch (...)
    {
        delete new_item;
        throw;
    }

    new_item->item_count = 0;
    new_item->data_size  = count;
    new_item->next      = nullptr;


    if (_index_blocks == nullptr)
    {
        _index_blocks = new_item;
    }

    if (_active_index_block != nullptr)
    {
        _active_index_block->next = new_item;
    }

    _active_index_block = new_item;
}

//*************************************************************************************************
void IndexedFileV100::freeIndexTable()
{
    if (_index_blocks != nullptr)
    {
        IndexBlockItem* item = _index_blocks;
        IndexBlockItem* next_item = nullptr;
        while (item != nullptr)
        {
            if (item->data != nullptr)
            {
                // InternalFree(item->data);
                delete [] item->data;
            }

            next_item = item->next;
            // InternalFree(item);
            delete item;
            item = next_item;
        }

        _index_blocks = nullptr;
    }

    _active_index_block   = nullptr;

    if (_file_header != nullptr)
    {
        _file_header->index_count     = 0;
    }
}

//*************************************************************************************************
void IndexedFileV100::setDescription(const std::string& description)
{
    utils5ext::memZero(_file_header->description, sizeof(_file_header->description));
    strncpy((char*) _file_header->description, description.c_str(), 1912);
    _file_header->description[1912-1] = 0;
}

//*************************************************************************************************
std::string IndexedFileV100::getDescription() const
{
    return (const char*) _file_header->description;
}

//*************************************************************************************************
void IndexedFileV100::setDateTime(const ifhd::v100::IndexedFileV100::DateTime& date_time)
{
    a_util::memory::copy(&_file_header->date_time, sizeof(DateTime), &date_time, sizeof(DateTime));
}

//*************************************************************************************************
IndexedFileV100::DateTime IndexedFileV100::getDateTime()
{
    return _file_header->date_time;
}

//*************************************************************************************************
void IndexedFileV100::getHeaderExtension(void** data, int* data_size)
{
    *data     = _header_extension;
    *data_size  = (int) _file_header->extension_size;
}

//*************************************************************************************************
void IndexedFileV100::setHeaderExtension(void* data, int data_size)
{
    if (!_write_mode)
    {
        throw std::runtime_error("not in write mode");
    }

    allocHeaderExtension(data_size);
    a_util::memory::copy(_header_extension, data_size, data, data_size);
}

void IndexedFileV100::allocCache(int size)
{
    freeCache();

    _cache = internalMalloc(size, true);
    _cache_size    = size;
}

void IndexedFileV100::freeCache()
{
    if (_cache != nullptr)
    {
        internalFree(_cache);
        _cache        = nullptr;
    }

    _cache_size = 0;
}

void*  IndexedFileV100::getCacheAddr()
{
    return _cache;
}

int IndexedFileV100::getSectorSize(const std::string& filename) const
{
    return utils5ext::getSectorSizeFor(filename);
}

void* IndexedFileV100::internalMalloc(int size, bool use_segment_size)
{
    #ifdef WIN32
        if (!_system_cache_disabled)
        {
            return (void*) (new uint8_t[size]);
        }
    
        if (use_segment_size)
        {
            size = (size + _sector_size - 1) & ~(_sector_size - 1);
        }
    
        void* memory = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE | PAGE_NOCACHE);
        if (memory == nullptr)
        {
            return nullptr;
        }
    
        void* alignedAddress = (void*) ((uint32_t) ((uint8_t*) memory + _sector_size - 1) & ~(_sector_size - 1));
    
        return alignedAddress;
    #else
        return (void*) (new uint8_t[size]);
    #endif
}

void IndexedFileV100::internalFree(void* memory)
{
    #ifdef WIN32
        if (!_system_cache_disabled)
        {
            delete [] ((uint8_t*) memory);
            return;
        }
    
        VirtualFree(memory, 0, MEM_RELEASE);
    #else
        delete [] ((uint8_t*) memory);
    #endif
}

} // namespace v100
} // namespace ifhd
