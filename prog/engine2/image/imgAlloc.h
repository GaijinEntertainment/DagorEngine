#pragma once

#include <image/dag_loadImage.h>

struct TexImage32Alloc : public IAllocImg
{
  TexImage32Alloc(IMemAlloc *a) : mem(a) {}

  virtual void *allocImg32(int w, int h, TexPixel32 **out_data, int *out_stride)
  {
    TexImage32 *im = TexImage32::create(w, h, mem);
    if (out_data)
      *out_data = im->getPixels();
    if (out_stride)
      *out_stride = w * sizeof(TexPixel32);
    return im;
  }
  virtual void finishImg32Fill(void *, TexPixel32 *) {}
  virtual void freeImg32(void *img) { delete (TexImage32 *)img; }

  IMemAlloc *mem;
};
