// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugin_occ.h"
#include <windows.h>

static occplugin::Plugin *plugin = NULL;


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

void init_plugin_occluders()
{
  IGenEditorPlugin *plugin = register_plugin(*DAGORED2);
  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}

static IGenEditorPlugin *register_plugin(IDagorEd2Engine &editor)
#endif
{
  daeditor3_init_globals(editor);

  ::plugin = new (inimem) occplugin::Plugin;

  return ::plugin;
}
