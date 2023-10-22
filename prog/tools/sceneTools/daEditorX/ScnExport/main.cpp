#include "scnExportPlugin.h"
#include <debug/dag_debug.h>


static ScnExportPlugin *plugin;


//===============================================================================
void init_plugin_scn_export()
{
  if (!DAGORED2->checkVersion())
  {
    debug_ctx("Incorrect version!");
    return;
  }

  plugin = new (inimem) ScnExportPlugin;

  if (!DAGORED2->registerPlugin(::plugin))
    del_it(plugin);
}
