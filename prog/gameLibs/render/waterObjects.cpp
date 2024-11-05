// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/waterObjects.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_tab.h>
#include <util/dag_globDef.h>
// #include <debug/dag_debug.h>

static Tab<waterobjects::Obj> objs(inimem);

void waterobjects::add(void *handle, ShaderMaterial *mat, const BBox3 &box, const Plane3 &plane)
{
  Obj &o = objs.push_back();
  G_ASSERT(mat);
  o.handle = handle;
  o.mat = mat;
  o.box = box;
  o.plane = plane;
  mat->addRef();
  // debug("add water obj: %p, %p, c=" FMT_P3 ",sz=" FMT_P3 ", plane=" FMT_P4 "",
  //   handle, mat, P3D(box.center()), P3D(box.width()), P3D(plane.n), plane.d);
}

void waterobjects::del(void *handle)
{
  for (int i = objs.size() - 1; i >= 0; i--)
    if (handle == objs[i].handle)
    {
      // debug("del water obj: %p", handle);
      objs[i].mat->delRef();
      erase_items(objs, i, 1);
    }
}

void waterobjects::del_all()
{
  for (int i = objs.size() - 1; i >= 0; i--)
  {
    // debug("del water obj: %p", objs[i].handle);
    objs[i].mat->delRef();
    erase_items(objs, i, 1);
  }
}

dag::Span<waterobjects::Obj> waterobjects::get_list() { return make_span(objs); }
