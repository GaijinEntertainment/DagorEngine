// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "decodeFnameSuffix.h"
#include <image/dag_texPixel.h>

TexImage32 *resample_img(TexImage32 *img, int pic_w, int pic_h, bool keep_ar, IMemAlloc *mem);

inline TexImage32 *resample_img_p(TexImage32 *img, int pic_w, int pic_h, bool keep_ar, bool premul, IMemAlloc *mem)
{
  img = resample_img(img, pic_w, pic_h, keep_ar, mem);
  if (premul)
    for (TexPixel32 *p = img->getPixels(), *pe = p + img->w * img->h; p < pe; p++)
    {
      p->r = unsigned(p->r) * p->a / 255;
      p->g = unsigned(p->g) * p->a / 255;
      p->b = unsigned(p->b) * p->a / 255;
      // p->a  is left intact
    }
  return img;
}
