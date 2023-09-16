#include "collision.h"

#include <EditorCore/ec_interface.h>

#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>


static InitOnDemand<CollisionPlugin> plugin;


void init_plugin_collision()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
