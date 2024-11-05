// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_texPixel.h>

template <typename T>
static inline T *alloc_img(int w, int h, int m, IMemAlloc *mem, void *(IMemAlloc::*allocFn)(size_t) = &IMemAlloc::alloc)
{
  if (!mem)
    mem = tmpmem;
  T *im = (T *)(mem->*allocFn)(sizeof(TexImage) + w * h * m);
  if (!im)
    return nullptr;
  im->w = w;
  im->h = h;
  return im;
}

TexImage32 *TexImage32::create(int w, int h, IMemAlloc *mem) { return alloc_img<TexImage32>(w, h, 4, mem); }

TexImage32 *TexImage32::tryCreate(int w, int h, IMemAlloc *mem) { return alloc_img<TexImage32>(w, h, 4, mem, &IMemAlloc::tryAlloc); }

TexImage8a *TexImage8a::create(int w, int h, IMemAlloc *mem) { return alloc_img<TexImage8a>(w, h, 2, mem); }

TexImage8 *TexImage8::create(int w, int h, IMemAlloc *mem) { return alloc_img<TexImage8>(w, h, 1, mem); }

TexImageR *TexImageR::create(int w, int h, IMemAlloc *mem) { return alloc_img<TexImageR>(w, h, 4, mem); }

TexImageF *TexImageF::create(int w, int h, IMemAlloc *mem) { return alloc_img<TexImageF>(w, h, 4 * 3, mem); }
