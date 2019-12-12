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

#ifndef INDEXEDFILE_HELPER_V201_V301_HEADER
#define INDEXEDFILE_HELPER_V201_V301_HEADER


namespace ifhd
{
namespace v201_v301
{
    
a_util::datetime::DateTime getDateTimeHelper(const FileHeader& file_header);

a_util::datetime::DateTime getDateTime(const a_util::filesystem::Path& filename);

/**
    * Returns the header of a file.
    * @param filename [in] The filename
    * @param fileHeader [out] This will be filled with the information.
    * @return Standard result.
    */
void getHeader(const std::string& filename,
                   FileHeader& file_header);

/**
    * Update header info.
    * @param filename [in] The filename.
    * @param fileHeader [in] The new header info.
    * @param mask [in] Which fields to update, see
    * \link IndexedFileHelper::tFieldMask \c IndexedFileHelper::tFieldMask 
    * \endlink.
    * @return Standard result.
    */
void updateHeader(const std::string& filename,
                      const FileHeader& file_header,
                      uint32_t mask);

/**
    * Returns the date and the description of a file in a string.
    * @param filename [in] The filename.
    * @param fileInfo [out] The info string.
    * @return Standard result.
    */
void queryFileInfo(const std::string& filename,
                        std::string& file_info);

/**
    * Returns the date and the description of a file in a string.
    * It will also check if the given extension exits.
    * @param filename   [in] The filename.
    * @param fileInfo   [out] The info string.
    * @param extensions [inout] the list of extensions.
    * @return Standard result.
    */
void queryFileInfo(const std::string& filename,
                        std::string& file_info,
                        std::list<std::string>& extensions);

/**
    * Return the requested file extension.
    * You have to release the memory for the return value data with 
    * delete [] data;
    *
    * @param filename     [in]  The input file name.
    * @param extension    [in]  The requested file extension.
    * @param extensionInfo [out] This will be filled with the file extension information.
    * @param data          [out] This will be filled with the file extension data.
    * @return Standard result.
    */
void getExtension(const std::string& filename,
                      const std::string& extension,
                      FileExtension* extension_info,
                      void** data);

/**
* Write an extension
* If the extension already exists, it will be overwritten
* @warning The extension 'GUID' is protected and could not be overwritten
*
* @param filename     [in]  The input file name.
* @param extensionInfo [in]  This will be filled with the file extension information.
* @param data           [in]  This will be filled with the file extension data.
* @return Standard result.
*/
void writeExtension(const std::string& filename,
                        const FileExtension& extension_info,
                        const void* data);

/**
    * Check if a particular file is a DAT file.
    *
    * @param filename     [in]  Name of the file to check.
    * @return Standard result.
    */
void isIfhdFile(const std::string& filename);

/**
    * Convert a file header structure to native byte order.
    * @param [in] fileHeader The header
    * @return Standard result.
    * @rtsafe
    */
void stream2FileHeader(FileHeader& file_header);

/**
    * Convert a file header extension structure to native byte order.
    * @param [in] fileHeader The header
    * @param [in] headerExt The header extension.
    * @param [in] numExtensions The number of Extensions.
    * @return Standard result.
    * @rtsafe
    */
void stream2FileHeaderExtension(const FileHeader& file_header, FileExtension* header_ext, size_t num_extensions);

/**
    * Convert a chunk header structure to native byte order.
    * @param [in] fileHeader The header
    * @param [in] psChunk The chunk header.
    * @return Standard result.
    * @rtsafe
    */
void stream2ChunkHeader(const FileHeader& file_header, ChunkHeader& chunk);

/**
    * Convert a chunk reference structure to native byte order.
    * @param [in] fileHeader The header
    * @param [in] psChunkRef The chunk reference
    * @return Standard result.
    * @rtsafe
    */
void stream2ChunkRef(const FileHeader& file_header, ChunkRef& chunk_ref);

/**
    * Convert a stream reference structure to native byte order.
    * @param [in] fileHeader The header
    * @param [in] streamRef The stream reference
    * @return Standard result.
    * @rtsafe
    */
void stream2StreamRef(const FileHeader& file_header, StreamRef& stream_ref);

/**
    * Convert a stream info header structure to native byte order.
    * @param [in] fileHeader The header
    * @param [in] psStreamInfo The stream info.
    * @return Standard result.
    * @rtsafe
    */
void stream2StreamInfoHeader(const FileHeader& file_header, StreamInfoHeader& stream_info);


/**
    * Convert a additional stream info structure to native byte order.
    * @param [in] fileHeader The header
    * @param [in] psAdditonalIndexInfo The additional index info.
    * @return Standard result.
    * @rtsafe
    */
void stream2AdditionalStreamIndexInfo(const FileHeader& file_header, AdditionalIndexInfo& additonal_index_info);


} // namespace
} // ifhd

//*************************************************************************************************
#endif // _INDEXEDFILE_HELPER_V201_V301_HEADER_
