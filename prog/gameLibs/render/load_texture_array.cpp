// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <3d/dag_render.h>
#include <generic/dag_tab.h>

TEXTUREID load_texture_array_immediate(const char *name, const char *param_name, const DataBlock &blk, int &count)
{
  count = 0;
  Tab<const char *> names;
  int nameId = blk.getNameId(param_name);
  for (int i = blk.findParam(nameId, -1); i >= 0; i = blk.findParam(nameId, i))
    names.push_back(blk.getStr(i));

  TEXTUREID id = names.size() ? ::add_managed_array_texture(name, names) : BAD_TEXTUREID;
  if (id == BAD_TEXTUREID)
    return id;
  ArrayTexture *array = (ArrayTexture *)::acquire_managed_tex(id);
  if (array)
    array->disableSampler();
  count = names.size();

  return id;
}
