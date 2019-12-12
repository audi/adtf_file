/**
 * @file
 * Class for file access. This class implements functions for file handling.
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

#ifndef FILE_CLASS_EXT_HEADER
#define FILE_CLASS_EXT_HEADER

namespace utils5ext
{

using FilePos  = int64_t;
using FileSize = FilePos;


/// The type FileHandle is used internally only
#ifdef WIN32
    typedef void* FileHandle;
#else
    typedef int FileHandle;
#endif

/**
 *
 * File class.
 *
**/
class DOEXPORT File
{
    public:
        /// Several flags to open files. Can be combined with bitwise or
        typedef enum
        {
            /// Open existing file. Can be combined with OM_SharedRead under Windows.
            /// Under Windows, the file must exist. Under Linux, if the file doesn't exist it will
            /// be created.
            om_read                   = 0x0001,

            /// Creates a new file or overwrites existing files.
            /// Can be combined with OM_SharedWrite under windows.
            om_write                  = 0x0002,

            /// Append data to existing file. Sets seeking position to the end of the file stream
            om_append                 = 0x0004,

            /// Open file in read-write mode. Under Windows, the file must exist. Under Linux,
            /// if the file doesn't exist it will be created.
            om_read_write              = 0x0008,

            /// Allows shared read access, use in addition to flags above.
            /// Exclusive write/read permissions for a whole file are not supported under Linux 
            /// platforms without any file locks. All files are readable!
            om_shared_read             = 0x0100,

            /// Allows shared write access, use in addition to flags above.
            /// Exclusive write/read permissions for a whole file are not supported under Linux
            /// platforms without any file locks
            om_shared_write            = 0x0200,

            /// Optimized for sequential access, use in addition to flags above
            om_sequential_access       = 0x0400,

            /// File is of temporary purpose and will be deleted after all handles on the file are
            /// closed. Use in addition to flags above
            om_temporary_file          = 0x0800,

            /// Advices the OS not to flush any data to the disk. Just supported on POSIX platforms.
            om_short_lived             = 0x1000,

            /// Opens the file in text mode instead of binary mode. Just supported on POSIX
            /// platforms
            om_text_mode               = 0x2000,

            /// Immediately flush file data to disk. Windows only.
            om_write_through           = 0x4000,

            /// Disable all file system caches. Windows only.
            om_disable_file_system_cache = 0x8000

        } OpenMode;

        /// File position reference
        typedef enum
        {
            /// offsets are measured from the beginning of the file
            fp_begin   = 0,

            /// offsets are measured from the current file position
            fp_current = 1,

            /// offsets are measured from the end of the file
            fp_end     = 2
        } FilePosRef;

    protected:
        FileHandle _file;                  //!< File handle
        uint8_t*   _read_cache;            //!< File read cache
        size_t     _file_cache_usage;      //!< File cache usage
        size_t     _file_cache_offset;     //!< File cache offset
        size_t     _file_cache_size;       //!< File cache size
        bool       _read_cache_enabled;    //!< Read cache enabled
        bool       _reference;             //!< File is reference
        bool       _system_cache_disabled; //!< System cache disabled
        int        _sector_size;           //!< Sector size
        FilePos    _sector_bytes_to_skip;  //!< Sector bytes that will be skipped

    public:
        /// Constructor
        File();

        /// Destructor. If handling an open file, the file is closed.
        virtual ~File();

        /**
         * This function opens an existing or creates a new file.
         *
         * @param  filename [in] string that is the path to the desired
         *              file. The path can be relative, absolute, or
         *              a network name (UNC). uint32_t Mode: A uint32_t
         *              that defines the file sharing and access
         *              mode. It specifies the action to take when
         *              opening the file. You can combine options by
         *              using the bitwise-OR ( | ) operator.
         * @param  mode [in] File open mode. For valid flags see the
         *              OpenMode enum.
         *
         */
        void open(const a_util::filesystem::Path& filename, uint32_t mode);

        /**
         *
         * Close file.
         *
        **/
        void close();

        /**
         *
         * This function attaches an existing instance of a file class to another.
         *
         * @param  file [in] Reference to an existing file object.
         *
         * @return Always return ERR_NOERROR
         *
         */
        void attach(File& file);

        /**
         *
         * This function detaches existing references to file objects.
         *
         * @return ERR_NOERROR if referenced file was detached
         * @return ERR_UNEXPECTED if no file was attached
         *
         */
        void detach();

        /**
         *
         * This function sets the file read cache size.
         *
         * @param  cacheSize [in] Number of bytes to be allocated as read buffer. If this less than zero,
         *                         a read cache the size of the default sector size is allocated
         *
         * @retval true if the read cache was allocated
         * @retval false in case of error
         *
         */
        void setReadCache(size_t cache_size);

        /**
         *
         * This function reads data into a buffer from the file associated with the File object.
         *
         * @param  buffer [in] Pointer to the user-supplied buffer that is to receive
         *              the data read from the file.
         * @param  bufferSize [in] The maximum number of bytes to be read from the file.
         *              For text-mode files, carriage return-linefeed pairs
         *              are counted as single characters.
         *
         * @return The number of bytes transferred to the buffer. Note that the return value
         *         may be less than bufferSize, if the end of file was reached. If there was
         *         a severe error, the function returns -1.
         *
        **/
        size_t read(void* buffer, size_t buffer_size);

        /**
         *
         * This function reads data into a buffer from the file associated with the File object.
         * It ensures that all requested bytes have been read.
         *
         * @param  buffer [in] Pointer to the user-supplied buffer that is to receive
         *              the data read from the file.
         * @param  bufferSize [in] The maximum number of bytes to be read from the file.
         *              For text-mode files, carriage return-linefeed pairs
         *              are counted as single characters.
         *
         * @retval ERR_NOERROR bufferSize bytes have been read.
         * @return ERR_IO_INCOMPLETE not all bytes could be read.
         *
        **/
        void readAll(void* buffer, size_t buffer_size);

         /**
         *
         * This function reads data from a file but does not store it .
         *
         * @param  numberOfBytes [in] The maximum number of bytes to be skipped.
         *
         * @return The number of bytes skipped. Note that the return value
         *         may be less than numberOfBytes, if the end of file was reached. If there was
         *         a severe error or the maximum number of bytes to skip was negative, the function returns -1.
         *
        **/
        size_t skip(size_t number_of_bytes);

        /**
         *
         * This function writes data from a buffer to the file associated with the File object.
         *
         * @param     buffer [in] Pointer to the user-supplied buffer that is to receive
         *                 the data read from the file.
         * @param     bufferSize [in] The maximum number of bytes to be read from the file.
         *                 For text-mode files, carriage return-linefeed pairs
         *                 are counted as single characters. If bufferSize is 0, the return value
         *                 is always 0 and nothing is written.
         *
         * @return    The number of bytes transferred to the buffer. If there was an error or buffer was nullptr or
         *            bufferSize was less than 0, -1 is returned.
         * @attention On win32 platforms only: If the file was opened with OM_DisableFileSystemCache there is a
         *            limitation of the maximum buffer size of approx. 64MB on x86 / approx. 32MB on x86-64.
         *            For further details have a look at MSDN WriteFile.
         *
         */
        size_t write(const void* buffer, size_t buffer_size);

        /**
         *
         * This function writes data from a buffer to the file associated with the File object.
         * It ensures that all requested bytes have been written.
         *
         * @param     buffer [in] Pointer to the user-supplied buffer that is to receive
         *                 the data read from the file.
         * @param     bufferSize [in] The maximum number of bytes to be read from the file.
         *                 For text-mode files, carriage return-linefeed pairs
         *                 are counted as single characters. If bufferSize is 0, the return value
         *                 is always 0 and nothing is written.
         *
         * @retval ERR_NOERROR bufferSize bytes have been written.
         * @return ERR_IO_INCOMPLETE not all bytes could be written.
         *
         */
        void writeAll(const void* buffer, size_t buffer_size);

        /**
         *
         * This function writes string data from a buffer to the file associated with the File object.
         *
         * @param  string [in] String object to be written to the file.
         *
         * @return The number of bytes transferred to the file. If there was an error, -1
         *         is returned.
         *
        **/
        void write(const std::string& string);

        /**
         *
         * This function reads a line of text data into a String object.
         *
         * @param  string [inout] String object to be filled with character data.
         *
         * @return The number of characters transferred to the string.
         *
        **/
        void readLine(std::string& string);

        /**
         *
         * This function writes string data from a buffer to the file associated with the File object. The string
         * is followed by a carriage return-linefeed pair.
         *
         * @param  string [in] String object to be written to the file.
         *
         * @return The number of bytes transferred to the buffer. If there was an error, -1
         *         is returned.
         *
        **/
        void writeLine(const std::string& string);

        /**
         *
         * This function retrieves the file size.
         *
         * @return The function returns the file size as 64-bit integer.
         *
        **/
        FileSize getSize() const;

        /**
         *
         * This function retrieves the file pointer offset.
         *
         * @return The function returns the file pointer offset from the beginning as a 64-bit integer.
         *
        **/
        FilePos getFilePos() const;

        /**
         *
         * This function moves the file pointer.
         *
         * @param  offset [in] Offset value for pointer movement, relative to MoveMode.
         * @param  MoveMode [in] Fixed reference point for movement.
         * @remark The file pointer may be set beyond the end of the File. The next write will
         *         increase the filesize accordingly.
         * @return Returns the file position which was set (must not be equal to offset)
         *         or -1 if any error occurred.
         *
        **/
        FilePos setFilePos(FilePos offset, FilePosRef move_mode);

 

        /**
         * Checks whether or not the end of the file has been reached.
         * @return Whether or not the end of the file has been reached.
         */
        bool isEof();

        /**
         *
         * This function checks if the file object has an valid file handle.
         *
         * @return Returns true, if the file handle is valid, otherwise false.
         * @rtsafe
         *
        **/
        bool isValid() const;

        /**
         * This method truncates the file at the given size.
         *
         * If the file is larger than the given size, the extra data is lost.
         * If it was smaller it is extended.
         * @param size
         * @return OK if the operation was successful.
         */
        void truncate(FilePos size);

    protected:
        /**
         * Initialization.
         */
        void initialize();

        /**
         * Allocate read cache.
         *
         * @param cacheSize [in] Size of cache.
         */
        void allocReadCache(size_t cache_size);

        /**
         * Free read cache.
         */
        void freeReadCache();

        /**
         * Internal allocation method.
         *
         * @param size [in] Size of cache.
         * @param useSegmentSize [in] Keep segment size in mind.
         *
         * @return Void pointer to allocated memory.
         */
        void* internalMalloc(size_t size, bool use_segment_size=false);

        /**
         * Internal free method.
         *
         * @param memory [in] Pointer to memory.
         * @param useSegmentSize [in] Keep segment size in mind.
         * @return void
         */
        void internalFree(void* memory, bool use_segment_size=false);
};

/**
*  This function returns the last access time of the file
*  @param dt a_util::cDateTime of the last access
*  @param filename Filename of the file to be checked
*  @return OK if the time has been retrieved, ERR_INVALID_ARG if filename is empty, ERR_UNKNOWN if time cannot be read;
*
*  @attention From MSDN chapter "File Times":
*  Not all file systems can record creation and last access times, and not all file systems record them in the same manner.
*  \n For example, the resolution of create time on FAT is 10 milliseconds, while write time has a resolution of 2 seconds and access time has a resolution of 1 day, so it is really the access date.
*  \n The NTFS file system delays updates to the last access time for a file by up to 1 hour after the last access.
*/
a_util::datetime::DateTime getTimeAccess(const a_util::filesystem::Path filename);
/**
* This function returns the creation time of the file.
*
* @param dt a_util::cDateTime of the last access
* @param filename Filename of the file to be checked
* @return OK if the time has been retrieved, ERR_INVALID_ARG if filename is empty, ERR_UNKNOWN if time cannot be read;
*
*  @attention From MSDN chapter "File Times":
*  Not all file systems can record creation and last access times, and not all file systems record them in the same manner.
*  \n For example, the resolution of create time on FAT is 10 milliseconds, while write time has a resolution of 2 seconds and access time has a resolution of 1 day, so it is really the access date.
*  \n The NTFS file system delays updates to the last access time for a file by up to 1 hour after the last access.
*/
a_util::datetime::DateTime getTimeCreation(const a_util::filesystem::Path filename);

/**
*  This function returns the last change (write) time of the file
*
*  @param  dt  a_util::cDateTime of the last access
*  @param  filename    Filename of the file to be checked
*  @return OK if the time has been retrieved, ERR_INVALID_ARG if filename is empty, ERR_UNKNOWN if time cannot be read;
*
*  @attention From MSDN chapter "File Times":
*  Not all file systems can record creation and last access times, and not all file systems record them in the same manner.
*  \n For example, the resolution of create time on FAT is 10 milliseconds, while write time has a resolution of 2 seconds and access time has a resolution of 1 day, so it is really the access date.
*  \n The NTFS file system delays updates to the last access time for a file by up to 1 hour after the last access.
*/
a_util::datetime::DateTime getTimeChange(const a_util::filesystem::Path filename);


size_t getDefaultSectorSize();

void* allocPageAlignedMemory(size_t size, size_t page_size);

void freePageAlignedMemory(void* memory);

size_t getSectorSizeFor(const a_util::filesystem::Path& filename);

inline void memZero(void* data, size_t bytes)
{
    a_util::memory::set(data, bytes, 0x00, bytes);
}

void fileRename(const a_util::filesystem::Path& from, const a_util::filesystem::Path& to);

} // namespace A_UTILS_NS

#endif // _FILE_CLASS_EXT_HEADER_
