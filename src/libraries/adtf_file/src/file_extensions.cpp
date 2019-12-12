/**
 * @file
 * extension impl.
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

#include <adtf_file/file_extensions.h>
#include <cstring>

namespace adtf_file
{

ReferencedFilesExtension::ReferencedFilesExtension(ReferencedFilesExtension&& reader)
{
    _files = std::move(reader._files);
}

ReferencedFilesExtension& ReferencedFilesExtension::operator=(ReferencedFilesExtension&& reader)
{
    _files = std::move(reader._files);
    return *this;
}

ReferencedFilesExtension::ReferencedFilesExtension(const Reader& reader)
{
    read(reader);
}

ReferencedFilesExtension::ReferencedFilesExtension(const std::vector<a_util::filesystem::Path>& files) : _files(files)
{
}


void ReferencedFilesExtension::read(const Reader& reader)
{
    auto extensions = reader.getExtensions();
    for (auto extension : extensions)
    {
        if (extension.name == referenced_files_id)
        {
            const char* current_ptr = static_cast<const char*>(extension.data);
            while ((size_t)(current_ptr - static_cast<const char*>(extension.data)) < extension.data_size)
            {
                std::string file = current_ptr;
                current_ptr += file.length() + 1;
                _files.push_back(file);
            }
            return;
        }
    }
    throw std::invalid_argument("extension for referenced files not found");
}

void ReferencedFilesExtension::write(Writer& writer) const
{
    auto extstream = writer.getExtensionStream(referenced_files_id, 0, 0, 0);
    for (auto& file : _files)
    {
        std::string file_string = file.toString();
        //always write with trailing "\0"
        extstream->write(file_string.c_str(), file_string.length() + 1);
    }
}
    
void ReferencedFilesExtension::change(a_util::filesystem::Path filename) const
{
    size_t bytecount(0);
    for (auto& file : _files)
    {
        std::string file_string = file.toString();
        bytecount += (file_string.length() + 1);
    }
    
    a_util::memory::MemoryBuffer buffer(bytecount);
    uint8_t* ptr = static_cast<uint8_t*>(buffer.getPtr());
    for (auto& file : _files)
    {
        std::string file_string = file.toString();
        size_t currentbytes = (file_string.length() + 1);
        a_util::memory::copy(ptr, currentbytes, file_string.c_str(), currentbytes);
        ptr = ptr + currentbytes;
    }
    ifhd::v400::FileExtension ext;
    strcpy((char*)ext.identifier, referenced_files_id);
    ext.stream_id = 0;
    ext.type_id   = 0;
    ext.user_id   = 0;
    ext.version_id = 0;
    ext.data_pos   = 0;
    ext.data_size = buffer.getSize();
    ifhd::v400::writeExtension(filename, ext, buffer.getPtr());
    return;
}
    
}
