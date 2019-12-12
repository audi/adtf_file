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
#include <ifhd/ifhd.h> 
#include <iostream>
#include "../../test_helper/test_helper.h"


#define TESTFILE TEST_FILES_DIR "/test_helper.dat"
#define TESTFILE_MD5 TEST_FILES_DIR "/test_helper.md5sum.dat"
#define TESTFILE_INCOMPLETE TEST_FILES_DIR "/test_incomplete_header.dat"
#define TESTFILE_INVALID    TEST_FILES_DIR "/test_invalid_header.dat"
#define TESTFILE_INVALID_MD5 TEST_FILES_DIR "/test_helper.invalid_md5sum"
#define TESTFILE_BZ2 TEST_FILES_DIR "test_helper.dat.bz2"
#define EXTENSION_ID "index0"


DEFINE_TEST(TesterIndexedFileHelper,
            TestGetHeader,
            "1.1",
            "GetHeader",
            "Test GetHeader",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader header;
    memset(&header,0,sizeof(FileHeader));
    A_UTILS_TEST_RESULT_EXT(getHeader(TESTFILE, header),
                              "failed to retrieve header of testfile");
    A_UTILS_TEST(0x44484649==header.file_id);
    A_UTILS_TEST(0x00000201==header.version_id);
    A_UTILS_TEST(0==header.flags);
    A_UTILS_TEST(5==header.extension_count);
    A_UTILS_TEST(112640==header.extension_offset);
    A_UTILS_TEST(2048==header.data_offset);
    A_UTILS_TEST(104832==header.data_size);
    A_UTILS_TEST(1456==header.chunk_count);
    A_UTILS_TEST(65==header.max_chunk_size);
    A_UTILS_TEST(14539139==header.duration);
    A_UTILS_TEST(1244192266==header.file_time); 
    A_UTILS_TEST(0x01==header.header_byte_order);
    A_UTILS_TEST(1487001==header.time_offset);
    std::string desc = "helper test file\nfile used to test the cIndexedFileHelper class";
    A_UTILS_TEST(desc == (char*) header.description);

    A_UTILS_TEST_ERR_RESULT_EXT(getHeader("bla",header),"should have failed, file does not exist");

    A_UTILS_TEST_ERR_RESULT_EXT(getHeader(TESTFILE_INCOMPLETE,header),"should fail to load incomplete file header");

}

DEFINE_TEST(TesterIndexedFileHelper,
            TestQueryFileInfo,
            // Id
            "1.2",
            // Title
            "Test QueryFileInfo",
            // Description
            "Test makes sure the QueryFileinfo works as expected.",
            // Strategy
            "",
            // Passed if
            "",
            // Remarks
            "#9299",
            "",
            // User
            "Automatic")
{
    using namespace ifhd::v400;
    std::string info;
    A_UTILS_TEST_RESULT_EXT(queryFileInfo(TESTFILE, info),
                              "failed to query info for test file");

    A_UTILS_TEST_EXT(info.find("[Data: 102.4 kB / File: 112.5 kB]\nhelper test file\nfile used to test the cIndexedFileHelper class") != std::string::npos, "info did not match expected");
    A_UTILS_TEST_ERR_RESULT_EXT(queryFileInfo("bla",info),
        "should have failed, file does not exist");

    std::list<std::string> ext;
    A_UTILS_TEST_RESULT(queryFileInfo(TESTFILE, info, ext));

    // invalid header
    A_UTILS_TEST_ERR_RESULT(queryFileInfo(TESTFILE_INVALID, info, ext));

    // invalid file
    A_UTILS_TEST_ERR_RESULT(queryFileInfo(TESTFILE_INCOMPLETE, info, ext));

    // not existing file
    A_UTILS_TEST_ERR_RESULT(queryFileInfo("xyz_not_existing.file", info, ext));

}

DEFINE_TEST(TesterIndexedFileHelper,
            TestUpdateHeader,
            "1.3",
            "UpdateHeader",
            "Test UpdateHeader",
            "",
            "",
            "none",
            "#5098",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader original_header;
    FileHeader updated_header;
    FileHeader compare_header;

    memset(&original_header,0,sizeof(FileHeader));
    memset(&updated_header,0,sizeof(FileHeader));
    memset(&compare_header,0,sizeof(FileHeader));

    strcpy((char*)updated_header.description,"blubber");
    updated_header.file_time = 6667;

    A_UTILS_TEST_RESULT_EXT(getHeader(TESTFILE,original_header),
        "failed to retrieve header of testfile");
    A_UTILS_TEST_RESULT_EXT(updateHeader(TESTFILE,updated_header,0),
        "is mask is null, NO_ERROR is expected");
    A_UTILS_TEST_RESULT_EXT(updateHeader(TESTFILE,updated_header, FieldMask::fm_description),
        "failed to update description");
    A_UTILS_TEST_RESULT_EXT(getHeader(TESTFILE,compare_header),
        "failed to retrieve header of testfile");
    A_UTILS_TEST_EXT(0==strcmp((char*)compare_header.description,(char*)updated_header.description),
        "header description not updated");
    A_UTILS_TEST_RESULT_EXT(updateHeader(TESTFILE,updated_header, FieldMask::fm_date_time),
        "failed to update description");
    A_UTILS_TEST_RESULT_EXT(getHeader(TESTFILE,compare_header),
        "failed to retrieve header of testfile");
    A_UTILS_TEST_EXT(6667 == compare_header.file_time,
        "header filetime not updated");
    A_UTILS_TEST_RESULT_EXT(updateHeader(TESTFILE,original_header,
        FieldMask::fm_date_time | FieldMask::fm_description),
        "failed to update description");
    A_UTILS_TEST_RESULT_EXT(getHeader(TESTFILE,compare_header),"failed to retrieve header of testfile");
    A_UTILS_TEST_EXT(original_header.file_time == compare_header.file_time,
        "header filetime not updated");
    A_UTILS_TEST_EXT(0==strcmp((char*)compare_header.description,(char*)original_header.description),
        "header description not updated");

    A_UTILS_TEST_ERR_RESULT_EXT(updateHeader("bla",compare_header,
        FieldMask::fm_date_time | FieldMask::fm_description),
        "should have failed, file does not exist");
}

/*
DEFINE_TEST(TesterIndexedFileHelper,
            TestCreatCheckSumFile,
            "1.4",
            "TestCreatCheckSumFile",
            "",
            "",
            "",
            "files/test_helper.dat",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    String result;
    __a_utils_test_err_result(IndexedFileHelper::CreateChecksumFile("invalid_file",result));
    __a_utils_test_result(IndexedFileHelper::CreateChecksumFile(TESTFILE,result));

    Filename mD5File;
    __a_utils_test_result(IndexedFileHelper::CheckFile(TESTFILE,result,mD5File));

    // Not existing or missing md5file returns NO_ERROR, too!
    mD5File = "xyz_invalid.fail";
    __a_utils_test_result(IndexedFileHelper::CheckFile(TESTFILE,result,mD5File));

    // MD5 file is wrong. There is no checksum in this file
    mD5File = TESTFILE_INCOMPLETE;
    __a_utils_test_err_result(IndexedFileHelper::CheckFile(TESTFILE,result,mD5File));

    // MD5 file is corrupted
    mD5File = TESTFILE_INVALID_MD5;
    __a_utils_test_err_result(IndexedFileHelper::CheckFile(TESTFILE,result,mD5File));

    // if MD5 file doesn't exist, the method returns ERR_NOERROR
    mD5File = "";
    __a_utils_test_result(IndexedFileHelper::CheckFile(TESTFILE_INCOMPLETE,result,mD5File));

    RETURN_TEST_NOERROR;
}        */

DEFINE_TEST(TesterIndexedFileHelper,
            TestGetExtension,
            "1.5",
            "TestGetExtension",
            "",
            "",
            "",
            "files/test_helper.dat",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileExtension file_ext;
    memset(&file_ext,0,sizeof(file_ext));
    void* data;
    A_UTILS_TEST_RESULT(getExtension(TESTFILE,EXTENSION_ID,&file_ext,&data));

    char* id = (char*)file_ext.identifier;
    A_UTILS_TEST(std::string(id)==EXTENSION_ID);

    // file is corrupted
    A_UTILS_TEST_ERR_RESULT(getExtension(TESTFILE_INCOMPLETE,EXTENSION_ID,&file_ext,&data));

    // extension doesn't exist
    A_UTILS_TEST_ERR_RESULT(getExtension(TESTFILE,"unknown_extension",&file_ext,&data));
}

/*
DEFINE_TEST(TesterIndexedFileHelper,
            TestExtractAndCheckDatIfNecessary,
            "1.6",
            "TestExtractAndCheckDatIfNecessary",
            "",
            "",
            "",
            "files/test_helper.dat",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    std::string error;
    a_util::filesystem::Path tmpFileName(TESTFILE);
    tmpFileName.ReplaceExtension("tmp");

    __a_utils_test_result(IndexedFileHelper::ExtractAndCheckDatIfNecessary(TESTFILE".bz2",tmpFileName,error));

    __a_utils_test_result(IndexedFileHelper::ExtractAndCheckDatIfNecessary(TESTFILE,tmpFileName,error));
    __a_utils_test_err_result(IndexedFileHelper::ExtractAndCheckDatIfNecessary("xyz_unknown_file",tmpFileName,error));

} */

DEFINE_TEST(TesterIndexedFileHelper,
            TestStream2FileHeaderExtension,
            "1.7",
            "Test the Stream2FileHeaderExtension-Methods",
            "Create the structores call the method and test the byte order",
            "",
            "",
            "none",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader header;
    header.header_byte_order = PLATFORM_BYTEORDER_UINT8;
    
    FileExtension file_ext[3];
    memset(&file_ext, 0, sizeof(file_ext[0]) * 3);

    for( int i = 0; i < 3; i++ )
    {
        file_ext[i].stream_id = 0x1122;
        file_ext[i].user_id = 0x11223344;
        file_ext[i].type_id = 0x11223344;
        file_ext[i].version_id = 0x11223344;
        file_ext[i].data_pos = 0x1122334455667788LL;
        file_ext[i].data_size = 0x1122334455667788LL;
    }

    // no swap
    A_UTILS_TEST_RESULT(stream2FileHeaderExtension(header, file_ext,3));

    for( int i = 0; i < 3; i++ )
    {
        A_UTILS_TEST(file_ext[i].stream_id == 0x1122);
        A_UTILS_TEST(file_ext[i].user_id == 0x11223344);
        A_UTILS_TEST(file_ext[i].type_id == 0x11223344);
        A_UTILS_TEST(file_ext[i].version_id == 0x11223344);
        A_UTILS_TEST(file_ext[i].data_pos == 0x1122334455667788LL);
        A_UTILS_TEST(file_ext[i].data_size == 0x1122334455667788LL);
    }

    // swap
   header.header_byte_order = static_cast<uint8_t>(
                                    ifhd::getCurrentPlatformByteorder() == ifhd::Endianess::platform_little_endian 
                                        ? ifhd::Endianess::platform_big_endian 
                                        : ifhd::Endianess::platform_big_endian
                                );
    A_UTILS_TEST_RESULT(stream2FileHeaderExtension(header, file_ext,3));

    for( int i = 0; i < 3; i++ )
    {
        A_UTILS_TEST(file_ext[i].stream_id == 0x2211);
        A_UTILS_TEST(file_ext[i].user_id == 0x44332211);
        A_UTILS_TEST(file_ext[i].type_id == 0x44332211);
        A_UTILS_TEST(file_ext[i].version_id == 0x44332211);
        A_UTILS_TEST(file_ext[i].data_pos == 0x8877665544332211LL);
        A_UTILS_TEST(file_ext[i].data_size == 0x8877665544332211LL);
    }

}

DEFINE_TEST(TesterIndexedFileHelper,
            TestStream2FileHeader,
            "1.8",
            "Test the Stream2FileHeader-Methods",
            "Create the structores call the method and test the byte order",
            "",
            "",
            "none",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader file_header;
    memset(&file_header, 0, sizeof(file_header));

    // invalid FileId => don't swap
    file_header.file_id = 0xffffffff;
    file_header.version_id = 0x200;
    A_UTILS_TEST_ERR_RESULT(stream2FileHeader(file_header));
    
    char* i_d = reinterpret_cast<char*>(&file_header.file_id);
    i_d[0] = 'I';
    i_d[1] = 'F';
    i_d[2] = 'H';
    i_d[3] = 'D';
    uint32_t orig_file_id = file_header.file_id;

    file_header.version_id = 0x100;
    file_header.header_byte_order = static_cast<uint8_t>(ifhd::Endianess::platform_little_endian);

    // on versions lower than 0x200 => don't swap
    A_UTILS_TEST_ERR_RESULT(stream2FileHeader(file_header));


    // platform not supported in byte order => don't swap
    file_header.version_id = 0x200;
    file_header.header_byte_order = static_cast<uint8_t>(ifhd::Endianess::platform_not_supported);
    A_UTILS_TEST_ERR_RESULT(stream2FileHeader(file_header));

    // file has same byteorder than system => don't swap
    file_header.version_id = 0x200;
    file_header.header_byte_order = PLATFORM_BYTEORDER_UINT8;
    A_UTILS_TEST_RESULT(stream2FileHeader(file_header));

    // create a valid header which must be swapped
    file_header.version_id = 0x200;
    // set byte order to a different value than the system has
    file_header.header_byte_order = static_cast<uint8_t>((ifhd::getCurrentPlatformByteorder() == ifhd::Endianess::platform_big_endian) 
                                                        ? ifhd::Endianess::platform_little_endian 
                                                        : ifhd::Endianess::platform_big_endian);
    file_header.flags = 0x11223344;
    file_header.extension_count = 0x11223344;
    file_header.extension_offset = 0x1122334455667788LL;
    file_header.data_offset = 0x1122334455667788LL;
    file_header.data_size = 0x1122334455667788LL;
    file_header.chunk_count = 0x1122334455667788LL;
    file_header.max_chunk_size = 0x1122334455667788LL;
    file_header.duration = 0x1122334455667788LL;
    file_header.file_time = 0x1122334455667788LL;
    file_header.time_offset = 0x1122334455667788LL;

    A_UTILS_TEST_RESULT(stream2FileHeader(file_header));

    // File Id should not be swapped!
    A_UTILS_TEST(file_header.file_id == orig_file_id);
    A_UTILS_TEST(file_header.version_id == 0x00020000);
    A_UTILS_TEST(file_header.flags == 0x44332211);
    A_UTILS_TEST(file_header.extension_count == 0x44332211);
    A_UTILS_TEST(file_header.extension_offset == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.data_offset == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.data_size == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.chunk_count == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.max_chunk_size == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.duration == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.file_time == 0x8877665544332211LL);
    A_UTILS_TEST(file_header.time_offset == 0x8877665544332211LL);
}

DEFINE_TEST(TesterIndexedFileHelper,
            TestStream2ChunkHeader,
            "1.9",
            "Test the Stream2ChunkHeader-Methods",
            "Create the structores call the method and test the byte order",
            "",
            "",
            "none",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader header;
    header.header_byte_order = PLATFORM_BYTEORDER_UINT8;
    
    ChunkHeader chunk_header;
    memset(&chunk_header, 0, sizeof(chunk_header));

    chunk_header.time_stamp = 0x1122334455667788LL;
    chunk_header.ref_master_table_index = 0x11223344;
    chunk_header.offset_to_last = 0x11223344;
    chunk_header.size = 0x11223344;
    chunk_header.stream_id = 0x1122;
    chunk_header.flags = 0x1122;
    chunk_header.stream_index = 0x1122334455667788LL;

    // no swap
    A_UTILS_TEST_RESULT(stream2ChunkHeader(header, chunk_header));

    A_UTILS_TEST(chunk_header.time_stamp == 0x1122334455667788LL);

    // swap
   header.header_byte_order = static_cast<uint8_t>((ifhd::getCurrentPlatformByteorder() == ifhd::Endianess::platform_little_endian)  
                                                       ? ifhd::Endianess::platform_big_endian 
                                                       : ifhd::Endianess::platform_little_endian);
    A_UTILS_TEST_RESULT(stream2ChunkHeader(header, chunk_header));

    A_UTILS_TEST(chunk_header.time_stamp == 0x8877665544332211LL);
    A_UTILS_TEST(chunk_header.ref_master_table_index == 0x44332211LL);
    A_UTILS_TEST(chunk_header.offset_to_last == 0x44332211);
    A_UTILS_TEST(chunk_header.size == 0x44332211);
    A_UTILS_TEST(chunk_header.stream_id == 0x2211);
    A_UTILS_TEST(chunk_header.flags == 0x2211);
    A_UTILS_TEST(chunk_header.stream_index == 0x8877665544332211LL);

}

DEFINE_TEST(TesterIndexedFileHelper,
            TestStream2ChunkRef,
            "1.10",
            "Test the Stream2ChunkRef-Methods",
            "Create the structores call the method and test the byte order",
            "",
            "",
            "none",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader header;
    header.header_byte_order = PLATFORM_BYTEORDER_UINT8;
    
    ChunkRef chunk_ref;
    memset(&chunk_ref, 0, sizeof(chunk_ref));

    chunk_ref.time_stamp = 0x1122334455667788LL;
    chunk_ref.size = 0x11223344;
    chunk_ref.stream_id = 0x1122;
    chunk_ref.flags = 0x1122;
    chunk_ref.chunk_offset = 0x1122334455667788LL;
    chunk_ref.chunk_index = 0x1122334455667788LL;
    chunk_ref.stream_index = 0x1122334455667788LL;
    chunk_ref.ref_stream_table_index = 0x11223344;


    // no swap
    A_UTILS_TEST_RESULT(stream2ChunkRef(header, chunk_ref));

    A_UTILS_TEST(chunk_ref.time_stamp == 0x1122334455667788LL);

    // swap
    header.header_byte_order = static_cast<uint8_t>( (ifhd::getCurrentPlatformByteorder() == ifhd::Endianess::platform_little_endian)
                                                        ? ifhd::Endianess::platform_big_endian 
                                                        : ifhd::Endianess::platform_little_endian);
    A_UTILS_TEST_RESULT(stream2ChunkRef(header, chunk_ref));

    A_UTILS_TEST(chunk_ref.time_stamp == 0x8877665544332211LL);
    A_UTILS_TEST(chunk_ref.size == 0x44332211);
    A_UTILS_TEST(chunk_ref.stream_id == 0x2211);
    A_UTILS_TEST(chunk_ref.flags == 0x2211);
    A_UTILS_TEST(chunk_ref.chunk_offset == 0x8877665544332211LL);
    A_UTILS_TEST(chunk_ref.chunk_index == 0x8877665544332211LL);
    A_UTILS_TEST(chunk_ref.stream_index == 0x8877665544332211LL);
    A_UTILS_TEST(chunk_ref.ref_stream_table_index == 0x44332211);

}

DEFINE_TEST(TesterIndexedFileHelper,
            TestStream2StreamRef,
            "1.11",
            "Test the Stream2StreamRef-Methods",
            "Create the structores call the method and test the byte order",
            "",
            "",
            "none",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader header;
    header.header_byte_order = PLATFORM_BYTEORDER_UINT8;
    
    StreamRef stream_ref;
    memset(&stream_ref, 0, sizeof(stream_ref));

    stream_ref.ref_master_table_index = 0x11223344;

    // no swap
    A_UTILS_TEST_RESULT(stream2StreamRef(header, stream_ref));

    A_UTILS_TEST(stream_ref.ref_master_table_index == 0x11223344);

    // swap
    header.header_byte_order = static_cast<uint8_t>( (ifhd::getCurrentPlatformByteorder() == ifhd::Endianess::platform_little_endian)
                                                        ? ifhd::Endianess::platform_big_endian 
                                                        : ifhd::Endianess::platform_little_endian);
    A_UTILS_TEST_RESULT(stream2StreamRef(header, stream_ref));

    A_UTILS_TEST(stream_ref.ref_master_table_index == 0x44332211);

}

DEFINE_TEST(TesterIndexedFileHelper,
            TestStream2StreamInfoHeader,
            "1.12",
            "Test the Stream2StreamInfoHeader-Methods",
            "Create the structores call the method and test the byte order",
            "",
            "",
            "none",
            "#9170",
            "Automatic")
{
    using namespace ifhd::v400;
    FileHeader header;
    header.header_byte_order = PLATFORM_BYTEORDER_UINT8;
    
    StreamInfoHeader stream_info_header;
    memset(&stream_info_header, 0, sizeof(stream_info_header));

    stream_info_header.stream_index_count = 0x1122334455667788LL;
    stream_info_header.stream_first_time = 0x1122334455667788LL;
    stream_info_header.stream_last_time = 0x1122334455667788LL;
    stream_info_header.info_data_size = 0x11223344;

    // no swap
    A_UTILS_TEST_RESULT(stream2StreamInfoHeader(header, stream_info_header));

    A_UTILS_TEST(stream_info_header.stream_index_count == 0x1122334455667788LL);

    // swap
     header.header_byte_order = static_cast<uint8_t>( (ifhd::getCurrentPlatformByteorder() == ifhd::Endianess::platform_little_endian)
                                                         ? ifhd::Endianess::platform_big_endian 
                                                         : ifhd::Endianess::platform_little_endian);
    A_UTILS_TEST_RESULT(stream2StreamInfoHeader(header, stream_info_header));

    A_UTILS_TEST(stream_info_header.stream_index_count == 0x8877665544332211LL);
    A_UTILS_TEST(stream_info_header.stream_first_time == 0x8877665544332211LL);
    A_UTILS_TEST(stream_info_header.stream_last_time == 0x8877665544332211LL);
    A_UTILS_TEST(stream_info_header.info_data_size == 0x44332211);

}

#define TEST_FILE TEST_FILES_DIR "/test_extension_update.dat"

void CheckExtension(ifhd::v400::IndexedFileReader& reader, const char* name, int value, int size)
{
    ifhd::v400::FileExtension* ext;
    void* data;
    A_UTILS_TEST_RESULT(reader.findExtension(name, &ext, &data));
    A_UTILS_TEST(ext->data_size == size);
    for (int byte = 0; byte < size; ++byte)
    {
        A_UTILS_TEST(reinterpret_cast<uint8_t*>(data)[byte] == value);
    }
}

void CheckExtensions(int size1, int size2, int size3, int size4, int val1 = 1, int val2 = 2, int val3 = 3, int val4 = 4)
{
    ifhd::v400::IndexedFileReader reader;
    A_UTILS_TEST_RESULT(reader.open(TEST_FILE));
    CheckExtension(reader, "ext1", val1, size1);
    CheckExtension(reader, "ext2", val2, size2);
    CheckExtension(reader, "ext3", val3, size3);
    CheckExtension(reader, "ext4", val4, size4);
    reader.close();
}

DEFINE_TEST(TesterIndexedFileHelper,
            TestExtensionUpdate,
            "1.14",
            "TestExtensionUpdate",
            "Tests the WriteExtension method",
            "",
            "",
            "none",
            "#13851, #6030",
            "Automatic")
{
    using namespace ifhd::v400;
    uint8_t buffer[1024];
    
    // create test dat file
    {
        IndexedFileWriter writer;
        A_UTILS_TEST_RESULT(writer.create(TEST_FILE));

        uint8_t buffer[1024];
        a_util::memory::set(buffer, sizeof(buffer), 1, sizeof(buffer));
        A_UTILS_TEST_RESULT(writer.appendExtension("ext1", buffer, 121));
        a_util::memory::set(buffer, sizeof(buffer), 2, sizeof(buffer));
        A_UTILS_TEST_RESULT(writer.appendExtension("ext2", buffer, 251));
        a_util::memory::set(buffer, sizeof(buffer), 3, sizeof(buffer));
        A_UTILS_TEST_RESULT(writer.appendExtension("ext3", buffer, 511));
        a_util::memory::set(buffer, sizeof(buffer), 4, sizeof(buffer));
        A_UTILS_TEST_RESULT(writer.appendExtension("ext4", buffer, 1021));
        A_UTILS_TEST_RESULT(writer.close());

        CheckExtensions(121, 251, 511, 1021);
    }

    FileExtension extension;
    utils5ext::memZero(&extension, sizeof(extension));

    // update extension 1 with size smaller
    {
        strncpy((char*)extension.identifier, "ext1", MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
        extension.data_size = 50;
        a_util::memory::set(buffer, sizeof(buffer), 1, sizeof(buffer));
        A_UTILS_TEST_RESULT(writeExtension(TEST_FILE, extension, buffer));

        CheckExtensions(50, 251, 511, 1021);
    }

    // update extension 2 with size equal
    {
        strncpy((char*)extension.identifier, "ext2", MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
        extension.data_size = 251;
        a_util::memory::set(buffer, 1024, 5, 1024);
        A_UTILS_TEST_RESULT(writeExtension(TEST_FILE, extension, buffer));

        CheckExtensions(50, 251, 511, 1021, 1, 5);
    }

    // update extension 3 with size larger
    {
        strncpy((char*)extension.identifier, "ext3", MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
        extension.data_size = 600;
        a_util::memory::set(buffer, 1024, 6, 1024);
        A_UTILS_TEST_RESULT(writeExtension(TEST_FILE, extension, buffer));

        CheckExtensions(50, 251, 600, 1021, 1, 5, 6);
    }

    // update extension 4 with size larger
    {
        strncpy((char*)extension.identifier, "ext4", MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
        extension.data_size = 1024;
        a_util::memory::set(buffer, 1024, 7, 1024);
        A_UTILS_TEST_RESULT(writeExtension(TEST_FILE, extension, buffer));

        CheckExtensions(50, 251, 600, 1024, 1, 5, 6, 7);
    }

    // add a new extension
    {
        strncpy((char*)extension.identifier, "ext5", MAX_FILEEXTENSIONIDENTIFIER_LENGTH);
        extension.data_size = 700;
        a_util::memory::set(buffer, 1024, 8, 1024);
        A_UTILS_TEST_RESULT(writeExtension(TEST_FILE, extension, buffer));

        CheckExtensions(50, 251, 600, 1024, 1, 5, 6, 7);

        IndexedFileReader reader;
        A_UTILS_TEST_RESULT(reader.open(TEST_FILE));
        CheckExtension(reader, "ext5", 8, 700);
        reader.close();
    }
}
