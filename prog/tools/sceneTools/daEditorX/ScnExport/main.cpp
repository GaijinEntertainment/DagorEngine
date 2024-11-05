// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scnExportPlugin.h"
#include <debug/dag_debug.h>


static ScnExportPlugin *plugin;


//===============================================================================
void init_plugin_scn_export()
{
  if (!DAGORED2->checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  plugin = new (inimem) ScnExportPlugin;

  if (!DAGORED2->registerPlugin(::plugin))
    del_it(plugin);
}
