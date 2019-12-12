/**
 * @file
 * Reader.
 *
 * @copyright
 * @verbatim
   Copyright @ 2017 Audi Electronics Venture GmbH. All rights reserved.

       This Source Code Form is subject to the terms of the Mozilla
       Public License, v. 2.0. If a copy of the MPL was not distributed
       with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

   If it is not possible or desirable to put the notice in a particular file, then
   You may include the notice in a location (such as a LICENSE file in a
   relevant directory) where a recipient would be likely to look for such a notice.

   You may add additional accurate notices of copyright ownership.
   @endverbatim
 */

#ifndef ADTF_FILE_ADTF2_ADTF_CORE_MEDIA_SAMPLE_FACTORY
#define ADTF_FILE_ADTF2_ADTF_CORE_MEDIA_SAMPLE_FACTORY

#include <memory>
#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/default_sample.h>

namespace adtf_file
{

namespace adtf2
{

class AdtfCoreMediaSampleDeserializer: public SampleDeserializer
{
    public:
        static constexpr const char* id = "adtf.core.media_sample.adtf2_support.serialization.adtf.cid";

    public:
        AdtfCoreMediaSampleDeserializer();
        ~AdtfCoreMediaSampleDeserializer();

    public:
        void setStreamType(const StreamType& type) override;
        void deserialize(ReadSample& sample, InputStream& stream) override;

    private:
        void deserializeData(ReadSample& sample, InputStream& stream, size_t buffer_size);

    private:
        class Implementation;
        std::unique_ptr<Implementation> _implementation;
};

}

}

#endif
