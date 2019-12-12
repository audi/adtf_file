#pragma once
#include <adtfdat_processing/reader.h>
#include <queue>

template<int implementation_index, bool always_incompatible = false>
class TestReader : public adtf::dat::Reader
{
    public:
        std::string getReaderIdentifier() const override
        {
            return "test_" + std::to_string(implementation_index);
        }

        std::pair<bool, std::string> isCompatible(const std::string& url) const override
        {
            if (!always_incompatible && url == "compatible")
            {
                return {true, ""};
            }

            return {false, "not compatible"};
        }

        void open(const std::string& url) override
        {
            auto is_compatible = isCompatible(url);
            if (!is_compatible.first)
            {
                throw std::runtime_error(is_compatible.second);
            }

            for (uint32_t sample_index = 0; sample_index < 10; ++sample_index)
            {
                auto sample = std::make_shared<adtf_file::DefaultSample>();
                sample->setTimeStamp(std::chrono::seconds(sample_index));
                auto buffer = sample->beginBufferWrite(sizeof(sample_index));
                *reinterpret_cast<decltype(sample_index)*>(buffer) = sample_index;
                sample->endBufferWrite();

                _items.push({0, std::chrono::seconds(sample_index + implementation_index), sample});
                _items.push({3, std::chrono::seconds(sample_index + implementation_index), sample});
            }
        }

        std::vector<adtf_file::Stream> getStreams() const override
        {
            auto type = std::make_shared<adtf_file::DefaultStreamType>();
            type->setMetaType("adtf/anonymous");
            return {{0, "stream1", 10, std::chrono::seconds(0), std::chrono::seconds(9), type},
                    {3, "stream2", 10, std::chrono::seconds(0), std::chrono::seconds(9), type}};
        }

        adtf_file::FileItem getNextItem() override
        {
            if (_items.empty())
            {
                throw adtf_file::exceptions::EndOfFile();
            }

            auto next_item = _items.front();
            _items.pop();
            return next_item;
        }

        double getProgress() const override
        {
            return (20 - _items.size()) / 20.0;
        }

    private:
        std::queue<adtf_file::FileItem> _items;
};
