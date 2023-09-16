// stb_dxt.h - v1.04 - DXT1/DXT5 compressor - public domain
// original by fabian "ryg" giesen - ported to C by stb
// use '#define STB_DXT_IMPLEMENTATION' before including to create the implementation
//
// USAGE:
//   call stb_compress_dxt_block() for every block (you must pad)
//     source should be a 4x4 block of RGBA data in row-major order;
//     A is ignored if you specify alpha=0; you can turn on dithering
//     and "high quality" using mode.
//
// version history:
//   v1.04  - (ryg) default to no rounding bias for lerped colors (as per S3TC/DX10 spec);
//            single color match fix (allow for inexact color interpolation);
//            optimal DXT5 index finder; "high quality" mode that runs multiple refinement steps.
//   v1.03  - (stb) endianness support
//   v1.02  - (stb) fix alpha encoding bug
//   v1.01  - (stb) fix bug converting to RGB that messed up quality, thanks ryg & cbloom
//   v1.00  - (stb) first release

#ifndef STB_INCLUDE_STB_DXT_H
#define STB_INCLUDE_STB_DXT_H

// compression mode (bitflags)
namespace rygDXT
{

enum 
{
STB_DXT_FAST      = 0,
STB_DXT_NORMAL    = 1,
STB_DXT_HIGHQUAL  = 2,   // high quality mode, does two refinement steps instead of 1. ~30-40% slower.
};

void stb_compress_dxt1_block(const unsigned char *src, unsigned char *dest, int mode);
void stb_compress_dxt5_block(const unsigned char *src, unsigned char *dest, int mode);
void CompressImageDXT1( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int mode, int dxt_pitch);
void CompressImageDXT5( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int mode, int dxt_pitch);
void CompressImageDXT3( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int mode, int dxt_pitch);

}

#endif // STB_INCLUDE_STB_DXT_H
