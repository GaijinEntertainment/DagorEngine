// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_tga.h>
#include <osApiWrappers/dag_localConv.h>

class TgaLoadImageFactory : public ILoadImageFactory
{
public:
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || dd_stricmp(fn_ext, ".tga") != 0)
      return NULL;
    return load_tga32(fn, mem, out_used_alpha);
  }
  virtual TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || dd_stricmp(fn_ext, ".tga") != 0)
      return NULL;
    return load_tga32(crd, mem, out_used_alpha);
  }
  virtual bool supportLoadImage2() { return false; }
  virtual void *loadImage2(const char *, IAllocImg &, const char *) { return NULL; }
};

static TgaLoadImageFactory tga_load_image_factory;

void register_tga_tex_load_factory() { add_load_image_factory(&tga_load_image_factory); }
