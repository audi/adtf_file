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
#include "adtf3_sample_flags.h"
#include <adtf_file/adtf3/adtf3_sample_copy_deserializer.h>
#include <adtf_file/adtf3/adtf3_sample_info.h>


namespace adtf_file
{

namespace adtf3
{

namespace
{

int64_t copy_deserialize(ReadSample& sample, InputStream& stream)
{
    int64_t time;
    int32_t flags;
    size_t buffer_size = 0;
    stream >> time >> flags >> buffer_size;

    if (buffer_size)
    {
        void* buffer = sample.beginBufferWrite(buffer_size);
        stream.read(buffer, buffer_size);
        sample.endBufferWrite();
    }

    if (flags & InternalSampleFlags::sf_sample_info_present)
    {
        deserializeSampleInfo(sample, stream);
        flags ^= InternalSampleFlags::sf_sample_info_present;
    }

    if (flags & InternalSampleFlags::sf_substream_id_present)
    {
        uint32_t substream_id = 0;
        stream >> substream_id;
        sample.setSubStreamId(substream_id);
        flags ^= InternalSampleFlags::sf_substream_id_present;
    }

    sample.setFlags(flags);
    return time;
}

}

void SampleCopyDeserializer::setStreamType(const StreamType& type)
{
}

void SampleCopyDeserializer::deserialize(ReadSample& sample, InputStream& stream)
{
    sample.setTimeStamp(std::chrono::microseconds(copy_deserialize(sample, stream)));
}

void SampleCopyDeserializerNs::setStreamType(const StreamType& type)
{
}

void SampleCopyDeserializerNs::deserialize(ReadSample& sample, InputStream& stream)
{
    sample.setTimeStamp(std::chrono::nanoseconds(copy_deserialize(sample, stream)));
}

}

}
