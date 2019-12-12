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
#include <adtf_file/default_sample.h>

namespace adtf_file
{

void DefaultSample::setTimeStamp(std::chrono::nanoseconds time_stamp)
{
    _time_stamp = time_stamp;
}

void DefaultSample::setSubStreamId(uint32_t substream_id)
{
    _substream_id = substream_id;
}

void DefaultSample::setFlags(uint32_t flags)
{
    _flags = flags;
}

void* DefaultSample::beginBufferWrite(size_t size)
{
    _buffer.resize(size);
    return _buffer.data();
}

void DefaultSample::endBufferWrite()
{
}

void DefaultSample::addInfo(uint32_t key, DataType type, uint64_t raw_bytes)
{
    _info[key] = std::make_pair(type, raw_bytes);
}

std::chrono::nanoseconds DefaultSample::getTimeStamp() const
{
    return _time_stamp;
}

uint32_t DefaultSample::getSubStreamId() const
{
    return _substream_id;
}

uint32_t DefaultSample::getFlags() const
{
    return _flags;
}

std::pair<const void*, size_t> DefaultSample::beginBufferRead() const
{
    return std::make_pair(_buffer.data(), _buffer.size());
}

void DefaultSample::endBufferRead() const
{
}

void DefaultSample::iterateInfo(std::function<void(uint32_t key, DataType type, uint64_t raw_bytes)> functor) const
{
    for (auto& entry: _info)
    {
        functor(entry.first, entry.second.first, entry.second.second);
    }
}

const std::unordered_map<uint32_t, std::pair<DataType, uint64_t>>& DefaultSample::GetInfo() const
{
    return _info;
}

}
