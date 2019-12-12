/**
 * @file
 * adtf2 (core) mediasample serializer.
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

#ifndef ADTF_FILE_ADTF2_ADTF_CORE_MEDIA_SAMPLE_SERIALIZER
#define ADTF_FILE_ADTF2_ADTF_CORE_MEDIA_SAMPLE_SERIALIZER

#include <memory>
#include <adtf_file/adtf_file_writer.h>

namespace adtf_file
{

namespace adtf2
{

class AdtfCoreMediaSampleSerializer: public SampleSerializer
{
    public:
        static constexpr const char* id = "adtf.core.media_sample.adtf2_support.serialization.adtf.cid";

    public:
        AdtfCoreMediaSampleSerializer();
        ~AdtfCoreMediaSampleSerializer();

    public:
        std::string getId() const override;
        void setStreamType(const StreamType& stream_type) override;
        void serialize(const WriteSample& sample, OutputStream& stream) override;

    private:
        class Implementation;
        std::unique_ptr<Implementation> _implementation;
};


}
}

#endif
