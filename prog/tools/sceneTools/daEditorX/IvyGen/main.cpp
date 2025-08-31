// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugin.h"

#include <oldEditor/pluginService/de_IDagorPhys.h>
#include <coolConsole/coolConsole.h>
#include <de3_dllPlugin.h>

static IvyGenPlugin *plugin = nullptr;
IDagorPhys *dagPhys = NULL;

DE3_DLL_PLUGIN_LINKAGE int DE3_DLL_PLUGIN_CALLCONV get_plugin_version() { return IGenEditorPlugin::VERSION_1_1; }

DE3_DLL_PLUGIN_LINKAGE IGenEditorPlugin *DE3_DLL_PLUGIN_CALLCONV register_plugin(IDagorEd2Engine &editor, const char *dll_path)
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

DE3_DLL_PLUGIN_LINKAGE void DE3_DLL_PLUGIN_CALLCONV release_plugin() { del_it(plugin); }

#if _TARGET_STATIC_LIB
void init_plugin_ivygen()
{
  IGenEditorPlugin *plugin = register_plugin(*DAGORED2, nullptr);
  if (!DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}
#endif
