// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <image/dag_dxtCompress.h>
#include <render/dxtcompress.h>
#include <math/dag_imageFunctions.h>
#include <3d/dag_lockTexture.h>
#include <dag/dag_vector.h>

#ifdef _MSC_VER
#pragma optimize("gt", on)
#endif


void software_downsample_2x(unsigned char *dstPixel, const unsigned char *srcPixel, unsigned int dstWidth, unsigned int dstHeight)
{
  if ((((size_t)srcPixel) & 15) || dstWidth < 2)
    imagefunctions::downsample4x_simdu(dstPixel, srcPixel, dstWidth, dstHeight);
  else
    imagefunctions::downsample4x_simda(dstPixel, srcPixel, dstWidth, dstHeight);
}

Texture *convert_to_dxt_texture(int w, int h, unsigned flags, const char *linearData, int stride, int numMips, bool dxt5,
  const char *stat_name)
{
  return convert_to_custom_dxt_texture(w, h, flags, linearData, stride, numMips, dxt5 ? MODE_DXT5 : MODE_DXT1, 0, stat_name);
}

Texture *convert_to_bc4_texture(int w, int h, unsigned flags, const char *linearData, int row_stride, int numMips, int pixel_offset,
  const char *stat_name)
{
  return convert_to_custom_dxt_texture(w, h, flags, linearData, row_stride, numMips, MODE_BC4, pixel_offset, stat_name);
}

// todo gamma mip

Texture *convert_to_custom_dxt_texture(int w, int h, unsigned flags, const char *linearData, int row_stride, int numMips, int mode,
  int pixel_offset, const char *stat_name)
{
  flags = (flags & (~TEXFMT_MASK));

  int fmt = -1;
  if (mode == MODE_DXT1)
    fmt = TEXFMT_DXT1;
  else if (mode == MODE_DXT5)
    fmt = TEXFMT_DXT5;
  else if (mode == MODE_BC4)
    fmt = TEXFMT_ATI1N;
  else if (mode == MODE_R8)
    fmt = TEXFMT_R8;

  Texture *tex = d3d::create_tex(NULL, w, h, flags | fmt, numMips, stat_name ? stat_name : "dxt_texture");
  if (!tex)
    return NULL;

  const char *nostride_linear_data = linearData;
  dag::Vector<char> allocated_buffer;
  if (row_stride != w * 4)
  {
    allocated_buffer.resize(h * w * 4);
    nostride_linear_data = allocated_buffer.data();
    for (int i = 0; i < h; ++i)
      memcpy(allocated_buffer.data() + i * w * 4, linearData + i * row_stride, w * 4);
  }
  for (unsigned int mipNo = 0; mipNo < numMips; mipNo++)
    if (auto lockedTex =
          lock_texture(tex, mipNo, TEXLOCK_WRITE | ((mipNo != numMips - 1) ? TEXLOCK_DONOTUPDATE : TEXLOCK_DELSYSMEMCOPY)))
    {
      uint8_t *dxtData = lockedTex.get();
      int dxtPitch = lockedTex.getByteStride();

      unsigned int mipW = w >> mipNo;
      unsigned int mipH = h >> mipNo;

      if (mode == MODE_BC4)
        CompressBC4(reinterpret_cast<const unsigned char *>(nostride_linear_data) + pixel_offset, mipW, mipH, dxtPitch,
          reinterpret_cast<char *>(dxtData), mipW * 4, 4);
      else if (mode == MODE_R8)
      {
        for (int i = 0; i < mipH; ++i)
        {
          const uint8_t *src = reinterpret_cast<const uint8_t *>(nostride_linear_data) + i * mipW * 4 + pixel_offset;
          uint8_t *dst = reinterpret_cast<uint8_t *>(dxtData) + i * dxtPitch;
          for (int j = 0; j < mipW; ++j, src += 4, dst++)
            *dst = *src;
        }
      }
      else
        ManualDXT(mode, reinterpret_cast<const TexPixel32 *>(nostride_linear_data), mipW, mipH, dxtPitch,
          reinterpret_cast<char *>(dxtData), DXT_ALGORITHM_PRECISE);

      if (mipNo < numMips - 1)
      {
        // Calculate downsampled image (next mip) from nostride_linear_data = current mip
        unsigned int nextMipW = mipW >> 1;
        unsigned int nextMipH = mipH >> 1;

        // Store next mip (downsampled image) in allocated_buffer
        // Check buffer size
        if (allocated_buffer.empty())
          allocated_buffer.resize(nextMipW * nextMipH * 4);
        else
          G_ASSERT(allocated_buffer.size() > nextMipW * nextMipH * 4);

        software_downsample_2x(reinterpret_cast<unsigned char *>(allocated_buffer.data()),
          reinterpret_cast<const unsigned char *>(nostride_linear_data), nextMipW, nextMipH);

        nostride_linear_data = allocated_buffer.data(); // Ptr to next mip
      }
    }
    else
    {
      logerr("%s lockimg failed '%s'", __FUNCTION__, d3d::get_last_error());
      break;
    }

  return tex;
}
