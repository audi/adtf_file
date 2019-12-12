#include <gtest/gtest.h>
#include "dattool_helper.h"

namespace
{

std::string getTestOutputFileName(size_t counter)
{
    std::string output_file{TEST_BUILD_DIR "/test_export_" + std::to_string(counter) + ".csv"};
    std::remove(output_file.c_str());
    return output_file;
}

std::string getExampleDatFileName()
{
    return {TEST_SOURCE_DIR "/example_file.adtfdat"};
}

void checkExport(const std::string file_name,
                 const std::string& expected_stream_name,
                 const std::string& expected_property_value,
                 size_t expected_item_count)
{
    std::ifstream output_file;
    output_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    output_file.open(file_name);

    std::string stream_name;
    output_file >> stream_name;
    ASSERT_EQ(expected_stream_name, stream_name);
    std::string property_value;
    output_file >> property_value;
    ASSERT_EQ(expected_property_value, property_value);
    size_t item_count = 0;
    try
    {
        for (;; ++item_count)
        {
            uint64_t time_stamp;
            output_file >> time_stamp;
        }
    }
    catch(...)
    {
    }

    ASSERT_EQ(expected_item_count, item_count);
}

}

GTEST_TEST(dattool, exportStreams)
{
    auto dat_file = getExampleDatFileName();
    auto video_output_file = getTestOutputFileName(0);
    auto struct_output_file = getTestOutputFileName(1);
    auto output = launchDatTool("--plugin " TEST_PROCESSOR_PLUGIN 
                                " --export " + dat_file +
                                " --stream VIDEO" 
                                " --property test_prop=test_value1" 
                                " --output " + video_output_file +
                                " --stream NESTED_STRUCT"
                                " --processorid test"
                                " --property test_prop=test_value2"
                                " --output " + struct_output_file).first;
    ASSERT_TRUE(output.empty());

    checkExport(video_output_file, "VIDEO", "test_value1", 874);
    checkExport(struct_output_file, "NESTED_STRUCT", "test_value2", 595);
}

GTEST_TEST(dattool, exportExtension)
{
    std::string source_file_path{TEST_SOURCE_DIR "/test_modify_extension.adtfdat"};
    auto temp_file_path{TEST_BUILD_DIR "/attached_files.tar.gz"};

    auto dattool_results = launchDatTool("--export "
                            + source_file_path
                            + " --extension attached_files"
                            + " --output "
                            + temp_file_path);

    ASSERT_TRUE(dattool_results.second == 0 && dattool_results.first.empty());

    auto extension_buffer = getExtensionBuffer(source_file_path, "attached_files");

    auto extracted_file_size = std::experimental::filesystem::file_size(temp_file_path);
    std::vector<uint8_t> temp_file_buffer;
    std::ifstream temp_file(temp_file_path, std::ios::binary);
    ASSERT_TRUE(temp_file.is_open());
    temp_file_buffer.insert(temp_file_buffer.end(),
        (std::istreambuf_iterator<char>(temp_file)),
                           std::istreambuf_iterator<char>());

    ASSERT_EQ(temp_file_buffer, extension_buffer);
}

GTEST_TEST(dattool, exportAttachedFilesToStdout)
{
    auto output = launchDatTool("--export " TEST_SOURCE_DIR "/test_modify_extension.adtfdat"
                                " --extension adtf_version").first;
    ASSERT_STREQ (output.c_str(), "adtf 3.0.0 Build 65535 linux64 debug");
}
