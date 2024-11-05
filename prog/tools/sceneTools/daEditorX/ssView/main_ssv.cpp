// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugin_ssv.h"
#include <debug/dag_debug.h>

void init_plugin_ssview()
{
  if (!DAGORED2->checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }
  if (!ssviewplugin::Plugin::prepareRequiredServices())
    return;

  ssviewplugin::Plugin *plugin = new (inimem) ssviewplugin::Plugin;

  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}
