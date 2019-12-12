#pragma once
#include <adtfdat_processing/processor.h>
#include <vector>
#include <map>

extern std::map<size_t, std::vector<adtf_file::FileItem>> processor_items;

template<size_t compatible_stream_index>
class TestProcessor: public adtf::dat::Processor
{
    public:
        std::string getProcessorIdentifier() const override
        {
            return "test_" + std::to_string(compatible_stream_index);
        }

        bool isCompatible(const adtf_file::Stream& stream) const
        {
            return stream.name == ("stream" + std::to_string(compatible_stream_index));
        }

        void open(const adtf_file::Stream& stream, const std::string& destination_url) override
        {
            if (!isCompatible(stream))
            {
                throw std::runtime_error("not compatible");
            }
        }

        void process(const adtf_file::FileItem& item) override
        {
            processor_items[compatible_stream_index].push_back(item);
        }
};
