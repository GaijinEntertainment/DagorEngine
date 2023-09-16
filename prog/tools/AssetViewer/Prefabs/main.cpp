#include "prefabs.h"

#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>


static InitOnDemand<PrefabsPlugin> plugin;


void init_plugin_prefabs()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
