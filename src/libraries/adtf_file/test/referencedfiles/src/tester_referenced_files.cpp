/**
 * @file
 * Tester init.
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

#include "gtest/gtest.h"
#include <adtf_file/standard_adtf_file_reader.h>
#include <adtf_file/file_extensions.h>

using namespace adtf_file;


GTEST_TEST(TestReadReferenceFiles, ReferencedFiles)
{
    std::vector<a_util::filesystem::Path> check = {"./split_001.dat" };
    std::vector<a_util::filesystem::Path> check1 = { "./split.dat", "./split_002.dat" };
    std::vector<a_util::filesystem::Path> check2 = check;

    a_util::filesystem::Path filename = TEST_FILES_DIR;
    filename.append("split.dat");
    a_util::filesystem::Path filename1 = TEST_FILES_DIR;
    filename1.append("split_001.dat");
    a_util::filesystem::Path filename2 = TEST_FILES_DIR;
    filename2.append("split_002.dat");

    {
        adtf_file::StandardReader reader(filename.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        ASSERT_TRUE(check == referenced_files_ext.getFiles());
    }

    {
        adtf_file::StandardReader reader(filename1.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        ASSERT_TRUE(check1 == referenced_files_ext.getFiles());
    }
    {
        adtf_file::StandardReader reader(filename2.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        ASSERT_TRUE(check2 == referenced_files_ext.getFiles());
    }
}

GTEST_TEST(TestWriteReferenceFiles, ReferencedFiles)
{
    a_util::filesystem::Path filename = TEST_FILES_DIR;
    filename.append("test.adtfdat");
    try
    {
        a_util::filesystem::remove(filename);
    }
    catch (...)
    {

    }
    std::vector<a_util::filesystem::Path> check = { "./bla.dat", "./test.dat" , "ich bins" };
    {
        adtf_file::ReferencedFilesExtension referenced_files_ext(check);

        adtf_file::Writer writer(filename.toString(),
            std::chrono::seconds(0),
            adtf_file::adtf3::StandardTypeSerializers());
        referenced_files_ext.write(writer);
    }
    {
        adtf_file::StandardReader reader(filename.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        ASSERT_TRUE(check == referenced_files_ext.getFiles());
    }
}

GTEST_TEST(TestChangeReferenceFiles, ReferencedFiles)
{
    a_util::filesystem::Path filename = TEST_FILES_DIR;
    filename.append("test.adtfdat");
    try
    {
        a_util::filesystem::remove(filename);
    }
    catch (...)
    {

    }
    std::vector<a_util::filesystem::Path> check = { "./bla.dat", "./test.dat" , "ich bins" };
    std::vector<a_util::filesystem::Path> check_changed = { "./bla.adtfdat", "./test.adtfdat" , "ich bins nochmal" };
    {
        adtf_file::ReferencedFilesExtension referenced_files_ext(check);

        adtf_file::Writer writer(filename.toString(),
            std::chrono::seconds(0),
            adtf_file::adtf3::StandardTypeSerializers());
        referenced_files_ext.write(writer);
    }
    {
        adtf_file::StandardReader reader(filename.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        ASSERT_TRUE(check == referenced_files_ext.getFiles());
    }
    {
        adtf_file::ReferencedFilesExtension referenced_files_ext(check_changed);
        referenced_files_ext.change(filename);
    }
    {
        adtf_file::StandardReader reader(filename.toString());
        adtf_file::ReferencedFilesExtension referenced_files_ext(reader);
        ASSERT_TRUE(check_changed == referenced_files_ext.getFiles());
    }
}


