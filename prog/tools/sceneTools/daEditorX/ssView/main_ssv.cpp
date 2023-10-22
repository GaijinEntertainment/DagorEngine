#include "plugin_ssv.h"
#include <debug/dag_debug.h>

void init_plugin_ssview()
{
  if (!DAGORED2->checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }
  if (!ssviewplugin::Plugin::prepareRequiredServices())
    return;

  ssviewplugin::Plugin *plugin = new (inimem) ssviewplugin::Plugin;

  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}
