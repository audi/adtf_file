/**
 * @file
 * legacy types for adtf 2 file support.
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

#ifndef ADTF_FILE_ADTF2_LEGACY_TYPES
#define ADTF_FILE_ADTF2_LEGACY_TYPES

namespace adtf_file
{
namespace adtf2
{

struct BitmapFormat
{
    /// Specifies the width (in pixels) of the image
    int32_t width;
    /// Specifies the width (in pixels) of the image
    int32_t height;
    /// Specifies the number of bits used to represent the color of a single pixel
    int16_t bits_per_pixel;
    /// Pixel format specified in tPixelFormat. More information ca be found at @see cImage::tPixelFormat
    int16_t  pixel_format;
    /// Specifies the number of bytes used per line (width * bitsPerPixel / 8 + n PaddingBytes)
    /// normally the bytesPerLine is a multiple of 4, but don't trust on it!
    int32_t bytes_per_line;
    /// Size of bitmap in bytes (bytesPerLine * height)
    int32_t size;
    /// Size of colour palette. This can be used for image formats where the pixel values are indices of a color table.
    /// Most of the time you want to set this to 0!
    int32_t palette_size;
};

struct PaletteEntry
{
    /// The Blue value
    uint8_t blue;
    /// The Green value
    uint8_t green;
    /// The Red value
    uint8_t red;
    /// The Alpha value
    uint8_t alpha;
};

enum PixelFormats
{
    /// unknown / not set
    pf_unknown = 0,

    /// 8 bit
    pf_8_bit = 10,
    /// 8 bit greyscale - default format for 8 bpp
    pf_greyscale_8 = 11,
    /// 8 bit RGB (R: 3 bit, G: 3 bit, B: 2 Bit) - palletized 8 bpp format
    pf_rgb_8 = 12,

    /// 16 bit
    ///This definition is a placeholder a 16 Bit format, bitorder is undefined.
    pf_16_bit = 20,
    /// 10 bit greyscale
    /// To setup Format use tBitmapFormat::bitsPerPixel = 16 !
    /// the most significant (upper) bits are valid (11111111 11000000)
    /// A_UTILS convert just 8bit greyscales to display this format
    pf_greyscale_10 = 21,
    /// 12 bit greyscale
    /// To setup Format use tBitmapFormat::bitsPerPixel = 16 !
    /// the most significant (upper) bits are valid (11111111 11110000)
    /// A_UTILS convert just 8bit greyscales to display this format
    pf_greyscale_12 = 22,
    /// 14 bit greyscale
    /// To setup Format use tBitmapFormat::bitsPerPixel = 16 !
    /// the most significant (upper) bits are valid (11111111 11111100)
    /// A_UTILS convert just 8bit greyscales to display this format
    pf_greyscale_14 = 23,
    /// 16 bit greyscale
    pf_greyscale_16 = 24,
    /// 12 bit RGB (R: 4 bit, G: 4 bit, B: 4 bit)
    pf_rgb_444 = 25,
    /// 15 bit RGB (R: 5 bit, G: 5 bit, B: 5 bit) - default format for 16 bpp
    pf_rgb_555 = 26,
    /// 16 bit RGB (R: 5 bit, G: 6 bit, B: 5 bit)
    pf_rgb_565 = 27,
    /// 16 bit RGBA (R: 4 bit, G: 4 bit, B: 4 bit, A: 4 bit) with alpha value
    pf_rgba_4444 = 28,
    /// 16 bit ABGR (A: 4 bit, B: 4 bit, G: 4 bit, R: 4 bit) with alpha value - inverted RGBA
    pf_abgr_4444 = 29,
    pf_riii_10 = 30,
    pf_riii_12 = 31,
    pf_riii_14 = 32,
    pf_riii_16 = 33,
    pf_bgr_555 = 34,
    pf_bgr_565 = 35,

    /// 24 bit
    ///This definition is a placeholder a 24 Bit format, bitorder is undefined.
    pf_24_bit = 40,
    pf_greyscale_18 = 41,
    pf_greyscale_20 = 42,
    pf_greyscale_22 = 43,
    /// 24 bit greyscale
    pf_greyscale_24 = 44,
    /// 24 bit RGB (R: 8 bit, G: 8 bit, B: 8 bit) - default format for 24 bpp
    pf_rgb_888 = 45,
    /// 24 bit BGR (B: 8 bit, G: 8 bit, R: 8 bit) - inverted RGB
    pf_bgr_888 = 46,

    /// 32 bit
    ///This definition is a placeholder a 32 Bit format, bitorder is undefined.
    pf_32_bit = 50,
    /// 32 bit ARGB (A: 8 bit, R: 8 bit, G: 8 bit, B: 8 bit) with alpha value
    pf_argb_8888 = 51,
    /// 32 bit ABGR (A: 8 bit, B: 8 bit, G: 8 bit, R: 8 bit) with alpha value - inverted RGBA
    pf_abgr_8888 = 52,
    /// 32 bit RGBA (R: 8 bit, G: 8 bit, B: 8 bit, A: 8 bit) with alpha value - default format for 32 bpp
    pf_rgba_8888 = 53,
    /// 32 bit BGRA (B: 8 bit, G: 8 bit, R: 8 bit, A: 8 bit) with alpha value - inverted RGB + A
    pf_bgra_8888 = 54,
    /// 32 bit greyscale
    pf_greyscale_32 = 55,
    /// 32 bit greyscale float value
    pf_greyscale_float_32 = 56,

    /// 24 bit YUV (Y: 8 bit, U: 8 bit, V: 8 bit) - Y = luminance, U+V = chrominance
    /// If using this format in the VideoDisplay a conversion to RBG will be done.
    /// This is not very performant!
    pf_yuv420p_888 = 60
};

struct AudioFormat
{
    /// Format type
    int32_t format_type;
    /// Number of channels (1=mono, 2=stereo)
    int32_t channels;
    /// Samples per second
    int32_t samples_per_sec;
    /// Bits per sample
    int32_t bits_per_sample;
    /// Number of samples
    int32_t num_samples;
    /// Size of samples
    int32_t size;
};

}
}

#endif
