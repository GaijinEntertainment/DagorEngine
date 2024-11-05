// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "a2d.h"

#include <EditorCore/ec_interface.h>
#include <generic/dag_initOnDemand.h>

static InitOnDemand<A2dPlugin> plugin;


void init_plugin_a2d()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
