/**
 * @file
 * Indexed file helper.
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

/**
* A small helper class to use auto_ptr with arrays
*/

template <typename T>
class ArrDelAdapter
{
public:
    ArrDelAdapter(T *p) : _p(p) { }
    ~ArrDelAdapter() { delete [] _p; }
    T& operator*() const { return _p; };
    T& operator->() const { return _p; };
    T* get() const { return _p; };
private:
    T* _p;
};

static void CheckIfValidFile(FileHeader& file_header, const a_util::filesystem::Path& filename)
{
    // check if it is a dat file
    char i_d[4]; 
    a_util::memory::copy(i_d, 4, &file_header.file_id, 4);
    if (i_d[0] != 'I' ||
        i_d[1] != 'F' ||
        i_d[2] != 'H' ||
        i_d[3] != 'D')
    {
        throw std::runtime_error(filename + " is not a valid DAT file");
    }
}



a_util::datetime::DateTime getDateTimeHelper(const FileHeader& file_header)
{
    a_util::datetime::DateTime date_time;
    if (file_header.version_id == v500::version_id ||
        file_header.version_id == v400::version_id ||
        file_header.version_id == v201_v301::version_id ||
        file_header.version_id == v201_v301::version_id_beta ||
        file_header.version_id == v201_v301::version_id_with_history ||
        file_header.version_id == v201_v301::version_id_with_history_end_offset)
    {
        time_t uxtime = (time_t) file_header.file_time;
        if (uxtime == 0)
        {
            return date_time;
        }

        #ifdef WIN32
        _tzset();
        #else
        tzset();
        #endif

        struct tm* time_info = localtime(&uxtime);
        if (time_info == nullptr)
        {
             return date_time;
        } 
        {
            date_time.setYear(time_info->tm_year + 1900);
            date_time.setMonth(time_info->tm_mon +1);
            date_time.setDay(time_info->tm_mday);
            date_time.setHour(time_info->tm_hour);
            date_time.setMinute(time_info->tm_min);
            date_time.setSecond(time_info->tm_sec);
            date_time.setMicrosecond(0);
        }
    }
    else
    {
        //use this for further file versions
        date_time.set(static_cast<timestamp_t>(file_header.file_time));
    }
    return date_time;
}


void getHeader(const std::string& filename,
                   FileHeader& file_header)
{
    using namespace utils5ext;

    File file;
    file.open(filename, File::om_read | File::om_shared_read);

    FileHeader original_header;
    file.readAll(&original_header, sizeof(original_header));

    CheckIfValidFile(original_header, filename);

    stream2FileHeader(original_header);

    file_header = original_header;

    file.close();
}

a_util::datetime::DateTime getDateTime(const a_util::filesystem::Path& filename)
{
    FileHeader file_header;
    getHeader(filename, file_header);
    return getDateTimeHelper(file_header);
}

void updateHeader(const std::string& filename,
                      const FileHeader& file_header,
                      uint32_t mask)
{
    using namespace utils5ext;
    if (mask == 0)
    {
        return;
    }

    if (!a_util::filesystem::exists(filename))
    {
        throw std::runtime_error("file does not exist: " + filename);
    }

    File file;
    file.open(filename, File::om_read_write | File::om_shared_write | File::om_shared_read);

    FileHeader original_header;
    file.readAll(&original_header, sizeof(original_header));

    CheckIfValidFile(original_header, filename);

    // check the version.
    // check for old versions first
    uint32_t version_helper = original_header.version_id;
    if (ifhd::getCurrentPlatformByteorder() == Endianess::platform_big_endian)
    {
        version_helper = a_util::memory::swapEndianess(version_helper);
    }

    // this might also be true for future file version on big endian machines, but is very unlikely
    // unfotunately there is no other way
    if (version_helper == 0x0110 ||
        version_helper == 0x0100)
    {
        // its an ADTF 1.x file we cannot update the header
        throw std::runtime_error("unsupported file version");
    }
    
    if ((mask & fm_description) != 0)
    {
        a_util::memory::copy(&original_header.description, sizeof(original_header.description), file_header.description, sizeof(file_header.description));
    }

    if ((mask & fm_date_time) != 0)
    {
        original_header.file_time = file_header.file_time;
        if (original_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
        {
            original_header.file_time = a_util::memory::swapEndianess(original_header.file_time);
        }
    }

    file.setFilePos(0, File::fp_begin);
    file.writeAll(&original_header, sizeof(original_header));
    file.close();
}

void queryFileInfo(const std::string& filename, std::string& file_info)
{
    std::list<std::string> ext;
    ext.push_back("dummy");
    return queryFileInfo(filename, file_info, ext);
}

void queryFileInfo(const std::string& filename,
    std::string& file_info,
    std::list<std::string>& extensions)
{
    file_info = std::string();

    using namespace utils5ext;
    File file;
    file.open(filename, File::om_read | File::om_shared_read);

    FileHeader file_header;
    file.readAll(&file_header, sizeof(file_header));

    CheckIfValidFile(file_header, filename);

    stream2FileHeader(file_header);

    a_util::datetime::DateTime date_time = getDateTimeHelper(file_header);

    // Data size
    const char* data_extension;
    double data_size;

    // File size on disk
    const char* file_extension;
    double file_size = static_cast<double>(file.getSize());

    if ((1024 * 1024) > file_header.data_size)
    {
        data_size = static_cast<double>(file_header.data_size) / 1024.0;
        data_extension = "kB";
    }
    else if ((1024 * 1024 * 1024) > file_header.data_size)
    {
        data_size = static_cast<double>(file_header.data_size) / (1024.0 * 1024.0);
        data_extension = "MB";
    }
    else if ((1024 * 1024 * 1024 * 1024LL) > file_header.data_size)
    {
        data_size = static_cast<double>(file_header.data_size) / (1024.0 * 1024.0 * 1024.0);
        data_extension = "GB";
    }
    else 
    {
        data_size = static_cast<double>(file_header.data_size) / static_cast<double>(1024 * 1024 * 1024 * 1024LL);
        data_extension = "TB";
    }

    if ((1024 * 1024) > static_cast<int64_t>(file_size))
    {
        file_size = file_size / 1024.0;
        file_extension = "kB";
    }
    else if ((1024 * 1024 * 1024) > static_cast<int64_t>(file_size))
    {
        file_size = file_size / (1024.0 * 1024.0);
        file_extension = "MB";
    }
    else if ((1024 * 1024 * 1024 * 1024LL) > static_cast<int64_t>(file_size))
    {
        file_size = file_size / (1024.0 * 1024.0 * 1024.0);
        file_extension = "GB";
    }
    else 
    {
        file_size = file_size / static_cast<double>(1024 * 1024 * 1024 * 1024LL);
        file_extension = "TB";
    }

    char info[512];
    //Conversion from uint64_t to int64_t is ok, filesize should not exceed range
    sprintf(info,
        "\n%02d.%02d.%04d %02d:%02d [Data: %.01f %s / File: %.01f %s]",
        date_time.getDay(),
        date_time.getMonth(),
        date_time.getYear(),
        date_time.getHour(),
        date_time.getMinute(),
        data_size,
        data_extension, 
        file_size,
        file_extension);

    file_info = info;

    std::string description((const char*) file_header.description);
    a_util::strings::trim(description);
    if (!description.empty())
    {
        file_info.append("\n");
        file_info.append(description);
    }

    file.setFilePos(file_header.extension_offset, File::fp_begin);

    size_t num_extensions = file_header.extension_count;
    FileExtension* extension_tab = new FileExtension[num_extensions];
    size_t expected_size = sizeof(FileExtension) * num_extensions;
    try
    {
        file.readAll(extension_tab, expected_size);
    }
    catch(...)
    {
        file.close();
        delete [] extension_tab;
        throw;
    }

    FileExtension* extension_work = extension_tab;
    for (size_t extension_idx = 0; extension_idx < num_extensions; extension_idx++)
    {
        try
        {
            file.setFilePos(extension_work->data_pos, File::fp_begin);
        }
        catch (...)
        {
            file.close();
            delete [] extension_tab;
            throw;
        }

        extensions.push_back(std::string((const char*)extension_work->identifier));
        extension_work++;
    }

    delete [] extension_tab;
    file.close();
}


  /*
void IndexedFileHelper::CheckFile(const Filename& filename, std::string& error, const Filename& mD5File)
{
        // Check MD5 digest when it exists
    Filename checksumFile = mD5File;
    if (checksumFile.IsEmpty())
    {
        checksumFile = filename;
        checksumFile.SetExtension("md5sum");
    }
    if (FileSystem::Exists(checksumFile))
    {
        std::string digest;
        void result = mD5Checksum::CalcFile(filename, digest);
        if (IS_FAILED(result))
        {
            error = std::string::Format("Unable to calculate MD5 digest of %s", filename.GetPtr());
            RETURN_ERROR(result);
        }

        std::string fileDigest;
        if (FileSystem::ReadTextFile(checksumFile, fileDigest).IsFailed())
        {
            error = std::string::Format("Unable to read MD5 digest from %s", checksumFile.GetPtr());
            RETURN_ERROR(ERR_OPEN_FAILED);
        }

        if (fileDigest.GetLength() < 32)
        {
            error = std::string::Format("Invalid content in MD5 digest file %s", checksumFile.GetPtr());
            RETURN_ERROR(ERR_INVALID_FILE);
        }

        fileDigest = fileDigest.Left(32);
        if (fileDigest != digest)
        {
            error = std::string::Format("The file %s seems to be corrupt", filename.GetPtr());
            RETURN_ERROR(ERR_INVALID_FILE);
        }
    }

    IFHD_RETURN_NOERROR;
}            */

// we cache both extracted and checked files
typedef std::map<a_util::filesystem::Path, a_util::filesystem::Path> ExtractionCache;
static ExtractionCache extraction_cache;

void isIfhdFile(const std::string& filename)
{
    using namespace utils5ext;
    File file;
    file.open(filename, File::om_read | File::om_shared_read);

    FileHeader file_header;
    file.readAll(&file_header, sizeof(file_header));

    CheckIfValidFile(file_header, filename);
    
    file.close();
}

void getExtension(const std::string& filename,
                      const std::string& extension,
                      FileExtension* extension_info,
                      void** data)
{
    using namespace utils5ext;
    File file;
    file.open(filename, File::om_read | File::om_shared_read);

    FileHeader file_header;
    //Conversion is ok, sizeof(fileHeader) should not exceed range
    file.readAll(&file_header, sizeof(file_header));

    CheckIfValidFile(file_header, filename);

    stream2FileHeader(file_header); // this sorts out ADTF 1.x files

    file.setFilePos(file_header.extension_offset, File::fp_begin);

    size_t num_extensions = static_cast<size_t>(file_header.extension_count);
    FileExtension* extension_tab = new FileExtension[num_extensions];
    size_t size_expected = sizeof(FileExtension) * num_extensions;
    try
    {
        file.readAll(extension_tab, size_expected);
    }
    catch(...)
    {
        file.close();
        delete [] extension_tab;
        throw;
    }
    
    stream2FileHeaderExtension(file_header, extension_tab, num_extensions);

    FileExtension* extension_work = extension_tab;
    for (size_t extension_idx = 0; extension_idx < num_extensions; extension_idx++)
    {
        try
        {
            file.setFilePos(extension_work->data_pos, File::fp_begin);
        }
        catch (...)
        {
            file.close();
            delete [] extension_tab;
            throw;
        }

        if (a_util::strings::compare((const char*)extension_work->identifier, extension.c_str()) == 0)
        {
            size_t byte_to_write = (size_t)extension_work->data_size;
            *data = new uint8_t[byte_to_write];

            try
            {
                file.readAll(*data, byte_to_write);
            }
            catch(...)
            {
                file.close();
                delete [] (uint8_t*)*data;
                delete [] extension_tab;
                throw;
            }

            a_util::memory::copy(extension_info, sizeof(FileExtension), extension_work, sizeof(FileExtension));
            delete [] extension_tab;
            file.close();
            return;
        }
        extension_work++;
    }

    delete [] extension_tab;
    file.close();

    throw std::runtime_error("extension not found: " + extension);
}

void writeExtension(const std::string& filename,
                        const FileExtension& extension_info,
                        const void* data)
{
    using namespace utils5ext;
    // Adding a GUID is just allowed when a new/edited dat-file is written
    std::string i_d = (char*)extension_info.identifier;
    if (i_d == std::string("GUID"))
    {
        throw std::invalid_argument("The GUID extension is not publicly writeable");
    }

    File file;
    file.open(filename, File::om_read_write);

    // Read file header from file
    FileHeader file_header;
    file.readAll(&file_header, sizeof(file_header));

    CheckIfValidFile(file_header, filename);

    stream2FileHeader(file_header); // this sorts out ADTF 1.x files

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        throw std::runtime_error("Extension write Support is currently not available for mixed byte ordering (file/architecture)");
    }

    // Read extension table from file
    file.setFilePos(file_header.extension_offset, File::fp_begin);

    size_t num_extensions = static_cast<size_t>(file_header.extension_count);
    std::auto_ptr<ArrDelAdapter<FileExtension> > extension_tab(
        new ArrDelAdapter<FileExtension>(new FileExtension[num_extensions + 1] ));

    size_t size_expected = sizeof(FileExtension) * num_extensions;
    file.readAll(extension_tab.get()->get(), size_expected);

    stream2FileHeaderExtension(file_header, extension_tab.get()->get(), num_extensions);

    // Pointer to entry for new extension in extension table
    FileExtension* extension_work = extension_tab.get()->get() + num_extensions;

    // Search new extension in extension table
    bool extension_exists = false;
    size_t extension_work_idx = 0;
    for (size_t extension_idx = 0;  extension_idx < num_extensions;  ++extension_idx)
    {
        FileExtension* ext = extension_tab.get()->get() + extension_idx;
        if (a_util::strings::isEqual((const char*)ext->identifier, (const char*)extension_info.identifier))
        {
            extension_work_idx = extension_idx;
            extension_work = ext;
            extension_exists = true;
            break;
        }
    }

    // Extension does not exists in extenstion table -> add new extension
    if (!extension_exists)
    {
        // Store new extension at end of existing extensions
        *extension_work = extension_info;
        extension_work->data_pos = file_header.extension_offset;

        //Conversion from uint64_t to int64_t is ok, extensionWork->dataPos should not exceed the range of int64_t,
        //since the first parameter of SetFilePos() is treated as int64_t anyway. extensionWork->dataPos will be changed to
        //int64_t in a future version.
        file.setFilePos(extension_work->data_pos, File::fp_begin);

        size_t bytes_to_write = static_cast<size_t>(extension_work->data_size);

        file.writeAll(data, bytes_to_write);

        stream2FileHeaderExtension(file_header, extension_tab.get()->get(), num_extensions + 1);
        // Store extension table
        size_t ext_tab = sizeof(FileExtension) * (num_extensions + 1);
        file.writeAll(extension_tab.get()->get(), ext_tab);

        // Update file header
        
        file_header.extension_count = static_cast<uint32_t>(num_extensions + 1);
        file_header.extension_offset = extension_work->data_pos + extension_work->data_size;

        file.setFilePos(0, File::fp_begin);

        stream2FileHeader(file_header);
        file.writeAll(&file_header, sizeof(file_header));
    }
    // Update existing extension, if new size is smaller or equal of existing extension, or if its the last extension
    else if (extension_info.data_size <= extension_work->data_size || extension_work_idx == num_extensions - 1)
    {
        uint64_t ext_pos = extension_work->data_pos;
        *extension_work = extension_info;
        extension_work->data_pos = ext_pos;

        // Write extension data
        //Conversion from uint64_t to int64_t is ok, extensionWork->dataPos should not exceed the range of int64_t,
        //since the first parameter of SetFilePos() is treated as int64_t anyway. extensionWork->dataPos will be changed to
        //int64_t in a future version.
        file.setFilePos(ext_pos, File::fp_begin);
        size_t bytes_to_write = static_cast<size_t>(extension_work->data_size);
        file.writeAll(data, bytes_to_write);

        // Write extension table
        file.setFilePos(file_header.extension_offset, File::fp_begin);

        stream2FileHeaderExtension(file_header, extension_tab.get()->get(), num_extensions);
        size_t ext_tab = sizeof(FileExtension) * num_extensions;
        file.writeAll(extension_tab.get()->get(), ext_tab);
    }
    // Update all extensions and extension table, if new size is larger than existing extension
    else
    {
        // Read all extensions following new extension into temporary memory
        uint64_t next_ext_pos = (extension_work_idx < num_extensions - 1) ?
            extension_tab.get()->get()[extension_work_idx + 1].data_pos : file_header.extension_offset;

        size_t buffer_size = size_t(file_header.extension_offset - next_ext_pos);
        a_util::memory::MemoryBuffer ext_data(buffer_size);

        if (buffer_size > 0)
        {
            // Read following extensions
            //Conversion from uint64_t to int64_t is ok, extensionWork->dataPos should not exceed the range of int64_t,
            //since the first parameter of SetFilePos() is treated as int64_t anyway. extensionWork->dataPos will be changed to
            //int64_t in a future version.
            file.setFilePos(next_ext_pos, File::fp_begin);
            file.readAll(ext_data.getPtr(), buffer_size);
        }

        // Write new extension data
        uint64_t ext_pos = extension_work->data_pos;
        //Conversion from uint64_t to int64_t is ok, extensionWork->dataPos should not exceed the range of int64_t,
        //since the first parameter of SetFilePos() is treated as int64_t anyway. extensionWork->dataPos will be changed to
        //int64_t in a future version.
        file.setFilePos(ext_pos, File::fp_begin);
        size_t bytes_to_write = static_cast<size_t>(extension_info.data_size);
        file.writeAll(data, bytes_to_write);
        uint64_t ext_enlargement = extension_info.data_size - (next_ext_pos - ext_pos);

        if (buffer_size > 0)
        {
            file.writeAll(ext_data.getPtr(), buffer_size);

            // Update extension table of following extensions
            for (size_t extension_idx = extension_work_idx + 1; extension_idx < num_extensions; ++extension_idx)
            {
                FileExtension* ext = extension_tab.get()->get() + extension_idx;
                ext->data_pos += ext_enlargement;
            }
        }

        *extension_work = extension_info;
        extension_work->data_pos = ext_pos;
        file_header.extension_offset += ext_enlargement;

        stream2FileHeaderExtension(file_header, extension_tab.get()->get(), num_extensions);
        size_t ext_tab = sizeof(FileExtension) * num_extensions;
        file.writeAll(extension_tab.get()->get(), ext_tab);

        // Update file header
        file.setFilePos(0, File::fp_begin);

        stream2FileHeader(file_header);
        file.writeAll(&file_header, sizeof(file_header));
    }
}

void stream2FileHeader(FileHeader& file_header)
{
    /*
    typedef struct tagFileHeader
        {
            uint32_t fileId;
            uint32_t versionId;
            uint32_t flags;
            uint32_t extensionCount;
            uint64_t extensionOffset;
            uint64_t dataOffset;
            uint64_t dataSize;
            uint64_t chunkCount;
            uint64_t maxChunkSize;
            uint64_t duration;
            uint64_t fileTime;
            uint8_t  headerByteOrder;
            uint64_t                         timeOffset;
            uint8_t                          patchNumber;
            uint64_t                         firstChunkOffset;
            uint64_t                         continuousOffset;
            int8_t                           _reserved[38];
            int8_t                           description[1912];
        } FileHeader _set_aligment_1_;
    */
    
    // check the version.
    // check for old versions first
    uint32_t version_helper = file_header.version_id;
    if (ifhd::getCurrentPlatformByteorder() == Endianess::platform_big_endian)
    {
        version_helper = a_util::memory::swapEndianess(version_helper);
    }

    // this might also be true for future file version on big endian machines, but is very unlikely
    // unfortunately there is no other way
    if (version_helper == 0x0110 ||
        version_helper == 0x0100)
    {
        // its an ADTF 1.x file which are always little endian, we cannot handle it here
        throw std::runtime_error("unsupported file version");
    }

    // its an ADTF 2.x file
    auto byte_order = static_cast<Endianess>(file_header.header_byte_order);
    if (byte_order == Endianess::platform_not_supported ||
        (byte_order != Endianess::platform_little_endian &&
         byte_order != Endianess::platform_big_endian))
    {
        throw std::runtime_error("unsupported file");
    }

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        //a_util::memory::swapEndianess(&fileHeader->fileId);
        file_header.version_id = a_util::memory::swapEndianess(file_header.version_id);
        file_header.flags = a_util::memory::swapEndianess(file_header.flags);
        file_header.extension_count = a_util::memory::swapEndianess(file_header.extension_count);
        file_header.extension_offset = a_util::memory::swapEndianess(file_header.extension_offset);
        file_header.data_offset = a_util::memory::swapEndianess(file_header.data_offset);
        file_header.data_size = a_util::memory::swapEndianess(file_header.data_size);
        file_header.chunk_count = a_util::memory::swapEndianess(file_header.chunk_count);
        file_header.max_chunk_size = a_util::memory::swapEndianess(file_header.max_chunk_size);
        file_header.duration = a_util::memory::swapEndianess(file_header.duration);
        file_header.file_time = a_util::memory::swapEndianess(file_header.file_time);
        //fileHeader->headerByteOrder //muss nicht
        file_header.time_offset = a_util::memory::swapEndianess(file_header.time_offset);
        //fileHeader->patchNumber //muss nicht
        file_header.first_chunk_offset = a_util::memory::swapEndianess(file_header.first_chunk_offset);
        file_header.continuous_offset = a_util::memory::swapEndianess(file_header.continuous_offset);
        file_header.ring_buffer_end_offset = a_util::memory::swapEndianess(file_header.ring_buffer_end_offset);
        //fileHeader->_reserved //muss nicht
        //fileHeader->description //muss nicht
    }    
}

void stream2FileHeaderExtension(const FileHeader& file_header, FileExtension* header_ext, size_t num_extensions)
{
    /*
         typedef struct tagFileExtension
        {
            int8_t   identifier[384];
            uint16_t streamId;
            uint8_t  _reserved1[2];
            uint32_t userId;
            uint32_t typeId;
            uint32_t versionId;
            uint64_t dataPos;
            uint64_t dataSize;
            uint8_t  _reserved2[96];
        } FileExtension _set_aligment_1_;  // size is 512 Bytes
    */

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        for (size_t idx=0;
             idx < num_extensions;
             ++idx)
        {
            header_ext->stream_id = a_util::memory::swapEndianess(header_ext->stream_id);
            header_ext->user_id = a_util::memory::swapEndianess(header_ext->user_id);
            header_ext->type_id = a_util::memory::swapEndianess(header_ext->type_id);
            header_ext->version_id = a_util::memory::swapEndianess(header_ext->version_id);
            header_ext->data_pos = a_util::memory::swapEndianess(header_ext->data_pos);
            header_ext->data_size = a_util::memory::swapEndianess(header_ext->data_size);
            header_ext++;
        }
    }       
}

void stream2ChunkHeader(const FileHeader& file_header, ChunkHeader& chunk)
{
    /*
    typedef struct tagChunkHeader
        {
            uint64_t timeStamp;
            uint32_t refMasterTableIndex; // TODO: 64 Bit ??
            uint32_t offsetToLast;
            uint32_t size;
            uint16_t streamId;
            uint16_t flags;
            uint64_t streamIndex;
        } ChunkHeader _set_aligment_1_;  // size is 32 Bytes
        */

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        chunk.time_stamp = a_util::memory::swapEndianess(chunk.time_stamp);
        chunk.ref_master_table_index = a_util::memory::swapEndianess(chunk.ref_master_table_index);
        chunk.offset_to_last = a_util::memory::swapEndianess(chunk.offset_to_last);
        chunk.size = a_util::memory::swapEndianess(chunk.size);
        chunk.stream_id = a_util::memory::swapEndianess(chunk.stream_id);
        chunk.flags = a_util::memory::swapEndianess(chunk.flags);
        chunk.stream_index = a_util::memory::swapEndianess(chunk.stream_index);
    }
}

void stream2ChunkRef(const FileHeader& file_header, ChunkRef& chunk_ref)
{
    /*
    typedef struct tagChunkRef
        {
            uint64_t timeStamp;
            uint32_t size;
            uint16_t streamId;
            uint16_t flags;
            uint64_t chunkOffset;
            uint64_t chunkIndex;
            uint64_t streamIndex;
            uint32_t refStreamTableIndex; // TODO: 64 Bit ??
        } chunkRef _set_aligment_1_;    // size is 44 Bytes
    */

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        chunk_ref.time_stamp = a_util::memory::swapEndianess(chunk_ref.time_stamp);
        chunk_ref.size = a_util::memory::swapEndianess(chunk_ref.size);
        chunk_ref.stream_id = a_util::memory::swapEndianess(chunk_ref.stream_id);
        chunk_ref.flags = a_util::memory::swapEndianess(chunk_ref.flags);
        chunk_ref.chunk_offset = a_util::memory::swapEndianess(chunk_ref.chunk_offset);
        chunk_ref.chunk_index = a_util::memory::swapEndianess(chunk_ref.chunk_index);
        chunk_ref.stream_index = a_util::memory::swapEndianess(chunk_ref.stream_index);
        chunk_ref.ref_stream_table_index = a_util::memory::swapEndianess(chunk_ref.ref_stream_table_index);
    }
}

void stream2StreamRef(  const FileHeader& file_header, StreamRef& stream_ref)
{
    /*
    typedef struct tagStreamRef
        {
            uint32_t refMasterTableIndex; // TODO: 64 Bit ??
        } StreamRef _set_aligment_1_;    // size is 4 Bytes
        */

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        stream_ref.ref_master_table_index = a_util::memory::swapEndianess(stream_ref.ref_master_table_index);
    }
}

void stream2AdditionalStreamIndexInfo(const FileHeader& file_header, AdditionalIndexInfo& additonal_index_info)
{
    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        additonal_index_info.stream_table_index_offset = a_util::memory::swapEndianess(additonal_index_info.stream_table_index_offset);
        additonal_index_info.stream_index_offset = a_util::memory::swapEndianess(additonal_index_info.stream_index_offset);
    }
}

void stream2StreamInfoHeader(const FileHeader& file_header, StreamInfoHeader& stream_info)
{
    /*
        typedef struct tagStreamInfoHeader
        {
            uint64_t streamIndexCount;
            uint64_t streamFirstTime;
            uint64_t streamLastTime;
            uint32_t infoDataSize;
            int8_t   streamName[228]; //us Ascii 7
        } StreamInfoHeader _set_aligment_1_; // size is 256 Byte
        */

    if (file_header.header_byte_order != PLATFORM_BYTEORDER_UINT8)
    {
        stream_info.stream_index_count = a_util::memory::swapEndianess(stream_info.stream_index_count);
        stream_info.stream_first_time = a_util::memory::swapEndianess(stream_info.stream_first_time);
        stream_info.stream_last_time = a_util::memory::swapEndianess(stream_info.stream_last_time);
        stream_info.info_data_size = a_util::memory::swapEndianess(stream_info.info_data_size);
    }
}

} //v400

} // namespace
