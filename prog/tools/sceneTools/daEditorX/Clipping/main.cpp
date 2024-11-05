// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "clippingPlugin.h"

#include <debug/dag_debug.h>


static ClippingPlugin *plugin;


//===============================================================================
void init_plugin_clipping()
{
  if (!DAGORED2->checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  plugin = new (inimem) ClippingPlugin;

  if (!DAGORED2->registerPlugin(::plugin))
    del_it(plugin);
}
