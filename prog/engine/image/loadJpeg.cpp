// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdlib.h>
#include <stdio.h>
#include <ioSys/dag_fileIo.h>
#include <string.h>
#include <image/dag_texPixel.h>
#include <image/dag_jpeg.h>
#include "imgAlloc.h"
#include <util/dag_stdint.h>
#include "jpgCommon.h"

METHODDEF(void) dagor_jpeg_error_exit(j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr)cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}


bool read_jpeg32_dimensions(const char *fn, int &out_w, int &out_h)
{
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    return false;
  struct jpeg_decompress_struct cinfo;
  struct dagor_jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = dagor_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stream_src(&cinfo, crd);

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  out_w = cinfo.output_width;
  out_h = cinfo.output_height;

  jpeg_abort_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  return true;
}

TexImage32 *load_jpeg32(IGenLoad &crd, IMemAlloc *mem, eastl::string *comments)
{
  TexImage32Alloc a(mem);
  return (TexImage32 *)load_jpeg32(crd, a, comments);
}

void *load_jpeg32(IGenLoad &crd, IAllocImg &a, eastl::string *comments)
{
  struct jpeg_decompress_struct cinfo;
  struct dagor_jpeg_error_mgr jerr;

  unsigned char *bf;
  JSAMPARRAY buffer;
  int row_stride, x;
  void *im;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = dagor_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stream_src(&cinfo, crd);

  if (comments)
    jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);

  jpeg_read_header(&cinfo, TRUE);

  if (comments && cinfo.marker_list && cinfo.marker_list->data)
  {
    comments->clear();
    comments->append((const char *)cinfo.marker_list->data, cinfo.marker_list->data_length);
  }

  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * 4;

  buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

  TexPixel32 *pix;
  int im_w = cinfo.output_width, im_h = cinfo.output_height, im_stride;
  im = a.allocImg32(im_w, im_h, &pix, &im_stride);
  if (!im)
  {
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  bf = (unsigned char *)pix;

  while (cinfo.output_scanline < cinfo.output_height)
  {
    unsigned char *p;
    jpeg_read_scanlines(&cinfo, buffer, 1);
    p = buffer[0];
    switch (cinfo.num_components)
    {
      case 3:
        for (x = 0; x < im_w; ++x, p += 3)
        {
#if _TARGET_CPU_BE
          *bf++ = 255;
          *bf++ = p[0];
          *bf++ = p[1];
          *bf++ = p[2];
#else
          *bf++ = p[2];
          *bf++ = p[1];
          *bf++ = p[0];
          *bf++ = 255;
#endif
        }
        break;
      case 4:
        for (x = 0; x < im_w; ++x, p += 4)
        {
#if _TARGET_CPU_BE
          *bf++ = p[3];
          *bf++ = p[0];
          *bf++ = p[1];
          *bf++ = p[2];
#else
          *bf++ = p[2];
          *bf++ = p[1];
          *bf++ = p[0];
          *bf++ = p[3];
#endif
        }
        break;
      case 1:
        for (x = 0; x < im_w; ++x, ++p)
        {
#if _TARGET_CPU_BE
          *bf++ = 255;
          *bf++ = p[0];
          *bf++ = p[0];
          *bf++ = p[0];
#else
          *bf++ = p[0];
          *bf++ = p[0];
          *bf++ = p[0];
          *bf++ = 255;
#endif
        }
        break;
    }
    bf += im_stride - im_w * 4;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  a.finishImg32Fill(im, pix);

  return im;
}

TexImage8 *load_jpeg8(IGenLoad &crd, IMemAlloc *mem)
{
  struct jpeg_decompress_struct cinfo;
  struct dagor_jpeg_error_mgr jerr;

  unsigned char *bf;
  JSAMPARRAY buffer;
  int row_stride, x;
  TexImage8 *im;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = dagor_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stream_src(&cinfo, crd);

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * 4;

  buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

  im = (TexImage8 *)memalloc(cinfo.output_width * cinfo.output_height * 1 + sizeof(TexImage8), mem);
  if (!im)
  {
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  im->w = cinfo.output_width;
  im->h = cinfo.output_height;

  bf = (unsigned char *)(im + 1);

  while (cinfo.output_scanline < cinfo.output_height)
  {
    unsigned char *p;
    jpeg_read_scanlines(&cinfo, buffer, 1);
    p = buffer[0];
    switch (cinfo.num_components)
    {
      case 3:
        for (x = 0; x < im->w; ++x, p += 3)
        {
          *bf++ = (p[0] + p[1] + p[2]) / 3;
        }
        break;
      case 4:
        for (x = 0; x < im->w; ++x, p += 4)
        {
          *bf++ = (p[0] + p[1] + p[2]) / 3;
        }
        break;
      case 1:
        for (x = 0; x < im->w; ++x, ++p)
        {
          *bf++ = p[0];
        }
        break;
    }
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return im;
}

TexImage8a *load_jpeg8a(IGenLoad &crd, IMemAlloc *mem)
{
  struct jpeg_decompress_struct cinfo;
  struct dagor_jpeg_error_mgr jerr;

  unsigned char *bf;
  JSAMPARRAY buffer;
  int row_stride, x;
  TexImage8a *im;

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = dagor_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stream_src(&cinfo, crd);

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * 4;

  buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

  im = (TexImage8a *)memalloc(cinfo.output_width * cinfo.output_height * 2 + sizeof(TexImage8a), mem);
  if (!im)
  {
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  im->w = cinfo.output_width;
  im->h = cinfo.output_height;

  bf = (unsigned char *)(im + 1);

  while (cinfo.output_scanline < cinfo.output_height)
  {
    unsigned char *p;
    jpeg_read_scanlines(&cinfo, buffer, 1);
    p = buffer[0];
    switch (cinfo.num_components)
    {
      case 3:
        for (x = 0; x < im->w; ++x, p += 3)
        {
          *bf++ = (p[0] + p[1] + p[2]) / 3;
          *bf++ = 255;
        }
        break;
      case 4:
        for (x = 0; x < im->w; ++x, p += 4)
        {
          *bf++ = (p[0] + p[1] + p[2]) / 3;
          *bf++ = p[3];
        }
        break;
      case 1:
        for (x = 0; x < im->w; ++x, ++p)
        {
          *bf++ = p[0];
          *bf++ = 255;
        }
        break;
    }
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return im;
}

TexImage32 *load_jpeg32(const char *fn, IMemAlloc *mem)
{
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    return NULL;
  return load_jpeg32(crd, mem);
}

TexImage8 *load_jpeg8(const char *fn, IMemAlloc *mem)
{
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    return NULL;
  return load_jpeg8(crd, mem);
}

TexImage8a *load_jpeg8a(const char *fn, IMemAlloc *mem)
{
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    return NULL;
  return load_jpeg8a(crd, mem);
}
