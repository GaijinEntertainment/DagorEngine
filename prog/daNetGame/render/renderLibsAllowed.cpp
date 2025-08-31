// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderLibsAllowed.h"
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_bindumpReloadListener.h>

static DataBlock allowRenderLibsBlk;
static bool defRenderLibAllow = true;

static void sync_shadervars_with_render_libs()
{
  IShaderBindumpReloadListener::forEach([](IShaderBindumpReloadListener *var) {
    if (const char *tag = var->getTag())
      var->setEnabled(is_render_lib_allowed(tag));
  });
}

void load_render_libs_allowed_blk(const char *lib_excluded_blk_fn)
{
  dblk::load(allowRenderLibsBlk, lib_excluded_blk_fn);
  defRenderLibAllow = allowRenderLibsBlk.getBool("__def", true);
  sync_shadervars_with_render_libs();
}
void set_render_libs_allowed_blk(const DataBlock &lib_excluded_blk)
{
  allowRenderLibsBlk = lib_excluded_blk;
  defRenderLibAllow = allowRenderLibsBlk.getBool("__def", true);
  sync_shadervars_with_render_libs();
}

bool is_render_lib_allowed(const char *lib_name) { return allowRenderLibsBlk.getBool(lib_name, defRenderLibAllow); }
