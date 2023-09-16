#include "clippingPlugin.h"

#include <debug/dag_debug.h>


static ClippingPlugin *plugin;


//===============================================================================
void init_plugin_clipping()
{
  if (!DAGORED2->checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  plugin = new (inimem) ClippingPlugin;

  if (!DAGORED2->registerPlugin(::plugin))
    del_it(plugin);
}
