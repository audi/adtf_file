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
#include <string.h>

namespace ifhd
{
namespace v110
{

int IndexedFileV110::default_block_size          = 512;
int IndexedFileV110::default_cache_size          = 1024 * 1024 * 8;  // x MBytes
int IndexedFileV110::index_table_cluster_size     = 4096;             // always alloc N indices at once

IndexedFileV110::IndexedFileV110()
{
    initialize();
}

IndexedFileV110::~IndexedFileV110()
{
}

void IndexedFileV110::initialize()
{
    _sector_size                   = default_block_size;

    _cache                        = nullptr;
    _cache_size                    = 0;

    _buffer                       = nullptr;
    _buffer_size                   = 0;
    _file_pos                      = 0;

    _index_blocks                  = nullptr;
    _active_index_block             = nullptr;

    _file_header                  = nullptr;

    _system_cache_disabled          = false;
}

//*************************************************************************************************
void IndexedFileV110::close()
{
    freeExtensions();
    freeBuffer();
    freeIndexTable();
    freeHeader();

    _file_pos = 0;

    freeCache();

    if (_file != nullptr)
    {
        _file->close();
    }
}

//*************************************************************************************************
void IndexedFileV110::allocHeader()
{
    freeHeader();

    _file_header = (FileHeader*) internalMalloc(sizeof(FileHeader), true);
}

//*************************************************************************************************
void IndexedFileV110::freeHeader()
{
    if (_file_header != nullptr)
    {
        internalFree(_file_header);
        _file_header = nullptr;
    }
}

//*************************************************************************************************
void IndexedFileV110::getHeader(FileHeader** file_header) const
{
    *file_header = _file_header;
}

//*************************************************************************************************
void IndexedFileV110::allocExtensionPage(utils5ext::FileSize size, void** buffer)
{
    if (size == 0)
    {
        return;
    }

    void* allocation_buffer = internalMalloc((int) size, true);
    utils5ext::memZero(allocation_buffer, (int) size);
    
    *buffer = allocation_buffer;
}

//*************************************************************************************************
void IndexedFileV110::freeExtensions()
{
    FileExtensionStruct* extension_header;
    for (FileExtensionList::iterator it = _extensions.begin();
         it != _extensions.end();
         ++it)
    {
        extension_header = *it;
        if (extension_header != nullptr)
        {
            internalFree(extension_header->extension_page);
        }
        delete extension_header;
    }

    _extensions.clear();
}

//*************************************************************************************************
void IndexedFileV110::allocBuffer(int size)
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
void IndexedFileV110::freeBuffer()
{
    if (_buffer != nullptr)
    {
        internalFree(_buffer);
        _buffer = nullptr;
    }

    _buffer_size = 0;
}

//*************************************************************************************************
void IndexedFileV110::allocIndexBlock(int count)
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
        new_item->data = new ChunkRef[count];
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
void IndexedFileV110::freeIndexTable()
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
void IndexedFileV110::setDescription(const std::string& description)
{
    utils5ext::memZero(_file_header->description, sizeof(_file_header->description));
    strncpy((char*) _file_header->description, (const char*) description.c_str(), 1912);
}

//*************************************************************************************************
std::string IndexedFileV110::getDescription() const
{
    return std::string((const char*) _file_header->description);
}

//*************************************************************************************************
void IndexedFileV110::setDateTime(const a_util::datetime::DateTime* date_time)
{
    _file_header->date_time.year = static_cast<uint16_t>(date_time->getYear());
    _file_header->date_time.month = static_cast<uint16_t>(date_time->getMonth());
    _file_header->date_time.day = static_cast<uint16_t>(date_time->getDay());
    _file_header->date_time.hour = static_cast<uint16_t>(date_time->getHour());
    _file_header->date_time.minute = static_cast<uint16_t>(date_time->getMinute());
    _file_header->date_time.second = static_cast<uint16_t>(date_time->getSecond());
    _file_header->date_time.microseconds = date_time->getMicrosecond();
}

/**
 *  Retrieves the Date and Time of the File. 
 *  
 *  @param  dateTime  [in,out]  Pointer where to copy the Date and Time Structure. 
 *  @return Standard Result Code.
 */
void IndexedFileV110::getDateTime(a_util::datetime::DateTime* date_time) const
{
    // There is a offset displayed within the old files

    date_time->setYear(_file_header->date_time.year);
    date_time->setMonth(_file_header->date_time.month - 1);
    date_time->setDay(_file_header->date_time.day);
    date_time->setHour(_file_header->date_time.hour -1);
    date_time->setMinute(_file_header->date_time.minute);
    date_time->setSecond(_file_header->date_time.second);
    date_time->setMicrosecond(_file_header->date_time.microseconds);
}

//*************************************************************************************************
bool IndexedFileV110::findExtension(const std::string& identifier,
                                      FileExtension** extension_info, 
                                      void** data) const
{
    //void* extension;
    //FileExtension* extensionInfo;
    FileExtensionStruct* extention_header = nullptr;

    for (FileExtensionList::const_iterator it = _extensions.begin(); 
         it != _extensions.end(); 
         ++it)
    {
        extention_header = *it;
#ifdef WIN32
        if (0 == _stricmp(identifier.c_str(),
                         (const char*) extention_header->file_extension.identifier))
#else
        if (0 == strcasecmp(identifier.c_str(),
            (const char*) extention_header->file_extension.identifier))
#endif
        {
            *extension_info = (FileExtension*) &extention_header->file_extension;
            *data           = extention_header->extension_page;

            return true;
        }
    }

    //throw std::runtime_error("extension not found: " + identifier);
    return false;
}

//*************************************************************************************************
void IndexedFileV110::getExtension(int index, FileExtension** extension_info, void** data) const
{
    *data = nullptr;

    *extension_info = nullptr;

    int extension_count = getExtensionCount();
    if (index < 0 || index >= extension_count)
    {
        throw std::out_of_range("invalid extension index");
    }

    FileExtensionList::const_iterator it = _extensions.begin();
    while (index > 0 && it != _extensions.end())
    {
        ++it;
        index--;
    }

    // get extension data pointers

    FileExtensionStruct* extension_header = *it;

    *extension_info = (FileExtension*) &extension_header->file_extension;
    *data           = (void*) extension_header->extension_page;
}

//*************************************************************************************************
void IndexedFileV110::appendExtension(const std::string& identifier,
                                      const void* data,
                                      int data_size,
                                      uint32_t type_id,
                                      uint32_t version_id)
{
    FileExtension extension_info;
    utils5ext::memZero(&extension_info, sizeof(extension_info));

    if (identifier.empty())
    {
        strncpy((char*) extension_info.identifier, "", 32);
    }
    else
    {
        strncpy((char*)extension_info.identifier, identifier.c_str(), 32);
    }
    extension_info.type_id       = type_id;
    extension_info.version_id    = version_id;
    extension_info.data_pos         = 0;
    extension_info.data_size        = data_size;

    appendExtension(data, &extension_info);
}

//*************************************************************************************************
void IndexedFileV110::appendExtension(const void* data, const FileExtension* extension_info)
{
    if (!_write_mode)
    {
        throw std::runtime_error("not on write mode");
    }

    if (extension_info->data_size == 0)
    {
        throw std::invalid_argument("invalid extension size");
    }

    FileExtensionStruct* extension_struct = new FileExtensionStruct;

    void* extension_page = nullptr;
    allocExtensionPage(extension_info->data_size, &extension_page);

    extension_struct->extension_page = extension_page;
    a_util::memory::copy(&extension_struct->file_extension, sizeof(FileExtension), extension_info, sizeof(FileExtension));
    a_util::memory::copy(extension_page, (int)extension_info->data_size, data, (int) extension_info->data_size);

    _extensions.push_back(extension_struct);

    if (_file_header != nullptr)
    {
        _file_header->extension_count = (uint32_t) _extensions.size();
    }
}

int IndexedFileV110::getExtensionCount() const
{
    return (int) _extensions.size();
}

void IndexedFileV110::allocCache(int size)
{
    freeCache();

    _cache = internalMalloc(size, true);
    _cache_size = (int) getSizeOnDisk(size, true);
}

void IndexedFileV110::freeCache()
{
    if (_cache != nullptr)
    {
        internalFree(_cache);
        _cache        = nullptr;
    }

    _cache_size = 0;
}

void*  IndexedFileV110::getCacheAddr()
{
    return _cache;
}

static size_t local_get_default_sector_size()
{
    return 512;
}

int IndexedFileV110::getSectorSize(const a_util::filesystem::Path& filename) const
{
    std::string drive = filename.getRoot();
    if (drive.empty())
    {
        a_util::filesystem::Path current_dir = a_util::filesystem::getWorkingDirectory();
        drive = current_dir.getRoot();

        if (drive.empty())
        {
            drive = "C:";
        }
    }

    drive.append("\\");

#ifdef WIN32
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;

    BOOL result = GetDiskFreeSpace(drive.c_str(),
                                    &sectorsPerCluster,
                                    &bytesPerSector,
                                    &numberOfFreeClusters,
                                    &totalNumberOfClusters);

    if (!result)
    {
        return -1;
    }

    return bytesPerSector;
 #else
    return local_get_default_sector_size();
  #endif
}

utils5ext::FileSize IndexedFileV110::getSizeOnDisk(utils5ext::FileSize size, bool use_segmen_size) const
{
    if (_system_cache_disabled && use_segmen_size)
    {
        return (size + _sector_size - 1) & ~(_sector_size - 1);
    }
    else
    {
        return size;
    }
}

void* IndexedFileV110::internalMalloc(int size, bool use_segmensize)
{
    if (!_system_cache_disabled)
    {
        return (void*) (new uint8_t[size]);
    }

    if (use_segmensize)
    {
        size = (size + _sector_size - 1) & ~(_sector_size - 1);
    }

    return utils5ext::allocPageAlignedMemory(size, _sector_size);
}

void IndexedFileV110::internalFree(void* memory)
{
    if (!_system_cache_disabled)
    {
        delete [] ((uint8_t*) memory);
        return;
    }

    utils5ext::freePageAlignedMemory(memory);
}

} // namespace v110
} // namespace ifhd
