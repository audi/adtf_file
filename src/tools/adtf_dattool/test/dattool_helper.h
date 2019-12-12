#include <cstdlib>
#include <fstream>
#include <streambuf>
#include <utility>
#include <experimental/filesystem>

#include <ifhd/ifhd.h>

inline std::pair<std::string, int> launchDatTool(const std::string& arguments)
{
    using namespace std::experimental::filesystem;
    path output_file{TEST_BUILD_DIR "/__test_output.txt"};
    std::error_code dummy;
    remove(output_file, dummy);
    // If a pipe operator "|" is involved, Windows only accept native slashes for program start.
    std::string command{path(ADTF_DATTOOL_EXECUTABLE).string() + " "
        + arguments
        + " > " + output_file.string()};
    
    std::cout << "launching: " << command << std::endl;
    auto return_value = std::system(command.c_str());

    std::ifstream helper(output_file);
    std::string output((std::istreambuf_iterator<char>(helper)),
                       std::istreambuf_iterator<char>());
    return {output, return_value};
}

inline std::vector<uint8_t> getExtensionBuffer(const std::string dat_file, 
                                                       const std::string& file_extension)
{
    using namespace ifhd::v400;
    FileExtension extension{};
    void* extension_data{};
    getExtension(dat_file,
                    file_extension,
                    &extension,
                    &extension_data);
    return {static_cast<uint8_t*>(extension_data),
        static_cast<uint8_t*>(extension_data) + extension.data_size};
}
