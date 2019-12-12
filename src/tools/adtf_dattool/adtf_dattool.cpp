/**
*
* ADTF Dat Exporter.
*
* @file
* Copyright &copy; 2003-2016 Audi Electronics Venture GmbH. All rights reserved
*
* $Author: WROTHFL $
* $Date: 2017-06-06 15:51:27 +0200 (Di, 06 Jun 2017) $
* $Revision: 61312 $
*
* @remarks
*
*/

#ifdef WIN32
# include <io.h>
# include <fcntl.h>
# define SET_BINARY_MODE(handle) setmode(handle, O_BINARY)
# define isatty(handle) _isatty(handle)
#else
#include <unistd.h>
# define SET_BINARY_MODE(handle) ((void)0)
#endif
#include <array>
#include <iostream>
#include <fstream>

#include <clara.hpp>

#include <adtfdat_processing/adtfdat_processing.h>
#include <adtf_file/standard_factories.h>
#include <a_util/filesystem.h>

static adtf_file::Objects objects;
static adtf_file::PluginInitializer initializer([] {
    adtf_file::add_standard_objects();
    adtf_file::getObjects().push_back(
        std::make_shared<adtf::dat::ReaderFactoryImplementation<adtf::dat::AdtfDatReader>>());
});

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

void listReaderStreams(const std::shared_ptr<const adtf::dat::Reader>& reader,
                         const adtf::dat::ProcessorFactories& processor_factories)
{
    for (auto& stream : reader->getStreams())
    {
        auto property_type =
            std::dynamic_pointer_cast<const adtf_file::PropertyStreamType>(stream.initial_type);
        std::cout << "    " << stream.name << ":\n";
        if (property_type)
        {
            std::cout << "        type: " << property_type->getMetaType() << "\n";
        }
        
        std::cout << "        processors:";
        bool first = true;
        for (auto& processor : processor_factories.getCapableProcessors(stream))
        {
            if (!first)
            {
                std::cout << ",";
            }
            std::cout << " " << processor.first;
            first = false;
        }

        std::cout << "\n"
                  << "        time range (ns): [" << stream.timestamp_of_first_item.count() << ", "
                  << stream.timestamp_of_last_item.count() << "]\n"
                  << "        items: " << stream.item_count << "\n";
    }
}

class ProgressDisplay
{
public:
    bool operator()(double progress)
    {
        if (_last_percent == -1)
        {
            _start = std::chrono::high_resolution_clock::now();
        }

        int percent = static_cast<int>(progress * 100);

        if (_last_percent != percent)
        {
            if (_output_started)
            {
                std::cout << "\r";
            }

            std::cout << "[";

            int position = 0;
            for (; position < _last_percent / 2; ++position)
            {
                std::cout << "=";
            }
            std::cout << ">";
            ++position;
            for (; position < 50; ++position)
            {
                std::cout << " ";
            }

            std::cout << "] " << percent << "%";

            if (progress != 0.0)
            {
                std::chrono::duration<double> elapsed_seconds =
                    std::chrono::high_resolution_clock::now() - _start;
                auto eta = (elapsed_seconds * (1.0 - progress) / progress);
                std::cout << " ETA: " << static_cast<int>(eta.count() + 0.9) << "s";
            }

            std::cout << std::flush;

            _last_percent = percent;
            _output_started = true;
        }

        return true;
    }

    ~ProgressDisplay()
    {
        if (_output_started)
        {
            std::cout << "\n";
        }
    }

private:
    int _last_percent = -1;
    bool _output_started = false;
    std::chrono::high_resolution_clock::time_point _start;
};

std::string long_help = R"(
-----------------
  ADTF DAT Tool
-----------------

With the help of this tool you can extract data from ADTF DAT files, create new ADTF DAT files
from various inputs or add file extensions in combination with the adtf_datdump tool
to an existing ADTF DAT file.

Use --liststreams to query all information about a given input (ADTF DAT file, or any other
supported input).

To load additional plugins use the --plugin argument as often as you like.

-----------
  EXPORT:  
-----------
To extract data from streams or file extensions of an ADTF DAT file use the --export argument.
Select the streams you want to extract by using the --stream argument. You can specify a processor
for each stream with the --processorid argument. If you do not specify one explicitly, the first one
that supports the stream is used. Properties of processors can be specified with the --property
argument. 

Use the --extension argument to specify the required file extension. The destination
filename is specified with the --output argument. Without --output argument the file extension data
will be written to stdout.

Examples:
---------
Here is an example that exports two streams:
adtf_dattool --plugin csv_processor.adtffileplugin --export test.dat --stream in1
--processorid csv --output test_in1.csv --stream in2 --output test_in2.csv

And one that exports extension data:
adtf_dattool --export test.dat --extension adtf_version --output adtf_version.txt

-------------
  CREATION:  
-------------
To create a new ADTF DAT file use the --create argument. The --input argument is used to specify an
input file. The --readerid argument can be used to specify the reader that should be used to read
the file. If none is specified, the first one that supports the file is used.

Use the --start and --end arguments to select the range of the input that should be
imported into the ADTF DAT file. Use the --offset parameter to shift the timstamps of all imported
stream items.

To select streams from an input, use the --stream argument. If you do not select one or more
streams neither a extension, all streams will be added. Streams can be renamed with the --name argument.

Use the --serializerid argument to choose the serializer of your liking. If not specified,
sample_copy_serialization_ns.serialization.adtf.cid will be used.

To select file extensions from an input, use the --extension argument. If you do not select one or
more file extensions explicitly, no extension will be added or updated from the source file.

Examples:
---------
Here is an example that creates a new ADTF DAT file from two inputs:
adtf_dattool --create new.adtfdat --input input1.dat --readerid adtfdat --stream in1 --serializerid
special.serialization.adtf.cid --input input2.dat

And one that copies a stream and two extensions from a source file
adtf_dattool --create new.adtfdat --input input1.dat --stream in1 --extension attached_files
--extension attached_files_configuration

------------------------
  MODIFYING:
------------------------
To modify an existing ADTF DAT File use the --modify argument
Currently there is only support for adding and updating extensions.

Examples:
---------
An example to put a files content into an extension:
adtf_dattool --modify existing.adtfdat --extension my_extension --input input_file.txt

Mind that if you do not specify an --input for an extension its data will be read from stdin:
tar cz /myfolder | adtf_dattool --modify existing.adtfdat --extension attached_files

or use:
adtf_dattool --export source.adtfdat --extension attached_files | adtf_dattool --modify destination.adtfdat --extension attached_files

to copy a explicit file extension of an existing ADTF DAT file to another.
)";

std::string reformatHelpText(std::string text)
{
    for (size_t position = 0; position < text.length() - 1; ++position)
    {
        if (text[position] == '\n' && text[position + 1] != '\n')
        {
            text[position] = ' ';
        }
    }
    return text;
}

void printException(const std::exception& error, int level = 0)
{
    std::cerr << std::string(level, ' ') << "exception: " << error.what() << '\n';
    try
    {
        std::rethrow_if_nested(error);
    }
    catch (const std::exception& nested_exception)
    {
        printException(nested_exception, level + 1);
    }
    catch (...)
    {
    }
}

struct ExportStream
{
    std::string name;
    std::string processor_id;
    adtf::dat::Configuration configuration;
    std::string output_file_name;
};

struct ExportExtension
{
    std::string name;
    std::string output_file_name;
};

struct ExportJob
{
    std::string file_name;
    std::vector<ExportStream> streams;
    std::vector<ExportExtension> extensions;
};

struct InputStream
{
    std::string name;
    std::string output_name;
    std::string serializer_id;
};

struct InputExtension
{
    std::string name;
};

struct Input
{
    std::string source;
    std::string reader_id;
    std::chrono::nanoseconds start;
    std::chrono::nanoseconds end;
    std::chrono::nanoseconds offset;
    std::vector<InputStream> streams;
    std::vector<InputExtension> extensions;
    adtf::dat::Configuration configuration;
    std::shared_ptr<adtf::dat::Reader> reader_instance;
};

struct CreateJob
{
    std::string file_name;
    std::string file_version;
    std::vector<Input> inputs;
};

struct ModificationExtension
{
    std::string name;
    std::string input_file;
    uint32_t user_id;
    uint32_t type_id;
    uint32_t version_id;
};

struct ModificationJob
{
    std::string file_name;
    std::vector<ModificationExtension> extensions;
};

void listSourceStreams(const std::string& source,
                       const adtf::dat::ReaderFactories& reader_factories,
                       const adtf::dat::ProcessorFactories& processor_factories)
{
    auto capable_readers = reader_factories.getCapableReaders(source);
    for (auto& reader_id : capable_readers)
    {
        auto reader = reader_factories.make(reader_id.first);
        reader->open(source);
        std::cout << reader_id.first << ":\n";
        listReaderStreams(reader, processor_factories);
    }
}

std::vector<uint8_t> readFromStdin()
{
    if (isatty(fileno(stdin)) != 0)
    {
        std::cerr << "waiting for input on stdin" << std::endl;
    }
    SET_BINARY_MODE(fileno(stdin));
    
    if (std::ferror(stdin))
    {
        throw std::runtime_error(std::strerror(errno));
    }

    size_t length;
    std::array<uint8_t, 1024> buffer;
    std::vector<uint8_t> input;
    
    while ((length = std::fread(buffer.data(), sizeof(buffer[0]), buffer.size(), stdin)) > 0)
    {
        if (std::ferror(stdin) && !std::feof(stdin))
        {
            throw std::runtime_error(std::strerror(errno));
        }
        input.insert(input.end(), buffer.data(), buffer.data() + length);
    }
    return input;
}

void writeToStdout(const void* data, size_t data_size)
{
    SET_BINARY_MODE(fileno(stdout));

    if (std::ferror(stdout))
    {
        throw std::runtime_error(std::strerror(errno));
    }
    size_t all_bytes_written = 0;
    do
    {
        size_t bytes_written = fwrite(data, 1, data_size, stdout);
        data_size -= bytes_written;
        data = reinterpret_cast<const uint8_t*>(data) + bytes_written;
        all_bytes_written += bytes_written;
    } while (data_size > 0);
}

adtf_file::Extension find_required_extension(std::vector<adtf_file::Extension> extensions, const std::string& name)
{
    auto existing_extension = std::find_if(extensions.begin(), extensions.end(), [&](const adtf_file::Extension& extension)
    {
        return extension.name == name;
    });

    if (existing_extension == extensions.end())
    {
        throw std::runtime_error("no extension with name '" + name + "' found.");
    }

    return *existing_extension;
}

void processExportJob(const ExportJob& export_job,
                      std::function<bool(double)> progress_handler,
                      const adtf::dat::ProcessorFactories& processor_factories)
{
    auto reader = std::make_shared<adtf::dat::AdtfDatReader>();
    reader->open(export_job.file_name);

    adtf::dat::Demultiplexer demultiplexer(reader, processor_factories);

    if (export_job.streams.empty() && export_job.extensions.empty())
    {
        throw std::runtime_error(
            "no items specified, please select streams or extensions with one or more --stream or --extension arguments.");
    }

    for (const auto& selected_stream: export_job.streams)
    {
        std::string processor_id = selected_stream.processor_id;
        if (selected_stream.processor_id.empty())
        {
            auto stream = adtf::dat::findStream(*reader, selected_stream.name);
            auto capable_processors = processor_factories.getCapableProcessors(stream);
            if (capable_processors.empty())
            {
                throw std::runtime_error("no processor capable of handling the stream '" +
                                         selected_stream.name + "' found.");
            }
            processor_id = capable_processors.begin()->first;
        }

        demultiplexer.addProcessor(selected_stream.name,
                                   processor_id,
                                   selected_stream.output_file_name,
                                   selected_stream.configuration);
    }

    demultiplexer.process(progress_handler);

    auto extensions = reader->getExtensions();
    for (const auto& selected_extension: export_job.extensions)
    {
        auto existing_extension = find_required_extension(extensions, selected_extension.name);

        if (selected_extension.output_file_name.empty())
        {
            writeToStdout(existing_extension.data, existing_extension.data_size);
        }
        else
        {
            std::ofstream outfile;
            outfile.exceptions(std::ofstream::failbit|std::ofstream::badbit);
            outfile.open(a_util::filesystem::Path(selected_extension.output_file_name).toString(), std::ofstream::binary);
            outfile.write(static_cast<const char*>(existing_extension.data),
                          existing_extension.data_size);
        }
    }
}

void processCreateJob(CreateJob& create_job,
                      std::function<bool(double)> progress_handler,
                      bool skip_stream_types_and_triggers,
                      const adtf::dat::ReaderFactories& reader_factories,
                      const adtf_file::SampleSerializerFactories& sample_serializer_factories)
{
    static std::map<std::string, adtf_file::Writer::TargetADTFVersion> target_versions{{"adtf2", adtf_file::Writer::TargetADTFVersion::adtf2},
                                                                                       {"adtf3", adtf_file::Writer::TargetADTFVersion::adtf3},
                                                                                       {"adtf3ns", adtf_file::Writer::TargetADTFVersion::adtf3ns}};
    auto target_file_version = target_versions.at(create_job.file_version);
    adtf::dat::Multiplexer multiplexer(
        create_job.file_name, target_file_version, skip_stream_types_and_triggers);

    if (create_job.inputs.empty())
    {
        throw std::runtime_error(
            "no inputs specified, please select inputs with one or more --input arguments.");
    }

    for (auto& input: create_job.inputs)
    {
        auto reader_id = input.reader_id;
        if (reader_id.empty())
        {
            auto capable_readers = reader_factories.getCapableReaders(input.source);
            if (capable_readers.empty())
            {
                throw std::runtime_error("no capable reader for '" + input.source + "'found");
            }
            reader_id = capable_readers.begin()->first;
        }

        auto original_reader = reader_factories.make(reader_id);
        original_reader->setConfiguration(input.configuration);
        original_reader->open(input.source);

        auto selected_streams = input.streams;
        if (selected_streams.empty() && input.extensions.empty())
        {
            for (auto& stream : original_reader->getStreams())
            {
                selected_streams.push_back({stream.name, stream.name});
            }
        }

        input.reader_instance = original_reader;
        auto reader = std::make_shared<adtf::dat::OffsetReaderWrapper>(original_reader,
                                                                       input.offset,
                                                                       input.start,
                                                                       input.end);

        for (const auto& selected_stream: selected_streams)
        {
            auto serializer_id = selected_stream.serializer_id;
            if (serializer_id.empty())
            {
                switch (target_file_version)
                {
                    case adtf_file::Writer::TargetADTFVersion::adtf3:
                    {
                        serializer_id = adtf_file::adtf3::SampleCopySerializer::id;
                        break;
                    }
                    case adtf_file::Writer::TargetADTFVersion::adtf3ns:
                    {
                        serializer_id = adtf_file::adtf3::SampleCopySerializerNs::id;
                        break;
                    }
                    case adtf_file::Writer::TargetADTFVersion::adtf2:
                    {
                        serializer_id = adtf_file::adtf2::AdtfCoreMediaSampleSerializer::id;
                        break;
                    }
                }
            }

            multiplexer.addStream(reader,
                                  selected_stream.name,
                                  selected_stream.output_name,
                                  sample_serializer_factories.build(serializer_id));
        }
    }

    multiplexer.process(progress_handler);

    for (auto& input: create_job.inputs)
    {
        for (const auto& selected_extension: input.extensions)
        {
            auto dat_file_reader = std::dynamic_pointer_cast<adtf::dat::AdtfDatReader>(input.reader_instance);
            if (!dat_file_reader)
            {
                throw std::runtime_error("extensions can currently only be imported from ADTF DAT Files.");
            }

            auto existing_extension = find_required_extension(dat_file_reader->getExtensions(), selected_extension.name);
            multiplexer.addExtension(existing_extension.name,
                                     existing_extension.data,
                                     existing_extension.data_size,
                                     existing_extension.user_id,
                                     existing_extension.type_id,
                                     existing_extension.version_id);
        }

        input.reader_instance.reset();
    }
}

void processModificationJob(const ModificationJob& modification_job)
{
    for (const auto& extension: modification_job.extensions)
    {
        std::vector<uint8_t> data_buffer;
        if (extension.input_file.empty())
        {
            data_buffer = readFromStdin();
        }
        else
        {
            std::ifstream source_file(a_util::filesystem::Path(extension.input_file).toString());
            if (!source_file.is_open())
            {
                throw std::runtime_error("unable to open file: " + extension.input_file);
            }
            data_buffer.insert(data_buffer.end(),
                               (std::istreambuf_iterator<char>(source_file)),
                               std::istreambuf_iterator<char>());
        }

        ifhd::v400::FileExtension extension_info{};
        std::strncpy(reinterpret_cast<char*>(extension_info.identifier), extension.name.c_str(), sizeof(extension_info.identifier));
        extension_info.user_id = extension.user_id;
        extension_info.type_id = extension.type_id;
        extension_info.version_id = extension.version_id;
        extension_info.data_size = data_buffer.size();
        ifhd::v400::writeExtension(modification_job.file_name, extension_info, data_buffer.data());
    }
}

template<typename CONTAINER>
void check_order(const CONTAINER& container, const std::string& argument, const std::string& required_argument)
{
    if (container.empty())
    {
        throw std::runtime_error(("--" + argument) + " can only be specified after --" + required_argument);
    }
}

template <typename LAMBDA_T>
class MultiBoundLambda: public clara::detail::BoundLambda<LAMBDA_T>
{
    public:
        using clara::detail::BoundLambda<LAMBDA_T>::BoundLambda;
        bool isContainer() const override
        {
            return true;
        }
};

// clara does not support options that can be specified multiple times and that are bound to a lambda.
class MultiLambdaOpt: public clara::Opt
{
    public:
        template<typename LAMBDA_T>
        MultiLambdaOpt(LAMBDA_T const &ref, std::string const &hint): clara::Opt(ref, hint)
        {
            m_ref = std::make_shared<MultiBoundLambda<LAMBDA_T>>(ref);
        }
};

int main(int argc, char** argv) try
{
    bool show_usage = false;
    bool show_progress = false;
    bool skip_stream_types_and_triggers = false;
    std::vector<std::string> plugins;
    std::vector<std::string> list_stream_sources;
    std::string extension_name;
    std::string file_name;
    std::vector<ExportJob> export_jobs;
    std::vector<CreateJob> create_jobs;
    std::vector<ModificationJob> modification_jobs;

    enum class Target
    {
        input,
        stream,
        extension,
    };
    Target last_target = Target::input;

    enum class OperationMode
    {
        exporting,
        importing,
        modifying
    };
    OperationMode operation_mode = OperationMode::exporting;

    auto command_line_parser = 
        clara::Help(show_usage)|
        clara::Opt(show_progress)["--progress"]("Show progress.")|
        clara::Opt(skip_stream_types_and_triggers)["--skipstreamtypesandtriggers"]("Do not process stream types and triggers.")|
        clara::Opt(plugins, "plugin")["--plugin"]("Load an additional plugin.")|
        clara::Opt(list_stream_sources, "file name")["--liststreams"]("List all available information about the given file.")|

        MultiLambdaOpt([&](std::string file_name)
        {
            export_jobs.push_back({file_name});
            operation_mode = OperationMode::exporting;
        },
        "file name")["--export"]("Export streams from the given file.")|

        MultiLambdaOpt([&](std::string file_name)
        {
            create_jobs.push_back({file_name, "adtf3ns"});
            operation_mode = OperationMode::importing;
        },
        "file name")["--create"]("Create a new dat file.")|

        MultiLambdaOpt([&](std::string file_name)
        {
            modification_jobs.push_back({file_name});
            operation_mode = OperationMode::modifying;
        },
        "file name")["--modify"]("Modify an existing dat file.")|

        MultiLambdaOpt([&](std::string file_version)
        {
            check_order(create_jobs, "fileversion", "create");
            create_jobs.back().file_version = file_version;
        },
        "adtf2|adtf3|adtf3ns")["--fileversion"]("File Version of the created file. Creating "
                                                "ADTF 2 files is completely experimental!")|

        MultiLambdaOpt([&](std::string source)
        {
            switch (operation_mode)
            {
                case OperationMode::importing:
                {
                    check_order(create_jobs, "input", "create");
                    create_jobs.back().inputs.push_back({source});
                    break;
                }
                case OperationMode::modifying:
                {
                    check_order(modification_jobs, "input", "extension");
                    check_order(modification_jobs.back().extensions, "input", "extension");
                    modification_jobs.back().extensions.back().input_file = source;
                    break;
                }
                default: break;
            }

            last_target = Target::input;
        },
        "source")["--input"] ("Specifies an input for the new dat file or extension. "
                              "In case of extensions data will be read from stdin if this is not specified.")|

        MultiLambdaOpt([&](std::string reader_id)
        {
            check_order(create_jobs, "readerid", "input");
            check_order(create_jobs.back().inputs, "readerid", "input");
            create_jobs.back().inputs.back().reader_id = reader_id;
        },
        "reader id")["--readerid"]("The id of the reader implementation that should be used to open the last input source.")|

        MultiLambdaOpt([&](uint64_t start)
        {
            check_order(create_jobs, "start", "input");
            check_order(create_jobs.back().inputs, "start", "input");
            create_jobs.back().inputs.back().start = std::chrono::microseconds(start);
        },
        "timestamp")["--start"]("Process only chunks of the last input source with timestamps larger than this (microseconds).")|

        MultiLambdaOpt([&](uint64_t end)
        {
            check_order(create_jobs, "end", "input");
            check_order(create_jobs.back().inputs, "end", "input");
            create_jobs.back().inputs.back().end = std::chrono::microseconds(end);
        },
        "timestamp")["--end"]("Process only chunks of the last input source with timestamps smaller than this (microseconds).")|

        MultiLambdaOpt([&](int64_t offset)
        {
            check_order(create_jobs, "offset", "input");
            check_order(create_jobs.back().inputs, "offset", "input");
            create_jobs.back().inputs.back().offset = std::chrono::microseconds(offset);
        },
        "timestamp")["--offset"]("This offset is added to all chunk timestamps of the last input source (microseconds).")|

        MultiLambdaOpt([&](uint64_t start)
        {
            check_order(create_jobs, "start", "input");
            check_order(create_jobs.back().inputs, "start", "input");
            create_jobs.back().inputs.back().start = std::chrono::nanoseconds(start);
        },
        "timestamp")["--start-ns"]("Process only chunks of the last input source with timestamps larger than this (nanoseconds).")|

        MultiLambdaOpt([&](uint64_t end)
        {
            check_order(create_jobs, "end", "input");
            check_order(create_jobs.back().inputs, "end", "input");
            create_jobs.back().inputs.back().end = std::chrono::nanoseconds(end);
        },
        "timestamp")["--end-ns"]("Process only chunks of the last input source with timestamps smaller than this (nanoseconds).")|

        MultiLambdaOpt([&](int64_t offset)
        {
            check_order(create_jobs, "offset", "input");
            check_order(create_jobs.back().inputs, "offset", "input");
            create_jobs.back().inputs.back().offset = std::chrono::nanoseconds(offset);
        },
        "timestamp")["--offset-ns"]("This offset is added to all chunk timestamps of the last input source (nanoseconds).")|

        MultiLambdaOpt([&](std::string stream_name)
        {
            switch (operation_mode)
            {
                case OperationMode::exporting:
                {
                    check_order(export_jobs, "stream", "export");
                    export_jobs.back().streams.push_back({stream_name});
                    break;
                }
                case OperationMode::importing:
                {
                    check_order(create_jobs, "stream", "input");
                    check_order(create_jobs.back().inputs, "stream", "input");
                    create_jobs.back().inputs.back().streams.push_back({stream_name, stream_name});
                    break;
                }
                default:
                {
                    throw std::runtime_error("--stream can only be specified after --export or --input");
                }
            }

            last_target = Target::stream;
        },
        "stream name")["--stream"]("Select a stream for export or when creating a new dat file.")|

        MultiLambdaOpt([&](std::string stream_name)
        {
            check_order(create_jobs, "name", "stream");
            check_order(create_jobs.back().inputs, "name", "stream");
            check_order(create_jobs.back().inputs.back().streams, "name", "stream");
            create_jobs.back().inputs.back().streams.back().output_name = stream_name;
        },
        "stream name")["--name"]("Sets the name for the last specified stream that is used in the newly created dat file.")|

        MultiLambdaOpt([&](std::string processor_id)
        {
            check_order(export_jobs, "processorid", "stream");
            check_order(export_jobs.back().streams, "processorid", "stream");
            export_jobs.back().streams.back().processor_id = processor_id;
        },
        "processor id")["--processorid"]("The id of the processor implementation used for the last specified stream.") |

        MultiLambdaOpt([&](std::string name_and_value)
        {
            auto equals_position = name_and_value.find('=');
            auto name = name_and_value.substr(0, equals_position);
            auto value = name_and_value.substr(equals_position + 1);
            switch (last_target)
            {
                case Target::stream:
                {
                    check_order(export_jobs, "property", "stream");
                    check_order(export_jobs.back().streams, "property", "stream");
                    export_jobs.back().streams.back().configuration[name] = {value, "string"};
                    break;
                }
                case Target::input:
                {
                    check_order(create_jobs, "property", "input");
                    check_order(create_jobs.back().inputs, "property", "input");
                    create_jobs.back().inputs.back().configuration[name] = {value, "string"};
                    break;
                }
                default: break;
            }
        },
        "name=value")["--property"]("Sets a property of the last specified stream or input.")|

        MultiLambdaOpt([&](std::string extension_name)
        {
            switch (operation_mode)
            {
                case OperationMode::exporting:
                {
                    check_order(export_jobs, "extension", "export");
                    export_jobs.back().extensions.push_back({extension_name, ""});
                    break;
                }
                case OperationMode::importing:
                {
                    check_order(create_jobs, "extension", "input");
                    check_order(create_jobs.back().inputs, "extension", "input");
                    create_jobs.back().inputs.back().extensions.push_back({extension_name});
                    break;
                }
                case OperationMode::modifying:
                {
                    check_order(modification_jobs, "extension", "modify");
                    modification_jobs.back().extensions.push_back({extension_name});
                    break;
                }
            }
            last_target = Target::extension;
        },
        "extension name")["--extension"]("Select an extension for export, when creating a new dat file or updating one.")|

        MultiLambdaOpt([&](std::string file_name)
        {
            switch (last_target)
            {
                case Target::stream:
                {
                    check_order(export_jobs, "output", "stream");
                    check_order(export_jobs.back().streams, "output", "stream");
                    export_jobs.back().streams.back().output_file_name = file_name;
                    break;
                }
                case Target::extension:
                {
                    check_order(export_jobs, "output", "extension");
                    check_order(export_jobs.back().extensions, "output", "extension");
                    export_jobs.back().extensions.back().output_file_name = file_name;
                    break;
                }
                default: break;
            }
        },
        "file name")["--output"]("Sets the output file name for the last specified stream or extension. If not specified, "
                                 "the processor is free to choose one and in case of extensions data will be written to stdout")|

        MultiLambdaOpt([&](std::string serializer_id)
        {
            check_order(create_jobs, "serializerid", "stream");
            check_order(create_jobs.back().inputs, "serializerid", "stream");
            check_order(create_jobs.back().inputs.back().streams, "serializerid", "stream");
            create_jobs.back().inputs.back().streams.back().serializer_id = serializer_id;
        },
        "serializer id")["--serializerid"]("The id of the serializer implementation used for the last specified stream.")|

        MultiLambdaOpt([&](uint32_t user_id)
        {
            check_order(modification_jobs, "userid", "extension");
            check_order(modification_jobs.back().extensions, "userid", "extension");
            modification_jobs.back().extensions.back().user_id = user_id;
        },
        "user id")["--userid"]("Sets the user id of the last extension to be updated.")|

        MultiLambdaOpt([&](uint32_t type_id)
        {
            check_order(modification_jobs, "typeid", "extension");
            check_order(modification_jobs.back().extensions, "typeid", "extension");
            modification_jobs.back().extensions.back().type_id = type_id;
        },
        "type id")["--typeid"]("Sets the type id of the last extension to be updated.")|

        MultiLambdaOpt([&](uint32_t version_id)
        {
            check_order(modification_jobs, "versionid", "extension");
            check_order(modification_jobs.back().extensions, "versionid", "extension");
            modification_jobs.back().extensions.back().version_id = version_id;
        },
        "version id")["--versionid"]("Sets the version id of the last extension to be updated.");


    auto result = command_line_parser.parse(clara::Args(argc, argv));
    if (!result)
    {
       throw std::runtime_error("Error parsing command line arguments: " +
                                result.errorMessage());
    }

    if (show_usage)
    {
        command_line_parser.writeToStream(std::cout);
        std::cout << "\n" << long_help;
        return 0;
    }

    for (auto& plugin_file_name : plugins)
    {
        adtf_file::loadPlugin(plugin_file_name);
    }

    std::function<bool(double)> progress_handler;
    if (show_progress)
    {
        progress_handler = ProgressDisplay();
    }

    auto processor_factories =
        getAdtfDatFactories<adtf::dat::ProcessorFactories, adtf::dat::ProcessorFactory>();
    auto reader_factories =
        getAdtfDatFactories<adtf::dat::ReaderFactories, adtf::dat::ReaderFactory>();
    auto sample_serializer_factories =
        adtf_file::getFactories<adtf_file::SampleSerializerFactories,
                                 adtf_file::SampleSerializerFactory>();

    for (const auto& source: list_stream_sources)
    {
        listSourceStreams(source, reader_factories, processor_factories);
    }

    for (const auto& export_job: export_jobs)
    {
        processExportJob(export_job,
                         progress_handler,
                         processor_factories);
    }

    for (auto& create_job : create_jobs)
    {
        processCreateJob(create_job,
                         progress_handler,
                         skip_stream_types_and_triggers,
                         reader_factories,
                         sample_serializer_factories);
    }

    for (const auto& modification_job : modification_jobs)
    {
        processModificationJob(modification_job);
    }

    return 0;
}
catch (const std::exception& error)
{
    printException(error);
    return 1;
}
