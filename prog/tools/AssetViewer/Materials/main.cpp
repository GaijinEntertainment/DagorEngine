#include "materials.h"

#include <EditorCore/ec_interface.h>

#include <generic/dag_initOnDemand.h>
#include <debug/dag_debug.h>


static InitOnDemand<MaterialsPlugin> plugin;


void init_plugin_materials()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
}
