/**
 * @file
 * adtf3 copy deserializer.
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

#ifndef ADTF_FILE_ADTF3_SAMPLE_COPY_DESERIALIZER
#define ADTF_FILE_ADTF3_SAMPLE_COPY_DESERIALIZER

#include <adtf_file/adtf_file_reader.h>
#include <adtf_file/default_sample.h>

namespace adtf_file
{

namespace adtf3
{

class SampleCopyDeserializer: public SampleDeserializer
{
    public:
        static constexpr const char* id = "sample_copy_serialization.serialization.adtf.cid";

    public:
        void setStreamType(const StreamType& type) override;
        void deserialize(ReadSample& sample, InputStream& stream) override;
};

class SampleCopyDeserializerNs: public SampleDeserializer
{
    public:
        static constexpr const char* id = "sample_copy_serialization_ns.serialization.adtf.cid";

    public:
        void setStreamType(const StreamType& type) override;
        void deserialize(ReadSample& sample, InputStream& stream) override;
};


}

}

#endif
