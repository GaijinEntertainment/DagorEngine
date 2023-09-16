#include "nodes.h"

#include <EditorCore/ec_interface.h>

#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>


static InitOnDemand<NodesPlugin> plugin;


void init_plugin_nodes()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
