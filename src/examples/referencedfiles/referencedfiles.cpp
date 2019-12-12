/**
 * @file
 * Implements an example program that shows how to easily work with referenced file paths delivered
 * within the extension page of an ADTF dat-file.
 *
 * This example shows:
 * \li how to read and write referenced file paths from and to an ADTF dat-file respectively
 * \li how to iterate over referenced file paths
 * \li how to remove, add or replace referenced file paths
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

#include <iostream>         //std::cout, std::endl
#include <cstring>          //std::string
#include <adtf_file/standard_adtf_file_reader.h>
#include <adtf_file/adtf_file_writer.h>
#include <adtf_file/standard_factories.h>
#include <adtf_file/file_extensions.h>


//prototype to print referenced file paths
void print_referenced_files(std::vector<a_util::filesystem::Path> referencedfiles)
{
    for (auto file : referencedfiles)
    {
        std::cout << "    " << file.toString() << std::endl;
    }
}

//main function
int main(int argc, char* argv[])
{
    if (2 != argc)
    {
        std::cout << "usage: referencedfiles.exe <datfile>\n" << std::endl;
        return -1;
    } 

    a_util::filesystem::Path filename = argv[1];
    std::vector<a_util::filesystem::Path> referenced_files_list;
    {
        try 
        {
            adtf_file::StandardReader reader(filename.toString());
            adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
            std::cout << "The original referenced files: " << std::endl;
            referenced_files_list = referenced_files_ext.getFiles();
            print_referenced_files(referenced_files_list);
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
        }
    }

    a_util::filesystem::Path copy_extension_filename = filename.getParent().toString() + "copy";
    {
        adtf_file::ReferencedFilesExtension referenced_files_ext(referenced_files_list);
        adtf_file::Writer writer(copy_extension_filename.toString(),
                                 std::chrono::seconds(0),
                                 adtf_file::adtf3::StandardTypeSerializers());
        referenced_files_ext.write(writer);
    }

    try
    {
        adtf_file::StandardReader reader(copy_extension_filename.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        std::cout << "The copy referenced files: " << std::endl;
        referenced_files_list = referenced_files_ext.getFiles();
        print_referenced_files(referenced_files_list);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    

    return 0;
}
