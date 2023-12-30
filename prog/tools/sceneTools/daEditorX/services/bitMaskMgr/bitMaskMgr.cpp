#include <de3_bitMaskMgr.h>
#include <oldEditor/de_interface.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <image/dag_tga.h>
#include <image/dag_texPixel.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <hash/md5.h>

class TiffBitMaskImageMgr : public IBitMaskImageMgr
{
  struct PublicBitmapMask : public BitmapMask
  {
  public:
    void init()
    {
      w = h = 1;
      bpp = bmStride = 0;
      isImage = 0;
      tileSz = 1;
      wTiles = 1;
      memset(bitsTile, 0, sizeof(bitsTile));
    }
    void destroy()
    {
      dealloc();
      init();
    }
    void alloc(int _w, int _h, int _bpp, bool _is_image = false)
    {
      w = _w;
      h = _h;
      bpp = _bpp;
      isImage = _is_image;
      memset(bitsTile, 0, sizeof(bitsTile));

      if (isImage || (((int64_t(w) * h * bpp / 8) < (1 << 20)) && (w * bpp + 7) / 8 < 65536))
      {
      init_single_tile:
        G_ASSERTF(int64_t(w) * h < (64 << 20), "%dx%d bpp=%d", w, h, bpp);
        tileSz = 0;
        wTiles = 1;
        bmStride = !isImage ? (w * bpp + 7) / 8 : 0;
        int sz = bmStride ? bmStride * h : w * h * bpp / 8;
        bitsTile[0] = memalloc(sz, midmem);
        memset(bitsTile[0], 0, sz);
        // debug("alloc %dx%d bpp=%d sz=%d stride=%d", w, h, bpp, sz, bmStride);
        return;
      }

      tileSz = 2048;
      while (tileSz > 8 && (w <= tileSz / 2 || h <= tileSz / 2))
        tileSz /= 2;
      while (w > tileSz * 32 || h > tileSz * 32)
        tileSz *= 2;
      wTiles = (w + tileSz - 1) / tileSz;

      if (wTiles == 1 && w == tileSz && h == tileSz)
        goto init_single_tile;

      int stride = (tileSz * _bpp + 7) / 8;
      G_ASSERT(stride >= 0 && stride < 65536);
      bmStride = stride;
      debug("alloc %dx%d bpp=%d (tile=%d, tw=%d) stride=%d", w, h, bpp, tileSz, wTiles, bmStride);
    }
    void dealloc()
    {
      for (int i = 0; i < sizeof(bitsTile) / sizeof(bitsTile[0]); i++)
        if (bitsTile[i])
          memfree(bitsTile[i], midmem);
      memset(bitsTile, 0, sizeof(bitsTile));
    }

    void clear()
    {
      if (tileSz)
        dealloc();
      else
        memset(bitsTile[0], 0, w * h * bpp / 8);
    }

    void *singleTile() { return tileSz ? NULL : bitsTile[0]; }
    void copyFrom(const char *buf, int y0, int rows)
    {
      int linew = (w * bpp + 7) / 8;
      if (!tileSz)
      {
        memcpy((char *)bitsTile[0] + y0 * linew, buf, linew * rows);
        return;
      }

      int tilew = (tileSz * bpp + 7) / 8;
      for (; rows > 0; y0++, rows--, buf += linew)
      {
        int y_tile = y0 / tileSz;
        int y = y0 - y_tile * tileSz;
        for (int x_tile = 0; x_tile < wTiles; x_tile++)
        {
          int w_tile = w - x_tile * tileSz;
          if (w_tile > tileSz)
            w_tile = tileSz;
          w_tile = (w_tile * bpp + 7) / 8;

          int t_idx = x_tile + y_tile * wTiles;
          if (bitsTile[t_idx])
            memcpy((char *)bitsTile[t_idx] + y * tilew, buf + x_tile * tilew, w_tile);
          else
            for (const char *p = buf + x_tile * tilew, *pe = p + w_tile; p < pe; p++)
              if (*p)
              {
                int sz = !isImage ? tileSz * bmStride : tileSz * tileSz * bpp / 8;
                bitsTile[t_idx] = memalloc(sz, midmem);
                memset(bitsTile[t_idx], 0, sz);

                memcpy((char *)bitsTile[t_idx] + y * tilew, buf + x_tile * tilew, w_tile);
                break;
              }
        }
      }
    }
    void copyTo(char *buf, int y0, int rows) const
    {
      int linew = (w * bpp + 7) / 8;
      if (!tileSz)
      {
        memcpy(buf, (const char *)bitsTile[0] + y0 * linew, linew * rows);
        return;
      }

      int tilew = (tileSz * bpp + 7) / 8;
      for (; rows > 0; y0++, rows--, buf += linew)
      {
        int y_tile = y0 / tileSz;
        for (int x_tile = 0; x_tile < wTiles; x_tile++)
        {
          int w_tile = w - x_tile * tileSz;
          if (w_tile > tileSz)
            w_tile = tileSz;
          w_tile = (w_tile * bpp + 7) / 8;

          int t_idx = x_tile + y_tile * wTiles;
          int y = y0 - y_tile * tileSz;
          if (bitsTile[t_idx])
            memcpy(buf + x_tile * tilew, (const char *)bitsTile[t_idx] + y * tilew, w_tile);
          else
            memset(buf + x_tile * tilew, 0, w_tile);
        }
      }
    }
  };

public:
  virtual bool createImage32(BitmapMask &_img, int w, int h)
  {
    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    img.alloc(w, h, 32, true);
    return true;
  }
  virtual bool createBitMask(BitmapMask &_img, int w, int h, int bpp)
  {
    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    if (bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8 || bpp == 16 || bpp == 32)
    {
      img.alloc(w, h, bpp);
      return true;
    }
    img.init();
    return false;
  }
  virtual void destroyImage(BitmapMask &_img)
  {
    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    img.destroy();
  }

  virtual bool loadImage(BitmapMask &_img, const char *folder, const char *image_name)
  {
    String fname;
    makeFullPath(fname, folder, image_name, ".tif");

    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    img.init();

    TIFF *image;
    uint16_t photo, bps, spp, fillorder;
    uint32_t width, height;
    tsize_t stripSize;
    unsigned long result;
    int stripMax;
    char *buffer, *buffer_end, tempbyte;

    // debug("reading image <%s>", (char*)fname);
    //  Open the TIFF image
    if ((image = TIFFOpen(fname, "r")) == NULL)
    {
      debug("Could not open image <%s>", (char *)fname);

      // try load .tga, if exist
      makeFullPath(fname, folder, image_name, ".tga");
      TexImage32 *im = ::load_tga32(fname, tmpmem);
      if (!im)
      {
        debug("  Could not open image <%s>", (char *)fname);
        return false;
      }
      img.alloc(im->w, im->h, 32, true);
      if (img.singleTile())
        memcpy(img.singleTile(), (im + 1), im->w * im->h * 4);
      else
      {
        logerr("Could not initialize image <%s>", (char *)fname);
        memfree(im, tmpmem);
        return false;
      }
      debug("loaded %dx%dx%dbit from %s", img.getWidth(), img.getHeight(), img.getBitsPerPixel(), (char *)fname);
      memfree(im, tmpmem);
      return true;
    }

    // Do whatever it is we do with the buffer -- we dump it in hex
    if (TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0)
    {
      debug("Image does not define its width");
      TIFFClose(image);
      return false;
    }
    if (TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height) == 0)
    {
      debug("Image does not define its height");
      TIFFClose(image);
      return false;
    }

    // Check that it is of a type that we support
    if (TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0)
    {
      debug("undefined number of bits per sample");
      TIFFClose(image);
      return false;
    }
    if (bps != 1 && bps != 4 && bps != 8 && bps != 16 && bps != 32)
    {
      debug("unsupported number of bits per sample: %d", bps);
      TIFFClose(image);
      return false;
    }

    if (TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0)
    {
      debug("undefined number of samples per pixel");
      TIFFClose(image);
      return false;
    }

    if (spp != 1 && spp != 3 && spp != 4)
    {
      debug("unsupported number of samples per pixel: %d", spp);
      TIFFClose(image);
      return false;
    }

    // Deal with photometric interpretations
    if (TIFFGetField(image, TIFFTAG_PHOTOMETRIC, &photo) == 0)
    {
      debug("Image has an undefined photometric interpretation");
      TIFFClose(image);
      return false;
    }

    // Deal with fillorder
    if (TIFFGetField(image, TIFFTAG_FILLORDER, &fillorder) == 0)
      fillorder = FILLORDER_MSB2LSB;

    if (spp == 3 && bps <= 8)
    {
      img.alloc(width, height, 32, true);
      TIFFReadRGBAImage(image, width, height, (unsigned *)img.singleTile(), 0);
      TIFFClose(image);
      return true;
    }

    img.alloc(width, height, bps * spp);

    // Read in the possibly multiple strips
    stripSize = TIFFStripSize(image);
    stripMax = TIFFNumberOfStrips(image);

    if (spp == 1 && photo != PHOTOMETRIC_MINISBLACK)
      debug("Fixing the photometric interpretation");
    if (fillorder != FILLORDER_MSB2LSB)
      debug("Fixing the fillorder");

    // debug("width=%d bps=%d spp=%d stripSize=%d", width, bps, spp, stripSize);
    buffer = new char[stripSize];
    for (int stripCount = 0, img_y = 0; stripCount < stripMax; stripCount++)
    {
      if ((result = TIFFReadEncodedStrip(image, stripCount, buffer, stripSize)) == -1)
      {
        debug("Read error on input strip number %d", stripCount);
        img.destroy();
        TIFFClose(image);
        delete[] buffer;
        return false;
      }
      if (spp == 1 && photo != PHOTOMETRIC_MINISBLACK) // Flip bits
        for (char *b = buffer, *be = b + result; b < be; b++)
          *b = ~*b;
      if (fillorder != FILLORDER_MSB2LSB) // We need to swap bits -- ABCDEFGH becomes HGFEDCBA
        for (char *b = buffer, *be = b + result; b < be; b++)
        {
          tempbyte = 0;
          if (*buffer & 128)
            tempbyte += 1;
          if (*buffer & 64)
            tempbyte += 2;
          if (*buffer & 32)
            tempbyte += 4;
          if (*buffer & 16)
            tempbyte += 8;
          if (*buffer & 8)
            tempbyte += 16;
          if (*buffer & 4)
            tempbyte += 32;
          if (*buffer & 2)
            tempbyte += 64;
          if (*buffer & 1)
            tempbyte += 128;
          *buffer = tempbyte;
        }

      int rows = result / (width * bps * spp / 8);
      G_ASSERT(result = rows * (width * bps * spp / 8));

      G_ASSERT(img_y + rows <= img.getHeight());
      img.copyFrom(buffer, img_y, rows);
      img_y += rows;
    }
    delete[] buffer;

    TIFFClose(image);

    debug("loaded %dx%dx%dbit from %s", img.getWidth(), img.getHeight(), img.getBitsPerPixel(), (char *)fname);
    return true;
  }
  virtual bool getBitMaskProps(const char *folder, const char *image_name, int &out_w, int &out_h, int &out_bpp)
  {
    String fname;
    makeFullPath(fname, folder, image_name, ".tif");

    TIFF *image;
    uint16_t photo, bps, spp, fillorder;
    tsize_t stripSize;
    unsigned long result;
    int stripMax;
    char *buffer, *buffer_end, tempbyte;

    if ((image = TIFFOpen(fname, "r")) == NULL)
      return false;

    if (TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &out_w) == 0 || TIFFGetField(image, TIFFTAG_IMAGELENGTH, &out_h) == 0 ||
        TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0 || TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0)
    {
      TIFFClose(image);
      return false;
    }
    out_bpp = spp * bps;
    return true;
  }
  virtual bool checkBitMask(const char *folder, const char *image_name, int w, int h, int bpp)
  {
    String fname;
    makeFullPath(fname, folder, image_name, ".tif");

    TIFF *image;
    uint16_t photo, bps, spp, fillorder;
    uint32_t width, height;
    tsize_t stripSize;
    unsigned long result;
    int stripMax;
    char *buffer, *buffer_end, tempbyte;

    if ((image = TIFFOpen(fname, "r")) == NULL)
      return false;

    if (TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0 || TIFFGetField(image, TIFFTAG_IMAGELENGTH, &height) == 0 ||
        TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0 || TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0)
    {
      TIFFClose(image);
      return false;
    }

    if ((bps != 1 && bps != 4 && bps != 8 && bps != 16 && bps != 32) || (spp != 1 && spp != 3 && spp != 4) || width != w ||
        height != h || bpp != spp * bps)
    {
      TIFFClose(image);
      return false;
    }
    TIFFClose(image);
    debug("%s confirmed: %dx%d, %d bpp", fname.str(), w, h, bpp);
    return true;
  }

  virtual bool saveImage(BitmapMask &_img, const char *folder, const char *image_name)
  {
    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    String fname;
    makeFullPath(fname, folder, image_name, ".tif");
    TIFF *image;

    // Open the TIFF file
    if ((image = TIFFOpen(fname, "w")) == NULL)
    {
      debug("Could not open <%s> writing", (char *)fname);
      return false;
    }

    int rows = img.getHeight();
    while (rows > 4 && int64_t(rows) * img.getWidth() * img.getBitsPerPixel() / 8 > 128 << 10)
      rows /= 2;

    // We need to set some values for basic tags before we can add any data
    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, img.getWidth());
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, img.getHeight());
    if (img.isImage32())
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
      case 16:
      case 32: TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW); break;

      default: return false;
    }

    TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    TIFFSetField(image, TIFFTAG_XRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_YRESOLUTION, 72.0);
    TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    // Write the information to the file
    int ht = img.getHeight(), strip = 0, y = 0;
    char *buf = new char[img.getWidth() * img.getBitsPerPixel() / 8 * rows];
    while (ht > 0)
    {
      int h = ht > rows ? rows : ht;
      img.copyTo(buf, y, h);
      TIFFWriteEncodedStrip(image, strip, buf, img.getWidth() * h * img.getBitsPerPixel() / 8);
      ht -= h;
      y += h;
      strip++;
    }
    delete[] buf;

    // Close the file
    TIFFClose(image);
    debug("saved %dx%dx%dbit to %s (%d rows/strip)", img.getWidth(), img.getHeight(), img.getBitsPerPixel(), (char *)fname, rows);
    return true;
  }

  virtual bool resampleBitMask(BitmapMask &img, int new_w, int new_h, int new_bpp)
  {
    int bpp = img.getBitsPerPixel();

    if (img.isImage32())
    {
      logerr("can't resample non-bitmask image");
      return false;
    }

    if (new_bpp != 8 && new_bpp != 1)
    {
      logerr("can't resample to %dbpp", new_bpp);
      return false;
    }

    int w = img.getWidth();
    int h = img.getHeight();

    if (w == new_w && h == new_h && bpp == new_bpp)
      return false;

    BitmapMask nimg;
    createBitMask(nimg, new_w, new_h, new_bpp);

    for (int j = 0; j < new_h; j++)
      for (int i = 0; i < new_w; i++)
      {
        float fx = (float)i / (float)new_w;
        float fy = (float)j / (float)new_h;

        unsigned char out = samplePixel(img, fx, fy);

        if (new_bpp == 1)
          nimg.setMaskPixel1(i, j, out);
        else if (new_bpp == 8)
          nimg.setMaskPixel8(i, j, out);
      }

    destroyImage(img);
    memcpy(&img, &nimg, sizeof(BitmapMask));

    return true;
  }
  virtual void clearBitMask(BitmapMask &_img)
  {
    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    img.clear();
  }
  virtual void calcBitMaskMD5(BitmapMask &_img, void *md5_state)
  {
    PublicBitmapMask &img = static_cast<PublicBitmapMask &>(_img);
    int linew = (img.getWidth() * img.getBitsPerPixel() + 7) / 8;
    char *buf = new char[linew];
    for (int i = 0; i < img.getHeight(); i++)
    {
      img.copyTo(buf, i, 1);
      md5_append((md5_state_t *)md5_state, (unsigned char *)buf, linew);
    }
    delete[] buf;
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

    float c00 = 0, c01 = 0, c10 = 0, c11 = 0;
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

  void makeFullPath(String &fname, const char *folder, const char *image_name, const char *r_ext)
  {
    if (folder)
    {
      if (DAGORED2)
      {
        String base;
        DAGORED2->getProjectFolderPath(base);
        fname.printf(260, "%s/%s/", base.str(), folder);
      }
      else
        fname.printf(260, "%s/", folder);
    }
    else
    {
      if (strchr(image_name, ':') || image_name[0] == '/')
        fname = "";
      else
        fname.printf(260, "%s/", DAGORED2 ? DAGORED2->getSdkDir() : ".");
    }

    const char *ext = dd_get_fname_ext(image_name);
    if (ext && stricmp(ext, r_ext) == 0)
      fname += image_name;
    else if (ext)
      fname.aprintf(260, "%.*s%s", ext - image_name, image_name, r_ext);
    else
      fname.aprintf(260, "%s%s", image_name, r_ext);
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

void *get_tiff_bit_mask_image_mgr()
{
  TIFFSetWarningHandler(tiff_warn_handler);
  TIFFSetErrorHandler(tiff_err_handler);
  return &mgr;
}
