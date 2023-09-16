#include <util/dag_globDef.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_tex3d.h>
#include <image/dag_dxtCompress.h>
#include <render/dxtcompress.h>
#include <math/dag_imageFunctions.h>

#pragma optimize("gt", on)


void software_downsample_2x(unsigned char *dstPixel, unsigned char *srcPixel, unsigned int dstWidth, unsigned int dstHeight)
{
  if ((((size_t)srcPixel) & 15) || dstWidth < 2)
    imagefunctions::downsample4x_simdu(dstPixel, srcPixel, dstWidth, dstHeight);
  else
    imagefunctions::downsample4x_simda(dstPixel, srcPixel, dstWidth, dstHeight);
}

Texture *convert_to_dxt_texture(int w, int h, unsigned flags, char *linearData, int stride, int numMips, bool dxt5,
  const char *stat_name)
{
  return convert_to_custom_dxt_texture(w, h, flags, linearData, stride, numMips, dxt5 ? MODE_DXT5 : MODE_DXT1, 0, stat_name);
}

Texture *convert_to_bc4_texture(int w, int h, unsigned flags, char *linearData, int row_stride, int numMips, int pixel_offset,
  const char *stat_name)
{
  return convert_to_custom_dxt_texture(w, h, flags, linearData, row_stride, numMips, MODE_BC4, pixel_offset, stat_name);
}

// todo gamma mip

Texture *convert_to_custom_dxt_texture(int w, int h, unsigned flags, char *linearData, int row_stride, int numMips, int mode,
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
    fmt = TEXFMT_L8;

  Texture *tex = d3d::create_tex(NULL, w, h, flags | fmt, numMips, stat_name ? stat_name : "dxt_texture");
  if (!tex)
    return NULL;

  char *compressToData;
  char *nostride_linear_data = linearData;
  if (row_stride != w * 4)
  {
    nostride_linear_data = (char *)memalloc(w * h * 4);
    for (int i = 0; i < h; ++i)
      memcpy(nostride_linear_data, linearData, w * 4);
  }
  for (unsigned int mipNo = 0; mipNo < numMips; mipNo++)
  {
    char *dxtData;
    int dxtPitch;
    if (!tex->lockimg((void **)&dxtData, dxtPitch, mipNo,
          TEXLOCK_WRITE | ((mipNo != numMips - 1) ? TEXLOCK_DONOTUPDATEON9EXBYDEFAULT : TEXLOCK_DELSYSMEMCOPY)))
    {
      logerr("%s lockimg failed '%s'", __FUNCTION__, d3d::get_last_error());
      continue;
    }

    // int64_t reft = ref_time_ticks();
    unsigned int mipW = w >> mipNo;
    unsigned int mipH = h >> mipNo;
    compressToData = dxtData;

    if (mode == MODE_BC4)
      CompressBC4((unsigned char *)nostride_linear_data + pixel_offset, mipW, mipH, dxtPitch, dxtData, mipW * 4, 4);
    else if (mode == MODE_R8)
    {
      for (int i = 0; i < mipH; ++i)
      {
        const uint8_t *src = (const uint8_t *)nostride_linear_data + i * mipW * 4 + pixel_offset;
        uint8_t *dst = (uint8_t *)dxtData + i * dxtPitch;
        for (int j = 0; j < mipW; ++j, src += 4, dst++)
          *dst = *src;
      }
    }
    else
      ManualDXT(mode, (TexPixel32 *)nostride_linear_data, mipW, mipH, dxtPitch, dxtData, DXT_ALGORITHM_PRECISE);
    // compress += get_time_usec(reft);
    // reft = ref_time_ticks();
    if (mipNo < numMips - 1)
    {
      software_downsample_2x((unsigned char *)linearData, (unsigned char *)linearData, mipW >> 1, mipH >> 1);
    }

    tex->unlockimg();
  }

  if (row_stride != w * 4)
    memfree(nostride_linear_data, tmpmem);
  return tex;
}
