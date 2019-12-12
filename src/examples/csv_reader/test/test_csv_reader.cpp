#include <gtest/gtest.h>
#include <adtfdat_processing/reader.h>
#include <adtfdat_processing/demultiplexer.h>
#include <adtfdat_processing/ddl_helpers.h>
#include <experimental/filesystem>
#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <chrono>
#include <vector>

using namespace adtf::dat;

static adtf_file::Objects objects;

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

std::string getTestInputFile()
{
    auto output_file_name = std::experimental::filesystem::path(TEST_SOURCE_DIR).append("test.csv").string();
    return output_file_name;
}

GTEST_TEST(CsvReader, processing)
{
    auto input_file_name = getTestInputFile();
    adtf_file::loadPlugin(CSV_READER_PLUGIN);

    auto available_reader_factories = getAdtfDatFactories<ReaderFactories, ReaderFactory>();
    auto capable_reader = available_reader_factories.getCapableReaders(input_file_name);
    ASSERT_EQ(capable_reader.size(), 1);
    auto configuration = capable_reader.at("csv");
    ASSERT_EQ(configuration.count("stream_name"), 1);
    ASSERT_EQ(configuration.count("separator"), 1);
    ASSERT_EQ(configuration.count("timestamp_column_index"), 1);
    ASSERT_EQ(configuration.count("ddl_data_type"), 1);

    auto test_csv_reader = available_reader_factories.make("csv");
    test_csv_reader->setConfiguration({{"separator", {"|", "string"}}});
    test_csv_reader->open(input_file_name);

    auto streams = test_csv_reader->getStreams();
    ASSERT_EQ(1, streams.size());
    const auto& stream = streams[0];
    ASSERT_EQ(0, stream.stream_id);
    ASSERT_EQ("csv", stream.name);
    ASSERT_EQ(0, stream.item_count);
    ASSERT_EQ(0, stream.timestamp_of_first_item.count());
    ASSERT_EQ(0, stream.timestamp_of_last_item.count());
    ASSERT_EQ(0, stream.item_count);

    auto codec_factory = adtf::dat::createCodecFactoryFromStreamType(stream.initial_type);

    for (size_t sample_index = 0; sample_index < 10; ++sample_index)
    {
        const auto& sample_item = test_csv_reader->getNextItem();

        std::chrono::microseconds expected_timestamp(sample_index * 1000000);
        ASSERT_EQ(expected_timestamp, sample_item.time_stamp);

        auto sample = std::dynamic_pointer_cast<const adtf_file::WriteSample>(sample_item.stream_item);
        ASSERT_TRUE(sample);
        ASSERT_EQ(expected_timestamp, sample->getTimeStamp());

        {
            auto buffer = sample->beginBufferRead();
            auto decoder = std::get<0>(codec_factory).makeStaticDecoderFor(buffer.first, buffer.second);

            ASSERT_EQ(ddl::access_element::get_value(decoder, "counter1").asDouble(), sample_index);
            ASSERT_EQ(ddl::access_element::get_value(decoder, "counter2").asDouble(), sample_index + 0.12);
            sample->endBufferRead();
        }

        const auto& trigger_item = test_csv_reader->getNextItem();
        ASSERT_EQ(expected_timestamp, trigger_item.time_stamp);
        ASSERT_TRUE(std::dynamic_pointer_cast<const adtf_file::Trigger>(trigger_item.stream_item));
    }

    ASSERT_THROW(test_csv_reader->getNextItem(), adtf_file::exceptions::EndOfFile);
}
