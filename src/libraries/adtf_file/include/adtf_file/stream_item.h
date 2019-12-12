/**
 * @file
 * stream item definition.
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

#ifndef ADTF_FILE_STREAM_ITEM
#define ADTF_FILE_STREAM_ITEM

namespace adtf_file
{

class StreamItem
{
    public:
        virtual ~StreamItem() = default;
};

class StreamType: public StreamItem
{
    public:
        virtual ~StreamType() = default;
};

class Sample: public StreamItem
{
    public:
        virtual ~Sample() = default;
};

class Trigger: public StreamItem
{
    public:
        virtual ~Trigger() = default;
};

}

#endif
