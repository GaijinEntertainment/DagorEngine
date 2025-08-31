// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_psd.h>
#include <image/dag_texPixel.h>
#include <image/psdRead/bmgImage.h>
#include <ioSys/dag_fileIo.h>

IMemAlloc *bmg_cur_mem = midmem_ptr();

bool read_psd32_dimensions(const char *fn, int &out_w, int &out_h, bool &out_may_have_alpha)
{
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    return false;
  bool ret = false;
  BMGImageStruct img;
  ::InitBMGImage(&img);
  if (::ReadPSD(crd, &img, false) == BMG_OK)
  {
    out_w = img.width;
    out_h = img.height;
    out_may_have_alpha = (img.bits_per_pixel == 32);
    ret = true;
  }
  FreeBMGImage(&img);
  return ret;
}

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

  if (::ReadPSD(crd, &img, true) != BMG_OK)
  {
    FreeBMGImage(&img);
    return NULL;
  }

  TexImage32 *im = nullptr;
  if (img.bits_per_pixel == 32)
  {
    im = (TexImage32 *)(img.bits - 4);
    img.bits = NULL;
    im->w = img.width;
    im->h = img.height;
    if (out_used_alpha)
      *out_used_alpha = true;
  }
  else if (img.bits_per_pixel == 8 || img.bits_per_pixel == 16 || img.bits_per_pixel == 24)
  {
    im = TexImage32::tryCreate(img.width, img.height, mem);
    auto *bits = img.bits;
    if (im)
      switch (img.bits_per_pixel)
      {
        case 24:
          for (TexPixel32 *p = im->getPixels(), *pe = p + img.width * img.height; p < pe; p++, bits += 3)
            p->u = 0xFF000000 | unsigned(bits[0]) | (unsigned(bits[1]) << 8) | (unsigned(bits[2]) << 16);
          break;
        case 8:
          for (TexPixel32 *p = im->getPixels(), *pe = p + img.width * img.height; p < pe; p++, bits += 1)
            p->u = 0xFF000000 | unsigned(bits[0]);
          break;
        case 16:
          for (TexPixel32 *p = im->getPixels(), *pe = p + img.width * img.height; p < pe; p++, bits += 2)
            p->u = 0xFF000000 | unsigned(bits[0]) | (unsigned(bits[1]) << 8);
          break;
      }
    if (out_used_alpha)
      *out_used_alpha = false;
  }

  FreeBMGImage(&img);

  return im;
}
