#include "hmlPlugin.h"
#include <oldEditor/de_workspace.h>
#include <de3_interface.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <windows.h>

bool HmapLandPlugin::defMipOrdRev = false;
bool HmapLandPlugin::preferZstdPacking = false;
bool HmapLandPlugin::allowOodlePacking = false;

//==================================================================================================
BOOL __stdcall DllMain(HINSTANCE instance, DWORD reason, void *)
{
  if (reason == DLL_PROCESS_DETACH && HmapLandPlugin::self)
    delete HmapLandPlugin::self;

  return TRUE;
}


//==================================================================================================
extern "C" int __fastcall get_plugin_version() { return IGenEditorPlugin::VERSION_1_1; }


//==================================================================================================
extern "C" IGenEditorPlugin *__fastcall register_plugin(IDagorEd2Engine &editor)
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

  return ::new (inimem) HmapLandPlugin;
}
