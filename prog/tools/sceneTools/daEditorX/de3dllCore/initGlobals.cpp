#include <startup/dag_dllMain.inc.cpp>
#include <dllPluginCore/core.h>
#include <de3_interface.h>
#include <de3_huid.h>
#include <oldEditor/de_interface.h>

IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = NULL;
IDaEditor3Engine *IDaEditor3Engine::__daeditor3_global_instance = NULL;

void daeditor3_init_globals(IDagorEd2Engine &editor)
{
  IEditorCore &editor_core = *(IEditorCore *)editor.queryEditorInterfacePtr(HUID_IEditorCore);
  IDaEditor3Engine &editor3 = *(IDaEditor3Engine *)editor.queryEditorInterfacePtr(HUID_IDaEditor3Engine);
  G_ASSERT(&editor_core);
  G_ASSERT(&editor3);

  ::init_dll_plugin_core(editor_core);
  IEditorCoreEngine::set(&editor);
  IDagorEd2Engine::set(&editor);
  IDaEditor3Engine::set(&editor3);
}
