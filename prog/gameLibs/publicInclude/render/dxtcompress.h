//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <supp/dag_prefetch.h>
#include <image/dag_dxtCompress.h> //for dxt_modes

// additional (non-DXT) supported modes
enum
{
  MODE_R8 = 0x1000, // single channel 8-bit uncompressed format
  MODE_RGB565,      // 3-channel 16-bit uncompressed format
  MODE_RGBA4,       // 4-channel 16-bit uncompressed format
};

template <typename t_type>
static inline t_type roundDownToPowerOf2(const t_type &t, unsigned int power_of_2)
{
  return (t_type)(((uintptr_t)t) & ~((uintptr_t)power_of_2 - 1));
}

template <typename t_type>
static inline t_type roundUpToPowerOf2(const t_type &t, unsigned int power_of_2)
{
  return (t_type)((((uintptr_t)t) + ((uintptr_t)power_of_2 - 1)) & ~((uintptr_t)power_of_2 - 1));
}

static __forceinline void prefetch(const unsigned char *data, unsigned int size)
{
  static const unsigned int cacheLineSize = 128;

  // Expand input range to be cache-line aligned:
  const unsigned char *end = data + size;
  data = roundDownToPowerOf2(data, cacheLineSize);
  size = roundUpToPowerOf2(end, cacheLineSize) - data;

  for (size_t i = 0; i < size; i += cacheLineSize)
  {
    PREFETCH_DATA(i, data);
  }
}


class BaseTexture;
typedef BaseTexture Texture;

// linearData will be spoiled after call to this function
// always assume 32bit pixel
Texture *convert_to_dxt_texture(int w, int h, unsigned flags, char *linearData, int row_stride, int numMips, bool dxt5,
  const char *stat_name = NULL);

// always assume 32bit pixel
Texture *convert_to_bc4_texture(int w, int h, unsigned flags, char *linearData, int row_stride, int numMips, int pixel_offset,
  const char *stat_name = NULL);

// always assume 32bit pixel
Texture *convert_to_custom_dxt_texture(int w, int h, unsigned flags, char *linearData, int row_stride, int numMips, int mode,
  int pixel_offset, const char *stat_name = NULL);

void software_downsample_2x(unsigned char *dstPixel, unsigned char *srcPixel, unsigned int dstWidth, unsigned int dstHeight);
