// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_psd.h>
#include <image/dag_texPixel.h>
#include <image/psdRead/bmgImage.h>
#include <ioSys/dag_fileIo.h>

IMemAlloc *bmg_cur_mem = midmem_ptr();

TexImage32 *load_psd32(const char *fn, IMemAlloc *mem, bool *out_used_alpha = NULL)
{
  FullFileLoadCB crd(fn);
  if (crd.fileHandle)
    return load_psd32(crd, mem, out_used_alpha);
  return NULL;
}
TexImage32 *load_psd32(IGenLoad &crd, IMemAlloc *mem, bool *out_used_alpha = NULL)
{
  BMGImageStruct img;

  bmg_cur_mem = mem ? mem : tmpmem;

  ::InitBMGImage(&img);

  if (::ReadPSD(crd, &img) != BMG_OK)
  {
    FreeBMGImage(&img);
    return NULL;
  }

  TexImage32 *im = (TexImage32 *)(img.bits - 4);
  img.bits = NULL;
  im->w = img.width;
  im->h = img.height;
  if (out_used_alpha)
    *out_used_alpha = (img.bits_per_pixel == 32);

  FreeBMGImage(&img);

  return im;
}
