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
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || (dd_stricmp(fn_ext, ".tiff") != 0 && dd_stricmp(fn_ext, ".tif") != 0))
      return NULL;
    return load_tiff32(fn, mem, out_used_alpha);
  }
  virtual TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || (dd_stricmp(fn_ext, ".tiff") != 0 && dd_stricmp(fn_ext, ".tif") != 0))
      return NULL;
    return load_tiff32(crd, mem, out_used_alpha);
  }
  virtual bool supportLoadImage2() { return false; }
  virtual void *loadImage2(const char *, IAllocImg &, const char *) { return NULL; }
};

static TiffLoadImageFactory tiff_load_image_factory;

static void dag_tiff_error_handler(const char *module, const char *fmt, va_list ap)
{
  cvlogmessage(LOGLEVEL_ERR, module ? String(0, "%s: %s", module, fmt).str() : fmt, ap);
}
static void dag_tiff_warning_handler(const char *module, const char *fmt, va_list ap)
{
  cvlogmessage(LOGLEVEL_WARN, module ? String(0, "%s: %s", module, fmt).str() : fmt, ap);
}

void register_tiff_tex_load_factory()
{
  TIFFSetErrorHandler(&dag_tiff_error_handler);
  TIFFSetWarningHandler(&dag_tiff_warning_handler);
  add_load_image_factory(&tiff_load_image_factory);
}
