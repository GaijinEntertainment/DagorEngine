// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_dds.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>

static uint32_t get_dagor_texformat_image_size_alignment(uint32_t f)
{
  switch (f)
  {
    case TEXFMT_DXT1:
    case TEXFMT_DXT5:
    case TEXFMT_ATI1N:
    case TEXFMT_ATI2N:
    case TEXFMT_BC7:
    case TEXFMT_BC6H: return 4; break;
    default: return 0;
  }
}

Texture *create_dds_texture(bool srgb, const ImageInfoDDS &image_info, const char *tex_name, int flags)
{
  // We don't support loading in compressed dds textures that are not aligned
  int image_size_alignment = max<uint32_t>(1, get_dagor_texformat_image_size_alignment(image_info.format));
  if ((image_info.width % image_size_alignment != 0) || (image_info.height % image_size_alignment != 0))
  {
    logerr("invalid dds image size (f:%#X, w:%d, h:%d)", image_info.format, image_info.width, image_info.height);
    return NULL;
  }

  Texture *tex = d3d::create_tex(NULL, image_info.width, image_info.height,
    (srgb ? TEXCF_SRGBREAD : 0) | image_info.format | TEXCF_LOADONCE | flags, image_info.nlevels, tex_name ? tex_name : "dds_texture");
  if (!tex)
    return NULL;

  for (unsigned int mipNo = 0; mipNo < image_info.nlevels; mipNo++)
  {
    char *dxtData;
    int dxtPitch;
    if (!tex->lockimg((void **)&dxtData, dxtPitch, mipNo,
          TEXLOCK_WRITE | ((mipNo != image_info.nlevels - 1) ? TEXLOCK_DONOTUPDATE : TEXLOCK_DELSYSMEMCOPY)))
    {
      logerr("%s lockimg failed '%s'", __FUNCTION__, d3d::get_last_error());
      continue;
    }


    unsigned mipH = image_info.height;
    unsigned pitch = image_info.pitch;

    if (image_info.dxt)
    {
      mipH >>= 2;
      if (mipH < 1)
        mipH = 1;
    }

    mipH >>= mipNo;
    pitch >>= mipNo;

    if (dxtPitch == pitch)
      memcpy(dxtData, image_info.pixels[mipNo], mipH * pitch);
    else
      for (int i = 0; i < mipH; ++i)
        memcpy(dxtData + i * dxtPitch, (char *)image_info.pixels[mipNo] + i * pitch, pitch);

    tex->unlockimg();
  }

  return tex;
}
