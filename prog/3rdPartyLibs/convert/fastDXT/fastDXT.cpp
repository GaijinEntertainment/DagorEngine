#include <stdio.h>
#include <EASTL/utility.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <math.h>
#include <util/dag_globDef.h>
#include "rygDXT.h"
#include "fastDXT.h"
#include "internalDXT.h"
#ifdef DXT_INTR
#include "emitAlphaIndices.h"
#define GOOFYTC_IMPLEMENTATION 1
#include "goofy_tc.h"
#endif

using namespace fastDXT;

namespace fastDXT
{

void ExtractBlockSmall( const uint8_t *inPtr, int width, int height, uint8_t *colorBlock );


// for DXT5
static __forceinline void compress_dxt5a_block(const uint8_t *inBuf, int width, uint8_t *outData);

// Emit indices for DXT5
static __forceinline void compress_bc4_block(const uint8_t *__restrict inBuf, int row_stride, int pixel_stride, uint8_t * __restrict outData)
{
  alignas(16) uint8_t block[16];
  uint8_t lim[] = { 255, 0 };

  for ( int y = 0, i = 0 ; y < 4 ; ++y, inBuf += row_stride - 4 * pixel_stride )
  {
    for ( int x = 0 ; x < 4 ; ++x, ++i, inBuf += pixel_stride )
    {
      uint8_t p = *inBuf;
      block[i] = p;
      lim[0] = min( lim[0], p );
      lim[1] = max( lim[1], p );
    }
  }

  fastDXT::write_BC4_block( block, lim[0], lim[1], outData );
}

typedef void(*compress_dxt_block)(const uint8_t *inBuf, int width, uint8_t *outData);

template <int blocksize, compress_dxt_block compress>
void CompressImageDXT(const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch)
{
  ALIGN16(uint8_t *outData);

  outData = outBuf;
  int pitch_delta = dxt_pitch - (width + 3) / 4 * blocksize;

  if (width % 4 != 0 || height % 4 != 0)
  {
    for (int j = 0; j < height; j += 4, inBuf += width * 4 * 4, outData += pitch_delta)
    {
      for (int i = 0; i < width; i += 4, outData += blocksize)
      {
        ALIGN16(uint8_t block[64]);
        ExtractBlockSmall(inBuf + i * 4, width - i, height - j, block);
        compress(block, 4, outData);
      }
    }

    return;
  }

  for (int j = 0; j < height; j += 4, inBuf += width * 4 * 4, outData += pitch_delta)
   for (int i = 0; i < width; i += 4, outData += blocksize)
     compress(inBuf + i * 4, width, outData);
}

void CompressImageDXT1( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch)
{
#ifdef GOOFYTC_IMPLEMENTATION
  if (width % 16 != 0 || height % 4 != 0)
#endif
  {
    rygDXT::CompressImageDXT1( inBuf, outBuf, width, height, rygDXT::STB_DXT_NORMAL, dxt_pitch);
    return;
  }

#ifdef GOOFYTC_IMPLEMENTATION
  ALIGN16(uint8_t *outData);

  outData = outBuf;
  const int blocksize = 8;
  int pitch_delta = dxt_pitch - (width + 3) / 4 * blocksize;

  unsigned int blockW = width >> 2;
  unsigned int blockH = height >> 2;

  size_t inputStride = width<<2;
  for (uint32_t y = 0; y < blockH; y++)
  {
    const uint8_t* __restrict encoderPos = inBuf;
    for (uint32_t x = 0; x < blockW; x += 4)
    {
      goofy::goofySimdEncode<goofy::GOOFY_DXT1>(encoderPos, inputStride, outBuf);
      encoderPos += 64; // 16 rgba pixels (4 DXT blocks) = 16 * 4 = 64
      outBuf += 32;     // 4 DXT1 blocks = 8 * 4 = 32
    }
    inBuf += inputStride * 4; // 4 lines
    outBuf += pitch_delta;
  }
#endif
}

void CompressImageDXT5( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch)
{
#ifdef GOOFYTC_IMPLEMENTATION
  if (width % 16 != 0 || height % 4 != 0)
#endif
  {
    rygDXT::CompressImageDXT5( inBuf, outBuf, width, height, rygDXT::STB_DXT_NORMAL, dxt_pitch);
    return;
  }

#ifdef GOOFYTC_IMPLEMENTATION
  ALIGN16(uint8_t *outData);

  outData = outBuf;
  const int blocksize = 16;
  int pitch_delta = dxt_pitch - (width + 3) / 4 * blocksize;

  unsigned int blockW = width >> 2;
  unsigned int blockH = height >> 2;

  size_t inputStride = width<<2;
  for (uint32_t y = 0; y < blockH; y++)
  {
    const uint8_t* __restrict encoderPos = inBuf;
    for (uint32_t x = 0; x < blockW; x += 4)
    {
      goofy::goofySimdEncodeDXT5(encoderPos, inputStride, outBuf);
      encoderPos += 64; // 16 rgba pixels (4 DXT blocks) = 16 * 4 = 64
      outBuf += 64;     // 4 DXT5 blocks = 16 * 4 = 64
    }
    inBuf += inputStride * 4; // 4 lines
    outBuf += pitch_delta;
  }
#endif
}

void CompressImageDXT5Alpha( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch)
{
  CompressImageDXT<16, compress_dxt5a_block>(inBuf, outBuf, width, height, dxt_pitch);
}

template<size_t BLOCK_COUNT>
void CompressImageBC( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch, int row_stride, int pixel_stride)
{
  ALIGN16( uint8_t *outData );
  constexpr int BLOCK_SIZE = 8;

  outData = outBuf;
  if (width % 4 != 0 || height % 4 != 0)
  {
    const int alignedWidthToBlock = (width + 3) & ~3;
    const int strideStep = dxt_pitch - alignedWidthToBlock * 2 * BLOCK_COUNT;
    for ( int j = 0; j < height; j += 4, inBuf += row_stride*4 - width*pixel_stride, outData += strideStep)
      for ( int i = 0; i < width; i += 4, outData += BLOCK_SIZE * BLOCK_COUNT, inBuf += 4*pixel_stride )
      {
        if (height - j >= 4 && width - i >= 4)
        {
          for (int block_idx = 0; block_idx < BLOCK_COUNT; block_idx++)
            compress_bc4_block( inBuf + block_idx, row_stride, pixel_stride, outData + BLOCK_SIZE * block_idx);
        }
        else
        {
          for (int block_idx = 0; block_idx < BLOCK_COUNT; block_idx++)
          {
            ALIGN16( uint8_t block[16] );
            memset(block,0,16);
            for ( int y = 0; y < height - j; ++y)
              for ( int x = 0; x < width - i; ++x )
                block[y*4+x] = inBuf[y*row_stride + x*pixel_stride + block_idx];

            compress_bc4_block(block, 4, 1, outData + BLOCK_SIZE * block_idx);
          }
        }
      }
    return;
  }

  for ( int j = 0; j < height; j += 4, inBuf += row_stride*4 - width*pixel_stride, outData += dxt_pitch - width * 2 * BLOCK_COUNT)
    for ( int i = 0; i < width; i += 4, outData += BLOCK_SIZE * BLOCK_COUNT, inBuf += 4*pixel_stride )
    {
      for (int block_idx = 0; block_idx < BLOCK_COUNT; block_idx++)
        compress_bc4_block( inBuf + block_idx, row_stride, pixel_stride, outData + BLOCK_SIZE * block_idx);
    }
}

void CompressImageBC4( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch, int row_stride, int pixel_stride)
{
  CompressImageBC</*BLOCK_COUNT = */ 1>(inBuf, outBuf, width, height, dxt_pitch, row_stride, pixel_stride);
}

void CompressImageBC5( const uint8_t *inBuf, uint8_t *outBuf, int width, int height, int dxt_pitch, int row_stride, int pixel_stride)
{
  G_ASSERTF_RETURN(pixel_stride >= 2, , "BC5 expects pixel_stride[%d] more than 2, use BC4 for one channel textures", pixel_stride);
  CompressImageBC</*BLOCK_COUNT = */ 2>(inBuf, outBuf, width, height, dxt_pitch, row_stride, pixel_stride);
}

//
// Emit indices for DXT5 alpha/bc4
//

#ifdef DXT_INTR
void write_BC4_block( const uint8_t *__restrict block, const uint8_t minAlpha, const uint8_t maxAlpha, uint8_t *__restrict outData )
{
  emitDXT5_alpha_block(_mm_load_si128((const __m128i*)block),
                       _mm_cvtsi32_si128(minAlpha),
                       _mm_cvtsi32_si128(maxAlpha), (uint32_t* __restrict) outData);
}
#else

void write_BC4_block( const uint8_t *__restrict block, const uint8_t minAlpha, const uint8_t maxAlpha, uint8_t *__restrict outData )
{
//stb code
  uint8_t indices[16];
  int dist = maxAlpha-minAlpha;
  int dist4 = dist*4;
  int dist2 = dist*2;
  int bias = (dist < 8) ? (dist - 1) : (dist/2 + 2);
  bias -= minAlpha * 7;
  unsigned bits = 0,mask=0;
  for (int i=0;i<16;i++)
  {
    int a = block[i]*7 + bias;
    int ind,t;

    // select index. this is a "linear scale" lerp factor between 0 (val=min) and 7 (val=max).
    t = (a >= dist4) ? -1 : 0; ind =  t & 4; a -= dist4 & t;
    t = (a >= dist2) ? -1 : 0; ind += t & 2; a -= dist2 & t;
    ind += (a >= dist);
    // turn linear scale into DXT index (0/1 are extremal pts)
    ind = -ind & 7;
    ind ^= (2 > ind);

    // write index
    indices[i] = ind;
  }
  outData[0] = maxAlpha;
  outData[1] = minAlpha;
  outData[2] = (indices[ 0] >> 0) | (indices[ 1] << 3) | (indices[ 2] << 6);
  outData[3] = (indices[ 2] >> 2) | (indices[ 3] << 1) | (indices[ 4] << 4) | (indices[ 5] << 7);
  outData[4] = (indices[ 5] >> 1) | (indices[ 6] << 2) | (indices[ 7] << 5);
  outData[5] = (indices[ 8] >> 0) | (indices[ 9] << 3) | (indices[10] << 6);
  outData[6] = (indices[10] >> 2) | (indices[11] << 1) | (indices[12] << 4) | (indices[13] << 7);
  outData[7] = (indices[13] >> 1) | (indices[14] << 2) | (indices[15] << 5);
}
#endif

void write_BC4_block_rgba( const uint8_t *__restrict colorBlock, uint8_t minAlpha, uint8_t maxAlpha, uint8_t *__restrict outData )
{
  alignas(16) uint8_t block[16];
  colorBlock+=3;
  for (int i=0;i<16;i++)
    block[i] = colorBlock[i*4];
  write_BC4_block(block, minAlpha, maxAlpha, outData);
}

static __forceinline void compress_dxt5a_block(const uint8_t *inBuf, int width, uint8_t *outData)
{
  ALIGN16( uint8_t exBlock[64] );
  if (intptr_t((const void*)inBuf)&15)
    ExtractBlockU( inBuf, width, exBlock );
  else
    ExtractBlockA( inBuf, width, exBlock );

  alignas(16) uint8_t block[16];
  uint8_t maxAlpha = 0, minAlpha = 255;//use 16 instead of 4 just to avoid stack corruption
  const uint8_t *__restrict colorBlock = exBlock+3;
  for (int i=0;i<16;i++)
  {
    block[i] = colorBlock[i*4];
    maxAlpha = max(block[i], maxAlpha);
    minAlpha = min(block[i], minAlpha);
  }
  write_BC4_block(block, minAlpha, maxAlpha, outData);
}


}//namespace
