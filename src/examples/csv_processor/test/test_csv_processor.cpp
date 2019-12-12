#include <gtest/gtest.h>
#include <adtfdat_processing/reader.h>
#include <adtfdat_processing/demultiplexer.h>
#include <experimental/filesystem>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <chrono>
#include <vector>

using namespace adtf::dat;

static adtf_file::Objects objects;

struct TestStruct
{
    uint64_t counter1;
    double counter2;
};

static constexpr const char* test_struct_ddl =
R"(
<struct alignment="4" name="TestStruct" version="1">
    <element name="counter1" type="tUInt64" arraysize="1">
        "<deserialized alignment="4"/>
        "<serialized byteorder="LE" bytepos="0"/>
    </element>
    <element name="counter2" type="tFloat64" arraysize="1">
        "<deserialized alignment="4"/>
        "<serialized byteorder="LE" bytepos="8"/>
    </element>
</struct>
)";

std::shared_ptr<adtf_file::StreamType> createTestStructStreamType()
{
    auto type = std::make_shared<adtf_file::DefaultStreamType>();
    type->setMetaType("adtf/default");
    type->setProperty("md_struct", "cString", "TestStruct");
    type->setProperty("md_definitions", "cString", test_struct_ddl);
    return type;
}

template <typename FACTORIES, typename FACTORY>
FACTORIES getAdtfDatFactories()
{
    FACTORIES factories;

    for (const auto& factory: adtf_file::getObjects().getAllOfType<FACTORY>())
    {
        factories.add(factory);
    }

    return factories;
}

std::string getTestOutputFile()
{
    auto output_file_name = std::experimental::filesystem::path(TEST_BUILD_DIR).append("test.csv").string();
    std::error_code dummy;
    std::experimental::filesystem::remove(output_file_name, dummy);
    return output_file_name;
}

GTEST_TEST(CsvProcessor, processing)
{
    auto output_file_name = getTestOutputFile();
    adtf_file::loadPlugin(CSV_PROCESSOR_PLUGIN);

    const adtf_file::Stream test_stream
    {
        0,
        "test_ddl",
        10,
        std::chrono::seconds(0),
        std::chrono::seconds(9),
        createTestStructStreamType()
    };

    auto available_processor_factories = getAdtfDatFactories<ProcessorFactories, ProcessorFactory>();
    auto capable_processors = available_processor_factories.getCapableProcessors(test_stream);
    ASSERT_EQ(capable_processors.size(), 1);
    auto configuration = capable_processors.at("csv");
    ASSERT_EQ(configuration.count("decimal_places"), 1);
    ASSERT_EQ(configuration.count("separator"), 1);

    {
        auto test_csv_processor = available_processor_factories.make("csv");
        test_csv_processor->setConfiguration({{"decimal_places", {"2", "uint64"}}, {"separator", {"|", "string"}}});
        test_csv_processor->open(test_stream, output_file_name);

        for (uint64_t sample_index = 0; sample_index < 10; ++sample_index)
        {
            auto sample = std::make_shared<adtf_file::DefaultSample>();
            sample->setTimeStamp(std::chrono::seconds(sample_index));
            auto buffer = sample->beginBufferWrite(sizeof(TestStruct));
            *reinterpret_cast<TestStruct*>(buffer) = { sample_index, static_cast<double>(sample_index) + 0.12345 };
            sample->endBufferWrite();
            adtf_file::FileItem item{0, std::chrono::seconds(sample_index), sample};
            test_csv_processor->process(item);
        }
    }

    std::ifstream csv_file(output_file_name);
    std::vector<std::string> lines;
    std::string line;
    while (csv_file >> line)
    {
        lines.push_back(line);
    }

    ASSERT_EQ(11, lines.size());
    ASSERT_EQ("timestamp|counter1|counter2", lines[0]);
    ASSERT_EQ("0|0|0.12", lines[1]);
    ASSERT_EQ("1000000000|1|1.12", lines[2]);
    ASSERT_EQ("2000000000|2|2.12", lines[3]);
    ASSERT_EQ("3000000000|3|3.12", lines[4]);
    ASSERT_EQ("4000000000|4|4.12", lines[5]);
    ASSERT_EQ("5000000000|5|5.12", lines[6]);
    ASSERT_EQ("6000000000|6|6.12", lines[7]);
    ASSERT_EQ("7000000000|7|7.12", lines[8]);
    ASSERT_EQ("8000000000|8|8.12", lines[9]);
    ASSERT_EQ("9000000000|9|9.12", lines[10]);
}
