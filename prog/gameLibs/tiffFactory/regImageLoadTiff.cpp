#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <osApiWrappers/dag_localConv.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_fileIo.h>
#include <image/dag_texPixel.h>


static TexImage32 *load_tiff32(IGenLoad &, IMemAlloc *, bool *)
{
  G_ASSERTF(0, "load_tiff32(IGenLoad &crd) not implemented");
  return NULL;
}

static TexImage32 *load_tiff32(const char *fn, IMemAlloc *mem, bool *out_used_alpha = NULL)
{
  TIFF *image = TIFFOpen(fn, "rb");

  if (!image)
    return NULL;

  uint16 spp = 0, bpp = 0, format = 0;
  uint32 width = 1, height = 1;
  TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bpp);
  TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp);
  TIFFGetField(image, TIFFTAG_SAMPLEFORMAT, &format);

  if (format && (format != SAMPLEFORMAT_UINT && format != SAMPLEFORMAT_INT))
  {
    logerr("Invalid TIFF format, only 'integer' type is supported");
    TIFFClose(image);
    return NULL;
  }

  if (bpp != 1 && bpp != 8 && bpp != 16)
  {
    logerr("Invalid TIFF format, only 1, 8 and 16-bit is supported");
    TIFFClose(image);
    return NULL;
  }

  if (spp != 1 && spp != 3 && spp != 4)
  {
    logerr("Invalid TIFF format, only 1, 3 and 4 channels supported");
    TIFFClose(image);
    return NULL;
  }


  TexImage32 *im = (TexImage32 *)mem->tryAlloc(sizeof(TexImage32) + width * height * 4);
  if (!im)
  {
    TIFFClose(image);
    return NULL;
  }

  TexPixel32 *ptr = (TexPixel32 *)(im + 1);
  int res = TIFFReadRGBAImageOriented(image, width, height, (uint32_t *)ptr, ORIENTATION_TOPLEFT, 1);

  if (!res)
  {
    mem->free(im);
    TIFFClose(image);
    return NULL;
  }

  int pixelCount = width * height;
  uint8_t *bytePtr = (uint8_t *)ptr;
  for (int i = 0; i < pixelCount; i++)
  {
    uint8_t tmp = bytePtr[0];
    bytePtr[0] = bytePtr[2];
    bytePtr[2] = tmp;
    bytePtr += 4;
  }

  if (out_used_alpha)
    *out_used_alpha = (spp == 4);

  TIFFClose(image);

  im->w = width;
  im->h = height;

  return im;
}


class TiffLoadImageFactory : public ILoadImageFactory
{
  bool checkTiffExt(const char *fn_ext) { return fn_ext && (dd_stricmp(fn_ext, ".tif") == 0 || dd_stricmp(fn_ext, ".tiff") == 0); }

public:
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!checkTiffExt(fn_ext))
      return NULL;
    return load_tiff32(fn, mem, out_used_alpha);
  }

  virtual TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!checkTiffExt(fn_ext))
      return NULL;
    return load_tiff32(crd, mem, out_used_alpha);
  }

  virtual bool supportLoadImage2() { return false; }
  virtual void *loadImage2(const char *, IAllocImg &, const char *) { return NULL; }
};


static TiffLoadImageFactory tiff_load_image_factory;

void register_tiff_tex_load_factory() { add_load_image_factory(&tiff_load_image_factory); }
