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

#define TESTFILE TEST_FILES_DIR "/test_dat_file.dat"
#define TESTFILEHISTORY TEST_FILES_DIR "/test_history_dat_file.dat"


DEFINE_TEST(TesterIndexedFileWriter,
            TestInit,
            "1.1",
            "Init",
            "Test Create and Close",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    using namespace ifhd::v400;
    IndexedFileWriter writer;
    A_UTILS_TEST_RESULT_EXT(writer.close(), "Closing of not opened file should not fail" );
    A_UTILS_TEST_ERR_RESULT_EXT(writer.create(""), "accepted empty filename");
    A_UTILS_TEST_RESULT_EXT(writer.create(TESTFILE),"failed to create test dat file");
    A_UTILS_TEST_RESULT_EXT(writer.close(), "failed to close test dat file");
    A_UTILS_TEST_EXT(a_util::filesystem::exists(TESTFILE), "dat file was not created");
    A_UTILS_TEST_RESULT_EXT(a_util::filesystem::remove(TESTFILE),"failed to remove test file");

    A_UTILS_TEST_RESULT_EXT(writer.create(TESTFILE,
                                     1024*1024,
                                     OpenMode::om_sync_write |
        OpenMode::om_validate_chunk_header |
        OpenMode::om_disable_file_system_cache
                                     ),"Failed to create file with Flags");
}

DEFINE_TEST(TesterIndexedFileWriter,
            TestWrite,
            "1.2",
            "Write",
            "Test WriteChunk",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    using namespace ifhd::v400;
    IndexedFileWriter writer;
    uint8_t data[255];
    uint8_t *ref_data;
    for (uint8_t counter = 0; counter < 255;++counter)
    {
        data[counter]=counter;
    }

    A_UTILS_TEST_ERR_RESULT_EXT(writer.writeChunk(11,NULL,0,0,0),
        "writing a chunk if no file is created should have failed");
    A_UTILS_TEST_RESULT_EXT(writer.create(TESTFILE,-1,0,55),"failed to create testfile");
    A_UTILS_TEST_RESULT(writer.writeChunk(11,data,255,56,ChunkType::ct_data));
    A_UTILS_TEST_RESULT(writer.writeChunk(12,data,255,57, ChunkType::ct_data));
    A_UTILS_TEST_RESULT(writer.writeChunk(13,data,255,59, ChunkType::ct_data));
    writer.close();
    IndexedFileReader reader;
    ChunkHeader *chunk_header;
    A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE),"failed to open test file for verification");
    A_UTILS_TEST_EXT(56 == reader.getTimeOffset(),"wrong timeoffset");
    A_UTILS_TEST_EXT(56 == reader.getFirstTime(0),"wrong first time");
    A_UTILS_TEST_EXT(59 == reader.getLastTime(0),"wrong last time");
    A_UTILS_TEST_EXT(3 == reader.getDuration(),"wrong duration");
    A_UTILS_TEST_RESULT_EXT(reader.queryChunkInfo(&chunk_header), "failed to query chunk header");
    A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&ref_data),"failed to read chunk");
    A_UTILS_TEST_EXT( 255 + sizeof(ChunkHeader) == chunk_header->size, "size in chunk header does not match expected" );
    A_UTILS_TEST_EXT( 56 == chunk_header->time_stamp, "timestamp in chunk header does not match expected" );
    A_UTILS_TEST_EXT( 11 == chunk_header->stream_id, "size in chunk header does not match expected" );
    for (uint8_t counter = 0; counter < 255;++counter)
    {
        A_UTILS_TEST_EXT(counter==ref_data[counter],"chunk data does not match");
    }

    A_UTILS_TEST_RESULT_EXT(reader.readNextChunk(&chunk_header,(void**)&ref_data), "failed to query chunk header");
    A_UTILS_TEST_EXT( 255 + sizeof(ChunkHeader) == chunk_header->size, "size in chunk header does not match expected" );
    A_UTILS_TEST_EXT( 57 == chunk_header->time_stamp, "timestamp in chunk header does not match expected" );
    A_UTILS_TEST_EXT( 12 == chunk_header->stream_id, "size in chunk header does not match expected" );
    for (uint8_t counter = 0; counter < 255;++counter)
    {
        A_UTILS_TEST_EXT(counter==ref_data[counter],"chunk data does not match");
    }

    A_UTILS_TEST_RESULT_EXT(reader.readNextChunk(&chunk_header,(void**)&ref_data), "failed to query chunk header");
    A_UTILS_TEST_EXT( 255 + sizeof(ChunkHeader) == chunk_header->size, "size in chunk header does not match expected" );
    A_UTILS_TEST_EXT( 59 == chunk_header->time_stamp, "timestamp in chunk header does not match expected" );
    A_UTILS_TEST_EXT( 13 == chunk_header->stream_id, "size in chunk header does not match expected" );
    for (uint8_t counter = 0; counter < 255;++counter)
    {
        A_UTILS_TEST_EXT(counter==ref_data[counter],"chunk data does not match");
    }

}

DEFINE_TEST(TesterIndexedFileWriter,
            TestStream,
            "1.3",
            "Stream",
            "Test SetStreamName, SetAddiitionalStreamInfo",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    using namespace ifhd::v400;
    IndexedFileWriter writer;
    char additional[]="This is my extra info";
    char additional_modify[]="This is my unmodified info";
    char *data = "blabla";
    A_UTILS_TEST_ERR_RESULT_EXT(writer.setStreamName(77,"blubber"), "no file created yet");
    A_UTILS_TEST_ERR_RESULT_EXT(writer.setStreamName(77,NULL), "streamName is NULL-pointer");
    A_UTILS_TEST_ERR_RESULT_EXT(writer.setAdditionalStreamInfo(77,NULL,0,false),
        "allowed adding zero sized additional stream info");
    A_UTILS_TEST_RESULT_EXT(writer.create(TESTFILE),"failed to create testfile");
    A_UTILS_TEST_RESULT_EXT(writer.setStreamName(77,"blubber"), "failed to set stream name");
    A_UTILS_TEST_RESULT_EXT(writer.setAdditionalStreamInfo(77,additional,uint32_t(strlen(additional)+1),false),
        "allowed adding zero sized additional stream info");
    EXPECT_DEATH(writer.setStreamName(66,NULL), "");
    A_UTILS_TEST_RESULT_EXT(writer.setStreamName(66,"bla"), "failed to set stream name");
    A_UTILS_TEST_RESULT_EXT(writer.setAdditionalStreamInfo(66,additional_modify,uint32_t(strlen(additional_modify)+1),true),
        "allowed adding zero sized additional stream info");
    strcpy(additional_modify,"This is my   modified info");
    writer.writeChunk(77,data,6,32,ChunkType::ct_data);
    A_UTILS_TEST_RESULT_EXT(writer.close(),"failed to close testfile");

    IndexedFileReader reader;
    char *additional_info66=NULL;
    char *additional_info77=NULL;
    size_t additional_size=0;
    A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE),"failed to open testfile for verification");
    A_UTILS_TEST_EXT(std::string("bla") == (reader.getStreamName(66)),"stream name does not match");
    A_UTILS_TEST_EXT(std::string("blubber") == (reader.getStreamName(77)),"stream name does not match");
    A_UTILS_TEST_RESULT_EXT(reader.getAdditionalStreamInfo(66,(const void**)&additional_info66,&additional_size),
        "failed to get aditional info for 66");
    A_UTILS_TEST_EXT(strlen(additional_modify)+1 == additional_size,"size of additional data does not match");
    A_UTILS_TEST_RESULT_EXT(reader.getAdditionalStreamInfo(77,(const void**)&additional_info77,&additional_size),
        "failed to get aditional info for 77");
    A_UTILS_TEST_EXT(strlen(additional)+1 == additional_size,"size of additional data does not match");
    A_UTILS_TEST_EXT(std::string("This is my   modified info") == (additional_info66),
        "referenced additional info does not match");
    A_UTILS_TEST_EXT(std::string("This is my extra info") == (additional_info77),
        "additional info does not match");
    reader.close();
}

DEFINE_TEST(TesterIndexedFileWriter,
            TestNonSyncCacheFlags,
            "1.4",
            "Test Flags",
            "the test Create and Close a file with No om_sync_write set and Enable System Cache (no OM_DisableSystemCache set)",
            "The result file has chunks as expected",
            "",
            "none",
            "",
            "Automatic")
{
    using namespace ifhd::v400;
    IndexedFileWriter writer;
    char additional[]="This is my extra info";
    char *data = "blabla";


    A_UTILS_TEST_RESULT_EXT(writer.create(TESTFILE,
                                             1024*1024,
                                             OpenMode::om_validate_chunk_header
                                             ),"Failed to create file with Flags");

    A_UTILS_TEST_RESULT_EXT(writer.setStreamName(77,"blubber"), "failed to set stream name");
    A_UTILS_TEST_RESULT_EXT(writer.setAdditionalStreamInfo(77,additional,uint32_t(strlen(additional)+1),false),
        "allowed adding zero sized additional stream info");

    timestamp_t time = 32;
    for (int idx = 0; idx < 13; idx++)
    {
         A_UTILS_TEST_RESULT(writer.writeChunk(77,data,7,time,ChunkType::ct_data));
         time++;
    }


    A_UTILS_TEST_RESULT_EXT(writer.close(),"failed to close testfile");

    IndexedFileReader reader;

    A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE),"failed to open testfile for verification");
    A_UTILS_TEST_EXT(std::string("blubber") == (reader.getStreamName(77)),"stream name does not match");

    char* data_read;
    time = 32;
    for (int idx = 0; idx < 13; idx++)
    {
        ChunkHeader* chunk_header;
        A_UTILS_TEST_RESULT(reader.readNextChunk(&chunk_header, (void**)&data_read));
        A_UTILS_TEST(a_util::strings::isEqual(data_read, data));
        A_UTILS_TEST(chunk_header->time_stamp == time);
        time++;
    }



    reader.close();

}

void write_test_chunk(ifhd::v400::IndexedFileWriter& writer, uint16_t stream, size_t chunk, timestamp_t time, const char* format = "@%d|%d")
{
    std::string helper = a_util::strings::format(format, stream, chunk);
    A_UTILS_TEST_RESULT(writer.writeChunk(stream, helper.c_str(), helper.length(), time, ifhd::v400::ChunkType::ct_data));
}

DEFINE_TEST(TesterIndexedFileWriter,
            TestHistory,
            "1.5",
            "TestHistory",
            "Test History funktionality.",
            "",
            "",
            "none",
            "#14847",
            "Automatic")
{
    using namespace ifhd::v400;
    a_util::filesystem::remove(TESTFILEHISTORY);
    IndexedFileWriter writer;
    A_UTILS_TEST_RESULT(writer.create(TESTFILEHISTORY, 0, 0, 0, 9000000));
    A_UTILS_TEST_RESULT(writer.setStreamName(1, "stream1"));
    A_UTILS_TEST_RESULT(writer.setStreamName(2, "stream2"));

    timestamp_t time = 0;
    for (size_t chunk = 0; chunk < 800; ++chunk)
    {
        write_test_chunk(writer, 1, chunk, time);

        if (chunk % 3 == 0)
        {
            write_test_chunk(writer, 2, chunk, time);
        }

        time += 100001;
        if (chunk == 500)
        {
            A_UTILS_TEST_RESULT(writer.quitHistory());
        }
    }

    A_UTILS_TEST_RESULT(writer.close());

    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT(reader.open(TESTFILEHISTORY));

        uint64_t chunk_count = reader.getChunkCount();
        A_UTILS_TEST(chunk_count == 520);

        int compare = 409;
        for (uint64_t it_chunk = 0; it_chunk < chunk_count; ++it_chunk)
        {
            ChunkHeader* chunk;
            void* data;
            A_UTILS_TEST_RESULT(reader.readNextChunk(&chunk, &data));

            if (chunk->stream_id == 1)
            {
                ++compare;
            }

            std::string compare_string = a_util::strings::format("@%d|%d", chunk->stream_id, compare);
            std::string helper(static_cast<char*>(data), chunk->size - sizeof(ChunkHeader));
            printf(helper.c_str());
            printf(compare_string.c_str());
            A_UTILS_TEST(compare_string == helper);
        }
    }
}

DEFINE_TEST(TesterIndexedFileWriter,
            TestHistoryStartTimeStamps,
            "1.5",
            "TestHistoryStartTimeStamps",
            "Test update of stream start timestamps.",
            "",
            "",
            "none",
            "CDIFHD-21",
            "Automatic")
{
    using namespace ifhd::v400;
    a_util::filesystem::remove(TESTFILEHISTORY);
    IndexedFileWriter writer;
    A_UTILS_TEST_RESULT(writer.create(TESTFILEHISTORY, 0, 0, 0, 900000));
    A_UTILS_TEST_RESULT(writer.setStreamName(1, "stream1"));
    A_UTILS_TEST_RESULT(writer.setStreamName(2, "stream2"));

    const char* format = "@%d|%03d";

    timestamp_t time = 0;
    for (size_t chunk = 0; chunk < 30; ++chunk)
    {
        if (time == 2100000)
        {
            A_UTILS_TEST_RESULT(writer.quitHistory());
        }

        write_test_chunk(writer, 1, chunk, time, format);

        if (time == 500000 ||
            time == 2500000)
        {
            write_test_chunk(writer, 2, chunk, time, format);
        }

        time += 100000;
    }

    A_UTILS_TEST_RESULT(writer.close());

    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT(reader.open(TESTFILEHISTORY));
        ASSERT_EQ(reader.getFirstTime(1), 900000);
        ASSERT_EQ(reader.getFirstTime(2), 2500000);
    }
}

