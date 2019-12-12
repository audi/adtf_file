/**
 * @file
 * file extension support.
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

#ifndef ADTF_FILE_EXTENSIONS
#define ADTF_FILE_EXTENSIONS

#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/adtf_file_writer.h>


namespace adtf_file
{
//static constexpr const char* const referenced_files_id = "referencedfiles";

class ReferencedFilesExtension
{ 
    public:
        static constexpr const char* const referenced_files_id = "referencedfiles";
    
        ReferencedFilesExtension() = default;
        ReferencedFilesExtension(ReferencedFilesExtension&& reader);
        ReferencedFilesExtension& operator=(ReferencedFilesExtension&& reader);
        ReferencedFilesExtension(const Reader& reader);
        ReferencedFilesExtension(const std::vector<a_util::filesystem::Path>& files);

        void read(const Reader& reader);
        void write(Writer& writer) const;
        void change(a_util::filesystem::Path filename) const;

        std::vector<a_util::filesystem::Path> getFiles() const
        {
            return _files;
        }
        void setFiles(const std::vector<a_util::filesystem::Path>& files)
        {
            _files = files;
        }
    private:
        std::vector<a_util::filesystem::Path> _files;
};



}

#endif  //_ADTF_FILE_EXTENSIONS_
