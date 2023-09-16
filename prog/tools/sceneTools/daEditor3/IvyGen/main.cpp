#include "plugIn.h"

#include <oldEditor/pluginService/de_IDagorPhys.h>
#include <coolConsole/coolConsole.h>

#include <windows.h>

static IvyGenPlugin *plugin = NULL;
IDagorPhys *dagPhys = NULL;


//==================================================================================================
BOOL __stdcall DllMain(HINSTANCE instance, DWORD reason, void *)
{
  if (reason == DLL_PROCESS_DETACH && plugin)
  {
    delete plugin;
    plugin = NULL;
  }

  return TRUE;
}


//==================================================================================================
extern "C" int __fastcall get_plugin_version() { return IGenEditorPlugin::VERSION_1_1; }


//==================================================================================================

extern "C" IGenEditorPlugin *__fastcall register_plugin(IDagorEd2Engine &editor)
{
  daeditor3_init_globals(editor);
  CoolConsole &con = editor.getConsole();

  dagPhys = (IDagorPhys *)editor.queryEditorInterface<IDagorPhys>();

  if (!dagPhys)
  {
    con.addMessage(ILogWriter::FATAL, "Couldn't get IDagorPhys interface from DaEditor3.");
    return NULL;
  }

  ::plugin = new (inimem) IvyGenPlugin;

  return ::plugin;
}
