/**
 * @file
 * default sample.
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

#ifndef ADTF_FILE_DEFAULT_SAMPLE
#define ADTF_FILE_DEFAULT_SAMPLE

#include "stream_item.h"
#include "sample.h"

#include <chrono>
#include <cstring>
#include <functional>
#include <unordered_map>

namespace adtf_file
{

class DefaultSample: public Sample, public ReadSample, public WriteSample
{
    public:
        void setTimeStamp(std::chrono::nanoseconds time_stamp) override;
        void setSubStreamId(uint32_t substream_id) override;
        void setFlags(uint32_t flags) override;
        void* beginBufferWrite(size_t size) override;
        void endBufferWrite() override;
        void addInfo(uint32_t key, DataType type, uint64_t raw_bytes) override;

    public:
        std::chrono::nanoseconds getTimeStamp() const override;
        uint32_t getSubStreamId() const override;
        uint32_t getFlags() const override;
        std::pair<const void*, size_t> beginBufferRead() const override;
        void endBufferRead() const override;
        void iterateInfo(std::function<void(uint32_t key, DataType type, uint64_t raw_bytes)> functor) const override;

    public:
        const std::unordered_map<uint32_t, std::pair<DataType, uint64_t>>& GetInfo() const;

    public:
        template <typename T>
        void setContent(const T& value)
        {
            _buffer.resize(sizeof(value));
            memcpy(_buffer.data(), &value, sizeof(value));
        }

        template <typename T>
        const T& getContent() const
        {
            if (sizeof(T) > _buffer.size())
            {
                throw std::runtime_error("invalid sample size for requested type");
            }
            return *reinterpret_cast<const T*>(_buffer.data());
        }

    private:
        std::chrono::nanoseconds _time_stamp = std::chrono::nanoseconds(0);
        uint32_t _substream_id = 0;
        uint32_t _flags = 0;
        std::vector<uint8_t> _buffer;
        std::unordered_map<uint32_t, std::pair<DataType, uint64_t>> _info;
};

}

#endif
