// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ioSys/dag_fileIo.h>
#include <debug/dag_log.h>
#include <image/dag_texPixel.h>
#include <image/dag_jpeg.h>
#include "jpgCommon.h"

METHODDEF(void) dagor_jpeg_error_exit(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message)(cinfo, buffer);
  logwarn("jpeg_write: %s", buffer);
  my_error_ptr myerr = (my_error_ptr)cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}

bool save_jpeg32(unsigned char *ptr, int width, int height, int stride, int quality, IGenSave &cwr, const char *comments)
{
  JSAMPROW row_pointer[1];
  int row_stride;
  struct jpeg_compress_struct cinfo;
  struct dagor_jpeg_error_mgr jerr;

  unsigned char *buf = (unsigned char *)memalloc(width * 3, tmpmem);
  if (!buf)
    return false;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = dagor_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_compress(&cinfo);
    // status=1
    return false;
  }

  jpeg_create_compress(&cinfo);

  //-- Specify data destination for compression ----------------------------

  jpeg_stream_dest(&cinfo, cwr);

  //-- Initialize JPEG parameters ------------------------------------------

  cinfo.in_color_space = JCS_RGB;
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = 3;

  jpeg_set_defaults(&cinfo);

  cinfo.data_precision = 8;
  cinfo.arith_code = FALSE;
  cinfo.optimize_coding = FALSE;
  // cinfo.optimize_coding  = TRUE;
  cinfo.CCIR601_sampling = FALSE;
  // cinfo.smoothing_factor = smoothingfactor;

  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_default_colorspace(&cinfo);

  //-- Start compressor

  jpeg_start_compress(&cinfo, TRUE);

  //-- Write comments to JPEG
  if (comments)
  {
    int len = (int)strlen(comments);
    const int maxLen = 65000;
    if (len > maxLen)
    {
      logwarn("save_jpeg32: comments was too long (%d), cut to %d symbols", len, maxLen);
      len = maxLen;
    }
    jpeg_write_marker(&cinfo, JPEG_COM, (const JOCTET *)comments, len);
  }

  //-- Process data

  row_pointer[0] = buf;
  row_stride = width * 3;
  while (cinfo.next_scanline < cinfo.image_height)
  {
    {
      TexPixel32 *pix = (TexPixel32 *)ptr;
      for (int bi = 0, i = 0; i < width; ++i, ++pix)
      {
        buf[bi++] = pix->r;
        buf[bi++] = pix->g;
        buf[bi++] = pix->b;
      }
    }
    ptr += stride;
    (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  memfree(buf, tmpmem);

  //-- Finish compression and release memory

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  //-- Status
  return true;
}

bool save_jpeg32(struct TexPixel32 *pix, int w, int h, int stride, int quality, const char *fn, const char *comments)
{
  FullFileSaveCB cwr(fn);
  if (!cwr.fileHandle)
    return false;
  return save_jpeg32((unsigned char *)pix, w, h, stride, quality, cwr, comments);
}

bool save_jpeg32(TexImage32 *im, int quality, const char *fn, const char *comments)
{
  if (!im)
    return false;
  return save_jpeg32((TexPixel32 *)(im + 1), im->w, im->h, im->w * 4, quality, fn, comments);
}
