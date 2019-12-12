/**
 * @file
 * adtf copy serializer.
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
#include <adtf_file/adtf3/adtf3_sample_copy_serializer.h>
#include <adtf_file/adtf3/adtf3_sample_info.h>
#include <cstring>

namespace adtf_file
{

namespace adtf3
{

namespace
{

void copy_serialize(const WriteSample& sample,
                    OutputStream& stream,
                    int64_t time_stamp,
                    bool sub_streams)
{
    bool has_info = hasSampleInfo(sample);

    auto flags = sample.getFlags() & ~InternalSampleFlags::sf_sample_info_present;
    if (has_info)
    {
        flags |= InternalSampleFlags::sf_sample_info_present;
    }

    auto substream_id = sub_streams ? sample.getSubStreamId(): 0;
    if (substream_id > 0)
    {
        flags |= InternalSampleFlags::sf_substream_id_present;
    }

    stream << time_stamp
           << flags;

    auto buffer = sample.beginBufferRead();
    stream << buffer.second;
    stream.write(buffer.first, buffer.second);
    sample.endBufferRead();

    if (has_info)
    {
        serializeSampleInfo(sample, stream);
    }

    if (sub_streams &&
        substream_id > 0)
    {
        stream << substream_id;
    }
}

}

std::string SampleCopySerializer::getId() const
{
    return id;
}

void SampleCopySerializer::setStreamType(const StreamType& stream_type)
{
}

void SampleCopySerializer::serialize(const WriteSample& sample, OutputStream& stream)
{
    copy_serialize(sample,
                   stream,
                   static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(sample.getTimeStamp()).count()),
                   false);
}

std::string SampleCopySerializerNs::getId() const
{
    return id;
}

void SampleCopySerializerNs::setStreamType(const StreamType& stream_type)
{
}

void SampleCopySerializerNs::serialize(const WriteSample& sample, OutputStream& stream)
{
    copy_serialize(sample, stream, static_cast<int64_t>(sample.getTimeStamp().count()), true);
}

}
}
