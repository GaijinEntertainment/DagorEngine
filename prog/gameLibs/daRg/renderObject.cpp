// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_renderObject.h>
#include <daRg/dag_renderState.h>

#include <generic/dag_tabUtils.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <util/dag_string.h>


namespace darg
{


struct RendObjFactoryEntry
{
  int id;
  String name;
  RendObjFactory *factory;
};

static Tab<RendObjFactoryEntry> robj_factories(uimem);


int add_rendobj_factory(const char *name, RendObjFactory *f)
{
  int id = robj_factories.size();
  RendObjFactoryEntry &rec = robj_factories.push_back();
  rec.id = id;
  rec.name = name;
  rec.factory = f;
  G_ASSERT(id + 1 == robj_factories.size());
  return id;
}


int find_rendobj_factory_id(const char *name)
{
  for (int i = 0, n = robj_factories.size(); i < n; ++i)
  {
    RendObjFactoryEntry &rec = robj_factories[i];
    G_ASSERT(rec.id == i);
    if (rec.name == name)
      return rec.id;
  }
  return -1;
}


RendObjFactory *get_rendobj_factory(int id)
{
  if (id < 0 || id >= robj_factories.size())
    return nullptr;
  return robj_factories[id].factory;
}


RenderObject *create_rendobj(int id, void *stor)
{
  RendObjFactory *f = get_rendobj_factory(id);
  return f ? f->createRenderObject(stor) : NULL;
}


RendObjParams *create_robj_params(int id)
{
  RendObjFactory *f = get_rendobj_factory(id);
  return f ? f->createRendObjParams() : NULL;
}


void register_rendobj_script_ids(HSQUIRRELVM vm)
{
  Sqrat::ConstTable constTbl(vm);
  for (int i = 0, n = robj_factories.size(); i < n; ++i)
  {
    RendObjFactoryEntry &rec = robj_factories[i];
    G_ASSERT(rec.id == i);
    constTbl.Const(rec.name, rec.id);
  }
}


} // namespace darg
