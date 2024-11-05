// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textures.h"

#include <EditorCore/ec_interface.h>

#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>


static InitOnDemand<TexturesPlugin> plugin;


void init_plugin_textures()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
