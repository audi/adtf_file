#include <gtest/gtest.h>
#include "dattool_helper.h"
#include <adtf_file/standard_adtf_file_reader.h>

namespace
{

struct ExpectedStreamData
{
    std::string name;
    uint64_t item_count;
    uint64_t first_item;
    uint64_t last_item;
};

void checkStreams(const std::string& file_name,
                  const std::vector<ExpectedStreamData>& expected_streams)
{
    adtf_file::StandardReader reader(file_name);

    ASSERT_EQ(reader.getStreams().size(), expected_streams.size());
    size_t stream_index = 0;
    for (const auto& expected_stream: expected_streams)
    {
        auto& stream = reader.getStreams()[stream_index++];
        ASSERT_EQ(stream.name, expected_stream.name);
        ASSERT_EQ(stream.item_count, expected_stream.item_count);
        if (expected_stream.first_item != expected_stream.last_item)
        {
            ASSERT_EQ(stream.timestamp_of_first_item.count(), expected_stream.first_item);
            ASSERT_EQ(stream.timestamp_of_last_item.count(), expected_stream.last_item);
        }
    }
}

std::string getTestDatFileName()
{
    std::string output_file{TEST_BUILD_DIR "/test_create.adtfdat"};
    remove(output_file.c_str());
    return output_file;
}

std::string getExampleDatFileName()
{
    return {TEST_SOURCE_DIR "/example_file.adtfdat"};
}

}

GTEST_TEST(dattool, createWithSimpleInput)
{
    auto output_file = getTestDatFileName();
    auto dat_file = getExampleDatFileName();
    auto output = launchDatTool("--create " + output_file +
                                      " --input " + dat_file).first;
    ASSERT_TRUE(output.empty());

    checkStreams(output_file,
    {
        { "VIDEO", 872},
        { "NESTED_STRUCT", 593},
    });
}

GTEST_TEST(dattool, createWithStreamSelection)
{
    auto output_file = getTestDatFileName();
    auto dat_file = getExampleDatFileName();
    auto output = launchDatTool("--create " + output_file +
                                      " --input " + dat_file +
                                      " --readerid adtfdat" +
                                      " --stream VIDEO --name NEW_VIDEO" +
                                      " --serializerid sample_copy_serialization.serialization.adtf.cid").first;
    ASSERT_TRUE(output.empty());

    checkStreams(output_file,
    {
        { "NEW_VIDEO", 872}
    });
}

GTEST_TEST(dattool, createWithStartEndOffset)
{
    auto output_file = getTestDatFileName();
    auto dat_file = getExampleDatFileName();
    auto output = launchDatTool("--create " + output_file +
                                      " --input " + dat_file +
                                      " --start 10000000" +
                                      " --end 11000000" +
                                      " --offset 10000000"
                                      " --stream NESTED_STRUCT").first;
    ASSERT_TRUE(output.empty());

    checkStreams(output_file,
    {
        { "NESTED_STRUCT", 40, 20005703000, 20965556000}
    });
}

GTEST_TEST(dattool, createWithMultipleInputs)
{
    auto output_file = getTestDatFileName();
    auto dat_file = getExampleDatFileName();
    auto output = launchDatTool("--create " + output_file +
                                      " --input " + dat_file +
                                      " --readerid adtfdat"
                                      " --stream NESTED_STRUCT --name FROM1"
                                      " --serializerid sample_copy_serialization.serialization.adtf.cid"
                                      " --input " + dat_file +
                                      " --readerid adtfdat"
                                      " --stream NESTED_STRUCT --name FROM2"
                                      " --serializerid sample_copy_serialization.serialization.adtf.cid").first;
    ASSERT_TRUE(output.empty());

    checkStreams(output_file,
    {
        { "FROM1", 593 },
        { "FROM2", 593 }
    });
}

GTEST_TEST(dattool, createWithExtensions)
{
    std::string source_file_path{TEST_SOURCE_DIR "/test_modify_extension.adtfdat"};
    std::string temp_file_path{TEST_BUILD_DIR "/test_modify_extension.adtfdat"};

    auto dattool_results = launchDatTool("--create "
                            + temp_file_path
                            + " --input "
                            + source_file_path
                            + " --extension attached_files"
                            + " --extension attached_files_configuration");
    ASSERT_TRUE(dattool_results.second == 0 && dattool_results.first.empty());

    auto buffer = getExtensionBuffer(source_file_path, "attached_files");
    auto copied_buffer = getExtensionBuffer(temp_file_path, "attached_files");
    ASSERT_EQ(buffer, copied_buffer);

    buffer = getExtensionBuffer(source_file_path, "attached_files_configuration");
    copied_buffer = getExtensionBuffer(temp_file_path, "attached_files_configuration");
    ASSERT_EQ(buffer, copied_buffer);
}
