/**
 * @file
 * adtf2 mediatype serializer.
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

#ifndef ADTF_FILE_ADTF2_STREAM_TYPE_FACTORY
#define ADTF_FILE_ADTF2_STREAM_TYPE_FACTORY

#include <adtf_file/adtf_file_reader.h>
#include <unordered_map>
#include <vector>
#include <adtf_file/stream_type.h>
namespace adtf_file
{

namespace adtf2
{

class AdtfCoreMediaTypeDeserializer: public StreamTypeDeserializer
{
    public:
        std::string getId() const override;
        void deserialize(InputStream& stream, PropertyStreamType& stream_type) const override;
};

class AdtfCoreMediaTypeVideoDeserializer: public StreamTypeDeserializer
{
    public:
        std::string getId() const override;
        void deserialize(InputStream& stream, PropertyStreamType& stream_type) const override;
};

class AdtfCoreMediaTypeAudioDeserializer: public StreamTypeDeserializer
{
    public:
        std::string getId() const override;
        void deserialize(InputStream& stream, PropertyStreamType& stream_type) const override;
};

class IndexedDataDescriptionMediaTypeDeserializer: public StreamTypeDeserializer
{
    public:
        std::string getId() const override;
        void deserialize(InputStream& stream, PropertyStreamType& stream_type) const override;
};

}

}

#endif
