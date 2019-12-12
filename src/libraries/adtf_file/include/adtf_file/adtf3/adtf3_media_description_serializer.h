/**
 * @file
 * adtf media description serializer.
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

#ifndef ADTF_FILE_ADTF3_MEDIA_DESCRIPTION_SERIALIZER
#define ADTF_FILE_ADTF3_MEDIA_DESCRIPTION_SERIALIZER

#include <memory>
#include <adtf_file/adtf_file_writer.h>

namespace adtf_file
{
namespace adtf3
{

class MediaDescriptionSerializer: public SampleSerializer
{
    public:
        static constexpr const char* id = "media_description_sample_serialization.serialization.adtf.cid";

    public:
        MediaDescriptionSerializer();
        ~MediaDescriptionSerializer();

    public:
        std::string getId() const override;
        void setStreamType(const StreamType& stream_type) override;
        void serialize(const WriteSample& sample, OutputStream& stream) override;

    private:
        class Implementation;
        std::unique_ptr<Implementation> _implementation;
};

class MediaDescriptionSerializerNs: public SampleSerializer
{
    public:
        static constexpr const char* id = "media_description_sample_serialization_ns.serialization.adtf.cid";

    public:
        MediaDescriptionSerializerNs();
        ~MediaDescriptionSerializerNs();

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
