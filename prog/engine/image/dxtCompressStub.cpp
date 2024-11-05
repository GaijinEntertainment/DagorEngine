// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_dxtCompress.h>

void ManualDXT(int /*mode*/, const TexPixel32 * /*pImage*/, int /*width*/, int /*height*/, int /*dxt_pitch*/, char * /*pCompressed*/,
  int /*algorithm*/)
{}

void *CompressDXT(int /*mode*/, TexPixel32 * /*image*/, int /*stride_bytes*/, int /*width*/, int /*height*/, int /*levels*/,
  int * /*len*/, int /*algorithm*/, int /*zlib_lev*/)
{
  return NULL;
}

void CompressBC4(const unsigned char * /*image*/, int /*width*/, int /*height*/, int /*dxt_pitch*/, char * /*pCompressed*/,
  int /*row_stride*/, int /*pixel_stride*/)
{}

void CompressBC5(unsigned char * /*image*/, int /*width*/, int /*height*/, int /*dxt_pitch*/, char * /*pCompressed*/,
  int /*row_stride*/, int /*pixel_stride*/)
{}
