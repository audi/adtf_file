#include <adtfdat_processing/processor.h>
#include <fstream>

class TestProcessor : public adtf::dat::Processor
{
public:
    std::string getProcessorIdentifier() const override
    {
        return "test";
    }

    bool isCompatible(const adtf_file::Stream& stream) const override
    {
        return true;
    }

    void open(const adtf_file::Stream& stream, const std::string& destination_url) override
    {
        _output_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        try
        {
            _output_file.open(destination_url);
        }
        catch (...)
        {
            std::throw_with_nested(std::runtime_error("unable to open output file '" +
                                                      destination_url + "' for stream " + stream.name));
        }

        _output_file << stream.name << std::endl
                     << getConfiguration()["test_prop"].value << std::endl;
    }

    void process(const adtf_file::FileItem& item) override
    {
        _output_file << item.time_stamp.count() << std::endl;
    }

private:
    std::ofstream _output_file;
};

static adtf_file::PluginInitializer initializer([] {
    adtf_file::getObjects().push_back(
        std::make_shared<adtf::dat::ProcessorFactoryImplementation<TestProcessor>>());
});
