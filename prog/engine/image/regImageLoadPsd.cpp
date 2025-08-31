// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_psd.h>
#include <osApiWrappers/dag_localConv.h>

class PsdLoadImageFactory : public ILoadImageFactory
{
public:
  static inline bool checkFnExt(const char *ext) { return ext && dd_stricmp(ext, ".psd") == 0; }
  bool readImageDimensions(const char *fn, const char *fn_ext, int &out_w, int &out_h, bool &out_may_have_alpha) override
  {
    return checkFnExt(fn_ext) ? read_psd32_dimensions(fn, out_w, out_h, out_may_have_alpha) : false;
  }
  TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    return checkFnExt(fn_ext) ? load_psd32(fn, mem, out_used_alpha) : nullptr;
  }
  TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    return checkFnExt(fn_ext) ? load_psd32(crd, mem, out_used_alpha) : nullptr;
  }
  bool supportLoadImage2() override { return false; }
  void *loadImage2(const char *, IAllocImg &, const char *) override { return nullptr; }
};

static PsdLoadImageFactory psd_load_image_factory;

void register_psd_tex_load_factory() { add_load_image_factory(&psd_load_image_factory); }
