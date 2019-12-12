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
#include <adtf_file/adtf2/adtf2_indexed_data_media_sample_deserializer.h>


namespace adtf_file
{

namespace adtf2
{

void IndexedDataMediaSampleDeserializer::setStreamType(const StreamType& type)
{
}

void IndexedDataMediaSampleDeserializer::deserialize(ReadSample& sample, InputStream& stream)
{
    uint8_t version;
    uint32_t buffer_size = 0;
    uint64_t time_stamp;
    uint32_t flags;
    stream >> version >> buffer_size >> time_stamp >> flags;

    sample.setTimeStamp(std::chrono::microseconds(time_stamp));
    sample.setFlags(flags);

    if (buffer_size)
    {
        uint32_t nDataId;
        stream >> nDataId;
        sample.setSubStreamId(nDataId);

        buffer_size -= sizeof(nDataId);

        auto buffer = sample.beginBufferWrite(buffer_size);
        stream.read(buffer, buffer_size);
        sample.endBufferWrite();
    }
}

}
}
