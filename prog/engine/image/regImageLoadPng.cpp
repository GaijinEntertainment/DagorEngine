#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_png.h>
#include <osApiWrappers/dag_localConv.h>

class PngLoadImageFactory : public ILoadImageFactory
{
public:
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || dd_stricmp(fn_ext, ".png") != 0)
      return NULL;
    return load_png32(fn, mem, out_used_alpha);
  }
  virtual TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || dd_stricmp(fn_ext, ".png") != 0)
      return NULL;
    return load_png32(crd, mem, out_used_alpha);
  }
  virtual bool supportLoadImage2() { return false; }
  virtual void *loadImage2(const char *, IAllocImg &, const char *) { return NULL; }
};

static PngLoadImageFactory png_load_image_factory;

void register_png_tex_load_factory() { add_load_image_factory(&png_load_image_factory); }
