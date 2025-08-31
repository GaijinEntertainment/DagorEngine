// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"
#include <oldEditor/de_workspace.h>
#include <de3_dllPlugin.h>
#include <de3_interface.h>
#include <libTools/dtx/ddsxPlugin.h>

bool HmapLandPlugin::defMipOrdRev = false;
bool HmapLandPlugin::preferZstdPacking = false;
bool HmapLandPlugin::allowOodlePacking = false;
bool HmapLandPlugin::pcPreferASTC = false;
static HmapLandPlugin *plugin = nullptr;

DE3_DLL_PLUGIN_LINKAGE int DE3_DLL_PLUGIN_CALLCONV get_plugin_version() { return IGenEditorPlugin::VERSION_1_1; }

DE3_DLL_PLUGIN_LINKAGE IGenEditorPlugin *DE3_DLL_PLUGIN_CALLCONV register_plugin(IDagorEd2Engine &editor, const char *dll_path)
{
  daeditor3_init_globals(editor);

  DataBlock app_blk(DAGORED2->getWorkspace().getAppPath());
  const char *mgr_type = app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getStr("type", NULL);
#if defined(USE_HMAP_ACES)
  if (!mgr_type || strcmp(mgr_type, "aces") != 0)
    return NULL;
#else
  if (mgr_type && strcmp(mgr_type, "classic") != 0)
    return NULL;
#endif

  if (!HmapLandPlugin::prepareRequiredServices())
    return NULL;

  if (DAGORED2->getPluginByName("bin_scene_view"))
  {
    DAEDITOR3.conError("\"%s\" is not compatible with already inited \"%s\", skipped", "Landscape", "Scene view");
    return NULL;
  }

  ddsx::ConvertParams cp;
  HmapLandPlugin::defMipOrdRev = app_blk.getBlockByNameEx("projectDefaults")->getBool("defDDSxMipOrdRev", cp.mipOrdRev);
  HmapLandPlugin::preferZstdPacking = app_blk.getBlockByNameEx("projectDefaults")->getBool("preferZSTD", false);
  if (HmapLandPlugin::preferZstdPacking)
    debug("landscape prefers ZSTD");
  HmapLandPlugin::allowOodlePacking = app_blk.getBlockByNameEx("projectDefaults")->getBool("allowOODLE", false);
  if (HmapLandPlugin::allowOodlePacking)
    debug("landscape allows OODLE");

  if (app_blk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("hmap")->getBool("preferASTC", false) &&
      app_blk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("tex")->getBlockByNameEx("PC")->getBool(
        "allowASTC", false))
  {
    HmapLandPlugin::pcPreferASTC = true;
    debug("landscape prefers ASTC format for PC");
  }

  ::plugin = ::new (inimem) HmapLandPlugin;
  return ::plugin;
}

DE3_DLL_PLUGIN_LINKAGE void DE3_DLL_PLUGIN_CALLCONV release_plugin() { del_it(plugin); }

#if _TARGET_STATIC_LIB
void init_plugin_heightmapland()
{
  IGenEditorPlugin *plugin = register_plugin(*DAGORED2, nullptr);
  if (plugin && !DAGORED2->registerPlugin(plugin))
    del_it(plugin);
}
#endif
