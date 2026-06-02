// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "image_export.h"
#include <de3_interface.h>
#include <debug/dag_debug.h>
#include <image/dag_tga.h>
#include <drv/3d/dag_tex3d.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <math/dag_half.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_files.h>

String export_any_tga(const char *file_name, void *data, int width, int height, int /*stride*/, int in_channels,
  int in_bits_per_channel, bool in_float, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb)
{
  if ((out_channels != 1 && out_channels != 4) || out_bits_per_channel != 8 || out_float)
  {
    return String(0, "Cannot export file '%s', invalid output format. Expected ARGB8.", file_name);
  }

  int in_stride = width * in_channels * in_bits_per_channel / 8;
  int out_stride = width * out_channels * out_bits_per_channel / 8;
  if (out_stride == 0)
  {
    return String(0, "Cannot export file '%s', invalid image size", file_name);
  }

  Tab<char> buf;
  buf.resize(out_stride * height);
  for (int i = 0; i < height; i++)
  {
    if (!convert_image_line((char *)data + in_stride * i, width, in_channels, in_bits_per_channel, in_float,
          buf.data() + out_stride * i, out_channels, out_bits_per_channel, out_float, swap_rb, false))
    {
      return String(0, "internal error in convert_image_line");
    }
  }

  if (!save_tga32(file_name, (TexPixel32 *)buf.data(), width, height, out_stride))
  {
    return String(0, "cannot export file '%s'", file_name);
  }

  return String(0, "");
}


String export_any_raw(const char *file_name, void *data, int width, int height, int /*stride*/, int in_channels,
  int in_bits_per_channel, bool in_float, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb)
{
  int in_stride = width * in_channels * in_bits_per_channel / 8;
  int out_stride = width * out_channels * out_bits_per_channel / 8;
  if (out_stride == 0)
  {
    return String(0, "cannot export file '%s', invalid image size", file_name);
  }

  Tab<char> buf;
  buf.resize(out_stride * height);
  for (int i = 0; i < height; i++)
  {
    if (!convert_image_line((char *)data + in_stride * i, width, in_channels, in_bits_per_channel, in_float,
          buf.data() + out_stride * i, out_channels, out_bits_per_channel, out_float, swap_rb, false))
    {
      return String(0, "internal error in convert_image_line");
    }
  }

  file_ptr_t h = df_open(file_name, DF_WRITE | DF_CREATE);
  if (!h)
  {
    return String(0, "cannot create file '%s'", file_name);
  }

  if (df_write(h, buf.data(), out_stride * height) < 0)
  {
    return String(0, "cannot write to file '%s'", file_name);
  }

  df_close(h);

  return String(0, "");
}


static void tiff_warning_handler(const char * /*module*/, const char *fmt, va_list /*ap*/)
{
  DAEDITOR3.conWarning("TIFF Warning: %s", fmt);
}

static void tiff_error_handler(const char * /*module*/, const char *fmt, va_list /*ap*/) { DAEDITOR3.conError("TIFF Error: %s", fmt); }


String export_any_tiff(const char *file_name, void *data, int width, int height, int stride, int in_channels, int in_bits_per_channel,
  bool in_float, int out_channels, int out_bits_per_channel, bool out_float, bool swap_rb)
{
  int in_stride = stride; // width * in_channels * in_bits_per_channel / 8;
  int out_stride = width * out_channels * out_bits_per_channel / 8;
  if (out_stride == 0 || (out_stride & 3) != 0)
  {
    return String(0, "cannot export file '%s', invalid image size", file_name);
  }

  TIFFSetWarningHandler(tiff_warning_handler);
  TIFFSetErrorHandler(tiff_error_handler);

  TIFF *image = NULL;
  if ((image = TIFFOpen(file_name, "w")) == NULL)
  {
    return String(0, "cannot open file '%s' for write", file_name);
  }

  TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
  TIFFSetField(image, TIFFTAG_IMAGELENGTH, height);
  TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, out_bits_per_channel);
  TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, out_channels);
  TIFFSetField(image, TIFFTAG_PHOTOMETRIC, (out_channels <= 2) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);

  if (out_bits_per_channel == 1)
  {
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4);
  }
  else if (!out_float)
  {
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(image, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
  }
  else
  {
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    // TIFFSetField(image, TIFFTAG_PREDICTOR, PREDICTOR_FLOATINGPOINT);
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
  }


  if (out_channels == 4)
  {
    uint16_t out[1] = {EXTRASAMPLE_UNSPECIFIED};
    TIFFSetField(image, TIFFTAG_EXTRASAMPLES, 1, &out);
  }

  TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);

  TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
  TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField(image, TIFFTAG_XRESOLUTION, 72.0);
  TIFFSetField(image, TIFFTAG_YRESOLUTION, 72.0);
  TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);


  Tab<char> buf;
  buf.resize(out_stride);

  String error;

  for (int strip = 0; strip < height; strip++)
  {
    if (!convert_image_line((char *)data + strip * in_stride, width, in_channels, in_bits_per_channel, in_float, buf.data(),
          out_channels, out_bits_per_channel, out_float, swap_rb, false))
    {
      error = "internal error in convert_image_line";
      break;
    }

    if (TIFFWriteEncodedStrip(image, strip, (tdata_t)buf.data(), out_stride) < 0)
    {
      error = "failed to write tiff file";
      break;
    }
  }

  TIFFClose(image);

  return error;
}
