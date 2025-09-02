// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collisionPlugin.h"

#include <debug/dag_debug.h>


static CollisionPlugin *plugin;


//===============================================================================
void init_plugin_collision()
{
  if (!DAGORED2->checkVersion())
  {
    DEBUG_CTX("Incorrect version!");
    return;
  }

  plugin = new (inimem) CollisionPlugin;

  if (!DAGORED2->registerPlugin(::plugin))
    del_it(plugin);
}
