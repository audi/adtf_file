/**
 * @file
 * Indexed file writer.
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
#include <queue>
#include <string.h>
#include <assert.h>

#ifdef WIN32
    #include <windows.h>
#endif

#define ROUND_TO_ALIGNMENT(__value) \
    if ((__value & 0xF) != 0) \
    { \
        __value += 16-(__value & 0xF); \
    }

namespace ifhd
{
namespace v201_v301
{

/**
 * This additional struct is used with FileRingBuffer to managed the index table of cIndexedFilewriter
 * while OnDrop callback. (if history is used we need to drop already added Index Entries from table!).
 * @see IndexedFileWriter::cIndexedFileWriterPrivate::OnDrop
 *
 */
struct Additional
{
    Additional() {}
    Additional(uint64_t chunk_index, uint16_t stream_id, uint16_t chunk_flags, timestamp_t time):
        chunk_index(chunk_index),
        stream_id(stream_id),
        chunk_flags(chunk_flags),
        time(time)
    {
    }

    ///the index of chunk
    uint64_t chunk_index;
    ///corresponding stream id of the chunk
    uint16_t stream_id;
    ///the flags of the chunk
    uint16_t chunk_flags;
    ///time for the chunk
    timestamp_t time;
};

/**
 * Ringbuffer used withing IndexedFileWriter. Data written must be 16 Byte aligned !
 */
typedef utils5ext::FileRingBuffer<Additional, 16> RingBuffer;

//*************************************************************************************************

/// Definition of filling bytes. Chunks are filled up to 16 byte boundaries
static uint8_t chunk_fill_bytes[16] = {0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,
                                       0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE};

/**************************************/
/* This still needs to be implemented.*/
/**************************************/
class IndexedFileWriter::IndexedFileWriterImpl : public RingBuffer::DropCallback
{
    IndexedFileWriter* _p;

    public:
        ChunkHeader                 internal_write_chunk_header;

        std::queue<int>             chunk_header_positions;
        std::queue<ChunkHeader>     chunk_headers;

        bool                        check_chunk_header;

        a_util::memory::unique_ptr<RingBuffer> file_ring_buffer;
        timestamp_t                 history_time;
        timestamp_t                 first_time;
        bool                        first_time_set;
        utils5ext::FileSize        history_size;
        bool                        wrapping_started;
        ChunkHeader                header;
        int                         address_begin;
        int                         address_end;
        bool                        history_quitted;

        a_util::concurrency::recursive_mutex        critical_section;

        IndexedFileWriter::ChunkDroppedCallback* drop_callback = nullptr;

        std::thread writer_thread;
        std::atomic<bool> keep_writing_cache_to_disk;
        size_t cache_maximum_write_chunk_size;

    public:
        explicit IndexedFileWriterImpl(IndexedFileWriter& parent) :
            internal_write_chunk_header{},
            chunk_header_positions(),
            chunk_headers(),
            check_chunk_header(false),
            file_ring_buffer(),
            history_time(0),
            first_time(0),
            first_time_set(false),
            history_size(0),
            wrapping_started(false),
            header{},
            address_begin(0),
            address_end(0),
            history_quitted(false),
            keep_writing_cache_to_disk(true),
            _p(&parent)
        {
           utils5ext::memZero(&internal_write_chunk_header, sizeof(internal_write_chunk_header));
        }
        virtual ~IndexedFileWriterImpl()
        {
        }

        bool CheckEmpty()
        {
            std::lock_guard<a_util::concurrency::recursive_mutex> lck(critical_section);
            return (chunk_header_positions.empty());
        }
        void TopAddress()
        {
            std::lock_guard<a_util::concurrency::recursive_mutex> lck(critical_section);
            if (CheckEmpty())
            {
                return;
            }
            address_begin = chunk_header_positions.front();
            address_end   = address_begin + sizeof(ChunkHeader);
        }
        void TopHeader()
        {
            std::lock_guard<a_util::concurrency::recursive_mutex> lck(critical_section);
            ChunkHeader& c_h = chunk_headers.front();
            a_util::memory::copy(&header, sizeof(ChunkHeader), &c_h, sizeof(ChunkHeader));
        }

        void Push(const int insert, const ChunkHeader* header)
        {
            std::lock_guard<a_util::concurrency::recursive_mutex> lck(critical_section);
            chunk_header_positions.push(insert);
            chunk_headers.push(*header);
        }

        void Pop()
        {
            std::lock_guard<a_util::concurrency::recursive_mutex> lck(critical_section);
            chunk_headers.pop();
            chunk_header_positions.pop();
        }

        void CheckHeaderWritten(size_t data_size)
        {
            void* cache_addr = _p->getCacheAddr();
            bool run = !CheckEmpty();
            if (run)
            {
                TopAddress();

                while (run &&
                       ((address_begin >= _p->_cache_flush_ptr &&
                         address_begin < (_p->_cache_flush_ptr + data_size)) ||
                        (address_end >= _p->_cache_flush_ptr &&
                         address_end < (_p->_cache_flush_ptr + data_size)
                         ))
                      )
                {
                    TopHeader();
                    ChunkHeader* current_header = (ChunkHeader*)&header;

                    //the current_header begins in range is within the range
                    if (address_begin >= _p->_cache_flush_ptr)
                    {
                        //the current_header ends in range
                        if (address_end <= (_p->_cache_flush_ptr + data_size))
                        {
                            IFHD_ASSERT(0 == a_util::memory::compare(current_header, sizeof(ChunkHeader),
                                                                    ((ChunkHeader*)((uint8_t*)cache_addr + address_begin)), sizeof(ChunkHeader)));
                            #ifndef _DEBUG
                                if (0 != a_util::memory::compare(current_header, sizeof(ChunkHeader),
                                                                ((ChunkHeader*)((uint8_t*)cache_addr + address_begin)), sizeof(ChunkHeader)))
                                {
                                 //   LOG_ERROR("IFW: Writing of chunk current_header went wrong");
                                }
                            #endif
                            Pop();
                        }
                        //the current_header begins in range end ends outside
                        else
                        {
                            IFHD_ASSERT(0 == a_util::memory::compare(current_header,  _p->_cache_flush_ptr + data_size - address_begin,
                                                                   ((ChunkHeader*)((uint8_t*)cache_addr + address_begin)), _p->_cache_flush_ptr + data_size - address_begin));
                            #ifndef _DEBUG
                                if (0 != a_util::memory::compare(current_header, _p->_cache_flush_ptr + data_size - address_begin,
                                                                ((ChunkHeader*)((uint8_t*)cache_addr + address_begin)), _p->_cache_flush_ptr + data_size - address_begin))
                                {
                                    //LOG_ERROR("IFW: Writing of chunk current_header went wrong");
                                }
                            #endif
                            if (address_end >= _p->_cache_size)
                            {
                                Pop();
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        //the first part of current_header where at end
                        //the second part is at the very beginning
                        if (address_begin < 0)
                        {
                            IFHD_ASSERT(0 == a_util::memory::compare(((uint8_t*)current_header) - address_begin, sizeof(ChunkHeader) + address_begin,
                                                                   ((uint8_t*)cache_addr), sizeof(ChunkHeader) + address_begin));
                            #ifndef _DEBUG
                                if (0 != a_util::memory::compare(((uint8_t*)current_header) - address_begin, sizeof(ChunkHeader) + address_begin,
                                                                ((uint8_t*)cache_addr), sizeof(ChunkHeader) + address_begin))
                                {
                                  //  LOG_ERROR("IFW: Writing of chunk current_header went wrong");
                                }
                            #endif
                        }
                        //the first part of current_header where at end of last written
                        // the second part is now here
                        else
                        {
                            IFHD_ASSERT(0 == a_util::memory::compare(((uint8_t*)current_header) + _p->_cache_flush_ptr - address_begin, address_end - _p->_cache_flush_ptr,
                                                                    (((uint8_t*)cache_addr) + _p->_cache_flush_ptr), address_end -_p->_cache_flush_ptr));
                            #ifndef _DEBUG
                                if (0 != a_util::memory::compare(((uint8_t*)current_header) + _p->_cache_flush_ptr - address_begin, address_end - _p->_cache_flush_ptr,
                                                                (((uint8_t*)cache_addr) + _p->_cache_flush_ptr), address_end -_p->_cache_flush_ptr))
                                {
                                   // LOG_ERROR("IFW: Writing of chunk current_header went wrong");
                                }
                            #endif
                        }
                        Pop();
                    } //else
                    run = !(CheckEmpty());
                    if (run)
                    {
                         TopAddress();
                    }
                } //while
            }
        }

        bool InHistoryMode()
        {
            return history_time || history_size;
        }

    public:
        void onDrop(const RingBuffer::Item& dropped_item, const RingBuffer::Item& next_item)
        {
            _p->_index_table.remove(dropped_item.additional.chunk_index,
                                     dropped_item.additional.stream_id);

            if (drop_callback)
            {
                drop_callback->onChunkDropped(dropped_item.additional.chunk_index,
                                                dropped_item.additional.stream_id,
                                                dropped_item.additional.chunk_flags,
                                                dropped_item.additional.time);
            }
        }
};

IndexedFileWriter::IndexedFileWriter()
{
    _d.reset(new IndexedFileWriterImpl(*this));
    initialize();
}

IndexedFileWriter::~IndexedFileWriter()
{
    close();
}

void IndexedFileWriter::setDateTime(const a_util::datetime::DateTime& date_time)
{
    return IndexedFile::setDateTime(date_time);
}

void IndexedFileWriter::initialize()
{
    IndexedFile::initialize();

    _is_open               = false;
    _write_mode            = true;
    _sync_mode             = false;

    _use_prefix_temp_file_extension = false; // set a Prefix to Temp Files '~$'

    _write_guid = false;

    _cache_min_store_at_once  = 0;
    _cache_flush_ptr        = 0;
    _cache_insert_ptr       = 0;
    _cache_usage_count      = 0;

    _file_pos_last_chunk     = 0;

    _file_name           = "";
    _temp_file_name       = "";

    _cond_cache_used_processed = true;

    _prefix_of_temp_save_file_name = "~$";

    _catch_first_time       = false;
    _d->check_chunk_header = false;

    utils5ext::memZero(_stream_info_add, MAX_INDEXED_STREAMS * sizeof(_stream_info_add[0]));
    utils5ext::memZero(_stream_info, MAX_INDEXED_STREAMS * sizeof(_stream_info[0]));

    _last_write_result = true;
    _last_write_system_error = 0;
}

void IndexedFileWriter::create(const std::string& filename,
                                   size_t cache_size,
                                   uint32_t flags,
                                   uint64_t /*fileTimeOffset*/,
                                   timestamp_t history,
                                   utils5ext::FileSize history_size,
                                   size_t cache_minimum_write_chunk_size,
                                   size_t cache_maximum_write_chunk_size,
                                   ChunkDroppedCallback* drop_callback,
                                   timestamp_t index_delay)
{
    using namespace utils5ext;

    close();

    _file_name = "";
    _temp_file_name = "";

    _index_table.create(index_delay);

    _last_chunk_time = 0;
    _system_cache_disabled = false;

    if ((flags & om_sync_write) != 0)
    {
        _sync_mode = true;
    }

    if ((flags & om_validate_chunk_header) != 0)
    {
        _d->check_chunk_header = true;
    }

    if ((flags & om_disable_file_system_cache) != 0)
    {
        _system_cache_disabled = true;
    }

    if (_system_cache_disabled)
    {
        _sector_size = utils5ext::getSectorSizeFor(filename);
    }
    else
    {
        _sector_size = default_block_size;
    }

    // in history mode we do not support a memory cache
    if (history || history_size)
    {
        if (cache_size > 0)
        {
            throw std::invalid_argument("internal cache is not supported in history mode");
        }

        _sync_mode = true;
    }

    //no cache is needed for this option
    if (!_system_cache_disabled && _sync_mode)
    {
        ; // nothing to do here
    }
    else
    {
        _cache_size = cache_size;
        if ((int) _cache_size <= 0)
        {
            _cache_size = default_cache_size;
        }
        allocCache(static_cast<int>(_cache_size));
        if (cache_minimum_write_chunk_size != 0)
        {
            _cache_min_store_at_once = static_cast<int>(cache_minimum_write_chunk_size);
        }
        _d->cache_maximum_write_chunk_size = cache_maximum_write_chunk_size;
        if (_d->cache_maximum_write_chunk_size == 0)
        {
            _d->cache_maximum_write_chunk_size = _cache_size;
        }
    }

    uint32_t open_flags = File::om_write | File::om_sequential_access;

    if (_system_cache_disabled)
    {
        open_flags |= File::om_write_through | File::om_disable_file_system_cache;
    }

    std::string savename;
    createAFileWithPrefixdAndAFileWithoutPrefix(filename, savename);
    _file.open(savename, open_flags);

    allocHeader();

    _file_header->file_id          = getFileId();
    _file_header->version_id       = *getSupportedVersions().rbegin();
    _file_header->extension_offset = 0;
    _file_header->extension_count  = 0;
    _file_header->data_offset      = 0;
    _file_header->data_size        = 0;
    _file_header->chunk_count      = 0;
    _file_header->max_chunk_size    = 0;
    _file_header->time_offset      = 0;
    _file_header->header_byte_order  = byte_order;
    _file_header->patch_number      = 0x01;

    // set current file time
    setDateTime(a_util::datetime::getCurrentLocalDateTime());

    _catch_first_time = true;
    _time_offset = 0;

    utils5ext::memZero(_file_header->reserved, sizeof(_file_header->reserved));
    utils5ext::memZero(_file_header->description,  sizeof(_file_header->description));

    writeFileHeader();

    if (!_sync_mode)
    {
        _d->keep_writing_cache_to_disk = true;
        _d->writer_thread = std::thread(&IndexedFileWriter::writeCacheToDisk, this);
    }

    _is_open = true;

    _file_pos_last_chunk = _file_pos;

    if (history || history_size)
    {
        _d->file_ring_buffer.reset(new RingBuffer(&_file, _file_pos, 0, _d.get()));
        _d->history_time = history;
        _d->history_size = history_size;
        _d->wrapping_started = false;
        _d->drop_callback = drop_callback;
        //only if the file is recorded with a History Buffer we need to raise the Version ID of the supported file
        //reason: Every DAT File must be playable in lower ADTF versions if NO File History is used !!
        if (_file_header->version_id == version_id)
        {  
            _file_header->version_id = version_id_with_history_end_offset;
        }
    }

    // initialize ring buffer offsets for a file without history
    // in case of a history these will be updated later on
    _file_header->first_chunk_offset = _file_header->data_offset;
    _file_header->continuous_offset = _file_header->data_offset;
    _file_header->ring_buffer_end_offset = _file_header->data_offset;
}

/*
* Return the Name of the file during creation mode.
* It can be with or without prefix.
* @return Standard result.
*/
void IndexedFileWriter::createAFileWithPrefixdAndAFileWithoutPrefix(const std::string& filename,
                                                                           std::string& save_filename)
{
    using namespace utils5ext;
    if(_use_prefix_temp_file_extension)
    {
        // Create new File
        File file;
        file.open(filename, File::om_write);
        file.close();

        _file_name = filename;
        _temp_file_name = getNewFileNameWithPrefix(filename);
        save_filename =  _temp_file_name;
    }
    else
    {
        save_filename = filename;
    }
}

/*
* Set a Prefix to Filename
* @return an New Filename with Prefix.
*/
std::string IndexedFileWriter::getNewFileNameWithPrefix(const std::string& filename) const
{
    const a_util::filesystem::Path& filename_temp(filename);
    std::string name_of_temp = _prefix_of_temp_save_file_name + a_util::filesystem::Path(filename_temp).getLastElement().toString();
    a_util::filesystem::Path temp_prefix_name = filename_temp.getParent().append(name_of_temp);
    return temp_prefix_name;
}

void IndexedFileWriter::allocCache(int size)
{
    //This is not thread safe.
    //AllocCache can only be called, while Opening File or in sync mode within write chunk call

    _cache_flush_ptr   = 0; // points to the data next to be written to harddisk
    _cache_insert_ptr  = 0; // points to the next unused buffer position
    _cache_usage_count = 0; // counds the amount of data inside the cache

    IndexedFile::allocCache(size);

    //_cache_size will be set to new sector aligned
    // _cache_size            = (_cache_size + _sectorSize - 1) & ~(_sectorSize - 1);
    _cache_min_store_at_once = static_cast<int>(_cache_size / 8);
    _cache_min_store_at_once =
        static_cast<int>((_cache_min_store_at_once + _sector_size - 1) & ~(_sector_size - 1));
}

void IndexedFileWriter::close()
{
    if (_is_open)
    {
        _write_guid = false;

        _is_open = false;

        stopAndFlushCache();

        if (_d->InHistoryMode() && !_d->history_quitted)
        {
            quitHistory();
        }

        setGUID();                            // if new dat-file -> creates a new GUID and appends it to the
                                              // GUID History List        

        _file_header->chunk_count -= _index_table.getIndexOffset(0);

        writeIndexTable();                    // copy index table to header extension
        writeFileHeaderExt();                 // write header extension to disk
        writeFileHeader();                    // fill values to file header

        _write_guid = false;
    }

    _index_table.free();

    _d->check_chunk_header = false;

    for (int idx = 0;
         idx < MAX_INDEXED_STREAMS;
         ++idx)
    {
        if (_stream_info_add[idx].is_reference == 0 &&
            _stream_info_add[idx].data != nullptr)
        {
            delete [] _stream_info_add[idx].data;
            _stream_info_add[idx].data = nullptr;
        }
    }

    utils5ext::memZero(_stream_info_add, MAX_INDEXED_STREAMS * sizeof(_stream_info_add[0]));
    utils5ext::memZero(_stream_info, MAX_INDEXED_STREAMS * sizeof(_stream_info[0]));

    IndexedFile::close();

    if (_use_prefix_temp_file_extension)
    {
         renameTempSaveToFileName();
    }
}

void IndexedFileWriter::stopAndFlushCache()
{
    if (!_sync_mode)
    {
        if (_d->keep_writing_cache_to_disk)
        {
            _d->keep_writing_cache_to_disk = false;
            {
                std::lock_guard<std::mutex> guard(_mutex_cache_used);
                _cond_cache_used_processed = false;
            }
            _cond_cache_used.notify_all();
            _d->writer_thread.join();
        }
    }

    storeToDisk(true);
}


/*
* Rename the TempFile Name '~$FileName.dat' back to
* 'FileName.dat' Filename
*/
void IndexedFileWriter::renameTempSaveToFileName()
{
    using namespace utils5ext;
    // Delete Real File
    if (a_util::filesystem::exists(_file_name))
    {
        File file;
        file.open(_file_name, File::om_read);
        FileSize file_size = file.getSize();
        file.close();

        // delete only if the filesize is 0 (empty)
        if (file_size == 0)
        {
            a_util::filesystem::remove(_file_name);
        }
        else
        {
            throw std::runtime_error("cannot delete temporary file, it's not empty");
        }
    }
    else
    {
        return;
    }

    // rename the file
    utils5ext::fileRename(_temp_file_name, _file_name);
}

/*
* Return the Name of the file during creation mode.
* It can be with or without prefix.
*/
std::string IndexedFileWriter::getTempSaveFileName()
{
    if(_use_prefix_temp_file_extension)
    {
        return _file_name;
    }
    else
    {
        return _temp_file_name;
    }
}

/*
* Set the mode if a prefix is set by a file or not
*/
void IndexedFileWriter::setPrefixTempFileExtension(bool use_prefix)
{
    _use_prefix_temp_file_extension = use_prefix;
}

/*
* Returns the Prefix TempFile Extension Mode for save temp files
*/
bool IndexedFileWriter::getPrefixTempFileExtensionMode() const
{
    return _use_prefix_temp_file_extension;
}

/*
* Shape of a Extension
*/
std::string IndexedFileWriter::getPrefix() const
{
    return _prefix_of_temp_save_file_name;
}

void IndexedFileWriter::writeFileHeader()
{
    if (_file_header == nullptr)
    {
        return;
    }

    if (_file_header->time_offset == 0)
    {
        _file_header->time_offset = _time_offset;
    }

    _file_header->duration  = _last_chunk_time - _file_header->time_offset;

    _file.setFilePos(0, utils5ext::File::fp_begin);

    // has not to be changed because it checks <= 0 ==> DEVICE_IO
    internalWrite(_file_header, sizeof(FileHeader), true);

    //macht eigentlich keinen Sinn bei schliessen des files, bleibt aber drin
    _file_pos                     = _file.getFilePos();
    _file_pos_last_chunk            = _file_pos;
    _file_header->data_offset = _file_pos;
}


void IndexedFileWriter::writeFileHeaderExt()
{
    // layout on disk: [...|ext1|ext2|...|extN]ext-table]...]

    size_t num_extensions = getExtensionCount();
    if (num_extensions == 0)
    {
        return;
    }
    _file_header->extension_count = static_cast<uint32_t>(num_extensions);

    int index;
    FileExtensionStruct* extension_struct;
    FileExtension* extension_info;
    FileExtensionList::const_iterator it;

    // copy all extension info structures to linear memory (the extension table)
    void* temp_table_buffer = internalMalloc(sizeof(FileExtension) * num_extensions, true);

    utils5ext::memZero(temp_table_buffer, sizeof(FileExtension) * num_extensions);

    FileExtension* extension_tab = (FileExtension*) temp_table_buffer;

    index = 0;
    for (it = _extensions.begin(); it != _extensions.end(); ++it)
    {
        extension_struct = *it;
        extension_info = (FileExtension*) &extension_struct->file_extension;
        a_util::memory::copy(&extension_tab[index++], sizeof(FileExtension), extension_info, sizeof(FileExtension));
    }

    // store extension data packages to disk (in front of the linear extension table)
    index = 0;
    for (it = _extensions.begin(); it != _extensions.end(); ++it)
    {
        extension_struct = *it;
        extension_info = (FileExtension*) &extension_struct->file_extension;

        // store file offsets into extension table records
        if (extension_struct->extension_page)
        {
            extension_tab[index].data_pos = _file.getFilePos();

            // has not to be changed because it checks frees the internal values ==> DEVICE_IO

            //Conversion from iUInt64 to long is ok, extensionInfo->data_size should not exceed this range
            //because second parameter of InternalWrite treats extensionInfo->data_size as int anyway.
            try
            {
                internalWrite(extension_struct->extension_page,
                              (int) extension_info->data_size,
                              true);
            }
            catch(...)
            {
                internalFree(temp_table_buffer);
                throw;
            }
        }
        else
        {
            // data has already been written or the size is zero.
        }

        index++;
    }

    // store extension table to disk
    _file_header->extension_offset = _file.getFilePos();

    // has not to be changed because it checks <= 0 ==> DEVICE_IO
    try
    {
        internalWrite(extension_tab, sizeof(FileExtension) * num_extensions, true);
    }
    catch (...)
    {
        internalFree(temp_table_buffer);
        throw;
    }

    internalFree(temp_table_buffer);
}

void IndexedFileWriter::writeIndexTable()
{
    uint16_t max_stream_id = (uint16_t) _index_table.getMaxStreamId();

    if (_d->InHistoryMode())
    {
        // update global header fields
        RingBuffer::const_iterator it_history_item = _d->file_ring_buffer->begin();
        RingBuffer::const_iterator it_end = _d->file_ring_buffer->end();
        if (it_history_item != it_end)
        {
            _file_header->first_chunk_offset = it_history_item->file_pos;
            _time_offset = it_history_item->additional.time;

            // search for first sample of each stream and update the start timestamps
            std::set<uint16_t> streams_left;
            for (uint16_t stream_id = 1; stream_id <= max_stream_id; ++stream_id)
            {
                if (_stream_info[stream_id - 1].stream_name[0] != 0)
                {
                    streams_left.insert(stream_id);
                }
            }

            for(;it_history_item != it_end; ++it_history_item)
            {
                std::set<uint16_t>::iterator it_stream = streams_left.find(it_history_item->additional.stream_id);
                if (it_stream != streams_left.end())
                {
                    _stream_info[*it_stream - 1].stream_first_time = it_history_item->additional.time;
                    streams_left.erase(it_stream);
                }
                if (streams_left.empty())
                {
                    break;
                }
            }

            for (std::set<uint16_t>::iterator it_stream = streams_left.begin(); it_stream != streams_left.end(); ++it_stream)
            {
                if (_stream_info[*it_stream - 1].stream_first_time <= _time_offset)
                {
                    _stream_info[*it_stream - 1].stream_first_time = 0;
                    _stream_info[*it_stream - 1].stream_last_time = 0;
                }
            }
        }
    }

    char extension_name[512];

    //There are 511 Stream possible (1 - 511)
    //512 is reserved for markers
    for (uint16_t stream_id=0; stream_id<=max_stream_id; stream_id++)
    {
        int64_t index_table = _index_table.getBufferSize(stream_id);
        sprintf(extension_name, "%s%d", IDX_EXT_INDEX, (int) stream_id);

        FileExtension* extension_info = nullptr;
        void*          extension_data  = nullptr;
        if (stream_id == 0)
        {
            //writes the masterindex table to the extensions
            appendExtension(extension_name,
                            nullptr,
                            (int) index_table,
                            0x0,
                            0x0,
                            stream_id,
                            0x0);
            findExtension(extension_name, &extension_info, &extension_data);
            _index_table.copyToBuffer(stream_id, extension_data);
        }
        else if (_stream_info[stream_id - 1].stream_name[0] != 0)
        {
            //writes the streamindex table of the corresponding stream to the table
            //+ Additional StreamInfo of the stream (will be generated by cADTFFile or HDRecorder)
            index_table += sizeof(StreamInfoHeader) + _stream_info[stream_id - 1].info_data_size;
            appendExtension(extension_name,
                            nullptr,
                            (int) index_table,
                            0x0,
                            0x0,
                            stream_id,
                            0x0);
            findExtension(extension_name, &extension_info, &extension_data);
            a_util::memory::copy(extension_data, sizeof(StreamInfoHeader), &_stream_info[stream_id - 1], sizeof(StreamInfoHeader));
            static_cast<StreamInfoHeader*>(extension_data)->stream_index_count -= _index_table.getIndexOffset(stream_id);
            extension_data = (void*) (((uint8_t*)extension_data) + sizeof(StreamInfoHeader));
            if (_stream_info[stream_id - 1].info_data_size > 0)
            {
                a_util::memory::copy(extension_data, _stream_info[stream_id - 1].info_data_size, _stream_info_add[stream_id - 1].data, _stream_info[stream_id - 1].info_data_size);
                extension_data = (void*) (((uint8_t*)extension_data) + _stream_info[stream_id - 1].info_data_size);
            }
            _index_table.copyToBuffer(stream_id, extension_data);


        }

        if (stream_id == 0 || _stream_info[stream_id - 1].stream_name[0] != 0)
        {
            //Writes Additional Entries for offsets (if used History Mode of the Writer the file has new layout)
            AdditionalIndexInfo info;
            info.stream_index_offset = _index_table.getIndexOffset(stream_id);
            info.stream_table_index_offset =
                static_cast<uint32_t>(_index_table.getIndexTableOffset(stream_id));
            sprintf(extension_name, "%s%d", IDX_EXT_INDEX_ADDITONAL, (int) stream_id);
            appendExtension(extension_name,
                            &info,
                            sizeof(info),
                            0x0,
                            0x0,
                            stream_id,
                            0x0);
        }
    }
}

void IndexedFileWriter::writeChunk(uint16_t stream_id,
                                       const void* data,
                                       uint32_t data_size,
                                       timestamp_t time_stamp,
                                       uint32_t flags)
{
    bool index_appended = true;
    return writeChunk(stream_id,
                      data,
                      data_size,
                      time_stamp,
                      flags,
                      index_appended);
}


void IndexedFileWriter::writeChunk(uint16_t stream_id,
                                       const void* data,
                                       uint32_t data_size,
                                       timestamp_t time_stamp,
                                       uint32_t flags,
                                       bool& index_entry_appended)
{
    index_entry_appended = false;
    // this is only for async call and will only be set by the async file writer in UpdateCache()
    if (!_last_write_result)
    {
        throw std::runtime_error("write thread encountered an error");
    }

    if (!_is_open)
    {
        throw std::runtime_error("file not opened");
    }

    //Every Chunk size is written at least 16 byte aligned
    IFHD_ASSERT((_file_pos & 0xF) == 0); // check for correct alignment

    if (stream_id == 0 && stream_id > MAX_INDEXED_STREAMS) //a chunk needs to have a stream identifier
    {
        throw std::invalid_argument("invalid stream id");
    }
    if (time_stamp < 0)
    {
        throw std::invalid_argument("invalid timestamp");
    }

    uint32_t size = data_size + sizeof(ChunkHeader);

    if (_catch_first_time)
    {
        _time_offset    = time_stamp;
        _catch_first_time = false;
    }

    _d->internal_write_chunk_header.time_stamp           = (uint64_t) time_stamp;
    _d->internal_write_chunk_header.ref_master_table_index = (uint32_t) _index_table.getItemCount(0);
    // a possible invalid offset > 0 in the chunk at the front of the data is ignored by the reader
    _d->internal_write_chunk_header.offset_to_last        = (uint32_t) (_file_pos - _file_pos_last_chunk);
    _d->internal_write_chunk_header.size                = size;
    _d->internal_write_chunk_header.stream_id            = stream_id;
    _d->internal_write_chunk_header.flags               = (uint16_t) flags;
    _d->internal_write_chunk_header.stream_index         = _stream_info[stream_id - 1].stream_index_count;


    if (_stream_info[stream_id - 1].stream_index_count == _index_table.getIndexOffset(stream_id))
    {
        _stream_info[stream_id - 1].stream_first_time = (uint64_t)time_stamp;
    }
    _stream_info[stream_id - 1].stream_last_time = (uint64_t)time_stamp;


    // remember position before writing the next chunk after this
    _file_pos_last_chunk = _file_pos;

    //the fill bytes are for the 16 byte alignment within file
    int fill_bytes = 0;
    if ((data_size & 0xF) != 0)
    {
        fill_bytes = 16-(data_size & 0xF);
    }

    uint32_t whole_chunk = size + fill_bytes;
    int check_size = (int)whole_chunk;
    if (check_size < 0)
    {
        IFHD_ASSERT(check_size > 0);
        throw std::runtime_error("Size of Data is invalid (to big)");
    }

    if (_d->InHistoryMode())
    {
        // in history mode we do not support our own memory cache
        // this is checked in the create method.
        // _syncMode is handled by the File instance itself (via the open flags).

        RingBuffer::ItemPiece pieces[2];
        pieces[0].data = &_d->internal_write_chunk_header;
        pieces[0].data_size = sizeof(_d->internal_write_chunk_header);
        pieces[1].data = data;
        pieces[1].data_size = data_size;
        _d->file_ring_buffer->appendItem(pieces, 2, 
                                        Additional(_file_header->chunk_count, stream_id, static_cast<uint16_t>(flags), time_stamp),
                                        &_file_pos_last_chunk);

        _file_pos = _file_pos_last_chunk;

        if (!_d->wrapping_started)
        {
            if (!_d->first_time_set)
            {
                _d->first_time = time_stamp;
                _d->first_time_set = true;
            }

            if ((_d->history_time && (time_stamp - _d->first_time) > _d->history_time) ||
                (_d->history_size && _d->file_ring_buffer->getCurrentSize() > _d->history_size))
            {
                _d->file_ring_buffer->startWrappingAround();
                _d->wrapping_started = true;
            }
        }
    }
    else
    {
        if (_sync_mode)
        {
            if (_system_cache_disabled)
            {
                //fill up to sector size (thats why the internal cache is needed to fill up)
                if (check_size > (_cache_size - _cache_usage_count))
                {
                    storeToDisk(false);

                    if (check_size > (_cache_size - _cache_usage_count))
                    {
                        throw std::runtime_error("Chunk to big for cache");
                    }
                }

                writeToCache(&_d->internal_write_chunk_header, sizeof(_d->internal_write_chunk_header), _d->check_chunk_header);
                writeToCache(data, data_size);

                if (fill_bytes > 0)
                {
                    writeToCache(chunk_fill_bytes, fill_bytes);
                }
                while (_cache_min_store_at_once <= _cache_usage_count
                       && _cache_min_store_at_once != 0)
                {
                    storeToDisk(false);
                }
            }
            else
            {
                //this is with system cache a simple write operation to disk
                internalWrite(&_d->internal_write_chunk_header,
                              sizeof(_d->internal_write_chunk_header), false);
                internalWrite(data, data_size,false);

                if (fill_bytes > 0)
                {
                    internalWrite(chunk_fill_bytes, fill_bytes, false);
                }
            }
        }
        else
        {
            writeToCache(&_d->internal_write_chunk_header, sizeof(_d->internal_write_chunk_header), _d->check_chunk_header);
            writeToCache(data, data_size);
            if (fill_bytes > 0)
            {
                writeToCache(chunk_fill_bytes, fill_bytes);
            }
        }
    }

    // append index
    _index_table.append(stream_id,
                         _stream_info[stream_id - 1].stream_index_count,
            _file_header->chunk_count,
            _file_pos,
            size,
            time_stamp,
            flags,
            index_entry_appended);


    _last_chunk_time = time_stamp;
    _file_header->chunk_count++;

    if (size > _file_header->max_chunk_size)
    {
        _file_header->max_chunk_size = size;
    }

    _file_header->data_size += whole_chunk; // in history mode this will be updated in QuitHistory
    _file_pos += whole_chunk;

    _stream_info[stream_id - 1].stream_index_count++;

    //the filepos is 16 byte aligned
    IFHD_ASSERT((_file_pos & 0xF) == 0); // check for correct alignment
}

void IndexedFileWriter::quitHistory()
{
    if (!_d->InHistoryMode())
    {
        throw std::logic_error("not in history mode");
    }

    RingBuffer::Item rear_item;
    RingBuffer::Item last_item;
    _d->file_ring_buffer->startAppending(&rear_item, &last_item);

    if (rear_item.file_pos != -1)
    {
        _file_header->continuous_offset = rear_item.file_pos + rear_item.size;
        ROUND_TO_ALIGNMENT(_file_header->continuous_offset);
    }
    else
    {
        _file_header->continuous_offset = _file_header->data_offset;
    }

    if (last_item.file_pos != -1)
    {
        _file_header->ring_buffer_end_offset = last_item.file_pos + last_item.size;
        ROUND_TO_ALIGNMENT(_file_header->ring_buffer_end_offset);
    }
    else
    {
        _file_header->ring_buffer_end_offset = _file_header->data_offset;
    }

    // update data size correctly
    _file_header->data_size = _file_header->continuous_offset - _file_header->data_offset;

    // to make sure the history size and time checks are disabled.
    _d->wrapping_started = true;
    _d->history_quitted = true;
}

int IndexedFileWriter::getCacheUsage()
{
    return _cache_usage_count;
}

void IndexedFileWriter::writeToCache(const void* data,
                                         int data_size,
                                         const bool is_chunk_header)
{
    //_lock_ressource(RAW_FILE_LOCK);

    if (data_size == 0)
    {
        return;
    }

    void* cache_addr = getCacheAddr();
    uint8_t* data_src_ptr = (uint8_t*) data;

    int bytes_to_store;
    int data_stored = 0;
    while (data_stored < data_size)
    {
        bytes_to_store = data_size - data_stored;
        if (bytes_to_store > _cache_min_store_at_once)
        {
            bytes_to_store = _cache_min_store_at_once;
        }

        // printf("CA: usage=%d, block=%d, size=%d\n", _cache_size, _cacheUsageCount, bytesToStore);

        if (!_sync_mode)
        {
            while (_cache_size - _cache_usage_count < bytes_to_store)
            {
                // wait until write thread freed some blocks
                std::unique_lock<std::mutex> lck(_mutex_freed_event);
                _cond_freed_event.wait_for(lck, std::chrono::milliseconds(100));

                if (!_last_write_result)
                {
                    // check if it was a write error
                    throw std::runtime_error("write thread encountered an error");
                }
            }
        }

        if (_cache_insert_ptr + bytes_to_store <= _cache_size)
        {
            a_util::memory::copy((uint8_t*) cache_addr + _cache_insert_ptr, bytes_to_store, data_src_ptr, bytes_to_store);
            if (_d->check_chunk_header && is_chunk_header)
            {
                if (bytes_to_store >= (int)sizeof(ChunkHeader))
                {
                    int insert = _cache_insert_ptr;
                    _d->Push(insert, ((ChunkHeader*)data));
                }
                else
                {
                     int insert = 0 - bytes_to_store;
                    _d->Push(insert, ((ChunkHeader*)data));
                }
            }
            _cache_insert_ptr += bytes_to_store;
        }
        else
        {
            // wrap data inside the ring buffer
            int first_part = static_cast<int>(_cache_size) - _cache_insert_ptr;
            a_util::memory::copy((uint8_t*) cache_addr + _cache_insert_ptr, first_part, data_src_ptr, first_part);
            a_util::memory::copy((uint8_t*) cache_addr, bytes_to_store - first_part, data_src_ptr + first_part, bytes_to_store - first_part);
            if (_d->check_chunk_header && is_chunk_header)
            {
                int insert = _cache_insert_ptr;
                _d->Push(insert, ((ChunkHeader*)data));
            }
            _cache_insert_ptr = bytes_to_store - first_part;
        }

        data_src_ptr += bytes_to_store;
        data_stored += bytes_to_store;

        // Atomic update
        _cache_usage_count += bytes_to_store;

        if (!_sync_mode)
        {
            {
                std::lock_guard<std::mutex> guard(_mutex_cache_used);
                _cond_cache_used_processed = false;
            }
            _cond_cache_used.notify_all();
        }
    }
}

void IndexedFileWriter::storeToDisk(bool flush)
{
    // atomic access
    int available_data = _cache_usage_count;
    size_t cache_written = 0;

    if (!_sync_mode)
    {
        if (!flush && available_data < _cache_min_store_at_once)
        {
            // printf("HD: not enough data to store to harddisk (%d bytes)\n", availableData);

            _cond_freed_event.notify_all();
            std::unique_lock<std::mutex> lock(_mutex_cache_used);
            _cond_cache_used.wait(lock,
                [&]() -> bool {return !_cond_cache_used_processed; }); // wait for new data - TODO: adding the depending condition. Need feedback with MHE
            _cond_cache_used_processed = true;
            return;
        }
    }

    bool fill_up_sector_size = flush;

    size_t data_size = static_cast<size_t>(available_data);

    if (!flush)
    {
        data_size = std::min<size_t>(data_size, _d->cache_maximum_write_chunk_size);

        if (_system_cache_disabled)
        {
            data_size -= (data_size % _sector_size);
        }
    }

    void* cache_addr = getCacheAddr();

    if (_cache_flush_ptr + data_size <= _cache_size)
    {
        internalWrite((uint8_t*) cache_addr + _cache_flush_ptr, data_size, fill_up_sector_size);
        if (_d->check_chunk_header)
        {
            _d->CheckHeaderWritten(data_size);
        }

        //printf("HD: stored %d bytes to harddisk\n", data_size);

        _cache_flush_ptr += static_cast<int>(data_size);
        if (_cache_flush_ptr == _cache_size)
        {
            _cache_flush_ptr   = 0;
        }

        cache_written += data_size;
    }
    else
    {
        // chunks can be wrapped around the ring buffer - just write first part to disk
        int first_part = static_cast<int>(_cache_size) - _cache_flush_ptr;
        internalWrite((uint8_t*) cache_addr + _cache_flush_ptr, first_part, fill_up_sector_size);

        if (_d->check_chunk_header)
        {
            _d->CheckHeaderWritten(first_part);
        }

        _cache_flush_ptr   = 0;
        cache_written += first_part;
        available_data -= first_part;

        if (flush && data_size > 0)
        {
            // when flushing all data (while closing the file, also write the second part
            internalWrite((uint8_t*) cache_addr, data_size, fill_up_sector_size);
            if (_d->check_chunk_header)
            {
                _d->CheckHeaderWritten(data_size);
            }

            _cache_flush_ptr += static_cast<int>(data_size);
            _cache_flush_ptr %= _cache_size;
            cache_written += data_size;
            data_size = 0;
        }

        //printf("HD: stored %d bytes to harddisk\n", firstPart);
    }

    // atomic update
    _cache_usage_count -= static_cast<int>(cache_written);

    if (!_sync_mode)
    {
        _cond_freed_event.notify_all();
    }
}

void IndexedFileWriter::writeCacheToDisk()
{    
    try
    {
        while (_d->keep_writing_cache_to_disk)
        {
            storeToDisk(false);
        }
    }
    catch (...)
    {
        _last_write_result = false;
    #ifdef WIN32
        _last_write_system_error = GetLastError();
    #else
        _last_write_system_error = errno;
    #endif        
    }
}

void IndexedFileWriter::internalWrite
    (
    const void* buffer,
    size_t buffer_size,
    bool use_segment_size
    )
{
    size_t write_size = buffer_size;

    if (_system_cache_disabled && use_segment_size)
    {
        write_size = (write_size + _sector_size - 1) & ~(_sector_size - 1);
        if (write_size > buffer_size)
        {
            if (write_size <= _cache_size)
            {
                utils5ext::memZero(((uint8_t*) buffer) + buffer_size, write_size - buffer_size);
            }
            _file_pos += write_size - buffer_size;
        }
    }
    else
    {
        if (_system_cache_disabled)
        {
            IFHD_ASSERT((write_size % _sector_size) == 0);
        }
    }

    _file.writeAll(buffer, static_cast<size_t>(write_size));
}

void IndexedFileWriter::setStreamName(uint16_t stream_id,
                                       const char* stream_name)
{
    if (false == _is_open)
    {
        throw std::runtime_error("file not opended");
    }
    if (stream_id > 0
        && stream_id <= MAX_INDEXED_STREAMS)
    {
        if (stream_name[0] != '\0'
            && a_util::strings::getLength(stream_name) < MAX_STREAMNAME_LENGTH)
        {
            strncpy((char*)_stream_info[stream_id - 1].stream_name, stream_name, MAX_STREAMNAME_LENGTH);
            _stream_info[stream_id - 1].stream_name[MAX_STREAMNAME_LENGTH-1] = 0;
        }
        else
        {
            throw std::invalid_argument("invalid argument");
        }
    }
}

void IndexedFileWriter::setAdditionalStreamInfo(uint16_t stream_id,
                                                    const void* info_data,
                                                    uint32_t info_data_size,
                                                    bool use_as_reference)
{
    if (info_data_size > 0
        && stream_id > 0
        && stream_id <= MAX_INDEXED_STREAMS)
    {
        if (use_as_reference)
        {
            _stream_info_add[stream_id - 1].is_reference  = 1;
            _stream_info_add[stream_id - 1].data         = (uint8_t*) info_data;
            _stream_info[stream_id - 1].info_data_size = info_data_size;
        }
        else
        {
            _stream_info_add[stream_id - 1].is_reference  = 0;
            _stream_info_add[stream_id - 1].data         = new uint8_t[info_data_size];
            a_util::memory::copy(_stream_info_add[stream_id - 1].data, info_data_size, info_data, info_data_size);
            _stream_info[stream_id - 1].info_data_size = info_data_size;
        }

        return;
    }

    throw std::invalid_argument("invalid argument(s)");
}

uint32_t IndexedFileWriter::getLastSystemErrorFromWriteThread() const
{
    return _last_write_system_error;
}

} // v201_v301
} // namespace ifhd
