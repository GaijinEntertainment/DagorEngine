#ifndef _FASTDXT_H
#define _FASTDXT_H
#pragma once
#include <stdint.h>
namespace fastDXT
{

void CompressImageDXT1( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch);
// Compress to DXT5 format
void CompressImageDXT5( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch);

// Compress to DXT5 format, only alpha (not touching color)
void CompressImageDXT5Alpha( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch);

// Compress to DXT3 format
void CompressImageDXT3( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch);

// Compress to ATI1N format
void CompressImageBC4( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch, int row_stride, int pixel_stride);

// Compress to ATI2N format
void CompressImageBC5( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch, int row_stride, int pixel_stride);

//
// Emit indices for DXT5 alpha/bc4 block, if alpha[0]>alpha[1]
//
void write_BC4_block( const uint8_t *__restrict block, const uint8_t minAlpha, const uint8_t maxAlpha, uint8_t *__restrict outData );//source is 16 alpha
void write_BC4_block_rgba( const uint8_t *__restrict block, uint8_t minAlpha, uint8_t maxAlpha, uint8_t *__restrict outData );//source is 16 colors

}

#endif