/**
 * @file
 * Missing description.
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

#ifdef WIN32
    #include <windows.h>
    #include <io.h>
    #pragma warning(disable:4251)
    #pragma warning(disable:4786)
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <string.h>
    #include <errno.h>
#endif // WIN32

#include <utils5extension/utils5extension.h>

namespace utils5ext
{

#ifndef WIN32
    #define _O_CREAT         O_CREAT
    #define _O_RDONLY        O_RDONLY
    #define _O_WRONLY        O_WRONLY
    #define _O_APPEND        O_APPEND
    #define _O_RDWR          O_RDWR
    #define _O_TEXT          0 // O_TEXT
    #define _O_BINARY        0 // O_BINARY
    #define _O_SEQUENTIAL    0 // O_SEQUENTIAL
    #define _O_RANDOM        0 // O_RANDOM
    #define _O_TEMPORARY     0 // O_TEMPORARY
    #define _O_SHORT_LIVED   0 // O_SHORT_LIVED
    #ifndef S_IREAD          // obsolete synonym for BSD compatibility
    #define _S_IREAD         S_IRUSR
    #else                    // S_IREAD
    #define _S_IREAD         S_IREAD
    #endif                   // S_IREAD
    #ifndef S_IWRITER        // obsolete synonym for BSD compatibility
    #define _S_IWRITE         S_IWUSR
    #else                    // S_IREAD
    #define _S_IWRITE        S_IWRITE
    #endif                   // S_IREAD

    #if (defined(__APPLE__) || defined(ANDROID) )
        #define _open        open
    #else
        #define _open        open64
    #endif

    #define _close           close
    #define _read            read
    #define _write           ::write
    #define _eof             eof

    #ifndef __APPLE__
        #define _lseek       lseek64
    #else
        #define _lseek       lseek
    #endif

    #define _tell            tell
    #define _commit          commit
    #define _stat            stat64

    #if (defined(_ARM_ARCH_7) || defined(A_UTILS_IPHONE_SIMULATOR) || defined(A_UTILS_IPHONE) || defined(ANDROID) )
        #define _stat            stat
    #else
        #define _stat            stat64
    #endif

#endif


#ifdef WIN32
#define INVALID_FILE_HANDLE nullptr
#else
#define INVALID_FILE_HANDLE -1
#endif

void fileRename(const a_util::filesystem::Path& from, const a_util::filesystem::Path& to)
{
    if (0 != rename(from.toString().c_str(), to.toString().c_str()))
    {
        throw std::runtime_error("unable to rename " + from.toString() + " to " + to.toString());
    }
}

size_t getDefaultSectorSize()
{
    return 512;
}

/**
* \typedef MemoryAddress
* The MemoryAddress always defines a type for the platforms memory address
* pointer (@ref void*) (32 / 64 bit at the moment)
* (platform and compiler dependent type).
*/
#ifdef WIN32 
#ifdef _WIN64
typedef uint64_t MemoryAddress;
#else
typedef uint32_t MemoryAddress;
#endif
#else
#if (__WORDSIZE == 64)
typedef uint64_t MemoryAddress;
#else
#if (__WORDSIZE == 32)
typedef uint32_t MemoryAddress;
#else
#if (__WORDSIZE == 16)
typedef uint16_t MemoryAddress;
#else
typedef unsigned int MemoryAddress;
#endif
#endif
#endif
#endif

struct AlignedMemoryInfo
{
    unsigned int   magic;
    uint8_t* memory;
};
const  unsigned int aligned_memory_info_magic = 0x44434241;


void* allocPageAlignedMemory(size_t size, size_t page_size)
{

    if (page_size == 0)
    {
        page_size = getDefaultSectorSize();
    }

    size_t alloc_size = sizeof(AlignedMemoryInfo) + (size + page_size - 1);

    uint8_t* memory = new uint8_t[alloc_size];

    uint8_t* aligned_address = (uint8_t*)
            ((MemoryAddress)(memory + sizeof(AlignedMemoryInfo) + page_size - 1) & ~(page_size - 1));

    AlignedMemoryInfo* info = (AlignedMemoryInfo*)(aligned_address - sizeof(AlignedMemoryInfo));
    info->magic = aligned_memory_info_magic;
    info->memory = memory;

    return aligned_address;
}

void freePageAlignedMemory(void* memory)
{
    if (nullptr != memory)
    {
        AlignedMemoryInfo* info = (AlignedMemoryInfo*)((uint8_t*)memory - sizeof(AlignedMemoryInfo));
        if (info->magic == aligned_memory_info_magic)
        {
            delete[](uint8_t*) info->memory;
        }
    }
}

static size_t local_get_default_sector_size()
{
    return 512;
}

size_t getSectorSizeFor(const a_util::filesystem::Path& filename)
{
#ifdef WIN32

    a_util::filesystem::Path drive = filename.getRoot();
    if (drive.isEmpty())
    {
        a_util::filesystem::Path currentDir = a_util::filesystem::getWorkingDirectory();
        drive = currentDir.getRoot();

        if (drive.isEmpty())
        {
            drive = "C:";
        }
    }

    a_util::filesystem::Path newPath = drive.toString().append("\\");

    DWORD sectors_per_cluster;
    DWORD bytes_per_sector;
    DWORD number_of_free_clusters;
    DWORD total_number_of_clusters;

    BOOL result = GetDiskFreeSpace(newPath.toString().c_str(),
        &sectors_per_cluster,
        &bytes_per_sector,
        &number_of_free_clusters,
        &total_number_of_clusters);

    if (!result || bytes_per_sector == 0)
    {
        return getDefaultSectorSize();
    }

    return static_cast<size_t>(bytes_per_sector);

#else

    return local_get_default_sector_size();

#endif
}

File::File()
{
    initialize();
}

File::~File()
{
    close();
}

/**
 *
Initializes the File object *
 *
**/
void File::initialize()
{
    _file                     = INVALID_FILE_HANDLE;

    _read_cache                = nullptr;
    _file_cache_size            = 0;
    _file_cache_usage           = 0;
    _file_cache_offset          = 0;
    _read_cache_enabled         = false;
    _reference                = false;
    _system_cache_disabled      = false;
    _sector_size               = getDefaultSectorSize();
    _sector_bytes_to_skip        = 0;
}

void File::attach(File& file)
{
    close();

    initialize();

    _file             = file._file;
    _reference        = true;
}

void File::detach()
{
    if (!_reference)
    {
        throw std::runtime_error("the file is not attached");
    }

    initialize();
}

void File::setReadCache(size_t cache_size)
{
    if (cache_size > 0)
    {
        allocReadCache(cache_size);
        _read_cache_enabled = true;
    }
    else
    {
        allocReadCache(getDefaultSectorSize());

        // just enable for all operations if file system cache is disabled
        _read_cache_enabled = _system_cache_disabled;
    }
}

void File::allocReadCache(size_t cache_size)
{
    freeReadCache();

    if (cache_size > 0)
    {
        _read_cache = (uint8_t*) internalMalloc(cache_size, true);
    }

    _file_cache_size = cache_size;
}

void File::freeReadCache()
{
    if (_read_cache != nullptr)
    {
        internalFree(_read_cache, true);
        _read_cache = nullptr;
    }

    _file_cache_size    = 0;
    _read_cache_enabled = false;
}

void File::open(const a_util::filesystem::Path& filename, uint32_t mode)
{
    close();

    a_util::filesystem::Path open_filename = filename;
   // openFilename.MakeNativeSlash();

    _system_cache_disabled      = false;
    _sector_size               = getDefaultSectorSize();
    _sector_bytes_to_skip     = 0;

    #ifdef WIN32

        DWORD access = 0;
        DWORD openMode = 0;
        if ((mode & om_read) != 0)
        {
            access    |= GENERIC_READ;
            openMode  |= OPEN_EXISTING;
        }
        else if ((mode & om_read_write) != 0)
        {
            access    |= GENERIC_READ | GENERIC_WRITE;
            openMode  |= OPEN_EXISTING;
        }
        else if ((mode & om_write) != 0)
        {
            access    |= GENERIC_WRITE;
            openMode  |= CREATE_ALWAYS;
        }
        else if ((mode & om_append) != 0)
        {
            access    |= GENERIC_WRITE;
            openMode  |= OPEN_EXISTING;
        }

        DWORD share = 0;
        if ((mode & om_shared_read) != 0)
        {
            share     |= FILE_SHARE_READ;
        }

        if ((mode & om_shared_write) != 0)
        {
            share     |= FILE_SHARE_WRITE;
        }

        DWORD flags = 0;
        if ((mode & om_sequential_access) != 0)
        {
            flags     |= FILE_FLAG_SEQUENTIAL_SCAN;
        }

        if ((mode & om_temporary_file) != 0)
        {
            flags     |= FILE_FLAG_DELETE_ON_CLOSE;
        }

        if ((mode & om_write_through) != 0)
        {
            flags     |= FILE_FLAG_WRITE_THROUGH;
        }

        if ((mode & om_disable_file_system_cache) != 0)
        {
            flags     |= FILE_FLAG_NO_BUFFERING;
            _system_cache_disabled  = true;
            _sector_size           = getSectorSizeFor(open_filename);
        }

        _file = CreateFile(open_filename.toString().c_str(),
                             access,
                             share,
                             nullptr,
                             openMode,
                             flags,
                             nullptr);
        if (_file == INVALID_HANDLE_VALUE || _file == nullptr)
        {
            _file = INVALID_FILE_HANDLE;
            throw std::runtime_error("unable to open file " + open_filename.toString());
        }

        if ((mode & om_append) != 0)
        {
            setFilePos(0, fp_end);
        }

    #else // WIN32

        int open_mode = 0;
        mode_t permissions = 0;
        int protection = 0;

        if (mode & om_write || mode & om_read_write)
        {
            open_mode |= O_CREAT;
        }

        if (mode & om_read)
        {
            open_mode |= O_RDONLY;
        }
        else if (mode & om_write)
        {
            open_mode |= O_WRONLY | O_TRUNC;
        }
        else if (mode & om_append)
        {
            open_mode |= O_APPEND | O_WRONLY;
        }
        else if (mode & om_read_write)
        {
            open_mode |= O_RDWR;
        }
        else
        {
            throw std::invalid_argument("missing open mode parameter");
        }

        if (mode & om_text_mode) {
            open_mode |= _O_TEXT;
        } else {
            open_mode |= _O_BINARY;
}

        if (mode & om_sequential_access) {
            open_mode |= _O_SEQUENTIAL;
        } else {
            open_mode |= _O_RANDOM;
}

        if (mode & om_temporary_file) {
            open_mode |= _O_TEMPORARY;
}

        if (mode & om_short_lived) {
            open_mode |= _O_SHORT_LIVED;
}

        if (mode & om_read) {
            protection |= S_IREAD;
        } else if (mode & om_write) {
            protection |= S_IWRITE;
}

        if (mode & om_write)
        {
            // set default owner rights to "-rw-rw-r--"
            // please notice: umask might mask out some of the bits
            // clear mask by calling umask(0) to avoid that if necessary.
            permissions |= S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
        }

        int handle = _open(open_filename.toString().c_str(), open_mode, permissions);
        if (handle < 0)
        {
            _file = INVALID_FILE_HANDLE;
            throw std::runtime_error("unable to open file " + open_filename.toString());
        }

        _file = (FileHandle) handle;

        if (mode & om_append)
        {
            // make sure that a subsequent GetFilePos call returns the correct position
            // at the end of the file. On linux open with append does not initially move the
            // file pointer to the end. The pointer is positioned to the end of the file
            // only when a write operation is performed.
            setFilePos(0, fp_end);
        }


    #endif // WIN32

    setReadCache(0); // enable read cache just for ReadLine operations
}

void File::close()
{
    if (_reference)
    {
        detach();
    }

    if (_file != INVALID_FILE_HANDLE)
    {
        #ifdef WIN32
        CloseHandle(_file);
        #else
        ::close((int) _file);
        #endif
        _file = INVALID_FILE_HANDLE;
    }

    freeReadCache();
}

bool File::isValid() const
{
    return _file != INVALID_FILE_HANDLE;
}

static inline size_t internal_skip(FileHandle file, size_t number_of_bytes)
{
    if (0 == number_of_bytes)
    {
        return 0;
    }

    if (INVALID_FILE_HANDLE == file)
    {
        throw std::invalid_argument("invalid file handle");
    }

    #ifdef WIN32

        LARGE_INTEGER li;
        li.QuadPart = number_of_bytes;

        li.LowPart = ::SetFilePointer (file, li.LowPart, &li.HighPart, FILE_CURRENT);
        if (li.LowPart == INVALID_SET_FILE_POINTER && ::GetLastError() != NO_ERROR)
        {
            throw std::runtime_error("unable to seek in file");
        }

    #else // WIN32

        FilePos fp_pos = (FilePos) (_lseek(file, number_of_bytes, SEEK_CUR));
        if (fp_pos < 0)
        {
            throw std::runtime_error("unable to seek in file");
        }

    #endif // WIN32

    return number_of_bytes;
}

static inline size_t internal_read(FileHandle file, void* buffer, size_t buffer_size)
{
    if (0 == buffer_size)
    {
        return 0;
    }

    if ((INVALID_FILE_HANDLE == file)||
        (nullptr == buffer))
    {
        throw std::invalid_argument("invalid file handle or buffer");
    }

    #ifdef WIN32

        DWORD read_count = 0;

        BOOL status = ::ReadFile(file, (uint8_t*) buffer, static_cast<DWORD>(buffer_size), &read_count, nullptr);
        if (status)
        {
            return static_cast<size_t>(read_count);
        }
        else
        {
            throw std::runtime_error("ReadFile failed");
        }

    #else // WIN32

        int result = _read(file, (uint8_t*) buffer, buffer_size);
        if (result < 0)
        {
            throw std::runtime_error("read failed");
        }
        return static_cast<size_t>(result);

    #endif // WIN32
}

size_t File::skip(size_t buffer_size)
{
    if(INVALID_FILE_HANDLE == _file)
    {
        throw std::runtime_error("file not opened");
    }

    if( 0 == buffer_size)
    {
        return 0;
    }

    size_t skip_count = 0;

    if (_file_cache_usage > 0)
    {
        if (_file_cache_usage >= buffer_size)
        {
            _file_cache_usage  -= buffer_size;
            _file_cache_offset += buffer_size;

            return buffer_size;
        }
        else
        {
            skip_count += _file_cache_usage;
            _file_cache_usage  = 0;
            _file_cache_offset = 0;
        }
    }

    size_t bytes_to_read = buffer_size - skip_count;
    size_t read_operation_result;
    size_t bytes_to_skip;

    if (_system_cache_disabled)
    {
        while (bytes_to_read > 0)
        {
            if (_sector_bytes_to_skip != 0)
            {
                bytes_to_skip = (int) _sector_bytes_to_skip;
                _sector_bytes_to_skip = 0;
            }
            else
            {
                bytes_to_skip = 0;
            }

            read_operation_result = internal_read(_file, _read_cache, _file_cache_size);
            if (read_operation_result == 0)
            {
                if (skip_count > 0)
                {
                    return skip_count;
                }
            }

            read_operation_result -= bytes_to_skip;
            _file_cache_usage  = read_operation_result;
            _file_cache_offset = bytes_to_skip;

            size_t bytes_for_use = std::min(read_operation_result, bytes_to_read);

            _file_cache_usage  -= bytes_for_use;
            _file_cache_offset += bytes_for_use;

            skip_count   += bytes_for_use;
            bytes_to_read -= bytes_for_use;
        }
    }
    else if (!_read_cache_enabled || bytes_to_read > _file_cache_size)
    {
        read_operation_result = internal_skip(_file, bytes_to_read);
        if (read_operation_result == 0)
        {
            if (skip_count > 0)
            {
                return skip_count;
            }
        }

        skip_count += read_operation_result;
    }
    else
    {
        read_operation_result = internal_skip(_file, bytes_to_read);
        if (read_operation_result == 0)
        {
            if (skip_count > 0)
            {
                return skip_count;
            }
        }

        size_t bytes_for_use = std::min(read_operation_result, bytes_to_read);

        skip_count += bytes_for_use;
    }

    return skip_count;
}

size_t File::read(void* buffer, size_t buffer_size)
{
    if (INVALID_FILE_HANDLE == _file)
    {
        throw std::runtime_error("file not opened");
    }

    if (buffer_size == 0)
    {
        return 0;
    }

    if (nullptr == buffer)
    {
        throw std::invalid_argument("invalid buffer pointer");
    }

    size_t read_count = 0;

    if (_file_cache_usage > 0)
    {
        if (_file_cache_usage >= buffer_size)
        {
            a_util::memory::copy(buffer, buffer_size, _read_cache + _file_cache_offset, buffer_size);

            _file_cache_usage  -= buffer_size;
            _file_cache_offset += buffer_size;

            return buffer_size;
        }
        else
        {
            a_util::memory::copy(buffer, _file_cache_usage,  _read_cache + _file_cache_offset, _file_cache_usage);

            read_count += _file_cache_usage;

            _file_cache_usage  = 0;
            _file_cache_offset = 0;

        }
    }

    size_t bytes_to_read = buffer_size - read_count;
    size_t read_operation_result;
    int bytes_to_skip;

    if (_system_cache_disabled)
    {
        if (_sector_bytes_to_skip != 0)
        {
            bytes_to_skip = (int) _sector_bytes_to_skip;
            _sector_bytes_to_skip = 0;
        }
        else
        {
            bytes_to_skip = 0;
        }

        read_operation_result = internal_read(_file, _read_cache, _file_cache_size);
        if (read_operation_result == 0)
        {
            if (read_count > 0)
            {
                return read_count;
            }
        }

        read_operation_result -= bytes_to_skip;
        _file_cache_usage  = read_operation_result;
        _file_cache_offset = bytes_to_skip;

        size_t bytes_for_use = std::min(static_cast<size_t>(read_operation_result), bytes_to_read);

        a_util::memory::copy((uint8_t*) buffer + read_count, bytes_for_use, _read_cache + bytes_to_skip, bytes_for_use);

        _file_cache_usage  -= bytes_for_use;
        _file_cache_offset += bytes_for_use;

        read_count   += bytes_for_use;
        bytes_to_read -= bytes_for_use;
    }
    else if (!_read_cache_enabled || bytes_to_read > _file_cache_size)
    {
        read_operation_result = internal_read(_file, (uint8_t*) buffer + read_count, bytes_to_read);
        if (read_operation_result == 0)
        {
            if (read_count > 0)
            {
                return read_count;
            }
        }

        read_count += read_operation_result;
    }
    else
    {
        read_operation_result = internal_read(_file, _read_cache, _file_cache_size);
        if (read_operation_result == 0)
        {
            if (read_count > 0)
            {
                return read_count;
            }
        }

        size_t bytes_for_use = std::min(static_cast<size_t>(read_operation_result), bytes_to_read);

        a_util::memory::copy((uint8_t*) buffer + read_count, bytes_for_use, _read_cache, bytes_for_use);
        _file_cache_usage  = std::max(static_cast<size_t>(0), read_operation_result - bytes_to_read);
        _file_cache_offset = bytes_to_read;

        read_count += bytes_for_use;
    }

    return read_count;
}

void File::readAll(void* buffer, size_t buffer_size)
{
    while(buffer_size > 0)
    {
        size_t bytes_read = read(buffer, buffer_size);
        if (bytes_read == 0)
        {
            throw std::runtime_error("end of file");
        }
        buffer_size -= bytes_read;
        buffer = reinterpret_cast<uint8_t*>(buffer) + bytes_read;
    }
}

size_t File::write(const void* buffer, size_t buffer_size)
{
    if (INVALID_FILE_HANDLE == _file)
    {
        throw std::runtime_error("file not opened");
    }

    if (buffer_size == 0)
    {
        return 0;
    }

    if (nullptr == buffer)
    {
        throw std::invalid_argument("invalid buffer pointer");
    }

#ifdef WIN32
    DWORD bytes_written = 0;
    if (!::WriteFile(_file,
                     buffer,
                     std::min<DWORD>((DWORD)buffer_size, 33525760), // this is the limit of a single write call on windows
                     &bytes_written,
                     nullptr))
    {
        throw std::system_error(std::error_code(::GetLastError(), std::system_category()));
    }
#else
    int bytes_written = _write(_file, buffer, buffer_size);
#endif

    return static_cast<size_t>(bytes_written);
}

void File::writeAll(const void* buffer, size_t buffer_size)
{
    size_t all_bytes_written = 0;
    do
    {
        size_t bytes_written = write(buffer, buffer_size);
        buffer_size -= bytes_written;
        buffer = reinterpret_cast<const uint8_t*>(buffer) + bytes_written;
        all_bytes_written += bytes_written;
    }
    while(buffer_size > 0);
}

void File::write(const std::string& string)
{
    writeAll(static_cast<void*>(const_cast<char*>(string.c_str())), static_cast<int>(string.size()));
}

void File::readLine(std::string& s_string)
{
    s_string.clear();

    bool cache_was_empty = (_file_cache_usage <= 0);

    size_t read_count = 0;
    uint8_t c;

    const int local_buffer_size = 512;
    char local_buffer[local_buffer_size];
    int local_buffer_usage = 0;
    int char_count = 0;

    do
    {
        if (_file_cache_usage <= 0)
        {
            _file_cache_offset = 0;

            read_count = internal_read(_file, _read_cache, _file_cache_size);
            if (read_count > 0)
            {
                _file_cache_usage = read_count;
            }
            else
            {
                if (char_count == 0)
                {
                    if (cache_was_empty)
                    {
                        throw std::runtime_error("unable to read from file");
                    }
                }

                break;
            }
        }

        if (_file_cache_usage > 0)
        {
            c = _read_cache[_file_cache_offset++];
            _file_cache_usage--;

            if (c == '\n')
            {
                break;
            }
            else if (c == '\r')
            {
                // filter out 0D (\r) as we handle that as an optional
                // character within the two-byte sequence 0D0A (\r\n) used
                // on windows platforms
            }
            else
            {
                local_buffer[local_buffer_usage++] = c;

                if (local_buffer_usage >= local_buffer_size - 1)
                {
                    local_buffer[local_buffer_usage] = '\0';
                    s_string.append(local_buffer);
                    local_buffer_usage = 0;
                }

                char_count++;
            }
        }
        else
        {
            if (char_count == 0)
            {
                throw std::runtime_error("unable to read from file");
            }

            break;
        }
    } while (true);

    if (local_buffer_usage > 0)
    {
        local_buffer[local_buffer_usage] = '\0';
        s_string.append(local_buffer);
        local_buffer_usage = 0;
    }    
}

void File::writeLine(const std::string& string)
{
    write(string);
    writeAll((void*) "\r\n", 2);
}

FileSize File::getSize() const
{
    if (!isValid())
    {
        throw std::runtime_error("file not opened");
    }

    #ifdef WIN32

        DWORD file_size_low, file_size_high;

        file_size_low = ::GetFileSize(_file, &file_size_high);
        if (file_size_low == 0xFFFFFFFF)
        {
            return -1;
        }

        LARGE_INTEGER large_int;
        large_int.LowPart = file_size_low;
        large_int.HighPart = file_size_high;

        return (FileSize) large_int.QuadPart;

    #else

        FilePos file_pos = getFilePos();
        FilePos end_pos  = lseek((int) _file, 0, SEEK_END);
        lseek((int) _file, file_pos, SEEK_SET);

        return (FileSize) end_pos;

    #endif
}

FilePos File::getFilePos() const
{
    if (!isValid())
    {
        throw std::runtime_error("file not opened");
    }

    FilePos fp_current;

    #ifdef WIN32

        LARGE_INTEGER li;

        li.QuadPart = 0;
        li.LowPart = SetFilePointer (_file, li.LowPart, &li.HighPart, FILE_CURRENT);

        if (li.LowPart == INVALID_SET_FILE_POINTER && ::GetLastError() != NO_ERROR)
        {
            li.QuadPart = -1;
        }

        fp_current = li.QuadPart;

    #else

        fp_current = _lseek((int)_file, 0, SEEK_CUR);

    #endif

    if (_file_cache_usage > 0)
    {
        fp_current -= _file_cache_usage;
        if (fp_current < 0)
        {
            fp_current = 0;
        }
    }

    return fp_current;
}

///@todo
FilePos File::setFilePos(FilePos offset, FilePosRef move_mode)
{
    if (!isValid())
    {
        throw std::runtime_error("file not opened");
    }

    // invalidate cache
    _file_cache_usage  = 0;
    _file_cache_offset = 0;

    if (_system_cache_disabled)
    {
        if (move_mode != fp_begin)
        {
            throw std::runtime_error("when the system cache is disable, seeking is only allowed with FP_Begin");
        }

        FilePos fp_sector_aligned_pos = offset & ~(_sector_size - 1);
        _sector_bytes_to_skip = (offset - fp_sector_aligned_pos);
        offset = fp_sector_aligned_pos;
    }

    #ifdef WIN32

        DWORD move = 0;
        switch (move_mode)
        {
        case fp_begin:
            move = FILE_BEGIN;
            break;
        case fp_current:
            move = FILE_CURRENT;
            break;
        case fp_end:
            move = FILE_END;
            break;
        default:
            return -1;
        }

        LARGE_INTEGER li;
        li.QuadPart = offset;

        li.LowPart = SetFilePointer (_file, li.LowPart, &li.HighPart, move);

        if (li.LowPart == INVALID_SET_FILE_POINTER && ::GetLastError() != NO_ERROR)
        {
            throw std::runtime_error("unable to seek file");
        }

        return (FilePos) li.QuadPart + _sector_bytes_to_skip;

    #else // WIN32

        int seek_cmd;
        switch (move_mode)
        {
            case fp_current:
                seek_cmd = SEEK_CUR;
                break;
            case fp_end:
                seek_cmd = SEEK_END;
                break;
            default: // FP_Begin
                seek_cmd = SEEK_SET;
                break;
        }

        off_t result = _lseek((int)_file, offset, seek_cmd);
        if (result == (off_t) -1)
        {
            throw std::runtime_error(std::string("unable to seek file: ") + strerror(errno));
        }
        return (FilePos) (result + _sector_bytes_to_skip);

    #endif // WIN32
}

bool File::isEof()
{
    if (_file_cache_usage > 0)
    {
        return false;
    }

    FilePos  pos  = getFilePos();
    FileSize size = getSize();

    if (pos >= size || pos == (FilePos) -1)
    {
        return true;
    }

    return false;
}

void* File::internalMalloc(size_t size, bool use_segment_size)
{
    if (!_system_cache_disabled && !use_segment_size)
    {
        size = (size + _sector_size - 1) & ~(_sector_size - 1);
        return (void*) (new uint8_t[size]);
    }

    return allocPageAlignedMemory(size, _sector_size);
}

void File::internalFree(void* memory, bool use_segment_size)
{
    if (!_system_cache_disabled && !use_segment_size)
    {
        delete [] ((uint8_t*) memory);
        return;
    }

    freePageAlignedMemory(memory);
}


#ifndef WIN32
void File::truncate(FilePos size)
{
    if (ftruncate(_file, size) != 0)
    {
        throw std::runtime_error("unable to truncate file");
    }
}
#else
void File::truncate(FilePos size)
{
    if (size != setFilePos(size, fp_begin) ||
        TRUE != ::SetEndOfFile(_file))
    {
        throw std::runtime_error("unable to truncate file");
    }
}
#endif

a_util::datetime::DateTime getTimeAccess(const a_util::filesystem::Path filename)
{
    struct _stat buffer;
    int status = _stat(filename.toString().c_str(), &buffer);

    if (0 != status)
    {
        throw std::runtime_error("unable to stat " + filename.toString());
    }

    tm* time = localtime(&buffer.st_atime);
    if (time == nullptr)
    {
        throw std::runtime_error("unable to convert timestamp to localtime");
    }

    a_util::datetime::DateTime dt;
    dt.setYear(time->tm_year + 1900);
    dt.setMonth(time->tm_mon + 1);
    dt.setDay(time->tm_mday);
    dt.setHour(time->tm_hour);
    dt.setMinute(time->tm_min);
    dt.setSecond(time->tm_sec);
    dt.setMicrosecond(0);

    return dt;
}

a_util::datetime::DateTime getTimeCreation(const a_util::filesystem::Path filename)
{
    struct _stat buffer;
    int status = _stat(filename.toString().c_str(), &buffer);

    if (0 != status)
    {
        throw std::runtime_error("unable to stat " + filename.toString());
    }

    tm* time = localtime(&buffer.st_ctime);
    if (time == nullptr)
    {
        throw std::runtime_error("unable to convert timestamp to localtime");
    }

    a_util::datetime::DateTime dt;
    dt.setYear(time->tm_year + 1900);
    dt.setMonth(time->tm_mon + 1);
    dt.setDay(time->tm_mday);
    dt.setHour(time->tm_hour);
    dt.setMinute(time->tm_min);
    dt.setSecond(time->tm_sec);
    dt.setMicrosecond(0);

    return dt;
}

a_util::datetime::DateTime getTimeChange(const a_util::filesystem::Path filename)
{
    struct _stat buffer;
    int status = _stat(filename.toString().c_str(), &buffer);

    if (0 != status)
    {
        throw std::runtime_error("unable to stat " + filename.toString());
    }

    tm* time = localtime(&buffer.st_mtime);
    if (time == nullptr)
    {
        throw std::runtime_error("unable to convert timestamp to localtime");
    }

    a_util::datetime::DateTime dt;
    dt.setYear(time->tm_year + 1900);
    dt.setMonth(time->tm_mon + 1);
    dt.setDay(time->tm_mday);
    dt.setHour(time->tm_hour);
    dt.setMinute(time->tm_min);
    dt.setSecond(time->tm_sec);
    dt.setMicrosecond(0);

    return dt;
}

} // namespace utils5ext
