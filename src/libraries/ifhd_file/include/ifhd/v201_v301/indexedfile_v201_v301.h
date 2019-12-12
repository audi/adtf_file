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

#ifndef INDEXEDFILE_V201_V301_CLASS_HEADER
#define INDEXEDFILE_V201_V301_CLASS_HEADER

namespace ifhd
{
namespace v201_v301
{

///TYPES 

//*************************************************************************************************
/*

    FILE LAYOUT:

    +------------------------------------------------------------------------------------+
    |HDR|         DATA         |        EXTENSIONS         |    EXTENSION-HEADER TABLE   |
    +------------------------------------------------------------------------------------+

*/

/**
 * \if ADTF_DOC_BUILD
 * \page page_analyze_datfile How To Analyze DAT-File
 *
 *
 * There is a 'fileaccess' example in the \b Streaming \b Library. It exports all \b ChunkTimeS and \b SampleTimeS to a
 * csv file. This is helpful if you want to know whether these times are correct. @see @ref page_add_adtf_times
 * \endif
 *
 */

//*************************************************************************************************
class IndexedFile;



//*************************************************************************************************
/**
 * Base class for all indexed file classes.
 * 

    \par FILE LAYOUT for Version 2.0.1
    <br><br>
    +-------------------------------------------------------------------------------------+<br>
    |HDR|                   DATA                | EXTENSIONS | EXTENSIONSHEADER-HDR       |<br>
    +-------------------------------------------------------------------------------------+<br>
    <br><br>
    
    \par FILE LAYOUT for Version 2.0.2
    <br><br>
    +--------------------------------------------------------------------------------------------------+<br>
    |HDR| QUEUED HISTORY CHUNK DATA |   SORTED CHUNK DATA    | EXTENSIONS | EXTENSIONSHEADER-HDR       |<br>
    +--------------------------------------------------------------------------------------------------+<br>
    <br><br>

   \par Special Extensions to provide: 
   \li GUUID within the File 
   \li MASTER Index Table  
   \li Stream Index Tables

*/

class DOEXPORT IndexedFile
{
    protected:

        /// current value of platform ByteOrder (@see PLATFORM_BYTEORDER_UINT8)
        static const uint8_t byte_order;
        /// Default block size in bytes
        static int64_t default_block_size;
        /// Default cache size in bytes
        static int64_t default_cache_size;
        /// current sector size used for disk device
        /// depend on drive given by filename within Create method of IndexedFileReader or cIndexFileWriter
        int64_t _sector_size;
        /// whether system cache used or not
        /// depend on flags given within Create method of IndexedFileReader or cIndexFileWriter
        bool _system_cache_disabled;

        /// the open file
        utils5ext::File _file;
        /// current filepos
        FilePos _file_pos;

        /// internal Buffer
        uint8_t* _buffer;
        /// internal Buffer size
        int64_t _buffer_size;
        /// current file header
        FileHeader* _file_header;

        /// protection for writing GUID
        bool _write_guid;

        /**
         * combines a file extension information with a concrete extension page
         */
        typedef struct
        {
            FileExtension file_extension; //!< file extension intormation
            void* extension_page; //!< pointer to extension page
        } FileExtensionStruct;

        /// own type definition for a better work with file extension lists
        typedef std::list<FileExtensionStruct*> FileExtensionList;

        /// list with all extensions
        FileExtensionList _extensions;

        /// cache data area
        void* _cache;
        /// size of cache
        uint64_t _cache_size;

        /// For internal use only (will be moved to a private implementation).
        bool _write_mode;

    public:
        /**
         * Default constructor.
         */
        IndexedFile();
        /**
         * Destructor.
         */
        virtual ~IndexedFile();


        /**
         * Closes the file.
         * @return Standard result.
         */
        virtual void close();

        virtual std::set<uint32_t> getSupportedVersions() const;

        /**
         * Sets the description of the file.
         * @param description [in] The new description.
         * @return Standard result.
         * @rtsafe
         */
        void setDescription(const std::string& description);
        /**
         * Returns the description of the file.
         * @return The description of the file.
         * @rtsafe
         */
        std::string getDescription() const;

        /**
         * Get the GUID of the file.
         * @param gUID [out] the GUID of the file
         * @return Standard result.
         * @rtsafe
         */
        std::string getGUID() const;

        /**
         * Get the timestamp of the file.
         * @param dateTime [out] The timestamp of the file.
         * @return Standard result.
         * @rtsafe
         */
        a_util::datetime::DateTime getDateTime() const;

        /**
         * Returns the byteorder of the file.
         * @return PLATFORM_LITTLE_ENDIAN_8 or PLATFORM_BIG_ENDIAN_8
         * @rtsafe
         */
        uint8_t getByteOrder() const;

        /**
         * Get the amount of extensions in the file.
         * @return The amount of extensions in the file.
         * @rtsafe
         */
        size_t getExtensionCount() const;

        /**
         * Finds an extension with a specific identifier.
         * @param identifier [in] The identifier of the extension
         * @param extensionInfo [out] The extension info data.
         * @param data [out] The extension data.
         * @return Standard result.
         * @rtsafe
         */
        bool findExtension(const char* identifier, FileExtension** extension_info, void** data) const;

        /**
         * Get an extension with a specific index.
         * @param index [in] The index of the extension.
         * @param extensionInfo [out] The extension info data.
         * @param data [out] The extension data.
         * @return Standard result.
         * @rtsafe
         */
        void getExtension(size_t index, FileExtension** extension_info, void** data) const;

        /**
         * Adds a new extension to the file.
         * @warning The extension 'GUID' is protected and could not be overwritten
         * @param identifier [in] The identifier of the extension.
         * @param data         [in] The extension data.
         * @param dataSize     [in] The data size.
         * @param typeId    [in] An optional type id
         * @param versionId [in] An optional version id
         * @param streamId  [in] An optional stream id
         * @param userId    [in] An optional user id
         * @return Standard result.
         */
        void appendExtension(const char* identifier,
                                const void* data,
                                size_t data_size,
                                uint32_t type_id=0,
                                uint32_t version_id=0,
                                uint16_t stream_id=0,
                                uint32_t user_id=0);

        /**
         * Adds a new extension to the file.
         * @warning The extension 'GUID' is protected and could not be overwritten
         * @param data [in] The extension data.
         * @param extensionInfo [in] The extension info.
         * @return Standard result.
         */
        void appendExtension(const void* data, const FileExtension* extension_info);

        /**
         * Frees all extensions.
         * @return Standard result.
         */
        void freeExtensions();

        /**
         * Returns the file header.
         * @param fileHeader [out] Will point to the file header.
         * @return Standard result.
         * @rtsafe
         */
        void getHeaderRef(FileHeader** file_header) const;

    protected:
        /**
         * Set the timestamp of the file. The timestamp is calculated from
         * the passed date/time structure while the current time zone from the
         * system is used.
         * @param dateTime [in] The new date/time for the timestamp.
         * @return Standard result.
         */
        void setDateTime(const a_util::datetime::DateTime& date_time);
        /**
         * Initializes internal data.
         */
        virtual void initialize();

        /**
         * Allocates an internal buffer.
         * @param [in] size The size in bytes to allocate.
         * @return Standard result.
         */
        void allocBuffer(uint64_t size);

        /**
         * Releases the internal buffer.
         */
        void freeBuffer();

        /**
         * Allocates the header structure/data.
         */
        void allocHeader();

        /**
         * Releases the header data.
         */
        void freeHeader();

        /**
         * Allocate memory for an extension.
         * @param [in] size The amount of bytes to allocate.
         * @param [out] data Pointer to the allocated space.
         * @return Standard result.
         */
        void allocExtensionPage(utils5ext::FileSize size, void** data) const;

        /**
         * Sets the GUID of the file.
         * @return Standard result.
         * @rtsafe
         */
        void setGUID();
        
        /**
         * Generates a new GUID (XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)
         * @param generatedGUID [out] new generated GUID
         * @return Standard result.
         * @rtsafe
         */
        void generateNewGUID(std::string& generated_guid);

    protected:

        /**
         * Initialzes an internal cache.
         * @param [in] size The size of the cache.
         * @return Standard result.
         */
        void allocCache(int64_t size);

        /**
         * Releases the cache.
         */
        void freeCache();

        /**
         * Returns the cache pointer @internal.
         */
        void* getCacheAddr() const;

        /**
         * Calculates the size a data block requires on disk.
         * @param [in] size The size of the data block.
         * @param [in] useSegmentSize Include the segment size into the calculation.
         * @return The calculated size.
         */
        utils5ext::FileSize getSizeOnDisk(utils5ext::FileSize size, bool use_segment_size) const;
        
        /**
         * Allocates memory.
         * @param [in] size The size of the data block.
         * @param [in] useSegmentSize Whether or not to align the memory on segment boundaries.
         * @return Pointer to the allocated space.
         */
        void* internalMalloc(size_t size, bool use_segment_size) const;

        /**
         * Release memory.
         * @param [in] memory The memory which should be free'd.
         * @return void
         */
        void internalFree(void* memory) const;
};

} // namespace v400
} // namespace ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_V201_V301_CLASS_HEADER_
