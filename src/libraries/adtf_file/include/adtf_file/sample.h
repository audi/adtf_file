/**
 * @file
 * sample impl.
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

#ifndef ADTF_FILE_SAMPLE
#define ADTF_FILE_SAMPLE

#include <a_util/variant.h>
#include <functional>
#include <string>
#include <chrono>

namespace adtf_file
{

enum class DataType
{
    uint8,
    int8,
    uint16,
    int16,
    uint32,
    int32,
    uint64,
    int64,
    float32,
    float64
};

enum SampleInfoKeys: uint32_t
{
    sai_device_original_time = 1851393682,
    sai_counter = 1660111993
};

uint32_t createSampleInfoHashKey(const std::string& name);
uint32_t createAdtf2SampleInfoHashKey(uint32_t info_index);

class ReadSample
{
    public:
        virtual void setTimeStamp(std::chrono::nanoseconds time_stamp) = 0;
        virtual void setFlags(uint32_t flags) = 0;
        virtual void setSubStreamId(uint32_t substream_id) = 0;
        virtual void* beginBufferWrite(size_t size) = 0;
        virtual void endBufferWrite() = 0;
        virtual void addInfo(uint32_t key, DataType type, uint64_t raw_bytes) = 0;
};

class WriteSample
{
    public:
        virtual std::chrono::nanoseconds getTimeStamp() const = 0;
        virtual uint32_t getFlags() const = 0;
        virtual uint32_t getSubStreamId() const = 0;
        virtual std::pair<const void*, size_t> beginBufferRead() const = 0;
        virtual void endBufferRead() const = 0;
        virtual void iterateInfo(std::function<void(uint32_t key, DataType type, uint64_t raw_bytes)> functor) const = 0;

};

class ReadRawSampleInfo
{
    public:
        virtual void setRawSampleInfo(const void* data, size_t data_size, uint8_t layout_version) = 0;
};

class WriteRawSampleInfo
{
    public:
        virtual void getRawSampleInfo(std::function<void(const void*, size_t, uint8_t)> handler) const = 0;
};

}

#endif
