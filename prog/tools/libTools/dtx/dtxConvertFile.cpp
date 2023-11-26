#include <libTools/dtx/dtx.h>
#include <libTools/util/de_TextureName.h>
#include <nvtt/nvtt.h>
#include <nvimage/image.h>
#include <nvimage/directDrawSurface.h>
#include <nvimage/floatImage.h>
#include <nvcore/ptr.h>
#include <image/dag_texPixel.h>
#include <image/dag_loadImage.h>
#include <math/dag_adjpow2.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_fileIo.h>
#include <3d/dag_drv3d.h>
#include <3d/ddsFormat.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>

struct ErrorHandler : public nvtt::ErrorHandler
{
  virtual void error(nvtt::Error e) { logerr("nvtt: '%s'", nvtt::errorString(e)); }
};
struct OutputHandler : public nvtt::OutputHandler
{
  MemorySaveCB *cwr;

  virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) {}
  virtual bool writeData(const void *data, int size)
  {
    cwr->write(data, size);
    return true;
  }
};

static bool copyTo(MemorySaveCB &cwr, const char *dest)
{
  FullFileSaveCB fcwr(dest);
  if (!fcwr.fileHandle)
  {
    logerr("%s: Cannot write output file '%s'", __FUNCTION__, dest);
    return false;
  }
  cwr.copyDataTo(fcwr);
  return true;
}

bool ddstexture::Converter::copyTosrcLocation = true;

bool ddstexture::Converter::convert(const char *src_filename, const char *dst_filename, bool overwrite, bool make_pow_2,
  int pow2_strategy)
{
  if (!overwrite && dd_file_exist(dst_filename))
  {
    logerr("%s: dest %s exists, but overwrite=%d", __FUNCTION__, dst_filename, overwrite);
    return false;
  }

  DDSPathName src = src_filename;
  DDSPathName dst = dst_filename;
  if (copyTosrcLocation)
  {
    String dstDds(src);
    ::location_from_path(dstDds);
    dstDds = ::make_full_path(dstDds, ::get_file_name(dst));
    dst = dstDds;
  }

  nvtt::Compressor compressor;
  nvtt::InputOptions inpOptions;
  nvtt::CompressionOptions comprOptions;
  nvtt::OutputOptions outOptions;
  ErrorHandler errHandler;
  OutputHandler outHandler;
  int img_w = 0, img_h = 0;
  MemorySaveCB cwr;
  bool dxt_fmt = false;

  switch (format)
  {
    case fmtDXT1:
    case fmtDXT1a:
    case fmtDXT3:
    case fmtDXT5: dxt_fmt = true; break;
    default: break;
  }
  if (!make_pow_2 && dxt_fmt)
    make_pow_2 = true;
  if (make_pow_2)
    switch (pow2_strategy)
    {
      case -1: inpOptions.setRoundMode(nvtt::RoundMode_ToPreviousPowerOfTwo); break;
      case 0: inpOptions.setRoundMode(nvtt::RoundMode_ToNearestPowerOfTwo); break;
      case +1: inpOptions.setRoundMode(nvtt::RoundMode_ToNextPowerOfTwo); break;
      case +2: inpOptions.setRoundMode(nvtt::RoundMode_None); break;
      default: return false;
    }
  else
    inpOptions.setRoundMode(nvtt::RoundMode_None);
  if (stricmp(src.getExt(), "dds") == 0 || stricmp(src.getExt(), "dtx") == 0)
  {
    // Load surface.
    nv::DirectDrawSurface dds(src);
    if (!dds.isValid())
    {
      logerr("%s: The file '%s' is not a valid DDS file.", __FUNCTION__, src.str());
      return false;
    }

    if (!dds.isSupported() || dds.isTexture3D())
    {
      logerr("%s: The file '%s' is not a supported DDS file.", __FUNCTION__, src.str());
      return false;
    }

    uint faceCount;
    if (dds.isTexture2D())
    {
      inpOptions.setTextureLayout(nvtt::TextureType_2D, dds.width(), dds.height());
      faceCount = 1;
    }
    else
    {
      // nvDebugCheck(dds.isTextureCube());
      inpOptions.setTextureLayout(nvtt::TextureType_Cube, dds.width(), dds.height());
      faceCount = 6;
    }
    img_w = dds.width(), img_h = dds.height();

    uint srcMipmapCount = dds.mipmapCount();
    nv::Image mipmap;

    for (uint f = 0; f < faceCount; f++)
      for (uint m = 0; m < srcMipmapCount; m++)
      {
        dds.mipmap(&mipmap, f, m);
        inpOptions.setMipmapData(mipmap.pixels(), mipmap.width(), mipmap.height(), 1, f, m);
      }
  }
  else
  {
    // Regular image.
    bool alpha;
    TexImage32 *img = load_image(src, tmpmem, &alpha);

    if (!img)
    {
      logerr("%s: The file '%s' is not a supported image type.", __FUNCTION__, src.str());
      return false;
    }
    if (type != typeDefault && type != type2D)
    {
      logerr("%s: CUBETEX/VOLTEX not supported for input image '%s'", __FUNCTION__, src.str());
      return false;
    }

    img_w = img->w, img_h = img->h;
    switch (format)
    {
      case fmtL8:
      case fmtA8L8:
      case fmtASTC4:
      case fmtASTC8:
      case fmtRGBA4444:
      case fmtRGBA5551:
      case fmtRGB565:
        if (convertImage(cwr, img->getPixels(), img_w, img_h, img_w * 4, make_pow_2, pow2_strategy))
        {
          memfree(img, tmpmem);
          return copyTo(cwr, dst);
        }
        memfree(img, tmpmem);
        return false;
      default: break;
    }

    inpOptions.setTextureLayout(nvtt::TextureType_2D, img->w, img->h);
    inpOptions.setMipmapData(img->getPixels(), img->w, img->h);

    if (mipmapType == mipmapUseExisting)
      mipmapType = mipmapGenerate;

    if (mipmapType == mipmapGenerate && mipmapCount == AllMipMaps && dxt_fmt && (!make_pow_2 || pow2_strategy == 2) &&
        (!is_pow_of2(img->w) || !is_pow_of2(img->h)))
    {
      mipmapCount = 0;

      int w = img->w, h = img->h;
      while ((w || h) && ((w % 4) == 0 || w == 2 || w == 1) && ((h % 4) == 0 || h == 2 || h == 1))
        mipmapCount++, w /= 2, h /= 2;
      if (mipmapCount)
        mipmapCount--;
    }
    memfree(img, tmpmem);
  }
  if (dxt_fmt && (!make_pow_2 || pow2_strategy == 2) && ((img_w % 4) || (img_h % 4)))
  {
    logerr("%s: bad texture <%s> size %dx%d for DXT* format", __FUNCTION__, src_filename, img_w, img_h);
    return false;
  }

  compressor.enableCudaAcceleration(false);
  outOptions.setErrorHandler(&errHandler);
  outOptions.setOutputHandler(&outHandler);

  switch (quality)
  {
    case QualityFastest: comprOptions.setQuality(nvtt::Quality_Fastest); break;

    case QualityNormal: comprOptions.setQuality(nvtt::Quality_Normal); break;

    case QualityProduction: comprOptions.setQuality(nvtt::Quality_Production); break;

    case QualityHighest: comprOptions.setQuality(nvtt::Quality_Highest); break;
  }

  if (mipmapType == mipmapGenerate)
    inpOptions.setMipmapGeneration(true, mipmapCount == AllMipMaps ? -1 : mipmapCount);
  else
    inpOptions.setMipmapGeneration(false);

  switch (format)
  {
    case fmtRGB: comprOptions.setFormat(nvtt::Format_RGB); break;
    case fmtCurrent:
    case fmtARGB: comprOptions.setFormat(nvtt::Format_RGBA); break;

    case fmtDXT1: comprOptions.setFormat(nvtt::Format_DXT1); break;

    case fmtDXT1a: comprOptions.setFormat(nvtt::Format_DXT1a); break;

    case fmtDXT3: comprOptions.setFormat(nvtt::Format_DXT3); break;

    case fmtDXT5: comprOptions.setFormat(nvtt::Format_DXT5); break;

    default: logerr_ctx("type=%d format=%d failed: no support in convert()", type, format); return false;
  }

  switch (mipmapFilter)
  {
    case filterPoint:
    case filterBox: inpOptions.setMipmapFilter(nvtt::MipmapFilter_Box); break;
    case filterTriangle: inpOptions.setMipmapFilter(nvtt::MipmapFilter_Triangle); break;
    case filterKaiser:
    default:
      inpOptions.setMipmapFilter(nvtt::MipmapFilter_Kaiser);
      inpOptions.setKaiserParameters(3, 4, 1);
      break;
  }

  nv::FloatImage::WrapMode addrMode = nv::FloatImage::WrapMode_Clamp;
  inpOptions.setWrapMode(nvtt::WrapMode_Clamp);

  inpOptions.setGamma(mipmapFilterGamma, mipmapFilterGamma);
  outHandler.cwr = &cwr;
  if (compressor.process(inpOptions, comprOptions, outOptions))
    return copyTo(cwr, dst);
  logerr("%s: <%s> conversion failed", __FUNCTION__, src_filename);
  return false;
}
