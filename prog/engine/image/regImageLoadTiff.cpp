// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_tiff.h>
#include <osApiWrappers/dag_localConv.h>
#include <image/tiff-4.4.0/tiffio.h>
#include <debug/dag_log.h>
#include <util/dag_string.h>

class TiffLoadImageFactory : public ILoadImageFactory
{
public:
  static inline bool checkFnExt(const char *ext) { return ext && (dd_stricmp(ext, ".tif") == 0 || dd_stricmp(ext, ".tiff") == 0); }
  bool readImageDimensions(const char *fn, const char *fn_ext, int &out_w, int &out_h, bool &out_may_have_alpha) override
  {
    return checkFnExt(fn_ext) ? read_tiff32_dimensions(fn, out_w, out_h, out_may_have_alpha) : false;
  }
  TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    return checkFnExt(fn_ext) ? load_tiff32(fn, mem, out_used_alpha) : nullptr;
  }
  TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha) override
  {
    return checkFnExt(fn_ext) ? load_tiff32(crd, mem, out_used_alpha) : nullptr;
  }
  bool supportLoadImage2() override { return false; }
  void *loadImage2(const char *, IAllocImg &, const char *) override { return nullptr; }
};

static TiffLoadImageFactory tiff_load_image_factory;

static void dag_tiff_error_handler(const char *module, const char *fmt, va_list ap)
{
  cvlogmessage(LOGLEVEL_ERR, module ? String(0, "%s: %s", module, fmt).str() : fmt, ap);
}
/*
static void dag_tiff_warning_handler(const char *module, const char *fmt, va_list ap)
{
  cvlogmessage(LOGLEVEL_WARN, module ? String(0, "%s: %s", module, fmt).str() : fmt, ap);
}
*/

void register_tiff_tex_load_factory()
{
  TIFFSetErrorHandler(&dag_tiff_error_handler);
  // TIFFSetWarningHandler(&dag_tiff_warning_handler);
  TIFFSetWarningHandler(nullptr);
  add_load_image_factory(&tiff_load_image_factory);
}
