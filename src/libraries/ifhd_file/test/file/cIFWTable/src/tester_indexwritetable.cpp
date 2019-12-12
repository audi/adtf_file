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


#include "stdafx.h"
#include "tester_indexwritetable.h"
#include <iostream>


IMPLEMENT_TESTER_CLASS(TesterIndexWriteTable,
                       "1",
                       "Adtf_Utils::Utils_file",
                       "Several tests for IndexedFileReader",
                       "indexedfilrereader_test.dat");


DEFINE_TEST(TesterIndexWriteTable,
            TestInit,
            "1.1",
            "Init",
            "Test Create and Free",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    IndexWriteTable table;
    A_UTILS_TEST_EXT( MAX_INDEXED_STREAMS == table.GetMaxStreamId(),"does not match MAX_INDEXED_STREAMS");
    A_UTILS_TEST_RESULT_EXT(table.Create(), "failed to create index table");
    A_UTILS_TEST_EXT( 0 == table.GetItemCount(0), "master index table should be empty");
    A_UTILS_TEST_RESULT_EXT(table.Free(), "failed to free a created index table");
    A_UTILS_TEST_RESULT_EXT(table.Free(), "free to free a freed index table");
    RETURN_TEST_NOERROR;
}

DEFINE_TEST(TesterIndexWriteTable,
            TestBuffer,
            "1.2",
            "Buffer",
            "Test Buffer",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    IndexWriteTable table;
    A_UTILS_TEST_RESULT_EXT(table.Create(),"failed to create index table");
    uint64_t chunk_index = 0;
    for (int counter = 0; counter < 128; ++counter)
    {
        bool index_entry_appended = false;
        A_UTILS_TEST_RESULT(table.Append(1,counter,chunk_index,16*counter,14,0x0F4241*counter,
            IndexedFile::CT_DATA, index_entry_appended));
        ++chunk_index;
        ++chunk_index;
    }
    IndexedFile::chunkRef *chunkRefBuffer =
        reinterpret_cast<IndexedFile::tagChunkRef*>(
        new uint8_t[static_cast<unsigned int>(table.GetBufferSize(0))]);
    IndexedFile::tagStreamRef *streamRefBuffer =
        reinterpret_cast<IndexedFile::tagStreamRef*>(
        new uint8_t[static_cast<unsigned int>(table.GetBufferSize(1))]);
    A_UTILS_TEST_RESULT(table.CopyToBuffer(0,chunkRefBuffer));

    chunk_index = 0;
    for (int counter = 0; counter < 128; ++counter)
    {
        A_UTILS_TEST(1==chunkRefBuffer[counter].streamId);
        A_UTILS_TEST(chunk_index==chunkRefBuffer[counter].chunk_index);
        A_UTILS_TEST(0x0F4241*counter==chunkRefBuffer[counter].timeStamp);
        A_UTILS_TEST(14==chunkRefBuffer[counter].size);
        A_UTILS_TEST(IndexedFile::CT_DATA==chunkRefBuffer[counter].flags);
        A_UTILS_TEST(chunk_index==chunkRefBuffer[counter].chunk_index);
        A_UTILS_TEST(counter==chunkRefBuffer[counter].streamIndex);
        A_UTILS_TEST(counter==chunkRefBuffer[counter].refStreamTableIndex);
        
        ++chunk_index;
        ++chunk_index;
    }
    A_UTILS_TEST_RESULT(table.CopyToBuffer(1,streamRefBuffer));
    for (int counter = 0; counter < 128; ++counter)
    {
        A_UTILS_TEST(counter==streamRefBuffer[counter].refMasterTableIndex);
    }
    
    A_UTILS_TEST_ERR_RESULT_EXT(table.CopyToBuffer(0,NULL),"CopyToBuffer accepted NULL pointer");
    A_UTILS_TEST_ERR_RESULT_EXT(table.CopyToBuffer(MAX_INDEXED_STREAMS+5,streamRefBuffer),"CopyToBuffer accepted stream bigger than max_indexed_streams"); 



    delete[] chunkRefBuffer;
    delete[] streamRefBuffer;
}

DEFINE_TEST(TesterIndexWriteTable,
            TestAppend,
            "1.3",
            "Append",
            "Test Append",
            "",
            "",
            "none",
            "#11103",
            "Automatic")
{
    IndexWriteTable table;
    A_UTILS_TEST_RESULT_EXT(table.Create(),"failed to create index table");
    A_UTILS_TEST_EXT(0 == table.GetItemCount(0),"master table should be empty");
    A_UTILS_TEST_EXT(0 == table.GetItemCount(77),"itemcount for any stream should be empty");
    uint64_t chunk_index = 0;
    for (int counter = 0; counter < 128; ++counter)
    {
        bool index_entry_appended = false;
        A_UTILS_TEST_RESULT(table.Append(1,counter,chunk_index,16*counter,14,0x0F4241*counter,
            IndexedFile::CT_DATA, index_entry_appended));
        ++chunk_index;
        A_UTILS_TEST_RESULT(table.Append(2,counter,chunk_index,32*counter,27,10*counter,
            IndexedFile::CT_KEYDATA, index_entry_appended));
        A_UTILS_TEST(index_entry_appended);
        ++chunk_index;
    }
    A_UTILS_TEST_EXT( 128 == table.GetItemCount(1),"wrong itemcount for stream 1");
    A_UTILS_TEST_EXT( 128 == table.GetItemCount(2),"wrong itemcount for stream 2");
    A_UTILS_TEST_EXT( 256 == table.GetItemCount(0),"wrong itemcount for mastertable");
    A_UTILS_TEST_EXT(sizeof(IndexedFile::StreamRef)*128 == table.GetBufferSize(1),
        "GetBufferSize for teststream 1 is wrong");
    A_UTILS_TEST_EXT(sizeof(IndexedFile::StreamRef)*128 == table.GetBufferSize(2),
        "GetBufferSize for teststream 2 (forced with keydata) is wrong");
    A_UTILS_TEST_EXT(sizeof(IndexedFile::chunkRef)*128*2==table.GetBufferSize(0),"");
    A_UTILS_TEST_RESULT_EXT(table.Free(),"failed to create index table");
   
    
    for (uint16_t counter = 0; counter < MAX_INDEXED_STREAMS; ++counter)
    {
        A_UTILS_TEST_EXT(0 == table.GetBufferSize(counter),
            String::Format("Buffersize after free should be zero for streamid %d",counter));
    } 
    RETURN_TEST_NOERROR;
}
