//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// forward declarations for external classes
struct TexImage32;
struct TexPixel32;
class IGenLoad;


struct IAllocImg
{
  virtual void *allocImg32(int w, int h, TexPixel32 **out_data, int *out_stride) = 0;
  virtual void finishImg32Fill(void *img, TexPixel32 *data) = 0;
  virtual void freeImg32(void *img) = 0;
};

class ILoadImageFactory
{
public:
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_alpha_used = NULL) = 0;
  virtual TexImage32 *loadImage(IGenLoad & /*crd*/, IMemAlloc * /*mem*/, const char * /*fn_ext*/, bool * /*out_alpha_used*/ = NULL)
  {
    return NULL;
  }

  virtual bool supportLoadImage2() = 0;
  virtual void *loadImage2(const char *fn, IAllocImg &a, const char *fn_ext) = 0;
  virtual void *loadImage2(IGenLoad & /*crd*/, IAllocImg & /*a*/, const char * /*fn_ext*/) { return NULL; }
};

void add_load_image_factory(ILoadImageFactory *lif, bool add_first = false);
void del_load_image_factory(ILoadImageFactory *lif);

TexImage32 *load_image(const char *fn, IMemAlloc *mem, bool *out_alpha_used = NULL, const char *fn_ext = NULL);
TexImage32 *load_image(IGenLoad &crd, const char *fn_ext, IMemAlloc *mem, bool *out_alpha_used = NULL);
void *load_image2(const char *fn, IAllocImg &a, const char *fn_ext = NULL);
void *load_image2(IGenLoad &crd, const char *fn_ext, IAllocImg &a);
