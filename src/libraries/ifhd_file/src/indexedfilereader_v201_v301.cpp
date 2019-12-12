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

#define ADTF_RING_BUFFER_HANDLING_PRE_2_13_1

static uint8_t chunk_fill_buffer[16] = {0};  // chunks are filled up to 16 byte boundaries


namespace ifhd
{
namespace v201_v301
{

#define DELEGATE_PTR(pObj) ((ifhd::v110::IndexedFileReaderV110*) (pObj))

/**************************************/
/* This still needs to be implemented.*/
/**************************************/
class IndexedFileReader::IndexedFileReaderImpl
{
    public:
        bool                 chunk_header_search_possible;
        bool                 fast_check_possible;
        std::map<uint32_t, uint64_t> map_last_valid_stream_index;
#ifdef ADTF_RING_BUFFER_HANDLING_PRE_2_13_1
        bool                 pos_reset;
#endif

        typedef struct
        {
            bool                      set;
            FilePos                  file_pos;
            int64_t                   valid_chunk_index;
            ChunkHeader              valid_chunk_header;
        } SearchRef;

        SearchRef last_fast;
        SearchRef last_read;

        //index offset
        std::map<int64_t, FilePos> map_file_pos_offsets;
        IndexedFileReader* p = nullptr;

    public:
        explicit IndexedFileReaderImpl(IndexedFileReader& parent)
        {
            p = &parent;
            chunk_header_search_possible = false;
            fast_check_possible         = true;
            a_util::memory::set(&last_fast, sizeof(last_fast), 0x00, sizeof(last_fast));
            last_fast.set = false;
            a_util::memory::set(&last_read, sizeof(last_read), 0x00, sizeof(last_read));
            last_read.set = false;
#ifdef ADTF_RING_BUFFER_HANDLING_PRE_2_13_1
            pos_reset = true;
#endif
        }

        virtual ~IndexedFileReaderImpl()
        {
            map_file_pos_offsets.clear();
        }

        void SetRealValidChunkHeader(const ChunkHeader* header,
                                        const FilePos file_pos,
                                        const int64_t valid_chunk_index)
        {
            if (map_file_pos_offsets.empty())
            {
                last_read.set     = true;
                last_read.file_pos = file_pos;
                last_read.valid_chunk_index = valid_chunk_index;
                last_read.valid_chunk_header = *header;
            }
            else
            {
              //???
            }
        }

        bool ValidateHeader(const ChunkHeader* header)
        {
            //the fast check only checks the ranges
            if (p->_index_table.validateRawMasterIndex(header->ref_master_table_index)
                && header->stream_id > 0
                && header->stream_id <= MAX_INDEXED_STREAMS
                && header->time_stamp >= p->_file_header->time_offset
                && header->time_stamp <= (p->_file_header->time_offset + p->_file_header->duration)
                && header->size <= p->_file_header->max_chunk_size)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        bool ValidateHeader(const ChunkHeader* header,
                            const int64_t file_pos,
                            const int64_t valid_chunk_index)
        {
            bool res = ValidateHeader(header);
            if (res)
            {
                last_fast.set     = true;
                last_fast.file_pos = file_pos;
                last_fast.valid_chunk_index  = valid_chunk_index;
                last_fast.valid_chunk_header = *header;
            }
            return res;
        }

        bool TryToSearchValidNextChunkHeader(ChunkHeader** valid_header,
                                             FilePos& current_pos,
                                             int64_t& current_index)
        {
            FilePos offset     = 16;
            std::map<int64_t, FilePos>::iterator it = map_file_pos_offsets.find(current_index);
            if (it != map_file_pos_offsets.end())
            {
                offset = it->second;
            }

            SearchRef* search_ref = nullptr;
            int64_t index_to_search = current_index;
            if (last_fast.set && current_index >= last_fast.valid_chunk_index)
            {
                search_ref = &last_fast;
                if (current_index == last_fast.valid_chunk_index+1)
                {
                    current_index = last_fast.valid_chunk_index;
                }
                else
                {
                    current_index = last_fast.valid_chunk_index;
                    current_pos = last_fast.file_pos;
                }
            }
            else if (last_read.set && current_index >= last_fast.valid_chunk_index)
            {
                search_ref = &last_read;
                current_index = last_read.valid_chunk_index;
                current_pos = last_read.file_pos;
            }
            else
            {
                current_index = 0;
                current_pos = sizeof(FileHeader);
            }
            current_pos += offset;
            p->_file_pos_invalid = true;
            while ((current_pos + static_cast<FilePos>(sizeof(ChunkHeader))
                      <= p->_end_of_data_marker)
                   )
            {
                ChunkHeader new_search_header;

                try
                {
                    p->_file_pos_invalid = true;
                    p->checkFilePtr();
                    p->readDataBlock(&new_search_header,sizeof(ChunkHeader));
                }
                catch (...)
                {
                    p->setEOF();
                    return false;
                }

                {
                    if (ValidateHeader(&new_search_header))
                    {
                        if (search_ref != nullptr)
                        {
                            if (new_search_header.time_stamp >= search_ref->valid_chunk_header.time_stamp
                                && new_search_header.ref_master_table_index >= search_ref->valid_chunk_header.ref_master_table_index
                                && new_search_header.offset_to_last > search_ref->valid_chunk_header.size)
                            {
                                ValidateHeader(&new_search_header,
                                               current_pos,
                                               ++current_index);
                            }
                            else
                            {

                            }
                        }
                        else
                        {
                            ValidateHeader(&new_search_header,
                                            current_pos,
                                            ++current_index);
                        }
                        if (current_index == index_to_search)
                        {
                            *(*valid_header) = new_search_header;
                            map_file_pos_offsets[current_index] = current_pos;
                            return true;
                        }
                    }
                    offset += 16;
                    current_pos += 16;
                }
            }
            return false;
        }
};

IndexedFileReader::IndexedFileReader()
{
    _d.reset(new IndexedFileReaderImpl(*this));
    initialize();
}

IndexedFileReader::~IndexedFileReader()
{
    _d.release();
    close();
}

/**
*   Initializes the values
*
*   @returns Standard Result
*/
void IndexedFileReader::initialize()
{
    IndexedFile::initialize();

    _delegate          = nullptr;

    _flags          = 0;
    _write_mode         = false;
    _end_of_data_marker   = 0;

    _header_valid       = false;
    _data_valid         = false;
    _prefetched        = false;
    _chunk_index        = 0;
    _index_table_index   = 0;
    _current_chunk_data = nullptr;
    _file_pos_invalid    = true;
    _compatibility_v110 = false;
    _current_chunk     = nullptr;

    _filename        = "";

    _file_pos_current_chunk = 0;
}

/**
 *
 * This function opens a dat-file for reading.
 *
 * @param const string& filename [in] the file name to be opened
 * @param int cacheSize           [in] cache size; <=0: use system file caching (=default)
 * @param uint32_t flags         [in] a OpenMode value
 *
 * @returns void
 *
 */
void IndexedFileReader::open(const a_util::filesystem::Path& filename, int cache_size, uint32_t flags)
{
    using namespace utils5ext;
    close();

    _flags = flags;

    _system_cache_disabled = false;

    _filename = filename;

    uint32_t file_flags = File::om_shared_read | File::om_sequential_access | File::om_shared_write;

    // IndexedFileChanger will need write access also to write the changed extensions
    if ((flags & om_file_change_mode) == om_file_change_mode)
    {
        file_flags |= File::om_read_write;
    }
    else
    {
        file_flags |= File::om_read;
    }

    if ((flags & om_disable_file_system_cache) != 0)
    {
        file_flags |= File::om_disable_file_system_cache;
        _system_cache_disabled = true;
    }

    allocReadBuffers();

    _file.open(filename, file_flags);

    _sector_size = getSectorSizeFor(filename);
    if (_sector_size == 0)
    {
        _sector_size = default_block_size;
    }

    if (cache_size < 0)
    {
        cache_size = static_cast<int>(16 * _sector_size); // 8 kilobytes default
    }

    _file.setReadCache(cache_size);

    readFileHeader();

    if (_delegate)
    {
        return;
    }

    _index_table.create(this);

    //////////////////////////////////////////////////////////////////////////
    // The following cache is commented, because of the investigations of #9325.
    // There it was shown, that this IndexedFileReader-Cache has only a very
    // small effect for the performance of reading DAT-files. This is because,
    // the IndexedFileReader uses the File::Open() and File::Read() methods.
    // The File on the other side uses it's own cache, which is very effective.
    //////////////////////////////////////////////////////////////////////////

    //if (!_systemCacheDisabled)
    //{
    //    RETURN_IF_FAILED(AllocCache(cacheSize));
    //}

    readFileHeaderExt();

    if ((flags & om_query_info) == 0)
    {
        _index_table.readIndexTable();

        clearCache();

        _end_of_data_marker = _file_header->data_offset + _file_header->data_size;

        allocBuffer((size_t) _file_header->max_chunk_size);

        _current_chunk_data = nullptr;
        _header_valid = false;

        reset();
    }
    else    // just open for header and extension info
    {
        _current_chunk_data = nullptr;
        _header_valid       = false;
    }
}

/**
 *
 * This function closes all.
 *
 * @returns void
 *
 */
void IndexedFileReader::close()
{
    _compatibility_v110 = false;

    if (_delegate != nullptr)
    {
        delete (DELEGATE_PTR(_delegate));
        _delegate = nullptr;
    }

    _filename = "";

    freeReadBuffers();
    _index_table.free();

    return IndexedFile::close();
}

/**
*   Reads and initializes the Header struct
*
*   @returns Standard Result
*/
void IndexedFileReader::readFileHeader()
{
    using namespace utils5ext;
    allocHeader();

    clearCache();

    _file.setFilePos(0, File::fp_begin);
    _file.readAll(_file_header, sizeof(FileHeader));

    try
    {
        stream2FileHeader(*_file_header);
    }
    catch (...)
    {
        if (ifhd::getCurrentPlatformByteorder() != Endianess::platform_little_endian)
        {
          //  LOG_ERROR("ADTF 1.x files are not supported on big endian architectures.");
            throw;
        }

        // it's an adtf 1.x file
        if (_file_header->version_id == v100::version_id
            || _file_header->version_id == v110::version_id)
        {
            _compatibility_v110 = true;

            _delegate = new v110::IndexedFileReaderV110();
            DELEGATE_PTR(_delegate)->attach(&_file,
                                              _filename,
                                              -1,
                                              _flags);
            v110::IndexedFileV110::FileHeader* header;
            DELEGATE_PTR(_delegate)->getHeader(&header);
            a_util::datetime::DateTime date_time_of_file;
            DELEGATE_PTR(_delegate)->getDateTime(&date_time_of_file);
            setDateTime(date_time_of_file);
            _file_header->extension_offset = header->extension_offset;
            _file_header->header_byte_order = static_cast<uint8_t>(Endianess::platform_little_endian);
            _file_header->chunk_count = header->chunk_count;
            strncpy((char*)_file_header->description, (const char*)header->description, 1912);
            _file_header->description[1911] = 0;
            _chunk_index  = 0;
            _header_valid = false;

            return;
        }     

        throw;
    }

    _compatibility_v110 = false;
    const auto supported_versions = getSupportedVersions();
    if (supported_versions.find(_file_header->version_id) == supported_versions.end())
    {
        throw std::runtime_error("unsupported file version");
    }
    else
    {
        //there is a bug in some file,
        //because of file writing a certain amount of data, which are shit memory
        // so we need to check new patch number in Fileheader
        if (_file_header->patch_number == 0x00)
        {
           // _d->_chunkHeaderSearchPossible = true;
        }
    }

    if (_file_header->version_id < version_id_with_history)
    {
        // these have not been set previously
        _file_header->continuous_offset = _file_header->data_offset;
        _file_header->first_chunk_offset = _file_header->data_offset;
        _file_header->ring_buffer_end_offset = _file_header->data_offset;
    }
    else if (_file_header->version_id == version_id_with_history)
    {
        // This value has not been set in this file version.
        // we try to detect the end in ReadCurrentChunkHeader in this case.
        _file_header->ring_buffer_end_offset = _file_header->first_chunk_offset;
    }

    _chunk_index  = 0;
    _header_valid = false;
}

/**
*   Reads and initializes the ExtendedHeader
*
*   @returns Standard Result
*/
void IndexedFileReader::readFileHeaderExt()
{
    using namespace utils5ext;
    freeExtensions();

    int num_extensions = (int) _file_header->extension_count;
    if (num_extensions == 0)
    {
        return;
    }

    clearCache();

    _file.setFilePos(_file_header->extension_offset, File::fp_begin);

    //this is for intern
    void* header_table_buffer = internalMalloc(sizeof(FileExtension) * num_extensions, true);

    size_t expected_size = sizeof(FileExtension) * num_extensions;
    try
    {
        _file.readAll(header_table_buffer, expected_size);
    }
    catch(...)
    {
        internalFree(header_table_buffer);
        throw;
    }

    FileExtension* extension_tab = (FileExtension*) header_table_buffer;

    try
    {
        stream2FileHeaderExtension(*_file_header, extension_tab, num_extensions);
    }
    catch (...)
    {
        internalFree(header_table_buffer);
        throw;
    }

    FileExtension* extension_info = extension_tab;

    for (int extension_idx = 0; extension_idx < num_extensions; extension_idx++)
    {
        try
        {
            _file.setFilePos(extension_info->data_pos, utils5ext::File::fp_begin);
        }
        catch (...)
        {
            internalFree(header_table_buffer);
            throw;
        }

        void* extension_page;
        try
        {
            allocExtensionPage(extension_info->data_size, &extension_page);
        }
        catch (...)
        {
             internalFree(header_table_buffer);
             throw;
        }

        FileExtensionStruct* extension_struct = nullptr;
        try
        {
            extension_struct = new FileExtensionStruct;
        }
        catch (...)
        {
            internalFree(extension_page);
            internalFree(header_table_buffer);
            throw;
        }

        extension_struct->extension_page = extension_page;

        a_util::memory::copy(&extension_struct->file_extension, sizeof(FileExtension), extension_info, sizeof(FileExtension));

        size_t bytes_to_write = extension_info->data_size;
        try
        {
            _file.readAll(extension_page, bytes_to_write);
        }
        catch (...)
        {
            internalFree(extension_page);
            internalFree(header_table_buffer);
            delete extension_struct;
            throw;
        }

        _extensions.push_back(extension_struct);

        extension_info++;
    }

    internalFree(header_table_buffer);
}

/**
 *
 * This function resets the file to the beginning of data.
 *
 * @returns void
 *
 */
void IndexedFileReader::reset()
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->reset();
    }

    clearCache();

    _file_pos             = _file_header->first_chunk_offset;
    _file_pos_invalid      = true;
    _file_pos_current_chunk = _file_pos;

    _chunk_index          = 0;
    _current_chunk_data   = nullptr;
    _header_valid         = false;

    // usecase: EDS stores only files @Dat-file -> ReadCurrentHeader will Return ERR_END_OF_FILE
    if ( 0 < _file_header->chunk_count)
    {
        readCurrentChunkHeader();
    }
}

/**
 *
 * This function returns the current file position (Index or TimeStamp)
 *
 * @param int timeFormat [in] a format value (tTimeFormat)
 *
 * @returns int64_t -> the current file position (Index or TimeStamp)
 *
 */
int64_t IndexedFileReader::getCurrentPos(TimeFormat time_format) const
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

/**
 *
 * This function sets the file position.
 *
 * @param int64_t position [in] the new file position
 * @param int timeFormat [in] the format of position (tTimeFormat)
 *
 * @returns int64_t -> the current chunk index
 *
 */
int64_t IndexedFileReader::setCurrentPos(int64_t position, TimeFormat time_format)
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->setCurrentPos(position, time_format);
    }

    return seek(0, position, time_format);
}

/**
 *
 * This function sets the new file position and returns the new chunk index.
 *
 * @param  uint32_t streamId [in] the stream Id
 * @param  int64_t position     [in] the new file position
 * @param  int timeFormat     [in] the format of position (tTimeFormat)
 * @param  uint32_t flags    [in] a SeekFlags value
 *
 * @returns int64_t -> the current chunk index
 *
 */
int64_t IndexedFileReader::seek(uint16_t stream_id,
                                 int64_t position,
                                 TimeFormat time_format,
                                 uint32_t flags)
try
{
    if (nullptr != _delegate)
    {
        if (stream_id != 0)
        {
            throw std::runtime_error("in compatibility mode, seeking is only supported on stream id 0");
        }
        int seek_format_v110 = 0;
        if (time_format == tf_chunk_index)
        {
            seek_format_v110 = v110::IndexedFileV110::tf_chunk_index;
        }
        else if (time_format == tf_chunk_time)
        {
            seek_format_v110 = v110::IndexedFileV110::tf_chunk_time;
        }
        else if (time_format == tf_stream_index)
        {
            throw std::runtime_error("in compatibility mode, seeking by stream index is not supported");
        }
        uint32_t seek_flagsv110 = 0;
        if ((flags & sf_keydata) != 0)
        {
            seek_flagsv110 = seek_flagsv110 | v110::IndexedFileV110::sf_keydata;
        }
        return DELEGATE_PTR(_delegate)->seek(position, seek_format_v110, seek_flagsv110);
    }             

    _current_chunk_data            = nullptr;
    _header_valid                  = false;
    _prefetched                   = false;

    clearCache();

    int64_t master_index = 0;
    int64_t end_chunk_index = 0;
    if (time_format == tf_chunk_index && (flags & sf_keydata) != 0)
    {
        master_index = position;
    }
    else
    {
        _index_table.lookupChunkRef(stream_id, position, time_format,
                                     &_chunk_index, &_file_pos,
                                     &end_chunk_index, &master_index);
    }

    if (master_index < 0)
    {
        throw std::invalid_argument("invalid position");
    }

    _file_pos_invalid = true;

    // just seek for neares chunk using the index table
    if ((flags & sf_keydata) != 0)
    {
        _index_table.fillChunkHeaderFromIndex(static_cast<uint32_t>(master_index), _current_chunk,
                                               &_chunk_index, &_file_pos);
        return master_index;
    }

    int64_t last_matching_index = _chunk_index;
    FilePos last_matching_file_pos = _file_pos;

    checkFilePtr();

    timestamp_t first_index_time_stamp = -1;

    // seek in range between two border elements in index table
    int64_t current_index = _chunk_index;
    while (true)
    {
        if (current_index  == end_chunk_index)
        {
            throw std::runtime_error("position not found");
        }

        FilePos file_pos_before_read = _file_pos;

        readCurrentChunkHeader();
        readCurrentChunkData(_buffer);

        if (time_format == tf_chunk_index)
        {
            if (current_index == position)
            {
                break;
            }
        }
        else if (time_format == tf_chunk_time)
        {
            if ((flags & sf_before) == 0)
            {
                if ((stream_id == 0 || _current_chunk->stream_id == stream_id) &&
                    _current_chunk->time_stamp >= (uint64_t) position)
                {
                    break;
                }
            }
            else
            {
                if (first_index_time_stamp == -1)
                {
                    first_index_time_stamp = _current_chunk->time_stamp;
                }

                // if the current chunk timestamp is larger then the one we're seeking to
                // we reset the file pointer to the last matching chunk.
                // we can also stop searching after 1 second (default index delay), because otherwise
                // a new index entry would have been created (and used as a start for this search).
                if (_current_chunk->time_stamp > static_cast<uint64_t>(position) ||
                    (first_index_time_stamp != -1 &&
                    _current_chunk->time_stamp >=
                    static_cast<uint64_t>(first_index_time_stamp + 1100000)))
                {
                    current_index = _chunk_index = last_matching_index;
                    _file_pos = last_matching_file_pos;
                    _file_pos_invalid = true;
                    checkFilePtr();
                    readCurrentChunkHeader();
                    readCurrentChunkData(_buffer);
                    break;
                }

                if ((stream_id == 0 || _current_chunk->stream_id == stream_id))
                {
                    if (_current_chunk->time_stamp == (uint64_t) position)
                    {
                        // the chunk matches exactly so we're done
                        break;
                    }

                    // remember this chunk as the last matching one
                    last_matching_index = current_index;
                    last_matching_file_pos = file_pos_before_read;
                }
            }
        }
        else if (time_format == tf_stream_index)
        {
            if (_current_chunk->stream_index >= (uint64_t) position
                && _current_chunk->stream_id == stream_id)
            {
                break;
            }
        }

        current_index++;
    }

    _prefetched = true;
    _chunk_index = (uint64_t) current_index;
    _header_valid = true;

    return current_index;
}
catch (...)
{
    setEOF();
    throw;
}

/**
 *
 * This function returns the duration of the current file [microsec].
 *
 * @returns timestamp_t
 *
 */
timestamp_t IndexedFileReader::getDuration() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getDuration();
    }
    if (nullptr == _file_header)
    {
        return 0;
    }
    return _file_header->duration;
}

timestamp_t IndexedFileReader::getTimeOffset() const
{
    if (nullptr != _delegate)
    {
        return 0;
    }
    if (nullptr == _file_header)
    {
        return 0;
    }
    return _file_header->time_offset;
}

/**
 *
 * This function returns the current chunk index.
 *
 *  @returns int64_t
 *
 */
int64_t IndexedFileReader::getFilePos() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getFilePos();
    }

    return _chunk_index;
}

/**
 *
 * This function returns the number of chunks of the current file.
 *
 * @returns int64_t
 *
 */
int64_t IndexedFileReader::getChunkCount() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getChunkCount();
    }
    if (nullptr == _file_header)
    {
        return 0;
    }
    return _file_header->chunk_count;
}

bool IndexedFileReader::streamExists(uint16_t stream_id) const
{
    if (_delegate != nullptr)
    {
        return false;
    }
    return _index_table.streamExists(stream_id);
}

std::string IndexedFileReader::getStreamName(uint16_t stream_id) const
{
    if (_delegate != nullptr)
    {
        throw std::out_of_range("no stream_id exists");
    }

    return _index_table.getStreamName(stream_id);
}

/**
 *
 * This function gets the tSampleStreamHeader data of the given stream.
 *
 * @param  uint32_t streamId [in]  the stream Id
 * @param  void** infoData    [out] the tSampleStreamHeader data
 * @param  unsigned int*  infoSize   [out] the size of the returned data
 *
 * @return void
 *
 */
void IndexedFileReader::getAdditionalStreamInfo(uint16_t stream_id,
                                                    const void** info_data,
                                                    size_t*  info_size) const
{
    if (nullptr != _delegate)
    {
        throw std::runtime_error("not supported for this file version");
    }

    if (stream_id == 0)
    {
        throw std::invalid_argument("addintional stream info is only available for stream ids > 0");
    }
    else
    {
        *info_data = nullptr;
        *info_size = 0;

        const void *additional_info = _index_table.getAdditionalStreamInfo(stream_id);
        const StreamInfoHeader *stream_info_header = _index_table.getStreamInfo(stream_id);

        if (nullptr != additional_info &&
            nullptr != stream_info_header)
        {
            *info_data = additional_info;
            *info_size = stream_info_header->info_data_size;
        }
        else
        {
            throw std::runtime_error("no additional stream info available");
        }
    }
}

/**
 *
 * This function gets the first time stamp of the given stream.
 *
 * @param  uint32_t streamId           [in]  the stream Id
 *
 * @return timestamp_t
 *
 */
timestamp_t IndexedFileReader::getFirstTime(uint16_t stream_id) const
{
    if (nullptr != _delegate)
    {
        return 0;
    }

    return _index_table.getFirstTime(stream_id);
}

/**
 *
 * This function gets the last time stamp of the given stream.
 *
 * @param  uint32_t streamId [in]  the stream Id
 *
 * @return timestamp_t
 *
 */
timestamp_t IndexedFileReader::getLastTime(uint16_t stream_id) const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getDuration();
    }

    return _index_table.getLastTime(stream_id);
}

/**
 *
 * This function gets the number of chunks of the given stream.
 *
 * @param  uint32_t streamId [in]  the stream Id
 *
 * @return int64_t
 *
 */
int64_t IndexedFileReader::getStreamIndexCount(uint16_t stream_id) const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getChunkCount();
    }
    if (stream_id == 0)
    {
        return getChunkCount();
    }
    else
    {
        const StreamInfoHeader *stream_info_header = _index_table.getStreamInfo(stream_id);
        if (nullptr != stream_info_header)
        {
            return stream_info_header->stream_index_count;
        }
        else
        {
            return -1;
        }
    }
}

/**
 *
 * This function gets the number of indices of the given stream.
 *
 * @param  uint32_t streamId [in]  the stream Id
 *
 * @return int64_t
 *
 */
int64_t IndexedFileReader::getStreamTableIndexCount(uint16_t stream_id) const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getIndexCount();
    }

    return _index_table.getItemCount(stream_id);
}

/**
 *
 * Returns the Version Id of the current file.
 *
 * @returns uint32_t
 *
 */
uint32_t IndexedFileReader::getVersionId() const
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->getVersionId();
    }
    if (nullptr == _file_header)
    {
        return 0;
    }
    return _file_header->version_id;
}

void IndexedFileReader::checkFilePtr()
try
{
    using namespace utils5ext;
    if (_file_pos < 0 || _chunk_index < 0)
    {
        throw std::runtime_error("invalid file position");
    }

    IFHD_ASSERT((_file_pos & 0xF) == 0);

    if (_file_pos_invalid == true)
    {
        _file.setFilePos(_file_pos, File::fp_begin);
        _file_pos_invalid = false;

#ifdef ADTF_RING_BUFFER_HANDLING_PRE_2_13_1
        _d->pos_reset = true;
#endif
    }
}
catch(...)
{
    setEOF();
    throw;
}

void IndexedFileReader::readCurrentChunkHeader()
try
{
    if (_chunk_index < 0 || (uint64_t) _chunk_index >= _file_header->chunk_count)
    {
        throw exceptions::EndOfFile();
    }

    _current_chunk_data = nullptr;

    if (static_cast<int64_t>(_file_pos + sizeof(ChunkHeader)) > _end_of_data_marker)
    {
        throw exceptions::EndOfFile(); // EOF if chunk size exceeds data region
    }

#ifdef ADTF_RING_BUFFER_HANDLING_PRE_2_13_1
    // special handling for incomplete buffer info for ADTF 2.13.0 files
    timestamp_t last_chunk_time = 0;
    FilePos last_chunk_pos = -1;
    if (_file_header->version_id == version_id_with_history &&
        _current_chunk &&
        !_d->pos_reset && // we are during a continuous read sequence (no seeking)
        _chunk_index > 0)
    {
        last_chunk_time = _current_chunk->time_stamp;
        last_chunk_pos = _file_pos_current_chunk;
    }
#endif

    _file_pos_current_chunk = _file_pos;

    checkFilePtr();
    readDataBlock(_current_chunk, sizeof(ChunkHeader));
    stream2ChunkHeader(*_file_header, *_current_chunk);

    if (_d->chunk_header_search_possible)
    {
        if (!_d->ValidateHeader(_current_chunk, _file_pos, _chunk_index))
        {
            if (!_d->TryToSearchValidNextChunkHeader(&_current_chunk, _file_pos, _chunk_index))
            {
                throw std::runtime_error("unable to find next chunk");
            }
            _file_pos_current_chunk = _file_pos;
        }
    }

#ifdef ADTF_RING_BUFFER_HANDLING_PRE_2_13_1
    // special handling for incomplete buffer info for ADTF 2.13.0 files
    if (_file_header->version_id == version_id_with_history &&
        last_chunk_pos != -1 &&
        _file_pos_current_chunk < static_cast<FilePos>(_file_header->continuous_offset) &&
        _file_pos_current_chunk != static_cast<FilePos>(_file_header->first_chunk_offset) &&
        _file_pos_current_chunk > static_cast<FilePos>(_file_header->data_offset))
    {
        if (_file_pos_current_chunk - _current_chunk->offset_to_last != last_chunk_pos ||
            static_cast<timestamp_t>(_current_chunk->time_stamp) < last_chunk_time ||
            _current_chunk->stream_id > MAX_INDEXED_STREAMS ||
            _current_chunk->stream_id == 0)
        {
            // the current position is most probably after the end so we have to jump to the
            // continuous section
            _file_pos = _file_header->continuous_offset;
            _file_pos_invalid = true;
            _d->pos_reset = true;
            return readCurrentChunkHeader();
        }
    }

    _d->pos_reset = false;
#endif

    _index_table.adjustChunkHeader(_current_chunk);

    _file_pos += sizeof(ChunkHeader);

    _header_valid = true;
}
catch (...)
{
    setEOF();
    throw;
}

void IndexedFileReader::readCurrentChunkData(void* buffer)
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
        throw exceptions::EndOfFile(); // EOF if chunk size exceeds data region
    }

    checkFilePtr();
    readDataBlock(buffer, data_size);

    if (_d->chunk_header_search_possible)
    {
        _d->SetRealValidChunkHeader(_current_chunk,
                                    _file_pos - sizeof(ChunkHeader),
                                    _chunk_index);
    }

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
            _file.skip(skip_bytes);
        }

        _file_pos += skip_bytes;
    }

    if (_file_header->data_offset != _file_header->first_chunk_offset)
    {
        // file is really non-contingeous
        if (_file_pos == static_cast<FilePos>(_file_header->continuous_offset))
        {
            _file_pos = _file_header->data_offset;
            _file_pos_invalid = true;
        }
        else if (_file_pos == static_cast<FilePos>(_file_header->ring_buffer_end_offset))
        {
            _file_pos = _file_header->continuous_offset;
            _file_pos_invalid = true;
        }
    }

    IFHD_ASSERT((_file_pos & 0xF) == 0);

    _current_chunk_data = buffer;
}
catch (...)
{
    setEOF();
    throw;
}


/**
 *
 * This function returns the ChunkInfo of the current chunk.
 *
 * @param  ChunkHeader** chunkHeader [out] the current chunk info
 *
 * @returns void
 *
 */
void IndexedFileReader::queryChunkInfo(ChunkHeader** chunk_header)
{
    *chunk_header = nullptr;

    if (nullptr != _delegate)
    {
        v110::IndexedFileV110::ChunkHeader* chunk_header_v110 = nullptr;

        DELEGATE_PTR(_delegate)->queryChunkInfo(&chunk_header_v110);

        _current_chunk->time_stamp           = chunk_header_v110->time_stamp;
        _current_chunk->size                = chunk_header_v110->size;
        _current_chunk->stream_id            = 0;
        _current_chunk->flags               = (uint16_t) chunk_header_v110->flags;
        _current_chunk->ref_master_table_index = static_cast<uint32_t>(chunk_header_v110->ref_index);
        _current_chunk->stream_index         = 0;

        *chunk_header = _current_chunk;
        return;
    }

    if (!_header_valid)
    {
        readCurrentChunkHeader();
    }

    *chunk_header = _current_chunk;
}

/**
 *
 * This function reads and returns the current Chunk and increments
 * the IndexTable index
 *
 * @param  void** data    [out] the current chunk data
 * @param  uint32_t flags [in]  a tReadFlags the value
 *
 * @returns void
 *
 */
void IndexedFileReader::readChunk(void** data, uint32_t flags)
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->readChunk(data);
    }

    bool use_external_buffer = ((flags & rf_use_external_buffer) != 0);

    void* buffer = nullptr;
    if (use_external_buffer)
    {
        buffer = *data;
    }
    else
    {
        *data = nullptr;
        buffer = _buffer;
    }

    if (!_header_valid)
    {
        readCurrentChunkHeader();
    }

    if (!_prefetched)
    {
        readCurrentChunkData(buffer);
    }
    else
    {
        if (use_external_buffer)
        {
            a_util::memory::copy(buffer, _current_chunk->size, _buffer, _current_chunk->size);
        }

        // reset prefetch flag
        _prefetched = false;
    }

    _header_valid = false;

    if (!use_external_buffer)
    {
        *data = _buffer;
    }

    if ((flags & rf_backwards) != 0)
    {
        _chunk_index--;

        if (_chunk_index < 0)
        {
             setEOF();
            throw exceptions::EndOfFile();
        }

        if (_current_chunk->offset_to_last == 0)
        {
            setEOF();
            throw exceptions::EndOfFile();
        }

        ///@todo
        if (_file_pos_current_chunk == static_cast<FilePos>(_file_header->continuous_offset))
        {

        }
        else if (_file_pos_current_chunk == static_cast<FilePos>(_file_header->ring_buffer_end_offset))
        {

        }
        else
        {
            _file_pos = _file_pos_current_chunk - _current_chunk->offset_to_last;
        }
        _file_pos_invalid = true;
        IFHD_ASSERT((_file_pos & 0xF) == 0);
    }
    else
    {
        _chunk_index++;
    }
}

/**
 *
 * This function skips the next Chunk by incrementing the IndexTable index
 * and setting the new FilePosition.
 *
 * @returns void
 *
 */
void IndexedFileReader::skipChunk()
{
    if (nullptr != _delegate)
    {
        return DELEGATE_PTR(_delegate)->skipChunk();
    }

    if (!_header_valid)
    {
        readCurrentChunkHeader();
    }

    if (!_prefetched)
    {
        _current_chunk_data = nullptr;

        if (_chunk_index < 0 || (uint64_t) _chunk_index >= _file_header->chunk_count)
        {
            setEOF();
            throw exceptions::EndOfFile();
        }

        uint32_t data_size = _current_chunk->size - sizeof(ChunkHeader);

        if (_file_pos + data_size > _end_of_data_marker)
        {
            setEOF();
            throw exceptions::EndOfFile(); // EOF if chunk size exceeds data region
        }

        _file_pos += data_size;

        if ((data_size & 0xF) != 0)
        {
            int skip_bytes = 16-(data_size & 0xF);
            _file_pos += skip_bytes;
        }

        IFHD_ASSERT((_file_pos & 0xF) == 0);

        if (_cache_size != 0)
        {
            clearCache();
        }

        if (_file_header->data_offset != _file_header->first_chunk_offset)
        {
            if (_file_pos == (FilePos)(_file_header->continuous_offset))
            {
                _file_pos = _file_header->data_offset;
            }
            else if (_file_pos == (FilePos)(_file_header->ring_buffer_end_offset))
            {
                _file_pos = _file_header->continuous_offset;
            }
        }

        _file_pos_invalid = true;
    }

    _prefetched  = false;
    _header_valid = false;

    _chunk_index++;
}

void IndexedFileReader::readNextChunk(ChunkHeader** chunk_header, void** data, uint32_t flags, uint32_t stream_id)
{
    if (nullptr != _delegate)
    {
        if (stream_id == 0)
        {
            v110::IndexedFileV110::ChunkHeader* chunk_header_v110 = nullptr;

            DELEGATE_PTR(_delegate)->readNextChunk(&chunk_header_v110, data);

            _current_chunk->time_stamp           = chunk_header_v110->time_stamp;
            _current_chunk->size                = chunk_header_v110->size + (sizeof(ChunkHeader) - sizeof(v110::IndexedFileV110::ChunkHeader));
            _current_chunk->stream_id            = 0;
            _current_chunk->flags               = (uint16_t) chunk_header_v110->flags;
            _current_chunk->ref_master_table_index = (uint32_t) DELEGATE_PTR(_delegate)->getChunkIndexForIndexPos();
            _current_chunk->offset_to_last        = 0;
            _current_chunk->stream_index         = 0;

            *chunk_header = _current_chunk;
        }
        else
        {
            throw std::runtime_error("compatibility reader does not support reading based on stream index");
        }                        

        return;
    }

    if (stream_id == 0)
    {
        queryChunkInfo(chunk_header);
        return readChunk(data, flags);
    }
    else
    {
        do
        {
            queryChunkInfo(chunk_header);
            readChunk(data, flags);
            if ((*chunk_header)->stream_id == stream_id)
            {
                return;
            }
        } while (true);
    }
    return;
}

void IndexedFileReader::skipChunkInfo()
{
    _index_table_index++;
    setCurrentPos(_index_table_index, tf_chunk_index);
}

void IndexedFileReader::readNextChunkInfo(ChunkHeader** chunk_header)
{
    skipChunkInfo();
    queryChunkInfo(chunk_header);
}

void IndexedFileReader::readDataBlock(void* buffer, size_t buffer_size)
{
    uint8_t* dest  = (uint8_t*) buffer;
    int64_t read_size = (int64_t)buffer_size;
    uint8_t* cache = (uint8_t*) getCacheAddr();

    IFHD_ASSERT(_file_pos_invalid == false);

    if (_cache_size > 0)
    {
        if (read_size <= static_cast<int64_t>(_cache_size))
        {
            if (read_size > _cache_usage)  // data has to be read from disk
            {
                if (_cache_usage > 0)
                {
                    a_util::memory::copy(dest, _cache_usage, cache + _cache_offset, _cache_usage);
                    dest += _cache_usage;
                    read_size -= _cache_usage;
                }

                // refresh cache
                _cache_offset = 0;
                _cache_usage = 0;

                int64_t max_to_read = static_cast<int64_t>(_cache_size);
                while (_cache_usage < read_size) // in this case we stop reading when we have enough data for now (in case that the Read did not process all bytes requested)
                {
                    int64_t bytes_read = static_cast<int64_t>(_file.read(cache, max_to_read));
                    _cache_usage += bytes_read;
                    max_to_read -= bytes_read;
                }
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

            _file.readAll(dest, read_size);
        }
    }
    else // don't use any cache
    {
        _file.readAll(buffer, buffer_size);
    }
}

void IndexedFileReader::clearCache()
{
    _cache_offset = 0;
    _cache_usage  = 0;
}

void IndexedFileReader::setEOF()
{
    _chunk_index        = -1;
    _current_chunk_data = nullptr;
    _header_valid       = false;
    _file_pos           = 0;
    _file_pos_invalid    = true;
}

size_t IndexedFileReader::getExtensionCount() const
{
    if (nullptr != _delegate)
    {
       return DELEGATE_PTR(_delegate)->getExtensionCount();
    }

    return IndexedFile::getExtensionCount();
}

bool IndexedFileReader::findExtension(const char* identifier,
                                       FileExtension** extension_info_query,
                                       void** data) const
{
    if (nullptr != _delegate)
    {
        v110::IndexedFileReaderV110::FileExtension* extension_info_v110;
        DELEGATE_PTR(_delegate)->findExtension(identifier, &extension_info_v110, data);
        std::map<std::string, FileExtension>::iterator it = _extension_info_v110_by_name.find(std::string(identifier));
        if (it != _extension_info_v110_by_name.end())
        {
            (*extension_info_query) = &(it->second);
            return true;
        }
        else
        {
            FileExtension& extension_info = _extension_info_v110_by_name[identifier];
            strncpy((char*)extension_info.identifier, (char*)extension_info_v110->identifier, MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
            extension_info.identifier[MAX_FILEEXTENSIONIDENTIFIER_LENGTH -1] = 0;
            extension_info.type_id = extension_info_v110->type_id;
            extension_info.version_id = extension_info_v110->version_id;
            extension_info.data_size = extension_info_v110->data_size;
            extension_info.data_pos = extension_info_v110->data_pos;
            it = _extension_info_v110_by_name.find(identifier);
            if (it == _extension_info_v110_by_name.end())
            {
                return false;
            }
            else
            {
                (*extension_info_query) = &(it->second);
                return true;
            }
        }   
    }

    return IndexedFile::findExtension(identifier, extension_info_query, data);
}

/**
 *
 * This function gets the extension data.
 *
 * @param  int index                       [in]  the extension index
 * @param  FileExtension** extensionInfo [out] the FileExtension struct
 * @param  void** data                    [out] the extension data
 *
 * @return void
 *
 */
void IndexedFileReader::getExtension(size_t index, FileExtension** extension_info_query, void** data) const
{
    if (nullptr != _delegate)
    {
        v110::IndexedFileReaderV110::FileExtension* extension_info_v110;
        DELEGATE_PTR(_delegate)->getExtension(static_cast<int>(index), &extension_info_v110, data);
        std::string identifier((char*)extension_info_v110->identifier);
        std::map<std::string, FileExtension>::iterator it = _extension_info_v110_by_name.find(identifier);
        if (it != _extension_info_v110_by_name.end())
        {
            (*extension_info_query) = &(it->second);
            return;
        }
        else
        {
            FileExtension& extension_info = _extension_info_v110_by_name[identifier];
            strncpy((char*)extension_info.identifier, (char*)extension_info_v110->identifier, MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
            extension_info.identifier[MAX_FILEEXTENSIONIDENTIFIER_LENGTH - 1] = 0;
            extension_info.type_id = extension_info_v110->type_id;
            extension_info.version_id = extension_info_v110->version_id;
            extension_info.data_size = extension_info_v110->data_size;
            extension_info.data_pos = extension_info_v110->data_pos;
            it = _extension_info_v110_by_name.find(identifier);
            if (it == _extension_info_v110_by_name.end())
            {
                throw std::runtime_error("unexpected");
            }
            else
            {
                (*extension_info_query) = &(it->second);
                return;
            }
        }
        return;
    }  

    return IndexedFile::getExtension(index, extension_info_query, data);
}

int64_t IndexedFileReader::lookupChunkRef(uint16_t stream_id, int64_t position, TimeFormat time_format) const
{
    int64_t start_index = 0;
    int64_t offset = 0; 
    int64_t end_index = 0;
    int64_t master_index = 0;

    if (0 > _index_table.lookupChunkRef(stream_id, position, time_format,
                                         &start_index, &offset,
                                         &end_index, &master_index))
    {
        start_index = 0;
    }

    return start_index;
}

bool IndexedFileReader::getLastChunkWithFlagBefore(uint64_t chunk_index, uint16_t stream_id, uint16_t flag, ChunkHeader& header, std::vector<uint8_t>& vector_data)
{
    uint64_t master_index;
    if (chunk_index == 0 ||
        !_index_table.findNearestEntryWithFlags(stream_id, chunk_index,
                                                 flag, &master_index))
    {
        return false;
    }

    ChunkHeader dummy_header;
    int64_t type_chunk_index;
    int64_t type_chunk_offset;
    _index_table.fillChunkHeaderFromIndex(master_index, &dummy_header,
                                           &type_chunk_index, &type_chunk_offset);
    FilePos position_backup = _file_pos;
    int64_t chunk_index_backup = _chunk_index;
    FilePos chunk_position_backup = _file_pos_current_chunk;

    _file_pos = type_chunk_offset;
    _chunk_index = type_chunk_index;
    _file_pos_invalid = true;
    _header_valid = false;
    _prefetched = false;

    ChunkHeader* chunk_header;
    void* data;
    readNextChunk(&chunk_header, &data);

    header = *chunk_header;
    vector_data.assign(static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + chunk_header->size - sizeof(ChunkHeader));

    _file_pos = position_backup;
    _chunk_index = chunk_index_backup;
    _file_pos_current_chunk = chunk_position_backup;
    _file_pos_invalid = true;
    _header_valid = false;
    _prefetched = false;

    return true;
}

void IndexedFileReader::allocReadBuffers()
{
    _current_chunk = (ChunkHeader*) utils5ext::allocPageAlignedMemory(sizeof(ChunkHeader), utils5ext::getDefaultSectorSize());

    utils5ext::memZero(_current_chunk, sizeof(ChunkHeader));
}

void IndexedFileReader::freeReadBuffers()
{
    if (_current_chunk != nullptr)
    {
        utils5ext::freePageAlignedMemory(_current_chunk);

        _current_chunk = nullptr;
    }
}

} // namespace
} // namespace
