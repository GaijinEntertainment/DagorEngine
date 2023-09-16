#include "environmentPlugin.h"
#include <debug/dag_debug.h>


//===============================================================================
void init_plugin_environment()
{
  if (!DAGORED2->checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }
  if (!EnvironmentPlugin::prepareRequiredServices())
    return;

  EnvironmentPlugin *plugin = new (inimem) EnvironmentPlugin;

  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}
