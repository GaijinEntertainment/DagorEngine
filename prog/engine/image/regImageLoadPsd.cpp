#include <startup/dag_startupTex.h>
#include <image/dag_loadImage.h>
#include <image/dag_psd.h>
#include <osApiWrappers/dag_localConv.h>

class PsdLoadImageFactory : public ILoadImageFactory
{
public:
  virtual TexImage32 *loadImage(const char *fn, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || dd_stricmp(fn_ext, ".psd") != 0)
      return NULL;
    return load_psd32(fn, mem, out_used_alpha);
  }
  virtual TexImage32 *loadImage(IGenLoad &crd, IMemAlloc *mem, const char *fn_ext, bool *out_used_alpha = NULL)
  {
    if (!fn_ext || dd_stricmp(fn_ext, ".psd") != 0)
      return NULL;
    return load_psd32(crd, mem, out_used_alpha);
  }
  virtual bool supportLoadImage2() { return false; }
  virtual void *loadImage2(const char *, IAllocImg &, const char *) { return NULL; }
};

static PsdLoadImageFactory psd_load_image_factory;

void register_psd_tex_load_factory() { add_load_image_factory(&psd_load_image_factory); }
