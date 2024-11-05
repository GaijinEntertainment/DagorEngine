// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mgr.h"
#include <image/tiff-4.4.0/tiffio.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>

class TiffBitMaskImageMgr : public IBitMaskImageMgr
{
  struct PublicBitmapMask : public BitmapMask
  {
  public:
    void init()
    {
      w = h = 1;
      bpp = bmStride = 0;
      bits = NULL;
    }
    void destroy()
    {
      if (bits)
        memfree(bits, midmem);
      init();
    }
    void alloc(int _w, int _h, int _bpp)
    {
      w = _w;
      h = _h;
      bpp = _bpp;
      bmStride = (bpp <= 8) ? (w * bpp + 7) / 8 : 0;
      if (bpp <= 8)
        bits = memalloc(h * bmStride, midmem);
      else
        bits = memalloc(w * h * bpp / 8, midmem);
    }
  };

public:
  virtual bool createImage32(BitmapMask &_img, int w, int h)
  {
    PublicBitmapMask &img = (PublicBitmapMask &)_img;
    img.alloc(w, h, 32);
    memset(img.getBits(), 0, img.getBitsSize());
    return true;
  }
  virtual bool createBitMask(BitmapMask &_img, int w, int h, int bpp)
  {
    PublicBitmapMask &img = (PublicBitmapMask &)_img;
    if (bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8)
    {
      img.alloc(w, h, bpp);
      memset(img.getBits(), 0, img.getBitsSize());
      return true;
    }
    img.init();
    return false;
  }
  virtual void destroyImage(BitmapMask &_img)
  {
    PublicBitmapMask &img = (PublicBitmapMask &)_img;
    img.destroy();
  }

  virtual bool saveImage(BitmapMask &img, const char *fname)
  {
    TIFF *image;

    // Open the TIFF file
    if ((image = TIFFOpen(fname, "w")) == NULL)
    {
      debug("Could not open <%s> writing", (char *)fname);
      return false;
    }

    int rows = img.getHeight();
    while (rows > 4 && rows * img.getWidth() * img.getBitsPerPixel() / 8 > 32768)
      rows /= 2;

    // We need to set some values for basic tags before we can add any data
    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, img.getWidth());
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, img.getHeight());
    if (img.getBitsPerPixel() == 32)
    {
      TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8);
      TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, img.getBitsPerPixel() / 8);
      TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    }
    else
    {
      TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, img.getBitsPerPixel());
      TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
      TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    }
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, rows);

    switch (img.getBitsPerPixel())
    {
      case 1: TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4); break;
      case 2:
      case 4: TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS); break;
      case 8:
      case 32: TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW); break;

      default: return false;
    }

    TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    TIFFSetField(image, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    // Write the information to the file
    int ht = img.getHeight(), strip = 0;
    char *buf = (char *)img.getBits();
    while (ht > 0)
    {
      int h = ht > rows ? rows : ht;
      TIFFWriteEncodedStrip(image, strip, buf, img.getWidth() * rows * img.getBitsPerPixel() / 8);
      ht -= h;
      buf += h * img.getWidth() * img.getBitsPerPixel() / 8;
      strip++;
    }

    // Close the file
    TIFFClose(image);
    debug("saved %dx%dx%dbit to %s", img.getWidth(), img.getHeight(), img.getBitsPerPixel(), (char *)fname);
    return true;
  }

  static unsigned char samplePixel(BitmapMask &img, float fx, float fy)
  {
    int ix, iy, nx, ny;

    fx *= img.getWidth();
    fy *= img.getHeight();

    ix = int(floorf(fx));
    iy = int(floorf(fy));

    fx -= ix;
    fy -= iy;

    if (ix < 0)
    {
      ix = 0;
      fx = 0;
    }
    else if (ix >= img.getWidth() - 1)
    {
      ix = img.getWidth() - 1;
      fx = 0;
    }

    if (iy < 0)
    {
      iy = 0;
      fy = 0;
    }
    else if (iy >= img.getHeight() - 1)
    {
      iy = img.getHeight() - 1;
      fy = 0;
    }

    nx = ix + 1;
    if (nx >= img.getWidth())
      nx = 0;
    ny = iy + 1;
    if (ny >= img.getHeight())
      ny = 0;

    int bpp = img.getBitsPerPixel();

    float c00, c01, c10, c11;
    if (bpp == 1)
    {
      c00 = img.getMaskPixel1(ix, iy) ? 1.0 : 0.0;
      c01 = img.getMaskPixel1(nx, iy) ? 1.0 : 0.0;
      c10 = img.getMaskPixel1(ix, ny) ? 1.0 : 0.0;
      c11 = img.getMaskPixel1(nx, ny) ? 1.0 : 0.0;
    }
    else if (bpp == 8)
    {
      c00 = img.getMaskPixel8(ix, iy) / 255.0;
      c01 = img.getMaskPixel8(nx, iy) / 255.0;
      c10 = img.getMaskPixel8(ix, ny) / 255.0;
      c11 = img.getMaskPixel8(nx, ny) / 255.0;
    }

    float out = (c00 * (1 - fx) + c01 * fx) * (1 - fy) + (c10 * (1 - fx) + c11 * fx) * fy;
    out *= 255.0;

    return (unsigned char)out;
  }
};

static TiffBitMaskImageMgr mgr;

static void tiff_warn_handler(const char *module, const char *fmt, va_list ap)
{
  String msg;
  if (module != NULL)
    msg.printf(128, "%s: ", module);
  msg.cvaprintf(256, fmt, ap);
  logwarn("%s", msg.str());
}

static void tiff_err_handler(const char *module, const char *fmt, va_list ap)
{
  String msg;
  if (module != NULL)
    msg.printf(128, "%s: ", module);
  msg.cvaprintf(256, fmt, ap);
  logerr("%s", msg.str());
}

IBitMaskImageMgr *get_tiff_bit_mask_image_mgr()
{
  TIFFSetWarningHandler(tiff_warn_handler);
  TIFFSetErrorHandler(tiff_err_handler);
  return &mgr;
}
