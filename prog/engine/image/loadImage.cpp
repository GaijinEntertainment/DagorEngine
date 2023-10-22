#include <image/dag_loadImage.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <ioSys/dag_genIo.h>
#include <util/dag_string.h>
#include "resampleImage.h"

class TabLoadImageFactory : public Tab<ILoadImageFactory *>
{
public:
  TabLoadImageFactory() : Tab<ILoadImageFactory *>(inimem) {}
};

static InitOnDemand<TabLoadImageFactory> lif_list;

void add_load_image_factory(ILoadImageFactory *lif, bool add_first)
{
  lif_list.demandInit();

  for (int i = 0; i < lif_list->size(); i++)
    if ((*lif_list)[i] == lif)
      return;

  if (add_first)
    insert_item_at(*lif_list, 0, lif);
  else
    lif_list->push_back(lif);
}
void del_load_image_factory(ILoadImageFactory *lif)
{
  if (!lif_list)
    return;

  for (int i = 0; i < lif_list->size(); i++)
    if ((*lif_list)[i] == lif)
    {
      erase_items(*lif_list, i, 1);
      return;
    }
}

static const char *get_trimmed_fn_and_decode_suffix(const char *fn, const char *&fn_ext, String &fn_trimmed, int &w, int &h,
  bool &keep_ar, bool &premul)
{
  if (const char *suf = fn_ext ? strchr(fn_ext + 1, ':') : nullptr)
  {
    if (dd_strnicmp(fn_ext, ".svg", 4) == 0) // skip processing SVG to pass full encoded fname to factory
      return fn;

    if (!decode_fname_suffix(suf + 1, w, h, keep_ar, premul, fn_ext))
      return nullptr;

    if (fn_ext > fn && fn_ext < fn + strlen(fn))
    {
      fn_trimmed.setSubStr(fn, suf);
      fn_ext = dd_get_fname_ext(fn_trimmed);
      return fn_trimmed;
    }
  }
  return fn;
}

TexImage32 *load_image(const char *fn, IMemAlloc *mem, bool *out_used_alpha, const char *fn_ext)
{
  if (!lif_list)
    return NULL;

  if (!fn_ext)
    fn_ext = dd_get_fname_ext(fn);

  String fn_trimmed;
  int w = 0, h = 0;
  bool keep_ar = true, premul = false;
  fn = get_trimmed_fn_and_decode_suffix(fn, fn_ext, fn_trimmed, w, h, keep_ar, premul);
  if (!fn)
    return nullptr;

  if (out_used_alpha)
    *out_used_alpha = false;
  for (int i = 0; i < lif_list->size(); i++)
    if (TexImage32 *t = (*lif_list)[i]->loadImage(fn, mem, fn_ext, out_used_alpha))
      return resample_img_p(t, w, h, keep_ar, premul, mem);
  return NULL;
}
TexImage32 *load_image(IGenLoad &crd, const char *fn_ext, IMemAlloc *mem, bool *out_used_alpha)
{
  if (!lif_list)
    return NULL;

  String fn_trimmed;
  int w = 0, h = 0;
  bool keep_ar = true, premul = false;
  if (!get_trimmed_fn_and_decode_suffix(crd.getTargetName(), fn_ext, fn_trimmed, w, h, keep_ar, premul))
    return nullptr;

  if (out_used_alpha)
    *out_used_alpha = false;
  for (int i = 0; i < lif_list->size(); i++)
    if (TexImage32 *t = (*lif_list)[i]->loadImage(crd, mem, fn_ext, out_used_alpha))
      return resample_img_p(t, w, h, keep_ar, premul, mem);
  return NULL;
}
void *load_image2(const char *fn, IAllocImg &a, const char *fn_ext)
{
  if (!lif_list)
    return NULL;

  if (!fn_ext)
    fn_ext = dd_get_fname_ext(fn);

  String fn_trimmed;
  int w = 0, h = 0;
  bool keep_ar = true, premul = false;
  fn = get_trimmed_fn_and_decode_suffix(fn, fn_ext, fn_trimmed, w, h, keep_ar, premul);
  if (!fn)
    return nullptr;
  if (w || h || premul) // fallback to load_image() when image resampling or alpha premultiply is needed
    return nullptr;

  for (int i = 0; i < lif_list->size(); i++)
    if (void *t = (*lif_list)[i]->loadImage2(fn, a, fn_ext))
      return t;
  return NULL;
}
void *load_image2(IGenLoad &crd, const char *fn_ext, IAllocImg &a)
{
  if (!lif_list)
    return NULL;

  String fn_trimmed;
  int w = 0, h = 0;
  bool keep_ar = true, premul = false;
  if (!get_trimmed_fn_and_decode_suffix(crd.getTargetName(), fn_ext, fn_trimmed, w, h, keep_ar, premul))
    return nullptr;
  if (w || h || premul) // fallback to load_image() when image resampling or alpha premultiply is needed
    return nullptr;

  for (int i = 0; i < lif_list->size(); i++)
    if (void *t = (*lif_list)[i]->loadImage2(crd, a, fn_ext))
      return t;
  return NULL;
}
