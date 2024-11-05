// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_dllMain.inc.cpp>
#include <EditorCore/ec_IEditorCore.h>
#include <de3_interface.h>
#include <de3_huid.h>
#include <oldEditor/de_interface.h>
#include <libTools/util/strUtil.h>
#include <drv/3d/dag_interface_table.h>
#include <math/dag_math3d.h>
#include <math/random/dag_random.h>

IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = NULL;
IDaEditor3Engine *IDaEditor3Engine::__daeditor3_global_instance = NULL;
IEditorCoreEngine *IEditorCoreEngine::__global_instance = nullptr;

IDagorRender *editorcore_extapi::dagRender = nullptr;
IDagorGeom *editorcore_extapi::dagGeom = nullptr;
IDagorConsole *editorcore_extapi::dagConsole = nullptr;
IDagorTools *editorcore_extapi::dagTools = nullptr;
IDagorScene *editorcore_extapi::dagScene = nullptr;

D3dInterfaceTable d3di;
static SimpleString exePath;

void daeditor3_init_globals(IDagorEd2Engine &editor)
{
  IEditorCore &editor_core = *(IEditorCore *)editor.queryEditorInterfacePtr(HUID_IEditorCore);
  IDaEditor3Engine &editor3 = *(IDaEditor3Engine *)editor.queryEditorInterfacePtr(HUID_IDaEditor3Engine);
  G_ASSERT(&editor_core);
  G_ASSERT(&editor3);

  editorcore_extapi::dagRender = editor_core.getRender();
  G_ASSERT(editorcore_extapi::dagRender);
  editorcore_extapi::dagGeom = editor_core.getGeom();
  G_ASSERT(editorcore_extapi::dagGeom);
  editorcore_extapi::dagConsole = editor_core.getConsole();
  G_ASSERT(editorcore_extapi::dagConsole);
  editorcore_extapi::dagTools = editor_core.getTools();
  G_ASSERT(editorcore_extapi::dagTools);
  editorcore_extapi::dagScene = editor_core.getScene();
  G_ASSERT(editorcore_extapi::dagScene);
  exePath = editor_core.getExePath();

  editorcore_extapi::dagRender->fillD3dInterfaceTable(d3di);

  // some of Dagor engine inits
  ::set_rnd_seed(editorcore_extapi::dagTools->refTimeUsec());
  ::init_math();

  IEditorCoreEngine::set(&editor);
  IDagorEd2Engine::set(&editor);
  IDaEditor3Engine::set(&editor3);
}

String editorcore_extapi::make_full_start_path(const char *rel_path) { return ::make_full_path(exePath, rel_path); }
