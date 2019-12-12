/**
 * @file
 *  Creates a (adtf)dat file with a CAN stream
 *
 * Demonstrates:
 * - creating and releasing a adtf_file::Writer
 * - opening and closing a (adtf)dat file
 *   - creating a stream with non-basic sample type
 *   - writing data to this stream
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

#include <adtf_file/adtf_file_writer.h>
#include <adtf_file/standard_factories.h>
#include <adtf_file/default_sample.h>

#include <stdio.h>
#include <iostream>
#include <cstring>

#include "can_stream_type_definitions.h"

using namespace adtf_file;

std::shared_ptr<Writer> create_writer_for_adtf2(a_util::filesystem::Path& filename,
                                                size_t& stream_id)
{
    //create a writer to handle .dat files from ADTF 2.x
    std::shared_ptr<Writer> writer(new Writer(filename,
                                              std::chrono::microseconds(0), 
                                              adtf2::StandardTypeSerializers(), 
                                              Writer::adtf2));

    //define CAN stream type for ADTF 2.x 
    DefaultStreamType can_stream_type(ADTF2_STREAM_META_TYPE);
    can_stream_type.setProperty("major", "tUInt32", a_util::strings::format("%ul", ADTF2_MEDIA_TYPE_CAN));
    can_stream_type.setProperty("sub", "tUInt32", a_util::strings::format("%ul", ADTF2_MEDIA_SUBTYPE_CAN_DATA));

    //create stream for can messages
    //by default the copy serializer is adequate enough
    stream_id = writer->createStream("rawcan", can_stream_type, std::make_shared<adtf2::AdtfCoreMediaSampleSerializer>());
    
    return writer;
}

std::shared_ptr<Writer> create_writer_for_adtf3(a_util::filesystem::Path& filename,
                                                size_t& stream_id)
{
    //create a writer to handle .adtfdat files from ADTF 3.x
    std::shared_ptr<Writer> writer(new Writer(filename,
                                              std::chrono::microseconds(0),
                                              adtf3::StandardTypeSerializers(),
                                              Writer::adtf3));

    // define CAN stream type for ADTF 3.x 
    DefaultStreamType can_stream_type(ADTF3_STREAM_META_TYPE);
    can_stream_type.setProperty("md_struct", "cString", ADTF3_MEDIA_DESC_CANDATA_NAME);
    can_stream_type.setProperty("md_definitions", "cString", ADTF3_MEDIA_DESC_CANDATA);

    //create stream for can messages
    //by default the copy serializer is adequate enough
    stream_id = writer->createStream("rawcan", can_stream_type, std::make_shared<adtf3::SampleCopySerializer>());
    
    return writer;
}


int main(int argc, char* argv[])
{
    try
    {
        size_t stream_id;
        std::shared_ptr<Writer> writer;
        a_util::filesystem::Path filename(argv[1]);
        bool create_trigger = false;

        //setup writer
        if (argc > 1)
        {
            if (filename.getExtension() == "dat")
            {
                writer = create_writer_for_adtf2(filename, stream_id);
            }
            else if (filename.getExtension() == "adtfdat")
            {
                writer = create_writer_for_adtf3(filename, stream_id);
                create_trigger = true;
            }
            else
            {
                throw std::invalid_argument("usage: canwriter <adtfdat|dat>");
            }
        }
        else
        {
            throw std::invalid_argument("usage: canwriter <adtfdat|dat>");
        }

        //short/long description is separated by \n
        writer->setFileDescription("can test \nfile with a can_data stream");

        //fill CAN data
        tCANData message_data = {};
        {
            message_data.sHeader.ui8Tag = tCANData::MT_Data;
            message_data.sHeader.ui8Channel = 03;
            for (uint8_t data = 0; data < 8; ++data)
            {
                message_data.sData.aui8Data[data] = data;
            }
            message_data.sData.ui8Length = 8;
        }

        //write to stream
        for (uint32_t counter = 0; counter < 99;  ++counter)
        {
            message_data.sData.ui32Id = counter;
            DefaultSample sample;
            sample.setTimeStamp(std::chrono::microseconds(counter * 10000));
            sample.setContent(message_data);

            //write the sample to the file
            writer->write(stream_id, std::chrono::microseconds(counter * 10000), sample);
            
            //if you want a trigger within this stream you need to set it !! 
            //this will result in a data trigger within adtf 
            //otherwise the player will NOT create a Data Trigger
            if (create_trigger)
            {
                writer->writeTrigger(stream_id, std::chrono::microseconds(counter * 10000));
            }
        }
        std::cout << "File written to: " << filename.toString().c_str() << std::endl; 
     }
     catch (const std::exception& ex)
     {
         std::cout << ex.what() << std::endl;
     }

     return 0;
}

