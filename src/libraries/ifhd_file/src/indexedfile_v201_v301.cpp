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
namespace v201_v301
{

const uint8_t  IndexedFile::byte_order  = PLATFORM_BYTEORDER_UINT8;

int64_t IndexedFile::default_block_size      = 512;
int64_t IndexedFile::default_cache_size      = 1024 * 1024 * 8; // x MBytes


IndexedFile::IndexedFile()
{
    initialize();
}

IndexedFile::~IndexedFile()
{
}

void IndexedFile::initialize()
{
    _sector_size = default_block_size;

    _cache = nullptr;
    _cache_size = 0;

    _buffer = nullptr;
    _buffer_size = 0;
    _file_pos = 0;

    _file_header = nullptr;

    _write_guid = false;

    _system_cache_disabled = false;

    _write_mode = false;

    // set time zone information
    #ifdef WIN32
    _tzset();
    #else
    tzset();
    #endif
}

void IndexedFile::close()
{
    freeExtensions();
    freeBuffer();
    freeHeader();

    _file_pos = 0;

    freeCache();

    _write_guid = false;
    
    _file.close();    
}

std::set<uint32_t> IndexedFile::getSupportedVersions() const
{
    return {version_id_beta,
            version_id_with_history,
            version_id_with_history_end_offset,
            version_id};
}

void IndexedFile::allocHeader()
{
    freeHeader();

    _file_header = (FileHeader*) internalMalloc(sizeof(FileHeader), true);
    utils5ext::memZero(_file_header, sizeof(FileHeader));
}

void IndexedFile::freeHeader()
{
    if (_file_header != nullptr)
    {
        internalFree(_file_header);
        _file_header = nullptr;
    }
}

void IndexedFile::getHeaderRef(FileHeader** file_header) const
{
    if (!_file_header)
    {
        throw std::logic_error("file is not opened");
    }

    *file_header = _file_header;
}

void IndexedFile::allocExtensionPage(utils5ext::FileSize size, void** buffer) const
{
    void* allocation_buffer = internalMalloc((int) size, true);
    a_util::memory::set(allocation_buffer, static_cast<size_t>(size), 0x00, static_cast<size_t>(size));
    *buffer = allocation_buffer;
}

void IndexedFile::freeExtensions()
{
    FileExtensionStruct* extension_struct;
    for (FileExtensionList::iterator it = _extensions.begin();
         it != _extensions.end();
         ++it)
    {
        extension_struct = *it;
        if (extension_struct != nullptr)
        {
            if (extension_struct->extension_page != nullptr)
            {
                internalFree(extension_struct->extension_page);
            }
            delete extension_struct;
        }
    }

    _extensions.clear();
}

void IndexedFile::allocBuffer(uint64_t size)
{
    if (static_cast<int64_t>(size) <= _buffer_size && _buffer != nullptr)
    {
        return;
    }

    freeBuffer();

    _buffer = static_cast<uint8_t*>(internalMalloc(size, false));
    _buffer_size = size;
}

void IndexedFile::freeBuffer()
{
    if (_buffer != nullptr)
    {
        internalFree(_buffer);
        _buffer = nullptr;
    }

    _buffer_size = 0;
}

void IndexedFile::setDescription(const std::string& description)
{
    if (!_file_header)
    {
        throw std::logic_error("file is not opened");
    }
    a_util::memory::set(_file_header->description, sizeof(_file_header->description),0, sizeof(_file_header->description));

    if (!description.empty()) 
    {
        strncpy((char*) _file_header->description, description.c_str(), sizeof(_file_header->description));
        //nullterminate it in all cases
        _file_header->description[sizeof(_file_header->description) -1] = 0;
    }
}

std::string IndexedFile::getDescription() const
{
    if (nullptr == _file_header)
    {
        return std::string();
    }

    return std::string((const char*) (_file_header->description));
}

uint8_t IndexedFile::getByteOrder() const
{
    if (_file_header == nullptr)
    {
        return IndexedFile::byte_order;
    }
    else
    {
        return _file_header->header_byte_order; 
    }
}

void IndexedFile::setDateTime(const a_util::datetime::DateTime& date_time)
{
    if (!_file_header)
    {
        throw std::logic_error("file is not opened");
    }

    if (getSupportedVersions().count(_file_header->version_id))
    {
        time_t uxtime = 0;
        struct tm time_info;
        utils5ext::memZero(&time_info, sizeof(time_info));

        time_info.tm_year = date_time.getYear() - 1900;
        // dateTime::Month is 1-based
        
        // tm::tm_mon is 0-based
        time_info.tm_mon  = date_time.getMonth() - 1;
        time_info.tm_mday = date_time.getDay();

        time_info.tm_hour = date_time.getHour();
        time_info.tm_min  = date_time.getMinute();
        time_info.tm_sec  = date_time.getSecond();
        time_info.tm_isdst = -1;

        uxtime = mktime(&time_info);
        _file_header->file_time = time(&uxtime);
    } 
    else
    {
        //use this path for further fileversions
        _file_header->file_time = date_time.toTimestamp();
    }
}

a_util::datetime::DateTime IndexedFile::getDateTime() const
{
    a_util::datetime::DateTime date_time;
    if (_file_header == nullptr)
    {
        return a_util::datetime::DateTime();
    }
    return getDateTimeHelper(*_file_header);
}

bool IndexedFile::findExtension(const char* identifier,
                                    FileExtension** extension_info, 
                                    void** data) const
{
    FileExtensionStruct* extension_struct;

    for (FileExtensionList::const_iterator it = _extensions.begin(); 
         it != _extensions.end(); 
         ++it)
    {
        extension_struct = *it;
        if (a_util::strings::isEqualNoCase(identifier,
                                           (const char*) extension_struct->file_extension.identifier))
        {
            *extension_info = (FileExtension*) &extension_struct->file_extension;
            *data           = (void*) extension_struct->extension_page;

            return true;
        }
    }

    //throw std::runtime_error(std::string("unable to find extension ") + identifier);
    return false;
}

void IndexedFile::getExtension(size_t index,
                                FileExtension** extension_info,
                                void** data) const
{
    *data = nullptr;
    *extension_info = nullptr;

    size_t extension_count = getExtensionCount();
    size_t count = index;
    if (index >= extension_count)
    {
        throw std::out_of_range("invalid extension index");
    }

    FileExtensionList::const_iterator it = _extensions.begin();
    
    while (count > 0 && it != _extensions.end())
    {
        ++it;
        count--;
    }

    // get extension data pointers

    FileExtensionStruct* extension_struct = *it;
    *extension_info = (FileExtension*) &extension_struct->file_extension;
    *data           = (void*) extension_struct->extension_page;
}

void IndexedFile::appendExtension(const char* identifier,
                                      const void* data,
                                      size_t data_size,
                                      uint32_t type_id,
                                      uint32_t version_id,
                                      uint16_t stream_id,
                                      uint32_t user_id)
{
    // overwrite protection of GUID
    // _write_guid is just set in method IndexedFile::SetGUID()
    std::string i_d;
    if (identifier != nullptr)
    {
        i_d = identifier;
    }
    
    if (i_d == std::string("GUID")
        && !_write_guid)
    { 
        throw std::invalid_argument("The GUID extension is not publicly writeable");
    }

    FileExtension extension_info;
    a_util::memory::set(&extension_info, sizeof(extension_info), 0, sizeof(extension_info));

    if (identifier == nullptr || a_util::strings::getLength(identifier) == 0)
    {
        strcpy((char*) extension_info.identifier, "");
    }
    else
    {
        strcpy((char*) extension_info.identifier, identifier);
    }

    extension_info.stream_id  = stream_id;
    extension_info.user_id    = user_id;
    extension_info.type_id    = type_id;
    extension_info.version_id = version_id;
    extension_info.data_pos   = 0;
    extension_info.data_size  = data_size;

    return appendExtension(data, &extension_info);
}

void IndexedFile::appendExtension(const void* data, const FileExtension* extension_info)
{
    if (!_write_mode)
    {
        throw std::runtime_error("not in write mode");
    }

    FileExtensionStruct* extension_struct = new FileExtensionStruct;

    a_util::memory::copy(&extension_struct->file_extension, sizeof(FileExtension), extension_info, sizeof(FileExtension));
    extension_struct->extension_page = nullptr;

    if (extension_info->data_size)
    {
        allocExtensionPage(extension_info->data_size, &extension_struct->extension_page);
        if (nullptr != data)
        {
            a_util::memory::copy(extension_struct->extension_page, (int)extension_info->data_size, data, (int) extension_info->data_size);
        }
        else
        {
            a_util::memory::set(extension_struct->extension_page, (int)extension_info->data_size, 0, (int) extension_info->data_size);
        }
    }

    _extensions.push_back(extension_struct);

    if (_file_header != nullptr)
    {
        _file_header->extension_count = (uint32_t) _extensions.size();
    }
}

size_t IndexedFile::getExtensionCount() const
{
    return _extensions.size();
}

void IndexedFile::allocCache(int64_t size)
{
    freeCache();
    
    _cache_size = getSizeOnDisk(size, true);
    _cache = internalMalloc(_cache_size, true);
}

void IndexedFile::freeCache()
{
    if (_cache != nullptr)
    {
        internalFree(_cache);
        _cache = nullptr;
    }

    _cache_size = 0;
}

void* IndexedFile::getCacheAddr() const
{
    return _cache;
}

utils5ext::FileSize IndexedFile::getSizeOnDisk(utils5ext::FileSize size, bool use_segment_size) const
{
    if (_system_cache_disabled && use_segment_size)
    {
        return (size + _sector_size - 1) & ~(_sector_size - 1);
    }
    else
    {
        return size;
    }
}

void* IndexedFile::internalMalloc(size_t size, bool use_segment_size) const
{
    if (!_system_cache_disabled)
    {
        return new (std::nothrow)uint8_t[size];
    }

    if (use_segment_size)
    {
        size = (size + _sector_size - 1) & ~(_sector_size - 1);
    }

    return utils5ext::allocPageAlignedMemory(size, 
                                                _sector_size);
}

void IndexedFile::internalFree(void* memory) const
{
    if (!_system_cache_disabled)
    {
        delete[] (uint8_t*) memory;
        return;
    }

    utils5ext::freePageAlignedMemory(memory);
}

void IndexedFile::setGUID()
{
    // get new GUID
    std::string new_guid;
  //  RETURN_IF_FAILED(GenerateNewGUID(newGUID));

    // create new GUID extension
    _write_guid = true;
   // RETURN_IF_FAILED(AppendExtension("GUID", newGUID.GetPtr(), newGUID.GetLength() + 1));
    _write_guid = false;
}

std::string IndexedFile::getGUID() const
{
    FileExtension* extension_info = nullptr;
    void*          extension_data  = nullptr;

    // read GUID extension
    findExtension("GUID", &extension_info, &extension_data);

    if (nullptr != extension_data)
    {
        std::string g_uid = (char*)extension_data;
        return g_uid;
    }
    
    return std::string();
}

void IndexedFile::generateNewGUID(std::string& /*generatedGUID*/)
{    
    ///@todo
    throw std::runtime_error(" guid generation is not implemented yet");
   // RETURN_IF_FAILED(cGUID::GenerateGUID(generatedGUID));
}

} // namespace v201_v301
} // namespace ifhd
