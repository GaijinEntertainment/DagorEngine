#include <3d/dag_createTex.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_tab.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_fatal.h>

class TabCreateTexFactory : public Tab<ICreateTexFactory *>
{
public:
  TabCreateTexFactory() : Tab<ICreateTexFactory *>(inimem) {}
};

static InitOnDemand<TabCreateTexFactory> ctf_list;

void add_create_tex_factory(ICreateTexFactory *ctf)
{
  ctf_list.demandInit();

  for (int i = 0; i < ctf_list->size(); i++)
    if ((*ctf_list)[i] == ctf)
      return;

  ctf_list->push_back(ctf);
}
void del_create_tex_factory(ICreateTexFactory *ctf)
{
  if (!ctf_list)
    return;

  for (int i = 0; i < ctf_list->size(); i++)
    if ((*ctf_list)[i] == ctf)
    {
      erase_items(*ctf_list, i, 1);
      return;
    }
}

BaseTexture *create_texture_via_factories(const char *fn, int flg, int levels, const char *fn_ext, const TextureMetaData &tmd,
  ICreateTexFactory *excl_factory)
{
  for (int i = 0; i < ctf_list->size(); i++)
    if (ctf_list->at(i) != excl_factory)
      if (BaseTexture *t = ctf_list->at(i)->createTex(fn, flg, levels, fn_ext, tmd))
        return t;
  return NULL;
}

BaseTexture *create_texture(const char *fn, int flg, int levels, bool fatalerr, const char *fn_ext)
{
  if (!ctf_list)
  {
    if (fatalerr)
      fatal("can't create tex: %s", fn);
    return NULL;
  }

  TextureMetaData tmd;
  String stor;
  const char *fn_ptr = tmd.decode(fn, &stor);
  if (!fn_ptr)
  {
    if (fatalerr)
      fatal("invalid file string: %s", fn);
    return NULL;
  }

  if (!fn_ext)
    fn_ext = dd_get_fname_ext(fn_ptr);
  if (BaseTexture *t = create_texture_via_factories(fn_ptr, flg, levels, fn_ext, tmd, NULL))
    return t;

  if (fatalerr)
    fatal("can't create tex: %s", fn);
  return NULL;
}
