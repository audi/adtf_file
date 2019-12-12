/**
 * @file
 *  Definitions for CAN Stream Type
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

#pragma pack(push, 1)

#define ADTF2_STREAM_META_TYPE          "adtf2/legacy"

#define ADTF2_MEDIA_TYPE_CAN            0x0200

#define ADTF2_MEDIA_SUBTYPE_CAN_DATA    0x0003

#define ADTF3_MEDIA_DESC_CANDATA_NAME   "tCANData"

#define ADTF3_STREAM_META_TYPE          "adtf/devicetb/can"

#define ADTF3_MEDIA_DESC_CANDATA_ENUM   "<enums>" \
                                            "<enum name=\"eMessageTag\" type=\"tUInt8\">" \
                                                "<element name=\"MT_Data\" value=\"0\" />" \
                                                "<element name=\"MT_Status\" value=\"1\" />" \
                                            "</enum>" \
                                            "<enum name=\"eDataFlags\" type=\"tUInt16\">" \
                                                "<element name=\"DF_ERROR_FRAME\" value=\"1\" />" \
                                                "<element name=\"DF_NONE\" value=\"0\" />" \
                                                "<element name=\"DF_REMOTE_FRAME\" value=\"2\" />" \
                                                "<element name=\"DF_TX_COMPLETED\" value=\"4\" />" \
                                            "</enum>" \
                                        "</enums>"

#define ADTF3_MEDIA_DESC_CANDATA        "<?xml version=\"1.0\" encoding=\"iso-8859-1\" standalone=\"no\"?>"\
                                        "<adtf:ddl xmlns:adtf=\"adtf\">"\
                                            ADTF3_MEDIA_DESC_CANDATA_ENUM \
                                            "<structs>"\
                                                "<struct name=\"" ADTF3_MEDIA_DESC_CANDATA_NAME "\" alignment=\"1\" version=\"1\">" \
                                                    "<element type=\"eMessageTag\"   name=\"ui8Tag\"       bytepos=\"0\"  arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt8\"   name=\"ui8Channel\"   bytepos=\"1\"  arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tInt64\"   name=\"tmTimeStamp\"  bytepos=\"2\"  arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt32\"  name=\"ui32Id\"       bytepos=\"10\" arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt8\"   name=\"ui8Length\"    bytepos=\"14\" arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt8\"   name=\"ui8Reserved\"  bytepos=\"15\" arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"eDataFlags\"  name=\"ui16Flags\"    bytepos=\"16\" arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt16\"  name=\"ui16Reserved\" bytepos=\"18\" arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt32\"  name=\"ui32Reserved\" bytepos=\"20\" arraysize=\"1\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                    "<element type=\"tUInt8\"   name=\"aui8Data\"     bytepos=\"24\" arraysize=\"8\" byteorder=\"LE\" alignment=\"1\"/>" \
                                                "</struct>"\
                                            "</structs>"\
                                        "</adtf:ddl>"

/**
 *  The CAN data structure is used by CAN MediaSamples
 *  use following code within your filter: 
 */
struct tCANData
{
    /**
     *  This enum specifies the different kinds of messages
     *  that may be contained in the union
     */
    enum eMessageTag
    {
        MT_Data = 0,        //!< Data
        MT_Status = 1,      //!< Status
    };

    /**
     *  CAN message flags
     */
    enum eDataFlags
    {
        DF_NONE = 0,            //!< Standard flags
        DF_ERROR_FRAME = 1,     //!< Indicates an error frame
        DF_REMOTE_FRAME = 2,    //!< Indicates a remote frame
        DF_TX_COMPLETED = 4     //!< Notification for successful message transmission
    };

    /**
     *  CAN bus state flags. 
     */
    enum eBusStatus
    {
        BS_OFFLINE = 1,         //!< Bus is offline
        BS_ERROR_PASSIVE = 2,   //!< One of the error counters has reached the error level.
        BS_ERROR_WARNING = 4,   //!< One of the error counters has reached the warning level.
        BS_ERROR_ACTIVE = 8     //!< Bus is online
    };

    /**
     *  CAN message id masks. These masks should be used to check for extended or standard messages
     *  and to get the correct identifier from ui32Id in tData.
     */
    enum eMsgId
    {
        MSG_IDMASK_BASE  = 0x000007FF,      //!< Message IDs for base frame format use 11 bit identifiers
        MSG_IDMASK_EXTENDED  = 0x1FFFFFFF,  //!< Message IDs for extended frame format use 29 bit identifiers
        MSG_EXTENDED_FLAG = 0x80000000      //!< Extended CAN messages are marked by bit 31
    };

    /**
     *  CAN message header structure
     */
    struct tMessageHeader
    {
        uint8_t      ui8Tag;             //!< Type of contained message (see eMessageTag)
        uint8_t      ui8Channel;         //!< Channel that received this message
        int64_t      tmTimeStamp;        //!< Hardware timestamp in micro seconds 
    };

    /**
     *  CAN message data structure
     */
    struct tData
    {
        uint32_t     ui32Id;             //!< id of can message. For extended CAN messages bit 31 is set. Use the members of the enum eMsgId to get the identifier and check for extended messages.
        uint8_t      ui8Length;          //!< length of data [0..8]
        uint8_t      ui8Reserved;        //!< reserved, should be zero
        uint16_t     ui16Flags;          //!< Flags @see eDataFlags
        uint16_t     ui16Reserved;       //!< reserved, should be zero
        uint32_t     ui32Reserved;       //!< reserved, should be zero
        uint8_t      aui8Data[8];        //!< data field
    };

    /**
     *  CAN bus status structure
     */
    struct tStatus
    {
        uint32_t     ui32BitRate;        //!< CAN bus bitrate
        uint32_t     ui32RxBitCount;     //!< Count of received bits
        uint32_t     ui32TxBitCount;     //!< Count of transmitted bits
        uint16_t     ui16RxErrorCounter; //!< Error counter for the receive section of the CAN controller.
        uint16_t     ui16TxErrorCounter; //!< Error counter for the transmit section of the CAN controller.
        uint8_t      ui8BusStatus;       //!< Flags @see eBusStatus
        uint8_t      ui8Reserved;        //!< reserved, should be zero
        uint32_t     ui32Reserved;       //!< reserved, should be zero
    };

    tMessageHeader  sHeader;            //!< CAN message header structure

    union
    {
        tData   sData;                  //!< used when ui8Tag == MT_Data
        tStatus sStatus;                //!< used when ui8Tag == MT_Status
    };
};                                        
                                        
                                        
                                        
                                        
                                        
                                        
                                        
                                        
                                        
                                        
#pragma pack(pop)