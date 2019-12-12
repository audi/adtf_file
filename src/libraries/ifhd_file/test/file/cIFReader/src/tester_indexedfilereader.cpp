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

#define TESTFILE TEST_FILES_DIR "/Rec_20090326_155630.dat"
#define CHUNKCOUNT 114
#define DURATION 4373024
#define TIME_OFFSET 1730194
#define FIRST_TIME 1730194
#define LAST_TIME 6103218
#define VERSION_ID 0x0201

#define EXTENSION_COUNT 5
#define TEST_FILE_CHUNK_COUNT 10

//helper function
void test_GenerateTestFile()
{
    using namespace ifhd::v400;
    IndexedFileWriter writer;
    writer.create("files/reader_test_file.dat", -1, 0);
    writer.setStreamName(1, "blubber");
    writer.setStreamName(2, "bla");
    uint64_t data = 0;
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; ++counter)
    {
        data = 2 * counter;
        writer.writeChunk(1, &data, 8, counter, ChunkType::ct_data);
    }
    writer.close();
}

//helper function
void test_GenerateTestFileTwoStreams()
{
    using namespace ifhd::v400;
    IndexedFileWriter writer;
    writer.create("files/reader_test_file_two_streams.dat", -1, 0);
    writer.setStreamName(1, "blubber");
    writer.setStreamName(2, "bla");
    uint64_t data, data_second = 0;
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; ++counter)
    {
        data = counter;
        data_second = counter * 10;
        A_UTILS_TEST_RESULT(writer.writeChunk(1, &data, 8, counter, ChunkType::ct_data));
        A_UTILS_TEST_RESULT(writer.writeChunk(2, &data_second, 8, counter, ChunkType::ct_data));
    }
    writer.close();
}

DEFINE_TEST(TesterIndexedFileReader, 
            TestAccess, 
            "1.1", 
            "Access", 
            "Test Open and Close", 
            "", 
            "", 
            "none", 
            "", 
            "Automatic")
{
    using namespace ifhd::v400;
    //open/close wrong file
    IndexedFileReader reader;
    A_UTILS_TEST_ERR_RESULT_EXT(reader.open("bla.dat"), "should have failed, file does not exist");

    //open/close testfile
    A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
    A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");

}

DEFINE_TEST(TesterIndexedFileReader, 
            TestPosition,
            "1.2",
            "Position",
            "Test GetCurrentPos, SetCurrentPos, Seek, GetFilePos, Reset, LookUpChunkRef",
            "",
            "",
            "none",
            "",
            "Automatic")
{
    using namespace ifhd::v400;
    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
        reader.reset();
        int64_t pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        int64_t pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        int64_t pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST(0 == pos_chunk_index);
        A_UTILS_TEST(FIRST_TIME == pos_chunk_time);
        A_UTILS_TEST(0 == pos_stream_index);
        A_UTILS_TEST(0 == reader.getFilePos());

        A_UTILS_TEST_EXT(113 == reader.setCurrentPos(LAST_TIME, 
            TimeFormat::tf_chunk_time),
            "SetCurrentPos returned wrong new position");

        pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST(113 == pos_chunk_index);
        A_UTILS_TEST(LAST_TIME == pos_chunk_time);
        A_UTILS_TEST(113 == pos_stream_index);
        A_UTILS_TEST(113 == reader.getFilePos());

        A_UTILS_TEST_ERR_RESULT_EXT(reader.setCurrentPos(0, TimeFormat::tf_chunk_time),
            "SetCurrentPos did not fail for time before offset");

        A_UTILS_TEST(0 == reader.setCurrentPos(FIRST_TIME, TimeFormat::tf_chunk_time));

        pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST(0 == pos_chunk_index);
        A_UTILS_TEST(FIRST_TIME == pos_chunk_time);
        A_UTILS_TEST(0 == pos_stream_index);
        A_UTILS_TEST(0 == reader.getFilePos());

        A_UTILS_TEST_EXT(0 == reader.setCurrentPos(FIRST_TIME, TimeFormat::tf_chunk_time),
            "SetCurrentPos returned wrong new position");

        pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST(0 == pos_chunk_index);
        A_UTILS_TEST(FIRST_TIME == pos_chunk_time);
        A_UTILS_TEST(0 == pos_stream_index);
        A_UTILS_TEST(0 == reader.getFilePos());

        A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");
    }

    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");

        A_UTILS_TEST_EXT(113 == reader.setCurrentPos(113, 
            TimeFormat::tf_chunk_index),
            "SetCurrentPos returned wrong new position");

        int64_t pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        int64_t pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        int64_t pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST(113 == pos_chunk_index);
        A_UTILS_TEST(LAST_TIME == pos_chunk_time);
        A_UTILS_TEST(113 == pos_stream_index);
        A_UTILS_TEST(113 == reader.getFilePos());

        A_UTILS_TEST_EXT(0 == reader.setCurrentPos(0, TimeFormat::tf_chunk_index),
            "SetCurrentPos returned wrong new position");

        pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST(0 == pos_chunk_index);
        A_UTILS_TEST(FIRST_TIME == pos_chunk_time);
        A_UTILS_TEST(0 == pos_stream_index);
        A_UTILS_TEST(0 == reader.getFilePos());

        A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");
    }

    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
        A_UTILS_TEST(0 == reader.lookupChunkRef(0, FIRST_TIME, TimeFormat::tf_chunk_time));
        A_UTILS_TEST(112 == reader.lookupChunkRef(0, LAST_TIME, TimeFormat::tf_chunk_time));
        A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");
    }
    {
        IndexedFileReader reader;

        EXPECT_DEATH(reader.reset(), ""); //@crash

        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
        A_UTILS_TEST_EXT(113 == reader.setCurrentPos(113, 
            TimeFormat::tf_chunk_index),
            "SetCurrentPos returned wrong new position");
        A_UTILS_TEST_RESULT_EXT(reader.reset(), "failed to reset open file");

        int64_t pos_chunk_index = reader.getCurrentPos(TimeFormat::tf_chunk_index);
        int64_t pos_chunk_time = reader.getCurrentPos(TimeFormat::tf_chunk_time);
        int64_t pos_stream_index =reader.getCurrentPos(TimeFormat::tf_stream_index);

        A_UTILS_TEST_EXT(0 == pos_chunk_index, "reset failed for chunkindex");
        A_UTILS_TEST_EXT(FIRST_TIME == pos_chunk_time,  "reset failed for chunktime");
        A_UTILS_TEST_EXT(0 == pos_stream_index, "reset failed for streampos");
        A_UTILS_TEST_EXT(0 == reader.getFilePos(), "reset failed for filepos");
        A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");
        EXPECT_DEATH(reader.reset(), "");
    }
    {   //seek
        IndexedFileReader reader;
        ChunkHeader *header=NULL;
        uint8_t *data=NULL;
        //__a_utils_test_ext(-1 == reader.Seek(11, 12, 13), "seek on not opened file should have failed"); //@crash
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
        reader.seek(1, LAST_TIME, TimeFormat::tf_chunk_time);
        A_UTILS_TEST(LAST_TIME == reader.getCurrentPos(TimeFormat::tf_chunk_time));
        A_UTILS_TEST_RESULT_EXT(reader.readNextChunk(&header, (void**)&data, 0, 1), "failed to read last chunk");
        A_UTILS_TEST_ERR_RESULT_EXT(reader.readNextChunk(&header, (void**)&data, 0, 1),
            "read behind last chunk of stream");

        reader.seek(0, FIRST_TIME, TimeFormat::tf_chunk_time);
        A_UTILS_TEST(FIRST_TIME == reader.getCurrentPos(TimeFormat::tf_chunk_time));
        A_UTILS_TEST_RESULT_EXT(reader.queryChunkInfo(&header), "failed to read first chunk");
        A_UTILS_TEST(FIRST_TIME == header->time_stamp);

        A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");
    }
}

DEFINE_TEST(TesterIndexedFileReader, 
            TestInfo, 
            "1.3", 
            "Access", 
            "Test GetChunkCount, GetDuration, GetVersionId, GetTimeOffset, GetFirstTime, GetLastTime", 
            "", 
            "", 
            "none", 
            "", 
            "Automatic")
{
    using namespace ifhd::v400;
    {
        IndexedFileReader reader;
        A_UTILS_TEST_EXT(0 == reader.getChunkCount(), "chunkcount of unopened file should be zero"); //@crash
        A_UTILS_TEST_EXT(0 == reader.getDuration(), "duration of unopened file should be zero");
        A_UTILS_TEST_EXT(0 == reader.getVersionId(), "version id of unopened file should be zero");
        A_UTILS_TEST_EXT(0 == reader.getTimeOffset(), "time offset of unopened file should be zero");
        A_UTILS_TEST_EXT(0 == reader.getFirstTime(0), "first time of unopened file should be zero");
        A_UTILS_TEST_EXT(0 == reader.getLastTime(0), "last time of unopened file should be zero");
    }
    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");

        A_UTILS_TEST_EXT(CHUNKCOUNT == reader.getChunkCount(), 
            a_util::strings::format("Chunkcount does not match. Expected %l, got %l.", 
            CHUNKCOUNT, reader.GetChunkCount()));

        A_UTILS_TEST_EXT(DURATION == reader.getDuration(), 
            a_util::strings::format("Duration does not match. Expected %l, got %l.", 
            DURATION, reader.GetDuration()));

        A_UTILS_TEST_EXT(TIME_OFFSET == reader.getTimeOffset(), 
            a_util::strings::format("Time offset does not match. Expected %l, got %l.", 
            TIME_OFFSET, reader.GetTimeOffset()));

        A_UTILS_TEST_EXT(VERSION_ID == reader.getVersionId(), 
            a_util::strings::format("First time does not match. Expected %l, got %l.", 
            VERSION_ID, reader.GetVersionId()));

        A_UTILS_TEST_EXT(FIRST_TIME == reader.getFirstTime(0), 
            a_util::strings::format("First time does not match. Expected %l, got %l.", 
            FIRST_TIME, reader.GetFirstTime(0)));

        A_UTILS_TEST_EXT(LAST_TIME == reader.getLastTime(0), 
            a_util::strings::format("Last time does not match. Expected %d, got %d.", 
            LAST_TIME, reader.GetLastTime(0)));

        A_UTILS_TEST_RESULT_EXT(reader.close(), "failed to close test file");
    }
}

DEFINE_TEST(TesterIndexedFileReader, 
            TestExtension, 
            "1.4", 
            "Extension", 
            "Test FindExtension, GetExtension, GetExtensionCount", 
            "", 
            "", 
            "none", 
            "", 
            "Automatic")
{
    using namespace ifhd::v400;
    {
        IndexedFileReader reader;
        FileExtension *extension;
        void *data=NULL;
        A_UTILS_TEST_ERR_RESULT_EXT(reader.getExtension(-1, &extension, &data), 
            "Succeeded to retrieve extension -1 without open file");
        A_UTILS_TEST_ERR_RESULT_EXT(reader.getExtension(0, &extension, &data), 
            "Succeeded to retrieve extension 0 without open file");
        A_UTILS_TEST_ERR_RESULT_EXT(reader.getExtension(1, &extension, &data), 
            "Succeeded to retrieve extension 1 without open file");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", &extension, &data)
            , "found not existing blubber extension without open file");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", NULL, NULL),
            "succeeded with invaild arguments without open file");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", &extension, NULL),
            "succeeded with invaild arguments  without open file");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", NULL, &data),
            "succeeded with invaild arguments  without open file");
        A_UTILS_TEST_EXT(!reader.findExtension("", &extension, &data),
            "succeeded with empty identifier  without open file");
    }
    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
        A_UTILS_TEST_EXT(EXTENSION_COUNT == reader.getExtensionCount(), 
            a_util::strings::format("Expected %d extensions, but found %d", 
            EXTENSION_COUNT, reader.GetExtensionCount()));
        FileExtension *extension;
        void *data=NULL;
        A_UTILS_TEST_RESULT_EXT(reader.getExtension(0, &extension, &data), "Failed to retrieve extension 0");
        A_UTILS_TEST_RESULT_EXT(reader.getExtension(1, &extension, &data), "Failed to retrieve extension 1");
        A_UTILS_TEST_RESULT_EXT(reader.getExtension(2, &extension, &data), "Failed to retrieve extension 2");
        A_UTILS_TEST_RESULT_EXT(reader.getExtension(3, &extension, &data), "Failed to retrieve extension 3");
        A_UTILS_TEST_RESULT_EXT(reader.getExtension(4, &extension, &data), "Failed to retrieve extension 4");
        reader.close();
    }
    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");
        FileExtension *extension;
        void *data=NULL;
        A_UTILS_TEST_RESULT_EXT(reader.findExtension("referencedfiles", &extension, &data)
            , "failed to find referencedfiles extension");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", &extension, &data)
            , "found not existing blubber extension");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", NULL, NULL), "succeeded with invaild arguments");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", &extension, NULL), "succeeded with invaild arguments");
        A_UTILS_TEST_EXT(!reader.findExtension("blubber", NULL, &data), "succeeded with invaild arguments");
        A_UTILS_TEST_EXT(!reader.findExtension("", &extension, &data), "succeeded with empty identifier");
    }
}

DEFINE_TEST(TesterIndexedFileReader, 
            TestStream, 
            "1.5", 
            "Stream", 
            "Test GetStreamName, GetAdditionalStreaminfo, GetStreamindexCount, GetStreamTableIndexCount", 
            "", 
            "", 
            "none", 
            "", 
            "Automatic")
{   
    using namespace ifhd::v400;
    IndexedFileReader reader;
    A_UTILS_TEST_RESULT_EXT(reader.open(TESTFILE), "failed to open test file");

    int stream_counter=0;
    int stream_info_counter=0;
    for(uint16_t counter = 0; counter < MAX_INDEXED_STREAMS; ++counter)
    {
        const void *data = NULL;
        size_t data_size=0;
        try
        {
            reader.getAdditionalStreamInfo(counter, &data, &data_size);
        }
        catch (...)
        {
            continue;
        }

        {
            ++stream_info_counter;
            if(1 == stream_info_counter)
            {
                A_UTILS_TEST(0 != data_size);
            }else if (2 == stream_info_counter)
            {
                A_UTILS_TEST(0 != data_size);
            }else{
                //stupid tester macro... does only print for failed tests, so whats the purpose
                //__a_utils_log_test_ext("", true, 
                //                       a_util::strings::format("Test file should contain only two streams. "
                //                                        "This is stream <%d> which gets ignored.",
                //                                       streamInfoCounter).GetPtr());
            }
        }

        int64_t index_count = reader.getStreamIndexCount(counter);
        int64_t index_table_count = reader.getStreamTableIndexCount(counter);
        std::string stream_name = reader.getStreamName(counter);
        if(!stream_name.empty())
        {
            ++stream_counter;
            if(1 == stream_counter)
            {
                A_UTILS_TEST_EXT(57 == index_count, "index count for stream 1 does not match expected");
                A_UTILS_TEST_EXT(57 == index_table_count
                    , "index table  count for stream 1 does not match expected");
                A_UTILS_TEST_EXT(std::string("output") == std::string(reader.getStreamName(counter)), 
                    "stream one name does not match");
            }else if(2 == stream_counter)
            {
                A_UTILS_TEST_EXT(57 == index_count, "index count for stream 2 does not match expected");
                A_UTILS_TEST_EXT(57 == index_table_count
                    , "index table  count for stream 2 does not match expected");
                A_UTILS_TEST_EXT(std::string("output2") == std::string(reader.getStreamName(counter)),
                    "stream one name does not match");
            }else{
                A_UTILS_TEST_EXT(false, "Test file should contain only two streams");
            }
        }else{
            if(0 < counter)
            {
                A_UTILS_TEST_EXT(-1 == index_count, "");
                A_UTILS_TEST_EXT(-1 == index_table_count, "");
            }
        }
    }
    A_UTILS_TEST_EXT(2 == stream_counter, "wrong streamcount");
    reader.close();
}

DEFINE_TEST(TesterIndexedFileReader, 
            TestChunks, 
            "1.6", 
            "Chunks", 
            "Test QueryChunkInfo, ReadChunk, ReadNextChunk, ReadNextChunkInfo, SkipChunk, SkipChunkInfo", 
            "", 
            "", 
            "none", 
            "", 
            "Automatic")
{
    using namespace ifhd::v400;
    //test_GenerateTestFile();

    {
        IndexedFileReader reader;
        uint64_t *data=0;
        A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file.dat"), "failed to open test file");
        reader.reset();
        ChunkHeader *header_current, *header_next;

        for (unsigned int counter = 0; counter < reader.getChunkCount()-1; ++counter)
        {
            printf("%d\n", counter);
            A_UTILS_TEST_RESULT_EXT(reader.queryChunkInfo(&header_current), a_util::strings::format("failed to read current chunk header for %d", counter));
            A_UTILS_TEST_RESULT_EXT(reader.readNextChunkInfo(&header_next), "failed to read next chunk header");

            A_UTILS_TEST_EXT(counter+1 == header_current->time_stamp, "read wrong timestamp");
            A_UTILS_TEST_EXT(8+sizeof(ChunkHeader) == header_current->size, "read wrong size");
            A_UTILS_TEST_EXT(1 == header_current->stream_id, "read wrong stream id");
            A_UTILS_TEST_EXT(ChunkType::ct_data == header_current->flags, "read wrong flags");
            A_UTILS_TEST_EXT(counter+1 == header_current->stream_index, "read wrong streamindex");

            A_UTILS_TEST_EXT(counter+1 == header_next->time_stamp, "read wrong timestamp");
            A_UTILS_TEST_EXT(8+sizeof(ChunkHeader) == header_next->size, "read wrong size");
            A_UTILS_TEST_EXT(1 == header_next->stream_id, "read wrong stream id");
            A_UTILS_TEST_EXT(ChunkType::ct_data == header_current->flags, "read wrong flags");
            A_UTILS_TEST_EXT(counter+1 == header_current->stream_index, "read wrong streamindex");

            //__a_utils_test_result_ext(reader.ReadChunk((void**)&data), "failed to read current chunk");
            //__a_utils_test_ext(2 * (counter+1) == *data, "data of chunk is wrong");

            a_util::memory::MemoryBuffer memBlock(header_current->size);
            data = (uint64_t*)memBlock.getPtr();
            A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data, ReadFlags::rf_use_external_buffer), "failed to read current chunk");
            A_UTILS_TEST_EXT(2 * (counter+1) == *data, "data of chunk is wrong");

        }
        reader.close();
    }

    {
        IndexedFileReader reader;
        uint64_t *data=0;
        A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file.dat"), "failed to open test file");
        ChunkHeader *header_current;

        reader.reset();
        for (unsigned int counter = 0; counter < reader.getChunkCount()-1; ++counter)
        {
            A_UTILS_TEST_RESULT_EXT(reader.readNextChunk(&header_current, (void**)&data),
                              a_util::strings::format("failed to read chunk header and data for %d", counter));

            A_UTILS_TEST_EXT(counter == header_current->time_stamp, "read wrong timestamp");
            A_UTILS_TEST_EXT(8+sizeof(ChunkHeader) == header_current->size, "read wrong size");
            A_UTILS_TEST_EXT(1 == header_current->stream_id, "read wrong stream id");
            A_UTILS_TEST_EXT(ChunkType::ct_data == header_current->flags, "read wrong flags");
            A_UTILS_TEST_EXT(counter == header_current->stream_index, "read wrong streamindex");
            A_UTILS_TEST_EXT(2 * counter == *data, "data of chunk is wrong");
        }
        reader.close();
    }
    {
        IndexedFileReader reader;
        uint64_t *data=0;
        A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file.dat"), "failed to open test file");
        reader.reset();
        ChunkHeader *header_current;

        A_UTILS_TEST_ERR_RESULT_EXT(reader.readNextChunk(&header_current, (void**)&data, ChunkType::ct_info), 
                "there should be no info chunk");
        reader.close();
    }
    {
        IndexedFileReader reader;
        uint64_t *data=0;
        A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file.dat"), "failed to open test file");
        reader.reset();
        ChunkHeader *header_current;

        A_UTILS_TEST_ERR_RESULT_EXT(reader.readNextChunk(&header_current, (void**)&data, ChunkType::ct_data, 12), 
            "there should be no data chunks for stream 12");
        reader.close();
    }
    {
        IndexedFileReader reader;
        uint64_t *data=0;
        A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file.dat"), "failed to open test file");
        ChunkHeader *header_current;

        reader.reset();
        for (unsigned int counter = 0; counter < TEST_FILE_CHUNK_COUNT-1; counter+=2)
        {

            A_UTILS_TEST_RESULT_EXT(reader.readNextChunk(&header_current, (void**)&data), 
                "failed to read chunk header and data");
    
            A_UTILS_TEST_EXT(counter == header_current->time_stamp, "read wrong timestamp");
            A_UTILS_TEST_EXT(8+sizeof(ChunkHeader) == header_current->size, "read wrong size");
            A_UTILS_TEST_EXT(1 == header_current->stream_id, "read wrong stream id");
            A_UTILS_TEST_EXT(ChunkType::ct_data == header_current->flags, "read wrong flags");
            A_UTILS_TEST_EXT(counter == header_current->stream_index, "read wrong streamindex");
            A_UTILS_TEST_EXT(2 * counter == *data, "data of chunk is wrong");
            A_UTILS_TEST_RESULT_EXT(reader.skipChunk(), "failed to test chunk");
        }    
        reader.close();
    }
    {
        IndexedFileReader reader;
        A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file.dat"), "failed to open test file");
        ChunkHeader *header_current;

        reader.reset();
        for (unsigned int counter = 0; counter < TEST_FILE_CHUNK_COUNT-1; ++counter)
        {
            A_UTILS_TEST_RESULT_EXT(reader.queryChunkInfo(&header_current), 
                "failed to read chunk header");

            A_UTILS_TEST_EXT(counter == header_current->time_stamp, "read wrong timestamp");
            A_UTILS_TEST_EXT(8+sizeof(ChunkHeader) == header_current->size, "read wrong size");
            A_UTILS_TEST_EXT(1 == header_current->stream_id, "read wrong stream id");
            A_UTILS_TEST_EXT(ChunkType::ct_data == header_current->flags, "read wrong flags");
            A_UTILS_TEST_EXT(counter == header_current->stream_index, "read wrong streamindex");
            A_UTILS_TEST_RESULT_EXT(reader.skipChunkInfo(), "failed to skip header");
        }    
        reader.close();
    }
}


DEFINE_TEST(TesterIndexedFileReader, 
            TestOldVersions, 
            "1.7", 
            "TestOldVersions", 
            "Test some methods with an file with old version (1.1)", 
            "", 
            "", 
            "none", 
            "#9170, #9958", 
            "Automatic")
{
    using namespace ifhd::v400;
    uint64_t *data = NULL;
    IndexedFileReader reader;
    A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/adtf_recording_20071010_163125.dat"), "failed to open test file");
    
    A_UTILS_TEST(reader.getExtensionCount() == 0);
    A_UTILS_TEST(reader.getVersionId() == 0x110);
    A_UTILS_TEST(reader.getChunkCount() == 204);
    A_UTILS_TEST(reader.getDuration() == 7117928);
    A_UTILS_TEST(reader.getTimeOffset() == 0);
    A_UTILS_TEST(reader.getLastTime(0) == 7117928);
    A_UTILS_TEST(reader.getFirstTime(0) == 0);

    ChunkHeader *header_current, *header_next;
    void *pointer_to_void=NULL;

    for (unsigned int counter = 0; counter < reader.getChunkCount()-1; ++counter)
    {
        A_UTILS_TEST_RESULT_EXT(reader.queryChunkInfo(&header_current), a_util::strings::format("failed to read current chunk header for %d", counter));
        A_UTILS_TEST_RESULT_EXT(reader.readNextChunkInfo(&header_next), "failed to read next chunk header");

        data = NULL;
        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(data!=NULL);
    }

    reader.reset();
    A_UTILS_TEST_RESULT_EXT(reader.readNextChunk(&header_next, &pointer_to_void, 0, 0), "failed to read next chunk header");

    // seek
    A_UTILS_TEST_ERR_RESULT(reader.seek(1, 0, TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST(reader.seek(0, 0, TimeFormat::tf_chunk_index, 0) == 0);
    A_UTILS_TEST(reader.seek(0, 0, TimeFormat::tf_chunk_time, 0) == 0);
    A_UTILS_TEST(reader.seek(0, 0, TimeFormat::tf_chunk_index, SeekFlags::sf_keydata) == 0);
    A_UTILS_TEST_ERR_RESULT(reader.seek(0, 0, TimeFormat::tf_stream_index, 0));

    // Test old extensions
    IndexedFileReader reader2;
    A_UTILS_TEST_RESULT_EXT(reader2.open(TEST_FILES_DIR  "/IN-E_1584_20080424_141944.dat"), "failed to open test file");
    
    A_UTILS_TEST(reader2.getExtensionCount() == 56);
    A_UTILS_TEST(reader2.getVersionId() == 0x110);

}

DEFINE_TEST(TesterIndexedFileReader, 
            TestSeek, 
            "1.8", 
            "TestSeek", 
            "Test Seek Method", 
            "", 
            "", 
            "none", 
            "#9170", 
            "Automatic")
{
//    test_GenerateTestFileTwoStreams();
    using namespace ifhd::v400;
    IndexedFileReader reader;
    uint64_t *data = 0;
    A_UTILS_TEST_RESULT_EXT(reader.open(TEST_FILES_DIR "/reader_test_file_two_streams.dat"), "failed to open test file");
    reader.reset();

    A_UTILS_TEST(reader.seek(0, 0, TimeFormat::tf_chunk_index, 0) == 0);
    A_UTILS_TEST(reader.seek(0, 0, TimeFormat::tf_chunk_time, 0) == 0);
    A_UTILS_TEST_ERR_RESULT(reader.seek(0, 0, TimeFormat::tf_stream_index, 0));
    A_UTILS_TEST(reader.seek(1, 0, TimeFormat::tf_stream_index, 0) == 0);
    A_UTILS_TEST(reader.seek(0, 0, TimeFormat::tf_chunk_index, SeekFlags::sf_keydata) == 0);

    //test ChunkTime seek
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; ++counter)
    {
        auto posAfterSeek = reader.seek(1, counter, TimeFormat::tf_chunk_time, 0);
        auto filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter == *data);

        posAfterSeek = reader.seek(2, counter, TimeFormat::tf_chunk_time, 0);
        filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter * 10 == *data);
    }

    //test StreamIndex seek
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; ++counter)
    {
        auto posAfterSeek = reader.seek(1, counter, TimeFormat::tf_stream_index, 0);
        auto filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter == *data);

        posAfterSeek = reader.seek(2, counter, TimeFormat::tf_stream_index, 0);
        filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);
        
        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter * 10 == *data);
    }

    //test ChunkIndex seek
    //check for expected data of first stream
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; counter += 2)
    {
        auto posAfterSeek = reader.seek(1, counter, TimeFormat::tf_chunk_index, 0);
        auto filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter/2 == *data);
    }

    //check for expected data of second stream
    for (int counter = 1; counter < TEST_FILE_CHUNK_COUNT; counter += 2)
    {
        auto posAfterSeek = reader.seek(1, counter, TimeFormat::tf_chunk_index, 0);
        auto filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST((counter-1)*5 == *data);
    }

    //test seekFlags
    //test sf_keydata = 0x01 (just seek in index table)
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; ++counter)
    {
        auto posAfterSeek = reader.seek(1, counter, TimeFormat::tf_chunk_time, 1);
        auto filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);
        
        posAfterSeek = reader.seek(2, counter, TimeFormat::tf_chunk_time, 1);
        filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);
    }

    //test SF_BEFORE = 0x02 (search the last chunk before the seek time)
    for (int counter = 0; counter < TEST_FILE_CHUNK_COUNT; ++counter)
    {
        auto posAfterSeek = reader.seek(1, counter, TimeFormat::tf_chunk_time, 2);
        auto filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter == *data);

        posAfterSeek = reader.seek(2, counter, TimeFormat::tf_chunk_time, 2);
        filePosition = reader.getFilePos();
        A_UTILS_TEST(posAfterSeek == filePosition);

        A_UTILS_TEST_RESULT_EXT(reader.readChunk((void**)&data), "failed to read current chunk");
        A_UTILS_TEST(counter * 10 == *data);
    }

    //test index out of range
    A_UTILS_TEST_ERR_RESULT(reader.seek(1, 2 * TEST_FILE_CHUNK_COUNT, TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST_ERR_RESULT(reader.seek(1, (2 * TEST_FILE_CHUNK_COUNT + 1), TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST_ERR_RESULT(reader.seek(1, -3, TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST_ERR_RESULT(reader.seek(2, 2 * TEST_FILE_CHUNK_COUNT, TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST_ERR_RESULT(reader.seek(2, (2 * TEST_FILE_CHUNK_COUNT + 1), TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST_ERR_RESULT(reader.seek(2, -3, TimeFormat::tf_chunk_index, 0));

    //test not existing stream
    A_UTILS_TEST_ERR_RESULT(reader.seek(-1, 0, TimeFormat::tf_chunk_index, 0));
    A_UTILS_TEST_ERR_RESULT(reader.seek(3, 0, TimeFormat::tf_chunk_index, 0));
}

void test_GenerateTestFileWithHistory()
{
    using namespace ifhd::v400;
    static std::string file = TEST_FILES_DIR "/test_history.dat";
    a_util::filesystem::remove(file);
    IndexedFileWriter writer;
    A_UTILS_TEST_RESULT(writer.create(file, -1, 0, 0, 9000000));
    A_UTILS_TEST_RESULT(writer.setStreamName(1, "stream1"));
    A_UTILS_TEST_RESULT(writer.setStreamName(2, "stream2"));

    timestamp_t time = 0;
    for (int chunk = 0; chunk < 800; ++chunk)
    {
        std::string helper = a_util::strings::format("@1|%d", chunk);
        A_UTILS_TEST_RESULT(writer.writeChunk(1, 
                                                 helper.c_str(),
                                                 static_cast<uint32_t>(helper.length()),
                                                 time,
                                                 ChunkType::ct_data));

        if (chunk % 3 == 0)
        {
            helper = a_util::strings::format("@2|%d", chunk);
            A_UTILS_TEST_RESULT(writer.writeChunk(2,
                                                     helper.c_str(),
                                                     static_cast<uint32_t>(helper.length()),
                                                     time,
                ChunkType::ct_data));
        }

        time += 100001;
        if (chunk == 500)
        {
            A_UTILS_TEST_RESULT(writer.quitHistory());
        }
    }

    A_UTILS_TEST_RESULT(writer.close());
}

DEFINE_TEST(TesterIndexedFileReader,
            TestHistoryPlayback,
            "1.9",
            "TestHistoryPlayback",
            "Test continous playback of history files",
            "",
            "",
            "none",
            "#14847",
            "Automatic")
{
//    test_GenerateTestFileWithHistory();

    using namespace ifhd::v400;
    IndexedFileReader reader;
    A_UTILS_TEST_RESULT(reader.open(TEST_FILES_DIR "/test_history.dat"));

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

DEFINE_TEST(TesterIndexedFileReader,
            TestHistorySeekMasterIndex,
            "1.10",
            "TestHistorySeekMasterIndex",
            "test seeking to each chunk from reverse (master index table)",
            "",
            "",
            "none",
            "#14847",
            "Automatic")
{
//    GenerateTestFileWithHistory();
    using namespace ifhd::v400;
    IndexedFileReader reader;
    A_UTILS_TEST_RESULT(reader.open(TEST_FILES_DIR "/test_history.dat"));

    uint64_t chunk_count = reader.getChunkCount();

    // test seeking to each chunk from reverse (master index table)
    int compare = 799;
    for (int64_t it_chunk = chunk_count - 1; it_chunk > -1; --it_chunk)
    {
        A_UTILS_TEST(reader.seek(0, it_chunk, TimeFormat::tf_chunk_index) == it_chunk);

        ChunkHeader* chunk;
        void* data;
        A_UTILS_TEST_RESULT(reader.readNextChunk(&chunk, &data));

        std::string compare_string = a_util::strings::format("@%d|%d", chunk->stream_id, compare);
        std::string helper(static_cast<char*>(data), chunk->size - sizeof(ChunkHeader));
        printf(helper.c_str());
        printf(compare_string.c_str());
        A_UTILS_TEST(compare_string == helper);

        if (chunk->stream_id == 1)
        {
            --compare;
        }
    }
}

DEFINE_TEST(TesterIndexedFileReader,
            TestHistorySeekStreamIndex,
            "1.11",
            "TestHistorySeekStreamIndex",
            "test seeking to each chunk from reverse (stream index table)",
            "",
            "",
            "none",
            "#14847",
            "Automatic")
{
//    GenerateTestFileWithHistory();
    using namespace ifhd::v400;
    IndexedFileReader reader;
    A_UTILS_TEST_RESULT(reader.open(TEST_FILES_DIR "/test_history.dat"));

    // test seeking to each chunk from reverse (stream index table)
    for (uint16_t stream = 1; stream < 3; ++stream)
    {
        const int64_t stream_chunk_count = reader.getStreamIndexCount(stream);

        int compare;
        if (stream == 1)
        {
            compare = 799;
        }
        else
        {
            compare = 798;
        }

        for (int64_t it_chunk = stream_chunk_count - 1; it_chunk > -1; --it_chunk)
        {
            A_UTILS_TEST(reader.seek(stream, it_chunk, TimeFormat::tf_stream_index) >= 0);

            ChunkHeader* chunk;
            void* data;
            A_UTILS_TEST_RESULT(reader.readNextChunk(&chunk, &data));

            std::string compare_string = a_util::strings::format("@%u|%d", stream, compare);
            std::string helper(static_cast<char*>(data), chunk->size - sizeof(ChunkHeader));
            printf(helper.c_str());
            printf(compare_string.c_str());
            A_UTILS_TEST(compare_string == helper);

            if (stream == 2)
            {
                ----compare;
            }
            --compare;
        }
    }
}

DEFINE_TEST(TesterIndexedFileReader,
            TestHistorySeekTime,
            "1.12",
            "TestHistorySeekTime",
            "Test seeking to each chunk by time (master index table)",
            "",
            "",
            "none",
            "#14847",
            "Automatic")
{
//    GenerateTestFileWithHistory();
    using namespace ifhd::v400;
    IndexedFileReader reader;
    A_UTILS_TEST_RESULT(reader.open(TEST_FILES_DIR "/test_history.dat"));

    // test seeking to each chunk by time
    {
        int compare = 410;

        timestamp_t time_offset = reader.getTimeOffset();
        A_UTILS_TEST(time_offset == 410 * 100001);
        timestamp_t duration = reader.getDuration();
        A_UTILS_TEST(time_offset == 410 * 100001);

        for (timestamp_t time = time_offset; time < time_offset + duration; time += 100001, ++compare)
        {
            A_UTILS_TEST(reader.seek(0, time, TimeFormat::tf_chunk_time) >= 0);

            ChunkHeader* chunk;
            void* data;
            A_UTILS_TEST_RESULT(reader.readNextChunk(&chunk, &data));

            std::string compare_string = a_util::strings::format("@1|%d", compare);
            std::string helper(static_cast<char*>(data), chunk->size - sizeof(ChunkHeader));
            printf(helper.c_str());
            printf(compare_string.c_str());
            A_UTILS_TEST(compare_string == helper);

            if (compare % 3 == 0)
            {
                A_UTILS_TEST_RESULT(reader.readNextChunk(&chunk, &data));

                std::string compare_string = a_util::strings::format("@2|%d", compare);
                std::string helper(static_cast<char*>(data), chunk->size - sizeof(ChunkHeader));
                printf(helper.c_str());
                printf(compare_string.c_str());
                A_UTILS_TEST(compare_string == helper);
            }
        }
    }
}
