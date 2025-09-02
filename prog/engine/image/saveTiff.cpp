// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_tiff.h>
#include <image/dag_texPixel.h>
#include <image/tiff-4.4.0/tiff.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <string.h>

static tsize_t tiff_ReadProc(thandle_t h, void *buf, tsize_t size) { return df_read((file_ptr_t)h, buf, size); }
static tsize_t tiff_WriteProc(thandle_t h, void *buf, tsize_t size) { return df_write((file_ptr_t)h, buf, size); }
static toff_t tiff_SeekProc(thandle_t h, toff_t off, int whence)
{
  int base = 0;
  if (whence == SEEK_END)
  {
    df_flush((file_ptr_t)h);
    base = df_length((file_ptr_t)h);
  }
  else if (whence == SEEK_CUR)
    base = df_tell((file_ptr_t)h);
  return df_seek_to((file_ptr_t)h, base + off) == 0 ? base + off : -1;
}
static toff_t tiff_SizeProc(thandle_t h) { return df_length((file_ptr_t)h); }
static int tiff_CloseProc(thandle_t h) { return df_close((file_ptr_t)h); }
static int tiff_nullMapProc(thandle_t, void **, toff_t *)
{
  G_ASSERT(0);
  return 0;
}
static void tiff_nullUnmapProc(thandle_t, void *, toff_t) { G_ASSERT(0); }

static bool save_tiff(const char *fn, const TexPixel32 *img, int width, int height, int chan_cnt, int stride, unsigned char *app_data,
  unsigned int app_data_len)
{
  G_ASSERTF_RETURN(chan_cnt == 3 || chan_cnt == 4, false, "%s: unsupported chan_cnt=%d", fn, chan_cnt);
  file_ptr_t fp = df_open(fn, DF_CREATE | DF_WRITE);
  TIFF *tif = TIFFClientOpen(fn, "w", reinterpret_cast<thandle_t>(fp), &tiff_ReadProc, &tiff_WriteProc, &tiff_SeekProc,
    &tiff_CloseProc, &tiff_SizeProc, &tiff_nullMapProc, &tiff_nullUnmapProc);

  if (!tif)
  {
    logerr("Could create TIFF <%s> writing", fn);
    return false;
  }

  int rows = height;
  while (rows > 4 && int64_t(width) * rows * chan_cnt > (128 << 10))
    rows /= 2;

  // We need to set some values for basic tags before we can add any data
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, chan_cnt);
  if (chan_cnt > 3)
  {
    uint16_t sampleinfo[16];
    for (int i = 0; i < chan_cnt - 3; i++) //-V1008
      sampleinfo[i] = EXTRASAMPLE_UNSPECIFIED;
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, chan_cnt - 3, sampleinfo);
  }
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
  TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);

  TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField(tif, TIFFTAG_XRESOLUTION, 72.0);
  TIFFSetField(tif, TIFFTAG_YRESOLUTION, 72.0);
  TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
  if (app_data && app_data_len)
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, String(0, "%.*s", app_data_len, app_data).str());

  // Write the information to the file
  int ht = height, strip = 0, y = 0;
  const int strideInPixels = stride / sizeof(TexPixel32);
  char *buf = new char[width * chan_cnt * rows + 1];
  while (ht > 0)
  {
    int h = ht > rows ? rows : ht;
    const TexPixel32 *pix = img + y * strideInPixels;
    int pixOffset = 0;
    for (char *dst = buf, *dst_e = dst + width * h * chan_cnt; dst < dst_e; dst += chan_cnt)
    {
      dst[0] = pix[pixOffset].r;
      dst[1] = pix[pixOffset].g;
      dst[2] = pix[pixOffset].b;
      if (chan_cnt > 3)
        dst[3] = pix[pixOffset].a;

      if (pixOffset == width - 1)
      {
        pix += strideInPixels;
        pixOffset = 0;
      }
      else
      {
        ++pixOffset;
      }
    }

    TIFFWriteEncodedStrip(tif, strip, buf, width * h * chan_cnt);
    ht -= h;
    y += h;
    strip++;
  }
  delete[] buf;

  // Close the file
  TIFFClose(tif);
  return true;
}

bool save_tiff32(const char *fn, const TexPixel32 *img, int w, int h, int stride, unsigned char *app_data, unsigned int app_data_len)
{
  return save_tiff(fn, img, w, h, 4, stride, app_data, app_data_len);
}

bool save_tiff24(const char *fn, const TexPixel32 *img, int w, int h, int stride, unsigned char *app_data, unsigned int app_data_len)
{
  return save_tiff(fn, img, w, h, 3, stride, app_data, app_data_len);
}

bool save_tiff32(const char *fn, TexImage32 *img, unsigned char *app_data, unsigned int app_data_len)
{
  return save_tiff(fn, img->getPixels(), img->w, img->h, 4, img->w * 4, app_data, app_data_len);
}
bool save_tiff24(const char *fn, TexImage32 *img, unsigned char *app_data, unsigned int app_data_len)
{
  return save_tiff(fn, img->getPixels(), img->w, img->h, 3, img->w * 4, app_data, app_data_len);
}
