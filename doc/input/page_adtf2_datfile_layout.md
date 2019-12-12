# ADTF File Layout in ADTF 2 Files {#page_adtf2_datfile_layout}

## How the data is serialized to the IndexFile Format by ADTF 

The IndexedFile Format is a common File Format to store data by chunks. 
The complete layout of the file is described in \ref page_a_utils_indexedfileformat.
The IndexedFile Format does not declare how ADTF uses this format. 
This documentation is to describe how ADTFs software modules will store IMediaSample to chunks 
and IMediaType to the Stream Info Additional Data.

Both, the corresponding implementations of the IMediaSample and the IMediaType need to implement 
the interfaces ucom::ISerializable and ucom::IClassInfo.
The Recorder uses ucom::ISerializable::Serialize and ucom::IClassInfo::GetIdentifier to serialize the MediaSample to the Chunk Data, the Player 
uses ucom::ISerializable::Deserialize get them out.

### Following Information are stored within the Stream Info Additional Data

<table>
<tr>
<td>
512Bytes 
<br> tChar[512] 
<br><br> NULL terminated! strObjectIdentifier (Sample OID)
</td>
<td>
512Bytes
<br>tChar[512] 
<br><br> NULL terminated!\nstrTypeIdentifier  (Type OID)
</td>
<td>
serialized media_type data depending on the type 
<br><br> (by calling ISerializable::Serialize in ADTF 2)
</td>
</tr>

</table>

### Layout of the Serialized Data for OID_ADTF_MEDIA_TYPE, OID_ADTF_MEDIA_TYPE_VIDEO

Following Information are stored within the Stream Info Additional Data for "serialized media_type data".

#### Constraints
* Every single value is saved in little endian byteorder !
* Trailing data to the Media Type serialized stream depending on the implementation of ::Serialize implementation.

#### Following Information are stored within the serialized stream for cMediaType (OID_ADTF_MEDIA_TYPE)
This is valid for class with object Identifier @ref OID_ADTF_MEDIA_TYPE.
<table>
<tr>
<td>
ui32MajorType
</td>
<td>
ui32SubType
</td>
<td>
ui32Flags
</td>
</tr>
</table>

For description of values see @ref tMediaTypeInfo in ADTF 2

#### Following Information are stored within the serialized stream for cMediaTypeVideo (OID_ADTF_MEDIA_TYPE_VIDEO)
This is valid for class with object Identifier @ref OID_ADTF_MEDIA_TYPE_VIDEO.

<table>
<tr>
<td>
ui32MajorType
</td>
<td>
ui32SubType
</td>
<td>
ui32Flags
</td>
<td>
i16BitsPerPixel
</td>
<td>
i32BytesPerLine
</td>
<td>
i32Height
</td>
<td>
i32PaletteSize
</td>
<td>
i16PixelFormat
</td>
<td>
i32Size
</td>
<td>
i32Width
</td>
<td>
array of tColor[i32PaletteSize]
</td>
</tr>
</table>

* For description of values see @ref tMediaTypeInfo and @ref tBitmapFormat. 
* If i32PaletteSize is set to 0 the array of tColor[i32PaletteSize] is NOT present.


### Layout of the Serialized Data for OID_ADTF_MEDIA_SAMPLE

This section belongs ONLY to the class cMediaSample (@ref OID_ADTF_MEDIA_SAMPLE)!

#### Constraints
* Every single value is saved in little endian byteorder !
* For the class cMediaSample (@ref OID_ADTF_MEDIA_SAMPLE) the serialization of UserBuffer will be a simple memcpy of the User Buffer 
    to the stream if no MediaDescription is present on Pin! 
* If a MediaDescription is present (MUST be if MediaType set to {0,0} ) the serialization of UserBuffer will be done 
    by the IMediaSerializer::SerializeData (description dependend serialization).

#### Following Information are stored within the Chunk Data

<table>
<tr>
<td>
ui8CurrentVersion
</td>
<td>
ui32BufferSize
</td>
<td>
tmTimeStamp
</td>
<td>
ui32Flags
</td>
<td>
____UserBuffer____
</td>
<td>
ui32SampleInfoCount
</td>
<td>
Sample Info Array 
</td>
<td>
ui8SampleLogTraceLevel
</td>
<td>
Media Sample Log Trace
</td>
</tr>
</table>

The content and length of the Chunkdata depends on the value of ui8CurrentVersion.
It is to determine the serialized version. 
Keep in mind that the serialization of higher versions will only add data to the footer!!
Older versions of ADTF just read to the point where the old content ending is expected.

This description will describe only the serialization of @ref ADTF_MEDIASAMPLE_CLASS_VERSION_4.

#### Table of Values
<table>
<tr>
    <th> Value
    </th>
    <th> Description
    </th>
</tr>
<tr>
    <td> ui8CurrentVersion (tUInt8)
    </td>
    <td> See @ref ADTF_MEDIASAMPLE_CLASS_VERSION <br>
         See @ref ADTF_MEDIASAMPLE_CLASS_VERSION_2 <br>
         See @ref ADTF_MEDIASAMPLE_CLASS_VERSION_3 <br>
         See @ref ADTF_MEDIASAMPLE_CLASS_VERSION_4
    </td>
</tr>
<tr>
    <td> ui32BufferSize (tUInt32)
    </td>
    <td>  Size (in Byte) of UserBuffer depends on the implementation of cMediaSample::SerializeData 
          or a set MediaDescription on Pin (see @ref page_serialization)
    </td>
</tr>
<tr>
    <td> tmTimeStamp (tInt64)
    </td>
    <td> TimeStamp set by IMediaSample::SetTime
    </td>
</tr>
<tr>
    <td> ui32Flags (tUInt32)
    </td>
    <td> Flags set by IMediaSample::SetFlags
    </td>
</tr>
<tr>
    <td> UserBuffer
    </td>
    <td> Depends on the implementation of cMediaSample::SerializeData or a set MediaDescription on Pin (see @ref page_serialization) <br>
         For cMediaSample (OID_ADTF_MEDIA_SAMPLE) and no MediaDescription present it is only memcpy of the buffers content!
    </td>
</tr>
<tr>
    <td> ui32SampleInfoCount (tUInt32)
    </td>
    <td> value count of content of the IMediaSampleInfo 
    </td>
</tr>
<tr>
    <td> Sample Info Array
    </td>
    <td> No description here! set \c ui32SampleInfoCount to 0
    </td>
</tr>
<tr>
    <td> ui8SampleLogTraceLevel (tUInt8)
    </td>
    <td> Activated IMediaSampleLog (0 = deactivated)
    </td>
</tr>
<tr>
    <td> Media Sample Log Trace
    </td>
    <td> No description here! set \c ui8SampleLogTraceLevel to 0
    </td>
</tr>
</table>
