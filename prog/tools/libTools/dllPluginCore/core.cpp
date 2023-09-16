#include <dllPluginCore/core.h>
#include <libTools/util/strUtil.h>
#include <sepGui/wndGlobal.h>

#include <math/dag_math3d.h>
#include <math/random/dag_random.h>

#include <3d/dag_drv3di.h>

IDagorRender *dagRender = NULL;
IDagorGeom *dagGeom = NULL;
IDagorConsole *dagConsole = NULL;
IDagorTools *dagTools = NULL;
IDagorScene *dagScene = NULL;

IEditorCore *editorCore = NULL;
IEditorCoreEngine *IEditorCoreEngine::__global_instance = NULL;


D3dInterfaceTable d3di;
static SimpleString exePath;

void init_dll_plugin_core(IEditorCore &core_iface)
{
  ::editorCore = &core_iface;

  ::dagRender = core_iface.getRender();
  G_ASSERT(::dagRender);
  ::dagGeom = core_iface.getGeom();
  G_ASSERT(::dagGeom);
  ::dagConsole = core_iface.getConsole();
  G_ASSERT(::dagConsole);
  ::dagTools = core_iface.getTools();
  G_ASSERT(::dagTools);
  ::dagScene = core_iface.getScene();
  G_ASSERT(::dagScene);
  exePath = core_iface.getExePath();

  dagRender->fillD3dInterfaceTable(d3di);

  // some of Dagor engine inits
  ::set_rnd_seed(dagTools->refTimeUsec());
  ::init_math();
}

String make_full_start_path(const char *rel_path) { return ::make_full_path(exePath, rel_path); }
