#include <gtest/gtest.h>
#include "dattool_helper.h"

using namespace std::experimental::filesystem;

GTEST_TEST(dattool, modifyExtension)
{
    std::string source_file_path{TEST_SOURCE_DIR "/test_modify_extension.adtfdat"};
    std::string temp_file_path{TEST_BUILD_DIR "/test_modify_extension.adtfdat"};
    
    copy(path(source_file_path), path(temp_file_path), copy_options::overwrite_existing);

    source_file_path = TEST_SOURCE_DIR "/test_list_streams.cpp";

    auto dattool_results = launchDatTool("--modify " + temp_file_path
                            + " --extension attached_files"
                            + " --input " + source_file_path);

    ASSERT_TRUE(dattool_results.second == 0 && dattool_results.first.empty());

    std::vector<uint8_t> file_buffer;
    std::ifstream source_file(source_file_path);
    ASSERT_TRUE(source_file.is_open());
    file_buffer.insert(file_buffer.end(),
        (std::istreambuf_iterator<char>(source_file)),
                       std::istreambuf_iterator<char>());

    auto extension_buffer = getExtensionBuffer(temp_file_path, "attached_files");

    ASSERT_EQ(extension_buffer, file_buffer);
}

GTEST_TEST(dattool, copyExtensionWithModifiedIds)
{
    using namespace ifhd::v400;

    path source_file_path{TEST_SOURCE_DIR "/test_modify_extension.adtfdat"};
    path temp_file_path{TEST_BUILD_DIR "/test_modify_extension.adtfdat"};

    // If a pipe operator "|" is involved, Windows only accept native slashes for program start.
    auto dattool_results = launchDatTool("--export " + source_file_path.string()
                                         + " --extension attached_files"
                                         + " | "
                                         + path(ADTF_DATTOOL_EXECUTABLE).string()
                                         + " --modify " + temp_file_path.string()
                                         + " --extension attached_files"
                                         + " --userid 1"
                                         + " --typeid 1"
                                         + " --versionid 1");

    ASSERT_TRUE(dattool_results.second == 0 && dattool_results.first.empty());

    auto buffer = getExtensionBuffer(source_file_path.string(), "attached_files");
    auto copied_buffer = getExtensionBuffer(temp_file_path.string(), "attached_files");

    ASSERT_EQ(buffer, copied_buffer);

    FileExtension extension{};
    void* extension_data{};
    getExtension(source_file_path.string(),
                                 "attached_files",
                                 &extension,
                                 &extension_data);
    FileExtension copied_extension{};
    void* copied_extension_data{};
    getExtension(temp_file_path.string(),
                                 "attached_files",
                                 &copied_extension,
                                 &copied_extension_data);
    ASSERT_NE(extension.user_id, copied_extension.user_id);
    ASSERT_NE(extension.type_id, copied_extension.type_id);
    ASSERT_NE(extension.version_id, copied_extension.version_id);
}