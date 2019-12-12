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


#define TESTFILE1 TEST_FILES_DIR "/test_GUID_1.dat"
#define TESTFILE2 TEST_FILES_DIR "/test_GUID_2.dat"


class DummyIndexedFile
: public ifhd::v400::IndexedFile
{
    public:
        DummyIndexedFile(void): IndexedFile()
        {
        }

        void SetDateTime(const a_util::datetime::DateTime& date_time)
        {
            IndexedFile::setDateTime(date_time);
        }

        void* InternalMalloc(int64_t size, bool use_segment_size)
        {
            return IndexedFile::internalMalloc(size, use_segment_size);
        }

        void InternalFree(void* memory)
        {
            IndexedFile::internalFree(memory);
        }

        bool SetSystemCacheDisabled(bool disabled)
        {
            bool result = _system_cache_disabled;
            _system_cache_disabled = disabled;
            return result;
        }
        
        int GetSectorSize() const
        {
            return utils5ext::getSectorSizeFor("");
        }

        void AllocBuffer(size_t size)
        {
            IndexedFile::allocBuffer(size);
        }
        void FreeBuffer()
        {
            IndexedFile::freeBuffer();
        }

        bool CheckBuffer(uint64_t expected_size)
        {
            if (expected_size == 0)
            {
                return (_buffer == NULL) && (_buffer_size == 0);
            }
            else
            {
                return (_buffer != NULL) && (_buffer_size == expected_size);
            }
        }
        
        void AllocCache(int64_t size)
        {
            IndexedFile::allocCache(size);
        }

        void FreeCache()
        {
            IndexedFile::freeCache();
        }

        void* GetCacheAddr()
        {
            return IndexedFile::getCacheAddr();
        }

        int64_t GetCacheSize()
        {
            return _cache_size;
        }

        void AllocHeader()
        {
            IndexedFile::allocHeader();
        }
        
        void FreeHeader()
        {
            IndexedFile::freeHeader();
        }

        void SetWriteMode(bool mode=true)
        {
            _write_mode = mode;
        }

        void SetByteOrder(uint8_t byte_order)
        {
            if (_file_header != NULL)
            {
                _file_header->header_byte_order = byte_order;
            }
        }

        void SetByteOrder(ifhd::Endianess byte_order)
        {
            if (_file_header != NULL)
            {
                _file_header->header_byte_order = static_cast<uint8_t>(byte_order);
            }
        }

        uint32_t GetFileId()
        {
            return ifhd::v400::getFileId();
        }
};      

GTEST_TEST(TestNegative, TesterIndexedFile)
{
    using namespace ifhd::v400;
    IndexedFile file;
    FileExtension extension;
    memset(&extension,0, sizeof(FileExtension));
    FileExtension *other_extension;
    void *pointer_to_void;
    a_util::datetime::DateTime date_time = a_util::datetime::getCurrentLocalDateTime();
    FileHeader *header;
    
    char dummy;
    ASSERT_NO_THROW(file.close());
    ASSERT_ANY_THROW(file.appendExtension(NULL,NULL));
    ASSERT_ANY_THROW(file.appendExtension(&dummy,NULL));
    ASSERT_ANY_THROW(file.appendExtension(NULL,&extension));
    ASSERT_ANY_THROW(file.appendExtension("",NULL));
    ASSERT_ANY_THROW(file.appendExtension("bla",&dummy,1,0,0,0,0));
    EXPECT_EQ(0, file.getExtensionCount());
    ASSERT_ANY_THROW(file.setDescription("blabla"));
    a_util::datetime::DateTime test_date_time = file.getDateTime();
    EXPECT_EQ(test_date_time, a_util::datetime::DateTime());
    EXPECT_EQ(std::string(), file.getDescription());
    ASSERT_ANY_THROW(file.getHeaderRef(NULL));
    ASSERT_ANY_THROW(file.getHeaderRef(&header));
    ASSERT_FALSE(file.findExtension("",&other_extension,&pointer_to_void));
    ASSERT_FALSE(file.findExtension("bla",&other_extension,&pointer_to_void));
    ASSERT_FALSE(file.findExtension("bla",NULL,NULL));
    ASSERT_FALSE(file.findExtension("bla",&other_extension,NULL));
    ASSERT_FALSE(file.findExtension("bla",NULL,&pointer_to_void));
    ASSERT_ANY_THROW(file.getExtension(100,&other_extension,&pointer_to_void));
    ASSERT_ANY_THROW(file.getExtension(1,&other_extension,&pointer_to_void));
}


GTEST_TEST(TestOldFileDate, TesterIndexedFile)
{
    using namespace ifhd::v400;
    a_util::filesystem::Path filename = TEST_FILES_DIR "/old.dat";

    IndexedFileReader indext_file_reader;
    ASSERT_NO_THROW(indext_file_reader.open(filename));

    a_util::datetime::DateTime date_time;
    date_time = indext_file_reader.getDateTime();

    ASSERT_TRUE(date_time.getYear() == 2007);
    ASSERT_TRUE(date_time.getMonth() == 6);
    ASSERT_TRUE(date_time.getDay() == 19);
    ASSERT_TRUE(date_time.getHour() == 17);
    ASSERT_TRUE(date_time.getMinute() == 51);

}

GTEST_TEST(TestSetGetDatetime, TesterIndexedFile)
{
    using namespace ifhd::v400;
    a_util::filesystem::Path filename = TEST_FILES_DIR "/test_helper.dat";

    IndexedFileReader indext_file_reader;
    ASSERT_NO_THROW(indext_file_reader.open(filename));

    a_util::datetime::DateTime date_time_tmp;

    date_time_tmp = indext_file_reader.getDateTime();

    ASSERT_TRUE(date_time_tmp.getYear() == 2009);
    ASSERT_TRUE(date_time_tmp.getMonth() == 6);
    ASSERT_TRUE(date_time_tmp.getDay() == 05);
    ASSERT_TRUE(date_time_tmp.getHour() == 10);
    ASSERT_TRUE(date_time_tmp.getMinute() == 57);
    ASSERT_TRUE(date_time_tmp.getSecond() == 46);
}

GTEST_TEST(TestInternalMemory, TesterIndexedFile)
{
    // check following combinations:
    // - useSegmentSize == false && _system_cache_disabled == false
    // - useSegmentSize == false && _system_cache_disabled == true 
    // - useSegmentSize == true  && _system_cache_disabled == false
    // - useSegmentSize == true  && _system_cache_disabled == true 

    DummyIndexedFile idx_file;
    void* mem = idx_file.InternalMalloc(512,false);
    A_UTILS_TEST(mem != NULL);
    idx_file.InternalFree(mem);

    bool disabled = idx_file.SetSystemCacheDisabled(true);

    mem = idx_file.InternalMalloc(512, false);
    A_UTILS_TEST(mem != NULL);
    idx_file.InternalFree(mem);

    int sector_size = idx_file.GetSectorSize();
    A_UTILS_TEST(sector_size > 0);

    // check alignment (works only with _system_cache_disabled == true)
    for (int64_t size = 0; size < static_cast<int64_t>(sector_size) * 3; size += sector_size / 13)
    {
        void* mem1 = idx_file.InternalMalloc(size,true);
        A_UTILS_TEST(mem1 != NULL);
        A_UTILS_TEST(((uint64_t)mem1&(sector_size-1))==0); // test whether this block is aligned or not
        void* mem2 = idx_file.InternalMalloc(size,true);
        A_UTILS_TEST(mem2 != NULL);
        A_UTILS_TEST(((uint64_t)mem2&(sector_size-1))==0); // test whether this block is aligned or not
        idx_file.InternalFree(mem1);
        idx_file.InternalFree(mem2);
    }

    idx_file.SetSystemCacheDisabled(disabled);

    // test AllocBuffer and FreeBuffer
    A_UTILS_TEST(idx_file.CheckBuffer(0));
    A_UTILS_TEST_RESULT(idx_file.AllocBuffer(1000));
    A_UTILS_TEST(idx_file.CheckBuffer(1000));
    A_UTILS_TEST_RESULT(idx_file.FreeBuffer());
    A_UTILS_TEST(idx_file.CheckBuffer(0));

    A_UTILS_TEST_RESULT( idx_file.AllocBuffer(2000));
    A_UTILS_TEST(idx_file.CheckBuffer(2000));
    A_UTILS_TEST_RESULT( idx_file.AllocBuffer(4000));
    A_UTILS_TEST(idx_file.CheckBuffer(4000));
    A_UTILS_TEST_RESULT(idx_file.FreeBuffer());
    A_UTILS_TEST(idx_file.CheckBuffer(0));

    // if there is already a buffer and AllocBuffer is called 
    // with equal or smaller size, the buffer is kept (and so the size)
    A_UTILS_TEST_RESULT(idx_file.AllocBuffer(5000));
    A_UTILS_TEST(idx_file.CheckBuffer(5000));
    A_UTILS_TEST_RESULT(idx_file.AllocBuffer(1000));
    A_UTILS_TEST(idx_file.CheckBuffer(5000));
    A_UTILS_TEST_RESULT(idx_file.FreeBuffer());
    A_UTILS_TEST(idx_file.CheckBuffer(0));

    // negative test
    A_UTILS_TEST_RESULT(idx_file.AllocBuffer(0));

    // test Cache
    A_UTILS_TEST_RESULT(idx_file.AllocCache(1024*1024));
    A_UTILS_TEST(idx_file.GetCacheAddr()!=NULL);
    A_UTILS_TEST_RESULT(idx_file.FreeCache());
    A_UTILS_TEST(idx_file.GetCacheAddr()==NULL);

    // test CacheSize
    // if SystemCacheDisabled == false, the size us 
    // not increased to the next page size
    A_UTILS_TEST_RESULT(idx_file.AllocCache(1000));
    A_UTILS_TEST(idx_file.GetCacheSize()==1000);
    A_UTILS_TEST_RESULT(idx_file.FreeCache());

    disabled = idx_file.SetSystemCacheDisabled(true);
    A_UTILS_TEST_RESULT(idx_file.AllocCache(1000));
    A_UTILS_TEST((idx_file.GetCacheSize()&(sector_size-1))==0);
    A_UTILS_TEST_RESULT(idx_file.FreeCache());

    // found problem with cache size
    // if allocated cache size isn't a multiple of SectorSize
    A_UTILS_TEST_RESULT(idx_file.AllocCache(sector_size-4));
    memset(idx_file.GetCacheAddr(), -1, idx_file.GetCacheSize()); // should not crash or assert

    idx_file.SetSystemCacheDisabled(disabled);

    // try to allocate too much memory
    // this method should allocate as much memory as possible (1GB blocks)
    // if there's no more memory, the allocated memory will be free'd
    // there should be no unhandled exception
    void* block[32];
    int block_count = 0;

    for (; block_count < 32; block_count++)
    {
        block[block_count] = idx_file.InternalMalloc((int64_t)((int64_t)64*(int64_t)1024*(int64_t)1024*(int64_t)1024), false); //64 GB
        if (block[block_count] == NULL) {
            break;
}
    }

    for (int i = 0; i < block_count; i++)
    {
        idx_file.InternalFree(block[i]);
    }
}


DEFINE_TEST(TesterIndexedFile,
    TestHeader,
    "1.5",
    "Test the header methods",
    "Create a IndexedFile, add a header and test all methods affecting this header",
    "",
    "",
    "none",
    "#9170",
    "Automatic")
{
    using namespace ifhd::v400;
    // create a file
    DummyIndexedFile file;
    a_util::datetime::DateTime date_time;
    FileHeader* file_header = NULL;

    // call some getter without header set
    A_UTILS_TEST_ERR_RESULT(file.setDescription(std::string()));
    A_UTILS_TEST(file.getDescription() == std::string());
    A_UTILS_TEST_ERR_RESULT(file.SetDateTime(a_util::datetime::DateTime()));
    A_UTILS_TEST_ERR_RESULT(file.SetDateTime(date_time));
    ASSERT_TRUE(file.getDateTime() == a_util::datetime::DateTime());
    ASSERT_TRUE(file.getDateTime() == a_util::datetime::DateTime());
    A_UTILS_TEST_ERR_RESULT(file.getHeaderRef(NULL));
    A_UTILS_TEST_ERR_RESULT(file.getHeaderRef(&file_header));
    A_UTILS_TEST(file.getByteOrder() == PLATFORM_BYTEORDER_UINT8);

    // set the header
    A_UTILS_TEST_RESULT(file.AllocHeader());

    // set a header    
    A_UTILS_TEST_RESULT(file.getHeaderRef(&file_header));
    ASSERT_TRUE(file_header != nullptr);

    // set/get description
    const char* description = "This is a description!";
    A_UTILS_TEST(file.getDescription()[0] == '\0');
    file.setDescription(description);
    A_UTILS_TEST(strcmp(description, file.getDescription().c_str()) == 0);

    // test buffer overrun
    std::string over_run;
    for (int i = 0; i < 3 * sizeof(file_header->description); i++)
    {
        over_run.append("a");
    }
    A_UTILS_TEST_RESULT(file.setDescription(over_run));
    over_run.clear();

    // test date with 0 pointer to get current date
    A_UTILS_TEST_RESULT(file.SetDateTime(a_util::datetime::getCurrentLocalDateTime()));
    date_time = file.getDateTime();
    ASSERT_FALSE(date_time == a_util::datetime::DateTime());
    A_UTILS_TEST(date_time.getYear() > 2016);
    A_UTILS_TEST(date_time.getMonth() != 0);
    A_UTILS_TEST(date_time.getDay() != 0);

    // byteorder
    file.SetByteOrder(ifhd::Endianess::platform_big_endian);
    A_UTILS_TEST(file.getByteOrder() == static_cast<uint8_t>(ifhd::Endianess::platform_big_endian));

    // free header
    A_UTILS_TEST_RESULT(file.FreeHeader());
    file_header = NULL;
    // call some getter after header free'd
    A_UTILS_TEST_ERR_RESULT(file.getHeaderRef(NULL));
    A_UTILS_TEST_ERR_RESULT(file.getHeaderRef(&file_header));
    A_UTILS_TEST_ERR_RESULT(file.setDescription(""));
    A_UTILS_TEST(file.getDescription() == std::string());
    A_UTILS_TEST_ERR_RESULT(file.SetDateTime(a_util::datetime::DateTime()));
    A_UTILS_TEST_ERR_RESULT(file.SetDateTime(date_time));
    ASSERT_TRUE(file.getDateTime() == a_util::datetime::DateTime());
    A_UTILS_TEST(file.getByteOrder() == PLATFORM_BYTEORDER_UINT8);
}


DEFINE_TEST(TesterIndexedFile,
    TestExtensions,
    "1.6",
    "Test the extension methods",
    "Create a IndexedFile and call the extensions-methods",
    "",
    "",
    "none",
    "#9170",
    "Automatic")
{
    using namespace ifhd::v400;
    char* data = new char[32];
    const char* data_as_string = "0123456789012345678901234567890"; //31 chars + '\0'
    strcpy(data, data_as_string);

    DummyIndexedFile idx_file;
    idx_file.AllocHeader();
    A_UTILS_TEST(idx_file.getExtensionCount() == 0);

    // Extension only can be added if WriteMode is active
    ASSERT_ANY_THROW(idx_file.appendExtension("test_extension", data, 32, 1, 2, 3, 4));
    idx_file.SetWriteMode();

    // add an extension
    A_UTILS_TEST_RESULT(idx_file.appendExtension("test_extension", data, 32, 1, 2, 3, 4));
    A_UTILS_TEST(idx_file.getExtensionCount() == 1);

    // try to access this extension by index
    FileExtension* extension_info = NULL;
    void* data_result = NULL;
    A_UTILS_TEST_RESULT(idx_file.getExtension(0, &extension_info, &data_result));
    A_UTILS_TEST(data_result != NULL);
    A_UTILS_TEST(extension_info != NULL);

    // try to access this extension by name
    data_result = NULL;
    extension_info = NULL;
    A_UTILS_TEST_RESULT(idx_file.findExtension("test_extension", &extension_info, &data_result));
    A_UTILS_TEST(data_result != NULL);
    A_UTILS_TEST(extension_info != NULL);

    // delete this extension
    A_UTILS_TEST_RESULT(idx_file.freeExtensions());
    A_UTILS_TEST(idx_file.getExtensionCount() == 0);

    // add extensions without identifier and data
    data_result = NULL;
    extension_info = NULL;
    A_UTILS_TEST_RESULT(idx_file.appendExtension(NULL, NULL, 32, 1, 2, 3, 4));
    A_UTILS_TEST(idx_file.getExtensionCount() == 1);
    A_UTILS_TEST_RESULT(idx_file.getExtension(0, &extension_info, &data_result));
    A_UTILS_TEST(extension_info != NULL);

    // access an extension with an invalid index
    A_UTILS_TEST_ERR_RESULT(idx_file.getExtension(1, &extension_info, &data_result));

    // access an extension with an invalid identifier
    A_UTILS_TEST(!idx_file.findExtension("not_existing_id", &extension_info, &data_result));

    // delete extensions
    A_UTILS_TEST_RESULT(idx_file.freeExtensions());
    A_UTILS_TEST(idx_file.getExtensionCount() == 0);

    // add some more extensions
    for (int i = 0; i < 20; i++)
    {
        A_UTILS_TEST_RESULT(idx_file.appendExtension(data_as_string + i, data, i * 4, i * 3, i * 2, i, 0));
        A_UTILS_TEST(idx_file.getExtensionCount() == i + 1);
    }


    // access this extensions

    // #31890 indexedfile.cpp Revision 2102
    //    special case: when extension size == 0 -> no header is created => pointer to extesion is NULL 
        // by index
    data_result = NULL;
    extension_info = NULL;
    A_UTILS_TEST_RESULT(idx_file.getExtension(0, &extension_info, &data_result));
    ASSERT_TRUE(data_result == nullptr);

    // by name
    data_result = NULL;
    extension_info = NULL;
    A_UTILS_TEST_RESULT(idx_file.findExtension(data_as_string + 0, &extension_info, &data_result));
    ASSERT_TRUE(data_result == nullptr);


    // access this extensions - size > 0
    for (int i = 1; i < 20; i++)
    {
        // by index
        data_result = NULL;
        extension_info = NULL;
        A_UTILS_TEST_RESULT(idx_file.getExtension(i, &extension_info, &data_result));
        A_UTILS_TEST(data_result != NULL);
        A_UTILS_TEST(extension_info != NULL);

        // by name
        data_result = NULL;
        extension_info = NULL;
        A_UTILS_TEST_RESULT(idx_file.findExtension(data_as_string + i, &extension_info, &data_result));
        A_UTILS_TEST(data_result != NULL);
        A_UTILS_TEST(extension_info != NULL);
    }

    // delete extensions
    A_UTILS_TEST_RESULT(idx_file.freeExtensions());
    A_UTILS_TEST(idx_file.getExtensionCount() == 0);
}

/*
DEFINE_TEST(TesterIndexedFile,
            TestSetAndGetGUID,
            "1.7",
            "Test the GUID methods",
            "Create a IndexedFile and set/get the GUID",
            "",
            "",
            "none",
            "#17554",
            "Automatic")
{
    using namespace ifhd::v400;
    // +++++++++++++++++++++++++++
    // Test the mechanism manually
    // +++++++++++++++++++++++++++

    IndexedFileWriter file_writer;
    std::string fakeGUID = "936DA01F-9ABD-4D9D-80C7-02AF85C822A8";
    A_UTILS_TEST_RESULT(file_writer.Create(TESTFILE1));
    
    // Extension GUID is protected
    A_UTILS_TEST_ERR_RESULT(file_writer.AppendExtension("GUID", fakeGUID.GetPtr(), fakeGUID.GetLength() + 1));
    
    // Get GUID
    std::string gUID;
    A_UTILS_TEST_ERR_RESULT(file_writer.GetGUID(gUID));
    A_UTILS_TEST(NULL == gUID);

    // Close File Writer
    A_UTILS_TEST_RESULT(file_writer.Close());

    // ++++++++++++++++++++++++++++++++++++++
    // Test the mechanism automatic (UseCase)
    // ++++++++++++++++++++++++++++++++++++++

    // Create file
    // After close -> GUID should be written
    A_UTILS_TEST_RESULT(file_writer.Create(TESTFILE2));
    A_UTILS_TEST_RESULT(file_writer.Close());

    // Create File Reader
    IndexedFileReader file_reader;
    A_UTILS_TEST_RESULT(file_reader.Open(TESTFILE2));
    
    // Read GUID
    String readGUID;
    A_UTILS_TEST_RESULT(file_reader.GetGUID(readGUID));
    A_UTILS_TEST(NULL != readGUID);

    // Close File Reader
    A_UTILS_TEST_RESULT(file_reader.Close());

}
*/




