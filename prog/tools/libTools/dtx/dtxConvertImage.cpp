#include <libTools/dtx/dtx.h>

#include <nvtt/nvtt.h>
#include <nvimage/image.h>
#include <nvimage/floatImage.h>
#include <nvimage/filter.h>
#include <nvcore/ptr.h>

#include <debug/dag_log.h>

#include <ioSys/dag_genIo.h>

#include <PVRTexture.h>
#include <PVRTextureFormat.h>
#undef PVR_DLL
#define PVR_DLL __cdecl
#include <PVRTextureUtilities.h>
#undef PVR_DLL

#include <3d/ddsFormat.h>
#undef ERROR // after #include <supp/_platform.h>
#include <image/dag_texPixel.h>
#include <math/dag_adjpow2.h>
#include <debug/dag_debug.h>

struct ErrorHandler : public nvtt::ErrorHandler
{
  virtual void error(nvtt::Error e) { logerr("nvtt: '%s'", nvtt::errorString(e)); }
};
struct OutputHandler : public nvtt::OutputHandler
{
  IGenSave *cwr;

  virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) {}
  virtual bool writeData(const void *data, int size)
  {
    cwr->write(data, size);
    return true;
  }
};

bool ddstexture::Converter::convertImageFast(IGenSave &cb, TexPixel32 *pix, int w, int h, int stride, bool make_pow_2,
  int pow2_strategy)
{
  nvtt::TextureType nvtttype;
  switch (type)
  {
    case typeDefault:
    case type2D: nvtttype = nvtt::TextureType_2D; break;
    case typeCube: nvtttype = nvtt::TextureType_Cube; break;

    default: logerr("%s: unsupported type=%d", __FUNCTION__, type); return false;
  }

  nvtt::Compressor compressor;
  nvtt::InputOptions inpOptions;
  nvtt::CompressionOptions comprOptions;
  nvtt::OutputOptions outOptions;
  ErrorHandler errHandler;
  OutputHandler outHandler;

  compressor.enableCudaAcceleration(false);
  outOptions.setErrorHandler(&errHandler);
  outOptions.setOutputHandler(&outHandler);

  bool dxt_fmt = false;

  switch (format)
  {
    case fmtDXT1:
    case fmtDXT1a:
    case fmtDXT3:
    case fmtDXT5: dxt_fmt = true; break;
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
      default: logerr("%s: unsupported pow2_strategy=%d", __FUNCTION__, pow2_strategy); return false;
    }
  else
    inpOptions.setRoundMode(nvtt::RoundMode_None);

  switch (quality)
  {
    case QualityFastest: comprOptions.setQuality(nvtt::Quality_Fastest); break;

    case QualityNormal: comprOptions.setQuality(nvtt::Quality_Normal); break;

    case QualityProduction: comprOptions.setQuality(nvtt::Quality_Production); break;

    case QualityHighest: comprOptions.setQuality(nvtt::Quality_Highest); break;
  }

  if (mipmapType == mipmapGenerate && mipmapCount == AllMipMaps && dxt_fmt && (!make_pow_2 || pow2_strategy == 2) &&
      (!is_pow_of2(w) || !is_pow_of2(h)))
  {
    mipmapCount = 0;

    int mipw = w, miph = h;
    while ((mipw || miph) && ((mipw % 4) == 0 || mipw == 2 || mipw == 1) && ((miph % 4) == 0 || miph == 2 || miph == 1))
      mipmapCount++, mipw /= 2, miph /= 2;
    if (mipmapCount)
      mipmapCount--;
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

    case fmtL8:
    case fmtA8L8:
    case fmtASTC4:
    case fmtASTC8:
    case fmtRGBA4444:
    case fmtRGBA5551:
    case fmtRGB565: return false;
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
  int img_w = w, img_h = h;
  if (dxt_fmt && (!make_pow_2 || pow2_strategy == 2) && ((img_w % 4) || (img_h % 4)))
  {
    logerr("%s: bad texture size %dx%d for DXT* format", __FUNCTION__, img_w, img_h);
    return false;
  }

  nv::FloatImage::WrapMode addrMode = nv::FloatImage::WrapMode_Clamp;
  inpOptions.setWrapMode(nvtt::WrapMode_Clamp);
  nv::AutoPtr<nv::Image> image(new nv::Image);
  image->allocate(img_w, img_h);
  image->setFormat(image->Format_ARGB);
  memcpy(image->pixels(), pix, img_w * img_h * 4);

  inpOptions.setGamma(mipmapFilterGamma, mipmapFilterGamma);
  inpOptions.setTextureLayout(nvtttype, image->width(), image->height());
  inpOptions.setMipmapData(image->pixels(), image->width(), image->height());
  outHandler.cwr = &cb;
  return compressor.process(inpOptions, comprOptions, outOptions);
}

static inline unsigned short to_4444(unsigned r, unsigned g, unsigned b, unsigned a)
{
  return ((a + 15) * 15 / 255) | (((b + 15) * 15 / 255) << 4) | (((g + 15) * 15 / 255) << 8) | (((r + 15) * 15 / 255) << 12);
}
static inline unsigned short to_5551(unsigned r, unsigned g, unsigned b, unsigned a)
{
  return (a >= 127 ? 1 : 0) | (((b + 7) * 31 / 255) << 1) | (((g + 7) * 31 / 255) << 6) | (((r + 7) * 31 / 255) << 11);
}
static inline unsigned short to_565(unsigned r, unsigned g, unsigned b)
{
  return ((b + 7) * 31 / 255) | (((g + 3) * 63 / 255) << 5) | (((r + 7) * 31 / 255) << 11);
}
static inline unsigned short to_a8l8(unsigned l, unsigned a) { return (a << 8) | l; }

bool ddstexture::Converter::convertImage(IGenSave &cb, TexPixel32 *pix, int w, int h, int stride, bool make_pow_2, int pow2_strategy)
{
  if (convertImageFast(cb, pix, w, h, stride, make_pow_2, pow2_strategy))
    return true;

  if (type != type2D)
  {
    logerr("%s: CUBETEX/VOLTEX is not supported for format=%d in convertImage()", __FUNCTION__, format);
    return false;
  }

  int orig_w = w, orig_h = h;
  nv::AutoPtr<nv::Image> r_image(new nv::Image);
  nv::AutoPtr<nv::Filter> filter;

  if (mipmapFilter == filterBox)
    filter = new nv::BoxFilter();
  else if (mipmapFilter == filterTriangle)
    filter = new nv::TriangleFilter();
  else if (mipmapFilter == filterKaiser)
  {
    filter = new nv::KaiserFilter(3);
    ((nv::KaiserFilter *)filter.ptr())->setParameters(4, 1);
  }
  else
    filter = new nv::BoxFilter();


  if (make_pow_2 && (!is_pow_of2(w) || !is_pow_of2(h)))
  {
    int w_max = 1, h_max = 1, w_min = 1, h_min = 1;
    while (w_max < orig_w)
    {
      w_max <<= 1;
      if (w_max < orig_w)
        w_min = w_max;
    }
    while (h_max < orig_h)
    {
      h_max <<= 1;
      if (h_max < orig_h)
        h_min = h_max;
    }

    switch (pow2_strategy)
    {
      case -1:
        w = w_min;
        h = h_min;
        break;
      case 0:
        w = (w_max - w < w - w_min) ? w_max : w_min;
        h = (h_max - h < h - h_min) ? h_max : h_min;
        break;
      case +1:
        w = w_max;
        h = h_max;
        break;
      case +2: break;
      default: logerr("%s: bad pow2_strategy=%d", __FUNCTION__, pow2_strategy); return false;
    }

    // resample
    r_image->allocate(orig_w, orig_h);
    r_image->setFormat(r_image->Format_ARGB);
    memcpy(r_image->pixels(), pix, orig_w * orig_h * 4);

    nv::FloatImage fimage(r_image.ptr());
    fimage.toLinear(0, 3, mipmapFilterGamma);

    nv::AutoPtr<nv::FloatImage> fresult(fimage.resize(*filter, w, h, nv::FloatImage::WrapMode_Clamp));
    r_image = fresult->createImageGammaCorrect(mipmapFilterGamma);
    G_ASSERT(r_image->width() == w && r_image->height() == h);

    pix = (TexPixel32 *)r_image->pixels();
  }

#if _TARGET_PC_WIN
  if (format == fmtASTC4 || format == fmtASTC8)
  {
    if (!is_pow_of2(w))
    {
      logwarn("image %dx%d is NPOT - ASTC will fail for iOS, so export as %dx%d RGBA", orig_w, orig_h, w, h);
      format = fmtARGB;
      return convertImageFast(cb, pix, w, h, stride, make_pow_2, pow2_strategy);
    }
    using namespace pvrtexture;

    Tab<TexPixel32> pix_argb(tmpmem);
    Tab<char> packed(tmpmem);
    pix_argb.resize(w * h);
    for (TexPixel32 *s = pix, *se = s + w * h, *d = pix_argb.data(); s < se; s++, d++)
      d->u = s->r | (unsigned(s->g) << 8) | (unsigned(s->b) << 16) | (unsigned(s->a) << 24);

    CPVRTextureHeader thdr(PVRStandard8PixelType.PixelTypeID, h, w, 1, 1);
    CPVRTexture tex(thdr, pix_argb.data());
    if (mipmapType == mipmapGenerate)
      GenerateMIPMaps(tex, eResizeLinear);
    Transcode(tex, format == fmtASTC4 ? ePVRTPF_ASTC_4x4 : ePVRTPF_ASTC_8x8, ePVRTVarTypeUnsignedByteNorm,
      ePVRTCSpacelRGB /*, ePVRTCBest*/);

    packed.reserve(tex.getDataSize(PVRTEX_ALLMIPLEVELS));
    int mips = tex.getHeader().getNumMIPLevels();
    for (int j = 0; j < mips; j++)
      append_items(packed, tex.getDataSize(j), (char *)tex.getDataPtr(j));

    clear_and_shrink(pix_argb);

    DDSURFACEDESC2 targetHeader;
    DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;

    memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
    targetHeader.dwSize = sizeof(DDSURFACEDESC2);
    targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    targetHeader.dwHeight = h;
    targetHeader.dwWidth = w;
    targetHeader.dwDepth = 0;
    targetHeader.dwMipMapCount = mips;
    targetHeader.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    targetHeader.ddsCaps.dwCaps2 = 0;
    pf.dwSize = sizeof(DDPIXELFORMAT);
    pf.dwFlags = DDPF_FOURCC;
    pf.dwFourCC = format == fmtASTC4 ? MAKEFOURCC('A', 'S', 'T', '4') : MAKEFOURCC('A', 'S', 'T', '8');

    uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
    cb.write(&FourCC, sizeof(FourCC));
    cb.write(&targetHeader, sizeof(targetHeader));
    cb.write(packed.data(), data_size(packed));
    return true;
  }
  else
#endif
    if (format == fmtRGBA4444 || format == fmtRGBA5551 || format == fmtRGB565 || format == fmtA8L8)
  {
    Tab<Tab<unsigned short> *> pix16(tmpmem);

    if (mipmapType == mipmapGenerate)
    {
      nv::AutoPtr<nv::Image> image(new nv::Image);

      int img_w = w, img_h = h;
      image->allocate(img_w, img_h);
      image->setFormat(image->Format_ARGB);
      memcpy(image->pixels(), pix, img_w * img_h * 4);

      for (;;)
      {
        Tab<unsigned short> *p16 = new Tab<unsigned short>(tmpmem);
        p16->resize(img_w * img_h);
        pix16.push_back(p16);
        unsigned short *ptr16 = p16->data();

        if (format == fmtRGBA4444)
          for (TexPixel32 *s = (TexPixel32 *)image->pixels(), *se = s + img_w * img_h; s < se; s++, ptr16++)
            *ptr16 = to_4444(s->r, s->g, s->b, s->a);
        else if (format == fmtRGBA5551)
          for (TexPixel32 *s = (TexPixel32 *)image->pixels(), *se = s + img_w * img_h; s < se; s++, ptr16++)
            *ptr16 = to_5551(s->r, s->g, s->b, s->a);
        else if (format == fmtRGB565)
          for (TexPixel32 *s = (TexPixel32 *)image->pixels(), *se = s + img_w * img_h; s < se; s++, ptr16++)
            *ptr16 = to_565(s->r, s->g, s->b);
        else if (format == fmtA8L8)
          for (TexPixel32 *s = (TexPixel32 *)image->pixels(), *se = s + img_w * img_h; s < se; s++, ptr16++)
            *ptr16 = to_a8l8(s->r, s->a);


        if (img_w == 1 && img_h == 1)
          break;

        if (img_w > 1)
          img_w >>= 1;
        if (img_h > 1)
          img_h >>= 1;

        nv::FloatImage fimage(image.ptr());
        fimage.toLinear(0, 3, mipmapFilterGamma);

        nv::AutoPtr<nv::FloatImage> fresult(fimage.resize(*filter, img_w, img_h, nv::FloatImage::WrapMode_Clamp));
        image = fresult->createImageGammaCorrect(mipmapFilterGamma);
      }
    }
    else
    {
      Tab<unsigned short> *p16 = new Tab<unsigned short>(tmpmem);
      p16->resize(w * h);
      pix16.push_back(p16);
      unsigned short *ptr16 = p16->data();

      if (format == fmtRGBA4444)
        for (TexPixel32 *s = pix, *se = s + w * h; s < se; s++, ptr16++)
          *ptr16 = to_4444(s->r, s->g, s->b, s->a);
      else if (format == fmtRGBA5551)
        for (TexPixel32 *s = pix, *se = s + w * h; s < se; s++, ptr16++)
          *ptr16 = to_5551(s->r, s->g, s->b, s->a);
      else if (format == fmtRGB565)
        for (TexPixel32 *s = pix, *se = s + w * h; s < se; s++, ptr16++)
          *ptr16 = to_565(s->r, s->g, s->b);
      else if (format == fmtA8L8)
        for (TexPixel32 *s = pix, *se = s + w * h; s < se; s++, ptr16++)
          *ptr16 = to_a8l8(s->r, s->a);
    }

    DDSURFACEDESC2 targetHeader;
    DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;

    memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
    targetHeader.dwSize = sizeof(DDSURFACEDESC2);
    targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT | DDSD_ALPHABITDEPTH;
    targetHeader.dwHeight = h;
    targetHeader.dwWidth = w;
    targetHeader.dwDepth = 0;
    targetHeader.dwMipMapCount = pix16.size();
    targetHeader.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    targetHeader.ddsCaps.dwCaps2 = 0;

    pf.dwSize = sizeof(DDPIXELFORMAT);
    switch (format)
    {
      case fmtRGBA4444:
        pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0x000F;
        pf.dwBBitMask = 0x00F0;
        pf.dwGBitMask = 0x0F00;
        pf.dwRBitMask = 0xF000;
        break;
      case fmtRGBA5551:
        pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0x0001;
        pf.dwBBitMask = 0x003E;
        pf.dwGBitMask = 0x07C0;
        pf.dwRBitMask = 0xF800;
        break;
      case fmtRGB565:
        pf.dwFlags = DDPF_RGB;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0;
        pf.dwBBitMask = 0x001F;
        pf.dwGBitMask = 0x07E0;
        pf.dwRBitMask = 0xF800;
        break;
      case fmtA8L8:
        pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0xFF00;
        pf.dwBBitMask = 0x00FF;
        break;
    }

    uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
    cb.write(&FourCC, sizeof(FourCC));
    cb.write(&targetHeader, sizeof(targetHeader));
    for (int i = 0; i < pix16.size(); i++)
      cb.write(pix16[i]->data(), data_size(*pix16[i]));
    clear_all_ptr_items(pix16);
    return true;
  }
  else if (format == fmtL8)
  {
    Tab<Tab<unsigned char> *> pix8(tmpmem);

    if (mipmapType == mipmapGenerate)
    {
      nv::AutoPtr<nv::Image> image(new nv::Image);

      int img_w = w, img_h = h;
      image->allocate(img_w, img_h);
      image->setFormat(image->Format_ARGB);
      memcpy(image->pixels(), pix, img_w * img_h * 4);

      for (;;)
      {
        Tab<unsigned char> *p8 = new Tab<unsigned char>(tmpmem);
        p8->resize(img_w * img_h);
        pix8.push_back(p8);
        unsigned char *ptr8 = p8->data();

        if (format == fmtL8)
          for (TexPixel32 *s = (TexPixel32 *)image->pixels(), *se = s + img_w * img_h; s < se; s++, ptr8++)
            *ptr8 = s->r;


        if (img_w == 1 && img_h == 1)
          break;

        if (img_w > 1)
          img_w >>= 1;
        if (img_h > 1)
          img_h >>= 1;

        nv::FloatImage fimage(image.ptr());
        fimage.toLinear(0, 3, mipmapFilterGamma);

        nv::AutoPtr<nv::FloatImage> fresult(fimage.resize(*filter, img_w, img_h, nv::FloatImage::WrapMode_Clamp));
        image = fresult->createImageGammaCorrect(mipmapFilterGamma);
      }
    }
    else
    {
      Tab<unsigned char> *p8 = new Tab<unsigned char>(tmpmem);
      p8->resize(w * h);
      pix8.push_back(p8);
      unsigned char *ptr8 = p8->data();

      if (format == fmtL8)
        for (TexPixel32 *s = pix, *se = s + w * h; s < se; s++, ptr8++)
          *ptr8 = s->r;
    }

    DDSURFACEDESC2 targetHeader;
    DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;

    memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
    targetHeader.dwSize = sizeof(DDSURFACEDESC2);
    targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT | DDSD_ALPHABITDEPTH;
    targetHeader.dwHeight = h;
    targetHeader.dwWidth = w;
    targetHeader.dwDepth = 0;
    targetHeader.dwMipMapCount = pix8.size();
    targetHeader.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    targetHeader.ddsCaps.dwCaps2 = 0;

    pf.dwSize = sizeof(DDPIXELFORMAT);
    switch (format)
    {
      case fmtL8:
        pf.dwFlags = DDPF_RGB;
        pf.dwRGBBitCount = 8;
        pf.dwRBitMask = 0x00FF;
        break;
    }

    uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
    cb.write(&FourCC, sizeof(FourCC));
    cb.write(&targetHeader, sizeof(targetHeader));
    for (int i = 0; i < pix8.size(); i++)
      cb.write(pix8[i]->data(), data_size(*pix8[i]));
    clear_all_ptr_items(pix8);
    return true;
  }

  logerr_ctx("type=%d format=%d failed: no support in convertImage()", type, format);
  return false;
}
