//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
struct TexPixel32;

enum
{
  /// Quick color endpoints detection
  DXT_ALGORITHM_PRECISE = 0,

  /// fastest possible compression (realtime)
  DXT_ALGORITHM_QUICK = 1,
  /// use the same algorithm as in production, if possible
  DXT_ALGORITHM_PRODUCTION = 3,
  /// use the same algorithm as in production, on a highest quiality (very slow)
  DXT_ALGORITHM_EXCELLENT = 4,
};

enum
{
  MODE_DXT1 = 0,
  MODE_DXT3,
  MODE_DXT5,
  MODE_DXT5Alpha, // compress only dxt alpha, leave color untouched
  MODE_BC4,       // ATI1N
  MODE_BC5,       // ATI2N
};

/// Compresses image into provided pointer.
void ManualDXT(int mode, const TexPixel32 *pImage, int width, int height, int dxt_pitch, char *pCompressed,
  int algorithm = DXT_ALGORITHM_QUICK);

/// Compresses image.
/// @return pointer to compressed image date. Call memfree(ptr, tmpmem) to free.
void *CompressDXT(int mode, TexPixel32 *image, int stride_bytes, int width, int height, int levels, int *len,
  int algorithm = DXT_ALGORITHM_PRECISE, int zlib_lev = 0);

void CompressBC4(const unsigned char *image, int width, int height, int dxt_pitch, char *pCompressed, int row_stride,
  int pixel_stride);
void CompressBC5(unsigned char *image, int width, int height, int dxt_pitch, char *pCompressed, int pixel_stride, int pixel_offset);

void decompress_dxt(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data, bool is_dxt1);
