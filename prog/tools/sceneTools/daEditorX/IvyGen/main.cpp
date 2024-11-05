// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugIn.h"

#include <oldEditor/pluginService/de_IDagorPhys.h>
#include <coolConsole/coolConsole.h>

#include <windows.h>

static IvyGenPlugin *plugin = NULL;
IDagorPhys *dagPhys = NULL;


#if !_TARGET_STATIC_LIB
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
#else
static IGenEditorPlugin *register_plugin(IDagorEd2Engine &editor);

void init_plugin_ivygen()
{
  IGenEditorPlugin *plugin = register_plugin(*DAGORED2);
  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}

static IGenEditorPlugin *register_plugin(IDagorEd2Engine &editor)
#endif
{
  daeditor3_init_globals(editor);
  CoolConsole &con = editor.getConsole();

  dagPhys = (IDagorPhys *)editor.queryEditorInterface<IDagorPhys>();

  if (!dagPhys)
  {
    con.addMessage(ILogWriter::FATAL, "Couldn't get IDagorPhys interface from DaEditorX");
    return NULL;
  }

  ::plugin = new (inimem) IvyGenPlugin;

  return ::plugin;
}
