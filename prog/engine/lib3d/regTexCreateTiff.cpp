// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_createTex.h>
#include <3d/ddsxTex.h>
#include <3d/dag_texMgr.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <util/dag_texMetaData.h>


class TiffCreateTexFactory : public ICreateTexFactory
{
public:
  virtual BaseTexture *createTex(const char *fn, int /*flg*/, int /*levels*/, const char *fn_ext, const TextureMetaData &tmd)
  {
    if (!fn_ext || (dd_strnicmp(fn_ext, ".tif", 3) != 0 && dd_strnicmp(fn_ext, ".tiff", 4) != 0))
      return NULL;

    TIFF *image = TIFFOpen(fn, "rb");
    if (!image)
    {
      logerr("Cannot open TIFF file '%s'", fn);
      return NULL;
    }

    uint16_t spp = 1, bpp = 8, fileFormat = SAMPLEFORMAT_UINT;
    uint32_t width = 1, height = 1;
    TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bpp);
    TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetField(image, TIFFTAG_SAMPLEFORMAT, &fileFormat);

    struct FormatMap
    {
      int tiffFormat, tiffIsFloat, tiffBitsPerChannel, tiffChannels;
      int dagorFormat, dagorIsFloat, dagorBitsPerChannel, dagorChannels;
      bool swapRB;
    };

    const static struct FormatMap formatMap[] = {
      {SAMPLEFORMAT_UINT, false, 1, 1, TEXFMT_L8, false, 8, 1, false},
      {SAMPLEFORMAT_UINT, false, 8, 1, TEXFMT_L8, false, 8, 1, false},
      {SAMPLEFORMAT_UINT, false, 8, 2, TEXFMT_R8G8, false, 8, 2, false},
      {SAMPLEFORMAT_UINT, false, 8, 3, TEXFMT_A8R8G8B8, false, 8, 4, true},
      {SAMPLEFORMAT_UINT, false, 8, 4, TEXFMT_A8R8G8B8, false, 8, 4, true},
      {SAMPLEFORMAT_UINT, false, 16, 1, TEXFMT_L16, false, 16, 1, false},
      {SAMPLEFORMAT_UINT, false, 16, 2, TEXFMT_G16R16, false, 16, 2, false},
      {SAMPLEFORMAT_UINT, false, 16, 3, TEXFMT_A32B32G32R32F, true, 32, 4, false}, // TODO: figure out why A16B16G16R16 is not
                                                                                   // supported?
      {SAMPLEFORMAT_UINT, false, 16, 4, TEXFMT_A32B32G32R32F, true, 32, 4, false}, // *
      {SAMPLEFORMAT_UINT, false, 32, 1, TEXFMT_R32UI, false, 32, 1, false},
      {SAMPLEFORMAT_UINT, false, 32, 3, TEXFMT_A32B32G32R32UI, false, 32, 4, false},
      {SAMPLEFORMAT_UINT, false, 32, 4, TEXFMT_A32B32G32R32UI, false, 32, 4, false},
      {SAMPLEFORMAT_INT, false, 1, 1, TEXFMT_L8, false, 8, 1, false},
      {SAMPLEFORMAT_INT, false, 8, 1, TEXFMT_L8, false, 8, 1, false},
      {SAMPLEFORMAT_INT, false, 8, 2, TEXFMT_R8G8, false, 8, 2, false},
      {SAMPLEFORMAT_INT, false, 8, 3, TEXFMT_A8R8G8B8, false, 8, 4, true},
      {SAMPLEFORMAT_INT, false, 8, 4, TEXFMT_A8R8G8B8, false, 8, 4, true},
      {SAMPLEFORMAT_INT, false, 16, 1, TEXFMT_L16, false, 16, 1, false},
      {SAMPLEFORMAT_INT, false, 16, 2, TEXFMT_G16R16, false, 16, 2, false},
      {SAMPLEFORMAT_INT, false, 16, 3, TEXFMT_A32B32G32R32F, false, 16, 4, false}, // *
      {SAMPLEFORMAT_INT, false, 16, 4, TEXFMT_A32B32G32R32F, false, 16, 4, false}, // *
      {SAMPLEFORMAT_INT, false, 32, 1, TEXFMT_R32UI, false, 32, 1, false},
      {SAMPLEFORMAT_INT, false, 32, 2, TEXFMT_R32G32UI, false, 32, 2, false},
      {SAMPLEFORMAT_INT, false, 32, 3, TEXFMT_A32B32G32R32UI, false, 32, 4, false},
      {SAMPLEFORMAT_INT, false, 32, 4, TEXFMT_A32B32G32R32UI, false, 32, 4, false},
      {SAMPLEFORMAT_IEEEFP, true, 16, 1, TEXFMT_R16F, true, 16, 1, false},
      {SAMPLEFORMAT_IEEEFP, true, 16, 2, TEXFMT_G16R16F, true, 16, 2, false},
      {SAMPLEFORMAT_IEEEFP, true, 16, 3, TEXFMT_A16B16G16R16F, true, 16, 4, false},
      {SAMPLEFORMAT_IEEEFP, true, 16, 4, TEXFMT_A16B16G16R16F, true, 16, 4, false},
      {SAMPLEFORMAT_IEEEFP, true, 32, 1, TEXFMT_R32F, true, 32, 1, false},
      {SAMPLEFORMAT_IEEEFP, true, 32, 2, TEXFMT_G32R32F, true, 32, 2, false},
      {SAMPLEFORMAT_IEEEFP, true, 32, 3, TEXFMT_A32B32G32R32F, true, 32, 4, false},
      {SAMPLEFORMAT_IEEEFP, true, 32, 4, TEXFMT_A32B32G32R32F, true, 32, 4, false},
    };

    int formatIndex = -1;
    for (int i = 0; i < countof(formatMap); i++)
      if (formatMap[i].tiffFormat == fileFormat && formatMap[i].tiffBitsPerChannel == bpp && formatMap[i].tiffChannels == spp)
      {
        formatIndex = i;
        break;
      }

    if (formatIndex < 0)
    {
      logerr("Unsupported TIFF format: '%s' format=%d, bpp=%d, channels=%d", fn, fileFormat, bpp, spp);
      TIFFClose(image);
      return NULL;
    }

    const FormatMap &fm = formatMap[formatIndex];

    BaseTexture *t = NULL;
    BaseTexture *ref = NULL;
    if (!tmd.baseTexName.empty())
    {
      TEXTUREID id = get_managed_texture_id(tmd.baseTexName);
      if (id == BAD_TEXTUREID)
        id = add_managed_texture(tmd.baseTexName);
      ref = acquire_managed_tex(id);
      if (!ref)
      {
        release_managed_tex(id);
        TIFFClose(image);
        return t;
      }
    }

    int tiffStride = TIFFScanlineSize(image);
    Tab<uint8_t> tiffLine(tiffStride, 0);
    t = d3d::create_tex(NULL, width, height, fm.dagorFormat, 1, fn);

    if (t)
    {
      apply_gen_tex_props(t, tmd);

      uint8_t *data = NULL;
      int stride = 0;

      if (((Texture *)t)->lockimg((void **)&data, stride, 0, TEXLOCK_WRITE))
      {
        memset(data, 0x22, height * stride);

        for (int y = 0; y < height; y++)
        {
          int idx = stride * y;
          TIFFReadScanline(image, tiffLine.data(), y, 0);
          if (!convert_image_line(tiffLine.data(), width, fm.tiffChannels, fm.tiffBitsPerChannel, fm.tiffIsFloat, data + idx,
                fm.dagorChannels, fm.dagorBitsPerChannel, fm.dagorIsFloat, fm.swapRB))
          {
            break;
          }
        }

        ((Texture *)t)->unlockimg();
      }
      else
      {
        G_ASSERTF(0, "cannot lock texture");
        del_d3dres(t);
        TIFFClose(image);
        return NULL;
      }
    }

    TIFFClose(image);
    return t;
  }
};

static TiffCreateTexFactory tiff_create_tex_factory;

void register_tiff_tex_create_factory() { add_create_tex_factory(&tiff_create_tex_factory); }
