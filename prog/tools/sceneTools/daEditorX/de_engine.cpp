// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"
#include "de_plugindata.h"
#include "de_startdlg.h"

#include <oldEditor/de_common_interface.h>
#include <oldEditor/de_cm.h>
#include <oldEditor/de_workspace.h>
#include <oldEditor/de_clipping.h>
#include <de3_huid.h>
#include <de3_fileTracker.h>
#include <de3_interface.h>
#include <de3_entityFilter.h>
#include <de3_editorEvents.h>
#include <de3_fmodService.h>
#include <de3_dynRenderService.h>
#include <render/debugTexOverlay.h>
#include <de3_dxpFactory.h>
#include <generic/dag_sort.h>

#include <sepGui/wndGlobal.h>

#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_gizmofilter.h>
#include <EditorCore/ec_brushfilter.h>
#include <EditorCore/ec_imguiInitialization.h>

#include <assets/assetHlp.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/dtx/dtxSave.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/de_TextureName.h>
#include <libTools/util/fileUtils.h>
#include <util/dag_texMetaData.h>
#include <util/dag_oaHashNameMap.h>

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_basePath.h>

#include <fx/dag_hdrRender.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <scene/dag_visibility.h>

#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texPackMgr2.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_graphStat.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <windows.h>
#include <psapi.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <propPanel/control/panelWindow.h>
#include <propPanel/messageQueue.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/menu.h>

#include <de3_waterSrv.h>
#include <gui/dag_stdGuiRenderEx.h>

#include <gui/dag_imgui.h>

using hdpi::_pxActual;

extern void terminate_interface_de3();
extern void regular_update_interface_de3();
extern void services_act(float dt);
extern void services_before_render();
extern void services_render();
extern void services_render_trans();
extern bool services_catch_event(unsigned event_huid, void *user_data);
extern void daeditor3_set_plugin_prop_panel(void *);

extern void reset_colliders_data();
extern void init3d();
extern bool src_assets_scan_allowed;
extern OAHashNameMap<true> cmdline_force_enabled_plugins, cmdline_force_disabled_plugins;
extern InitOnDemand<DebugTexOverlay> de3_show_tex_helper;
extern bool minimize_dabuild_usage;

//==============================================================================
const char *hotkey_names[HOTKEY_COUNT] = {
  "None",
  "F5",
  "F6",
  "F7",
  "F8",
  "Shift+F1",
  "Shift+F2",
  "Shift+F3",
  "Shift+F4",
  "Shift+F5",
  "Shift+F6",
  "Shift+F7",
  "Shift+F8",
  "Shift+F9",
  "Shift+F10",
  "Shift+F11",
  "Shift+F12",
};


//==============================================================================
int hotkey_values[HOTKEY_COUNT] = {
  -1,
  CM_SWITCH_PLUGIN_F5,
  CM_SWITCH_PLUGIN_F6,
  CM_SWITCH_PLUGIN_F7,
  CM_SWITCH_PLUGIN_F8,
  CM_SWITCH_PLUGIN_SF1,
  CM_SWITCH_PLUGIN_SF2,
  CM_SWITCH_PLUGIN_SF3,
  CM_SWITCH_PLUGIN_SF4,
  CM_SWITCH_PLUGIN_SF5,
  CM_SWITCH_PLUGIN_SF6,
  CM_SWITCH_PLUGIN_SF7,
  CM_SWITCH_PLUGIN_SF8,
  CM_SWITCH_PLUGIN_SF9,
  CM_SWITCH_PLUGIN_SF10,
  CM_SWITCH_PLUGIN_SF11,
  CM_SWITCH_PLUGIN_SF12,
};


const char *hotkey_int_to_char(int hk)
{
  for (int i = 1; i < HOTKEY_COUNT; ++i)
    if (hk == ::hotkey_values[i])
      return ::hotkey_names[i];

  return NULL;
}


//==============================================================================

struct PluginMenuRec
{
  const char *name;
  int idx;
};

FastNameMap DagorEdAppWindow::plugin_forced_export_order;

static int sortPluginsByName(const PluginMenuRec *a, const PluginMenuRec *b) { return ::strcmp(a->name, b->name); }


//==============================================================================
static unsigned get_face_count(StaticGeometryContainer &container)
{
  unsigned ret = 0;

  for (int i = 0; i < container.nodes.size(); ++i)
    ret += container.nodes[i]->mesh->mesh.face.size();

  return ret;
}


//==============================================================================
static void get_textures(StaticGeometryContainer &container, Tab<String> &tex_fnames)
{
  for (int i = 0; i < container.nodes.size(); ++i)
  {
    for (int j = 0; j < container.nodes[i]->mesh->mats.size(); ++j)
    {
      if (!container.nodes[i]->mesh->mats[j])
        continue;

      for (int k = 0; k < StaticGeometryMaterial::NUM_TEXTURES; ++k)
      {
        if (container.nodes[i]->mesh->mats[j]->textures[k])
        {
          const char *tex_fname = container.nodes[i]->mesh->mats[j]->textures[k]->fileName;

          if (tex_fname && *tex_fname)
          {
            bool known = false;

            for (int l = 0; l < tex_fnames.size(); ++l)
            {
              if (!stricmp(tex_fname, tex_fnames[l]))
              {
                known = true;
                break;
              }
            }

            if (!known)
              tex_fnames.push_back(String(tex_fname));
          }
        }
      }
    }
  }
}


//==============================================================================
bool DagorEdAppWindow::ignorePlugin(IGenEditorPlugin *p)
{
  if (!wsp)
    return true;

  int i;
  const Tab<String> &ignore = wsp->getDagorEdDisabled();
  const char *plugInterName = p->getInternalName();
  const char *plugMenuName = p->getMenuCommandName();

  if (cmdline_force_enabled_plugins.getNameId(plugMenuName) < 0)
    for (i = 0; i < ignore.size(); ++i)
      if (!::stricmp(ignore[i], plugMenuName))
        return true;
  if (cmdline_force_disabled_plugins.getNameId(plugMenuName) >= 0)
    return true;

  for (i = 0; i < plugin.size(); ++i)
    for (int j = 0; j < ignore.size(); ++j)
      if (cmdline_force_enabled_plugins.getNameId(ignore[j]) < 0 && !::stricmp(ignore[j], plugin[i].p->getMenuCommandName()))
        DAG_FATAL("Disabled plugin already registered <%s> enId=%d", ignore[j], cmdline_force_enabled_plugins.getNameId(ignore[j]));

  for (i = 0; i < plugin.size(); ++i)
  {
    if (plugin[i].p == p)
    {
      debug("Try to multiple register %p/%s!", p, plugInterName);
      return true;
    }
    else if (!stricmp(plugin[i].p->getInternalName(), plugInterName))
    {
      debug("Another plugin with the same internal name is already registered: %p/%s!", p, p->getInternalName());
      return true;
    }
  }

  return false;
}


// register/unregister plugins (every plugin should be registered once)
//==============================================================================
bool DagorEdAppWindow::registerPlugin(IGenEditorPlugin *p)
{
  if (!ignorePlugin(p))
  {
    debug("== register plugin %p/%s", p, p->getInternalName());

    Plugin pd;
    pd.p = p;
    pd.data = new DagorEdPluginData;

    plugin.push_back(pd);
    sortPlugins();

    p->registered();

    return true;
  }

  debug("\"%s\" (\"%s\") plugin disabled due to \"application.blk\" settings", p->getMenuCommandName(), p->getInternalName());

  return false;
}


//==============================================================================
bool DagorEdAppWindow::registerDllPlugin(IGenEditorPlugin *p, void *dll_handle)
{
  if (registerPlugin(p))
  {
    debug("== register DLL plugin %p/%s", p, p->getInternalName());
    for (int i = 0; i < plugin.size(); ++i)
      if (plugin[i].p == p)
      {
        plugin[i].data->dll = dll_handle;
        return true;
      }
  }

  return false;
}


//==============================================================================
bool DagorEdAppWindow::unregisterPlugin(IGenEditorPlugin *p)
{
  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p == p)
    {
      /*
      ::appWnd.removePlugin(p);
      */

      debug("== unregister plugin %p/%s", p, p->getInternalName());
      plugin[i].p->unregistered();

      if (plugin[i].data)
      {
        if (plugin[i].data->dll)
        {
          char buf[512];
          if (::GetModuleFileNameEx(GetCurrentProcess(), (HMODULE)plugin[i].data->dll, buf, 512))
            ::symhlp_unload(buf);
          ::FreeLibrary((HMODULE)plugin[i].data->dll);
        }
        else
          del_it(plugin[i].p);

        delete plugin[i].data;
      }

      erase_items(plugin, i, 1);
      return true;
    }

  return false;
}


//==============================================================================
DataBlock de3scannedResBlk;
void DagorEdAppWindow::initPlugins(const DataBlock &global_settings)
{
  // detect new gameres system model and setup engine support for it
  DataBlock appblk(String(260, "%s/application.blk", wsp->getAppDir()));

  if (const DataBlock *mnt = appblk.getBlockByName("mountPoints"))
  {
    const bool useAddonVromSrc = true;
    dblk::iterate_child_blocks(*mnt, [p = useAddonVromSrc ? "forSource" : "forVromfs"](const DataBlock &b) {
      dd_set_named_mount_path(b.getBlockName(), b.getStr(p));
    });
    dd_dump_named_mounts();
  }

  const DataBlock &blk = *appblk.getBlockByNameEx("assets");
  const DataBlock *exp_blk = appblk.getBlockByNameEx("assets")->getBlockByName("export");
  bool loadDDSxPacks = appblk.getBlockByNameEx("assets")->getStr("ddsxPacks", NULL) != NULL || minimize_dabuild_usage;
  if (exp_blk)
  {
    ::set_gameres_sys_ver(2);
    if (!loadDDSxPacks)
      init_dxp_factory_service();
  }


  // scan game resources
  ::reset_game_resources();
  de3scannedResBlk.reset();
  set_gameres_scan_recorder(&de3scannedResBlk, blk.getStr("gameRes", NULL), blk.getStr("ddsxPacks", NULL));

  if (exp_blk)
  {
    const DataBlock &expBlk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
    String ri_desc_fn, dm_desc_fn;
    if (const char *fn = blk.getBlockByNameEx("build")->getBlockByNameEx("rendInst")->getStr("descListOutPath", nullptr))
      ri_desc_fn.setStrCat(fn, ".bin");
    if (const char *fn = blk.getBlockByNameEx("build")->getBlockByNameEx("dynModel")->getStr("descListOutPath", nullptr))
      dm_desc_fn.setStrCat(fn, ".bin");

    const char *packlist = expBlk.getStr("packlist", NULL);
    const char *v_dest_fname = expBlk.getBlockByNameEx("destGrpHdrCache")->getStr("PC", NULL);
    const char *v_game_relpath = expBlk.getBlockByNameEx("destGrpHdrCache")->getStr("gameRelatedPath", NULL);

    String patch_dir;
    if (const char *dir = appblk.getBlockByNameEx("game")->getStr("res_patch_folder", NULL))
    {
      patch_dir.printf(0, "%s/%s", wsp->getAppDir(), dir);
      simplify_fname(patch_dir);
      DAEDITOR3.conNote("resource patches location: %s", patch_dir);
    }
    String game_dir(0, "%s/%s", wsp->getAppDir(), appblk.getBlockByNameEx("game")->getStr("game_folder", "game"));
    simplify_fname(game_dir);
    int game_dir_prefix_len = i_strlen(game_dir);

    if (packlist && expBlk.getBlockByNameEx("destination")->getStr("PC", NULL))
    {
      String mntPoint;
      assethlp::build_package_dest(mntPoint, expBlk, nullptr, wsp->getAppDir(), "PC", nullptr);

      String vrom_name;
      VirtualRomFsData *vrom = NULL;
      String plname(260, "%s/%s", mntPoint, packlist);
      simplify_fname(plname);

      if (v_dest_fname)
      {
        vrom_name.printf(260, "%s/%s", wsp->getAppDir(), v_dest_fname);
        simplify_fname(vrom_name);
        vrom = ::load_vromfs_dump(vrom_name, tmpmem);
        plname.printf(260, "%s/%s", v_game_relpath, packlist);
        if (vrom)
          ::add_vromfs(vrom);
        else if (!dd_file_exist(plname))
          plname = "";
        DAEDITOR3.conNote("loading main resources: %s (from VROMFS %s, %p)", plname.str(), vrom_name.str(), vrom);
      }
      else
        DAEDITOR3.conNote("loading main resources: %s", plname.str());

      if (!plname.empty())
        ::load_res_packs_from_list(plname, true, loadDDSxPacks, mntPoint);
      if (vrom)
      {
        ::remove_vromfs(vrom);
        tmpmem->free(vrom);
      }
      if (!ri_desc_fn.empty())
        gameres_append_desc(gameres_rendinst_desc, mntPoint + ri_desc_fn, "*");
      if (!dm_desc_fn.empty())
        gameres_append_desc(gameres_dynmodel_desc, mntPoint + dm_desc_fn, "*");

      String patch_fn(0, "%s/res/%s", patch_dir, dd_get_fname(vrom_name));
      const char *base_pkg_res_dir = nullptr;
      if (const char *add_folder_c = expBlk.getBlockByNameEx("destination")->getBlockByNameEx("additional")->getStr("PC", NULL))
      {
        String add_folder(0, "%s/%s", wsp->getAppDir(), add_folder_c);
        simplify_fname(add_folder);
        if (const char *pstr = strstr(mntPoint, add_folder))
          if (const char *add_folder_last = strrchr(add_folder, '/'))
            base_pkg_res_dir = pstr + strlen(add_folder) - strlen(add_folder_last) + 1;
      }
      if (base_pkg_res_dir)
        patch_fn.printf(0, "%s/%s/%s", patch_dir, base_pkg_res_dir, dd_get_fname(vrom_name));

      simplify_fname(patch_fn);
      if (!patch_dir.empty() && dd_file_exists(patch_fn) && (vrom = load_vromfs_dump(patch_fn, tmpmem)) != NULL)
      {
        DAEDITOR3.conNote("loading main resources (patch)");
        String mnt(0, "%s/res/", patch_dir);
        if (base_pkg_res_dir)
        {
          mnt.printf(0, "%s/%s/", patch_dir, base_pkg_res_dir);
          simplify_fname(mnt);
        }

        ::add_vromfs(vrom, true, mnt);
        load_res_packs_patch(mnt + packlist, vrom_name, true, loadDDSxPacks);
        ::remove_vromfs(vrom);
        tmpmem->free(vrom);
      }

      if (!patch_dir.empty() && base_pkg_res_dir)
      {
        if (!ri_desc_fn.empty())
          gameres_patch_desc(gameres_rendinst_desc, patch_dir + "/" + base_pkg_res_dir + ri_desc_fn, "*", mntPoint + ri_desc_fn);
        if (!dm_desc_fn.empty())
          gameres_patch_desc(gameres_dynmodel_desc, patch_dir + "/" + base_pkg_res_dir + dm_desc_fn, "*", mntPoint + dm_desc_fn);
      }

      if (const char *add_pkg_folder = expBlk.getBlockByNameEx("destination")->getBlockByNameEx("additional")->getStr("PC", NULL))
      {
        // loading additional packages
        String base(260, "%s/%s/", wsp->getAppDir(), add_pkg_folder);
        simplify_fname(base);
        const char *v_name = dd_get_fname(v_dest_fname);
        alefind_t ff;
        FastNameMapEx pkg_list;

        const DataBlock &pkgBlk = *expBlk.getBlockByNameEx("packages");
        const DataBlock &skipBuilt = *expBlk.getBlockByNameEx("skipBuilt");
        bool no_pkg_scan = pkgBlk.getBool("dontScanOtherFolders", false);

        for (int i = 0; i < pkgBlk.blockCount(); i++)
          pkg_list.addNameId(pkgBlk.getBlock(i)->getBlockName());

        for (bool ok = ::dd_find_first(base + "*.*", DA_SUBDIR, &ff); ok; ok = ::dd_find_next(&ff))
        {
          if (!(ff.attr & DA_SUBDIR))
            continue;
          if (stricmp(ff.name, "CVS") == 0 || stricmp(ff.name, ".svn") == 0)
            continue;
          if (stricmp(patch_dir, String(0, "%s%s", base.str(), ff.name)) == 0)
            continue;
          if (no_pkg_scan)
          {
            if (pkg_list.getNameId(ff.name) < 0)
              DAEDITOR3.conNote("skip: %s (due to dontScanOtherFolders:b=yes)", ff.name);
            continue;
          }
          pkg_list.addNameId(ff.name);
        }
        ::dd_find_close(&ff);

        for (int i = 0; i < pkg_list.nameCount(); i++)
        {
          const char *ff_name = pkg_list.getName(i);
          if (skipBuilt.getBool(ff_name, false) && src_assets_scan_allowed)
          {
            DAEDITOR3.conNote("skip: %s", ff_name);
            continue;
          }
          if (!expBlk.getBlockByNameEx("destGrpHdrCache")->getBool(ff_name, true))
          {
            DAEDITOR3.conNote("scanning resources in package: %s", ff_name);
            ::scan_for_game_resources(String(260, "%s/%s/res/", base.str(), ff_name), true, true);
            continue;
          }
          if (strnicmp(ff_name, "uhq_", 4) == 0 && minimize_dabuild_usage)
          {
            DAEDITOR3.conNote("skip loading resource package: %s (due to -min_dabuild)", ff_name);
            continue;
          }

          assethlp::build_package_dest(mntPoint, expBlk, ff_name, wsp->getAppDir(), "PC", nullptr);
          vrom = ::load_vromfs_dump(mntPoint + v_name, tmpmem);
          if (vrom)
          {
            DAEDITOR3.conNote("loading resource package: %s", ff_name);
            ::add_vromfs(vrom, false, mntPoint.str());
            mntPoint.resize(strlen(mntPoint) + 1); // required to compensate \0-pos changes

            ::load_res_packs_from_list(mntPoint + packlist, true, loadDDSxPacks, mntPoint);
            G_VERIFY(::remove_vromfs(vrom) == mntPoint.str());
            tmpmem->free(vrom);

            if (!ri_desc_fn.empty())
              gameres_append_desc(gameres_rendinst_desc, mntPoint + ri_desc_fn, ff_name);
            if (!dm_desc_fn.empty())
              gameres_append_desc(gameres_dynmodel_desc, mntPoint + dm_desc_fn, ff_name);
          }

          patch_fn.printf(0, "%s/%s/%s", patch_dir, mntPoint.str() + game_dir_prefix_len, v_name);
          simplify_fname(patch_fn);
          if (!patch_dir.empty() && dd_file_exists(patch_fn) && (vrom = load_vromfs_dump(patch_fn, tmpmem)) != NULL)
          {
            DAEDITOR3.conNote("loading resource package: %s (patch)", ff_name);
            String mnt(0, "%s/%s/", patch_dir, mntPoint.str() + game_dir_prefix_len);
            simplify_fname(mnt);
            ::add_vromfs(vrom, true, mnt);
            load_res_packs_patch(mnt + packlist, mntPoint + v_name, true, loadDDSxPacks);
            ::remove_vromfs(vrom);
            tmpmem->free(vrom);
          }

          if (!patch_dir.empty())
          {
            if (!ri_desc_fn.empty())
              gameres_patch_desc(gameres_rendinst_desc, patch_dir + "/" + (mntPoint.str() + game_dir_prefix_len) + ri_desc_fn, ff_name,
                mntPoint + ri_desc_fn);
            if (!dm_desc_fn.empty())
              gameres_patch_desc(gameres_dynmodel_desc, patch_dir + "/" + (mntPoint.str() + game_dir_prefix_len) + dm_desc_fn, ff_name,
                mntPoint + dm_desc_fn);
          }
        }
      }
    }
  }
  if (blk.getStr("gameRes", NULL) || blk.getStr("ddsxPacks", NULL))
  {
    int nid = blk.getNameId("prebuiltGameResFolder");
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid)
      {
        String resDir(260, "%s/%s/", wsp->getAppDir(), blk.getStr(i));
        ::scan_for_game_resources(resDir, true, blk.getStr("ddsxPacks", NULL));
      }
  }
  set_gameres_scan_recorder(NULL, NULL, NULL);
  // de3scannedResBlk.saveToTextFile("res_list_de3.blk");
  gameres_final_optimize_desc(gameres_rendinst_desc, "riDesc");
  gameres_final_optimize_desc(gameres_dynmodel_desc, "dynModelDesc");

  IFmodService *fmod = queryEditorInterface<IFmodService>();
  if (fmod)
    fmod->init(DAGORED2->getSoundFileName());

  if (exp_blk && loadDDSxPacks)
    ::init_dxp_factory_service();

  // init asset base
  DAEDITOR3.initAssetBase(wsp->getAppDir());

  String phmat_fn(wsp->getPhysmatPath());
  if (::dd_file_exist(phmat_fn))
    ::init_tps_physmat(phmat_fn);
  else if (!phmat_fn.empty())
    DAEDITOR3.conError("PhysMat: can't find physmat file: '%s'", phmat_fn);

  // init dynamic or classic renderer
  queryEditorInterface<IDynRenderService>()->setup(wsp->getAppDir(), appblk);
  queryEditorInterface<IDynRenderService>()->init();

  // init statically linked plugins and services
  ::dagored_init_all_plugins(appblk);

#if !_TARGET_STATIC_LIB
  // load DLL plugins
  initDllPlugins(::make_full_path(sgg::get_exe_path_full(), "plugins/daEditorX/"));

  const char *additionalPlug = wsp->getAdditionalPluginsPath();

  if (additionalPlug && *additionalPlug)
    initDllPlugins(additionalPlug);
#endif

  const DataBlock &pluginsRootSettings = *global_settings.getBlockByNameEx("plugins");
  for (int i = 0; i < getPluginCount(); ++i)
    if (IGenEditorPlugin *plugin = getPlugin(i))
      plugin->loadSettings(*pluginsRootSettings.getBlockByNameEx(plugin->getInternalName()));

  fillPluginTabs();

  de3scannedResBlk.reset();

  if (const DataBlock *b = appblk.getBlockByName("daEditorExportOrder"))
    for (int i = 0; i < b->paramCount(); i++)
      if (b->getParamType(i) == b->TYPE_STRING)
      {
        if (getPluginByMenuName(b->getStr(i)))
          plugin_forced_export_order.addNameId(b->getStr(i));
        else
          DAEDITOR3.conWarning("daEditorExportOrder{} refences unknown <%s>", b->getStr(i));
      }
}


//==============================================================================

void DagorEdAppWindow::preparePluginsListmenu()
{
  PropPanel::IMenu *_menu = getMainMenu();
  if (_menu)
    _menu->clearMenu(CM_PLUGINS_MENU);

  Tab<PluginMenuRec> plugins(tmpmem);
  plugins.reserve(plugin.size());

  int i;
  for (i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->showInMenu())
    {
      PluginMenuRec &r = plugins.push_back();
      r.name = plugin[i].p->getMenuCommandName();
      r.idx = i;
    }

  sort(plugins, &::sortPluginsByName);

  for (i = 0; i < plugins.size(); ++i)
    _menu->addItem(CM_PLUGINS_MENU, CM_PLUGINS_MENU_1ST + plugins[i].idx, plugins[i].name);

  if (curPluginId != -1)
    _menu->setRadioById(CM_PLUGINS_MENU_1ST + curPluginId, CM_PLUGINS_MENU_1ST, CM_PLUGINS_MENU_1ST + plugin.size());
}


void DagorEdAppWindow::fillPluginTabs()
{
  if (mTabPanel)
    mTabPanel->clear();

  Tab<PluginMenuRec> plugins(tmpmem);
  plugins.reserve(plugin.size());

  int i;
  for (i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->showInTabs())
    {
      PluginMenuRec &r = plugins.push_back();
      r.name = plugin[i].p->getMenuCommandName();
      r.idx = i;
    }

  sort(plugins, &::sortPluginsByName);

  for (i = 0; i < plugins.size(); ++i)
    mTabPanel->createTabPage(TAB_BASE_ID + plugins[i].idx, plugins[i].name);

  mTabWindow->setInt(TAB_PANEL_ID, -1);
}


void DagorEdAppWindow::switchPluginTab(bool next)
{
  Tab<PluginMenuRec> plugins(tmpmem);
  plugins.reserve(plugin.size());

  if (!plugin.size())
    return;

  int i;
  for (i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->showInTabs())
    {
      PluginMenuRec &r = plugins.push_back();
      r.name = plugin[i].p->getMenuCommandName();
      r.idx = i;
    }

  sort(plugins, &::sortPluginsByName);

  int _index = -1;
  for (i = 0; i < plugins.size(); ++i)
    if (plugins[i].idx == curPluginId)
    {
      _index = i;
      break;
    }

  (next) ? ++_index : --_index;

  if (_index < 0)
    _index = plugins.size() - 1;
  else if (_index >= plugins.size())
    _index = 0;

  switchToPlugin(plugins[_index].idx);
}


//==============================================================================

void DagorEdAppWindow::resetCore()
{
  undoSystem->clear();

  switchToPlugin(-1);

  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p)
    {
      debug("clearing %s...", plugin[i].p->getInternalName());
      plugin[i].p->clearObjects();
    }
  undoSystem->clear();

  while (plugin.size())
    unregisterPlugin(plugin[0].p);

  clear_and_shrink(plugin);
  reset_colliders_data();

  IFmodService *fmod = queryEditorInterface<IFmodService>();
  if (fmod)
    fmod->term();
}

//==============================================================================
void DagorEdAppWindow::terminateInterface()
{
  de3_show_tex_helper.demandDestroy();
  resetCore();
  terminate_interface_de3();
  IDagorEd2Engine::set(NULL);
}


//==============================================================================
bool DagorEdAppWindow::selectWorkspace(const char *app_blk_path)
{
  if (!canCloseScene("Workspace select"))
    return false;

  bool ret = false;

  wsp->save();

  wsp->initWorkspaceBlk(::make_full_path(sgg::get_exe_path_full(), "../.local/workspace.blk"));
  const DataBlock *wspBlk = wsp->findWspBlk(app_blk_path);

  const char *wspName = wspBlk ? wspBlk->getStr("name", NULL) : NULL;

  if (wspName ? wsp->load(wspName) : wsp->loadIndirect(app_blk_path))
  {
    //== FIXME: clear export pathes list!

    DAGORED2->addExportPath(_MAKE4C('PC'));

    dag::ConstSpan<unsigned> platf = wsp->getAdditionalPlatforms();
    for (int i = 0; i < platf.size(); ++i)
      DAGORED2->addExportPath(platf[i]);

    IGenEditorPlugin *cur = curPlugin();
    String switchToName;

    if (cur)
      switchToName = cur->getInternalName();

    resetCore();
    init3d();
    editor_core_initialize_imgui(nullptr);

    const String settingsPath = ::make_full_path(sgg::get_exe_path_full(), "../.local/de3_settings.blk");
    DataBlock settingsBlk(settingsPath);
    initPlugins(*settingsBlk.getBlockByNameEx("pluginSettings"));

    if (switchToName.length())
    {
      for (int i = 0; i < plugin.size(); ++i)
        if (plugin[i].p && !strcmp(plugin[i].p->getInternalName(), switchToName))
        {
          switchToPlugin(i);
          break;
        }
    }

    ret = true;
  }

  wsp->freeWorkspaceBlk();

  return ret;
}


//==============================================================================
int DagorEdAppWindow::getPluginCount() { return plugin.size(); }


//==============================================================================
IGenEditorPlugin *DagorEdAppWindow::getPlugin(int idx)
{
  if (idx >= 0 && idx < plugin.size())
    return plugin[idx].p;

  return NULL;
}


//==============================================================================
IGenEditorPlugin *DagorEdAppWindow::getPluginByName(const char *name) const
{
  if (name && *name)
  {
    const int plugCount = plugin.size();

    for (int i = 0; i < plugCount; ++i)
      if (plugin[i].p && !stricmp(name, plugin[i].p->getInternalName()))
        return plugin[i].p;
  }

  return NULL;
}
IGenEditorPlugin *DagorEdAppWindow::getPluginByMenuName(const char *name) const
{
  if (!name || !*name)
    return NULL;

  for (int j = 0; j < plugin.size(); j++)
    if (plugin[j].p && stricmp(plugin[j].p->getMenuCommandName(), name) == 0)
      return plugin[j].p;
  return NULL;
}

DagorEdPluginData *DagorEdAppWindow::getPluginData(int idx)
{
  if (idx >= 0 && idx < plugin.size())
    return plugin[idx].data;

  return NULL;
}


bool DagorEdAppWindow::spawnEvent(unsigned event_huid, void *user_data)
{
  if (event_huid == HUID_UseVisibilityOpt)
    updateUseOccluders();

  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->catchEvent(event_huid, user_data))
      return true;
  return services_catch_event(event_huid, user_data);
}


int DagorEdAppWindow::getNextUniqueId() { return ++lastUniqueId; }


// project files management
//==============================================================================
void DagorEdAppWindow::getProjectFolderPath(String &base_path)
{
  String projectName;
  splitProjectFilename(sceneFname, base_path, projectName);
}


//==============================================================================
String DagorEdAppWindow::getProjectFileName()
{
  String basePath, projectName;
  splitProjectFilename(sceneFname, basePath, projectName);
  return projectName;
}


//==============================================================================
const char *DagorEdAppWindow::getSdkDir() const { return wsp->getSdkDir(); }


//==============================================================================
const char *DagorEdAppWindow::getGameDir() const { return wsp->getGameDir(); }


//==============================================================================
const char *DagorEdAppWindow::getLibDir() const { return wsp->getLibDir(); }


//==============================================================================
const char *DagorEdAppWindow::getSoundFileName() const { return wsp->getSoundFileName(); }

//==============================================================================
const char *DagorEdAppWindow::getScriptLibrary() const { return wsp->getScriptLibrary(); }

//==============================================================================
const char *DagorEdAppWindow::getSoundFxFileName() const { return wsp->getSoundFxFileName(); }


//==============================================================================
float DagorEdAppWindow::getMaxTraceDistance() const { return wsp->getMaxTraceDistance(); }


//==============================================================================
dag::ConstSpan<unsigned> DagorEdAppWindow::getAdditionalPlatforms() const { return wsp->getAdditionalPlatforms(); }


//==============================================================================
void DagorEdAppWindow::addExportPath(int platf_id)
{
  for (int i = 0; i < exportPaths.size(); i++)
    if (exportPaths[i].platfId == platf_id)
      return;

  PlatformExportPath p;
  p.platfId = platf_id;

  exportPaths.push_back(p);
}

//==============================================================================
const char *DagorEdAppWindow::getExportPath(int platf_id) const
{
  for (int i = 0; i < exportPaths.size(); i++)
    if (exportPaths[i].platfId == platf_id)
      return exportPaths[i].path;

  return NULL;
}

//==============================================================================
const DeWorkspace &DagorEdAppWindow::getWorkspace() const
{
  G_ASSERT(wsp);
  return *wsp;
}
const EditorWorkspace &DagorEdAppWindow::getBaseWorkspace() { return GenericEditorAppWindow::getWorkspace(); }


//==============================================================================
void DagorEdAppWindow::getPluginProjectPath(const IGenEditorPlugin *plug, String &base_path) const
{
  String scene(sceneFname);

  if (!redirectPathTo.empty())
    if (redirectPluginList.hasPtr(plug))
    {
      ::location_from_path(scene);
      scene.aprintf(128, "/%s", redirectPathTo.str());
      goto final_add;
    }

  // if external plugin
  for (int i = 0; i < plugin.size(); i++)
    if (plugin[i].p == plug && plugin[i].data->externalSource && plugin[i].data->externalPath.length())
      scene = ::make_full_path(::de_get_sdk_dir(), plugin[i].data->externalPath);

  ::location_from_path(scene);

final_add:
  const char *name = plug->getInternalName();
  base_path = ::make_full_path(scene, name);
}


//==============================================================================
String DagorEdAppWindow::getPluginFilePath(const IGenEditorPlugin *plugin, const char *fname) const
{
  String dir;
  getPluginProjectPath(plugin, dir);

  return ::make_full_path(dir, fname);
}


//==============================================================================
bool DagorEdAppWindow::copyFileToProject(const char *from_filename, const char *to_filename, bool overwrite, bool add_to_cvs)
{
  //== check that to_filename is inside project?
  // TODO: remove overwrite parameter because it is not used

  /*
  CtlSingleProcessWindow spw(-1, -1, 500, 1, 50, getMainWnd());
  spw.start(true);

  spw.setActionDesc(String("Copy ") + from_filename);
  spw.setCurPos(0);
  */

  bool res = dag_copy_file(from_filename, to_filename);

  // spw.stop();

  if (!res)
    return false;

  if (!add_to_cvs)
    return true;

  return addFileToProject(to_filename);
}


//==============================================================================
bool DagorEdAppWindow::addFileToProject(const char *filename)
{
  //== check that it's inside project?
  //== check that it exists?
  //== add file to cvs
  return true;
}


//==============================================================================
bool DagorEdAppWindow::removeFileFromProject(const char *filename, bool remove_from_cvs)
{
  //== check that it's inside project?

  if (!dd_erase(filename))
    return false;

  if (remove_from_cvs)
  {
    //== remove from cvs
  }

  return true;
}


IWndManager *DagorEdAppWindow::getWndManager() const
{
  G_ASSERT(mManager && "window manager is NULL!");
  return mManager;
}


PropPanel::ContainerPropertyControl *DagorEdAppWindow::getCustomPanel(int id) const
{
  switch (id)
  {
    case GUI_MAIN_TOOLBAR_ID: return mToolPanel;
    case GUI_PLUGIN_TOOLBAR_ID: return mPlugTools;
  }

  return NULL;
}


void *DagorEdAppWindow::addRawPropPanel(hdpi::Px width)
{
  void *propbar = mManager->splitNeighbourWindow(mPlugTools->getParentWindowHandle(), 0, width, WA_RIGHT);
  if (!propbar)
    propbar = mManager->splitWindow(0, 0, width, WA_RIGHT);
  daeditor3_set_plugin_prop_panel(propbar);
  return propbar;
}


void DagorEdAppWindow::addPropPanel(int type, hdpi::Px width)
{
  panelsToAdd.push_back(type);
  panelsToAddWidth.push_back(_px(width));
}


void DagorEdAppWindow::removePropPanel(void *hwnd)
{
  panelsToDelete.push_back(hwnd);
  mManager->removeWindow(hwnd);
}


void DagorEdAppWindow::managePropPanels()
{
  if (panelsSkipManage)
    return;

  for (int i = 0; i < panelsToAdd.size(); ++i)
  {
    void *hwnd = addRawPropPanel(_pxActual(panelsToAddWidth[i]));
    mManager->setHeader(hwnd, HEADER_TOP);
    mManager->fixWindow(hwnd, true);
    mManager->setWindowType(hwnd, panelsToAdd[i]);
  }

  for (int i = 0; i < panelsToDelete.size(); ++i)
    mManager->removeWindow(panelsToDelete[i]);

  clear_and_shrink(panelsToAdd);
  clear_and_shrink(panelsToAddWidth);
  clear_and_shrink(panelsToDelete);
}


void DagorEdAppWindow::skipManagePropPanels(bool skip) { panelsSkipManage = skip; }


PropPanel::PanelWindowPropertyControl *DagorEdAppWindow::createPropPanel(ControlEventHandler *eh, const char *caption)
{
  return new PropPanel::PanelWindowPropertyControl(0, eh, nullptr, 0, 0, hdpi::Px(0), hdpi::Px(0), caption);
}


PropPanel::IMenu *DagorEdAppWindow::getMainMenu() { return GenericEditorAppWindow::getMainMenu(); }


void DagorEdAppWindow::deleteCustomPanel(PropPanel::ContainerPropertyControl *panel) { delete panel; }


PropPanel::DialogWindow *DagorEdAppWindow::createDialog(hdpi::Px w, hdpi::Px h, const char *title)
{
  return new PropPanel::DialogWindow(0, w, h, title);
}

void DagorEdAppWindow::deleteDialog(PropPanel::DialogWindow *dlg) { delete dlg; }


//==============================================================================
void DagorEdAppWindow::switchToPlugin(int plgId)
{
  if (!mPlugTools)
    return;

  ged.activateCurrentViewport();

  int curId = curPluginId;
  if (plgId == curPluginId)
    return;

  // delete gizmo
  setGizmo(NULL, MODE_None);

  IGenEditorPlugin *cur = curPlugin(), *next = (plgId == -1) ? NULL : plugin[plgId].p;

  // close current plugin mode
  if (cur && !cur->end())
    return;

  setEnabledColliders(activeColliders);

  //-------------------------------------------
  // clear menu, toolbar, their eh

  if (PropPanel::IMenu *mainMenu = getMainMenu())
    mainMenu->clearMenu(CM_PLUGIN_PRIVATE_MENU);

  if (mPlugTools)
  {
    mPlugTools->clear();
    mPlugTools->setEventHandler(NULL);
  }

  //-------------------------------------------

  // enter new plugin mode
  curPluginId = plgId;
  if (next)
  {
    if (!next->begin(GUI_PLUGIN_TOOLBAR_ID, CM_PLUGIN_PRIVATE_MENU))
    {
      if (cur)
        cur->begin(GUI_PLUGIN_TOOLBAR_ID, CM_PLUGIN_PRIVATE_MENU);

      curPluginId = curId;
      return;
    }
    ged.curEH = next->getEventHandler();

    // help
    String helpName(128, "%s help", next->getMenuCommandName());

    //::appWnd.setMenuItem(CM_PLUGIN_HELP, helpName);
    //::appWnd.enableMenuItem(CM_PLUGIN_HELP, (bool)next->getHelpUrl());

    // select all
    //::appWnd.enableMenuItem(CM_SELECT_ALL, next->showSelectAll());
    if (CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
      addEditorAccelerators();
  }

  // event handler setup
  ged.setEH(appEH);

  if (curPluginId != -1)
  {
    // select plugin's tab
    if (next && next->showInTabs())
      mTabWindow->setInt(TAB_PANEL_ID, TAB_BASE_ID + curPluginId);
    else
      mTabWindow->setInt(TAB_PANEL_ID, -1);

    // select in menu
    getMainMenu()->setRadioById(CM_PLUGINS_MENU_1ST + curPluginId, CM_PLUGINS_MENU_1ST, CM_PLUGINS_MENU_1ST + plugin.size());

    getMainMenu()->setCaptionById(CM_PLUGIN_PRIVATE_MENU, next->getMenuCommandName());
  }

  EDITORCORE->managePropPanels();
}


//==============================================================================


void DagorEdAppWindow::updateViewports() { shouldUpdateViewports = true; }


void DagorEdAppWindow::setViewportCacheMode(ViewportCacheMode mode) { ged.setViewportCacheMode(mode); }


void DagorEdAppWindow::invalidateViewportCache() { ged.invalidateCache(); }


int DagorEdAppWindow::getViewportCount() { return ged.getViewportCount(); }


IGenViewportWnd *DagorEdAppWindow::getViewport(int n) { return ged.getViewport(n); }


IGenViewportWnd *DagorEdAppWindow::getRenderViewport() { return ged.getRenderViewport(); }


IGenViewportWnd *DagorEdAppWindow::getCurrentViewport() { return ged.getCurrentViewport(); }


IDagorEd2Engine::BasisType DagorEdAppWindow::getGizmoBasisType() { return ged.tbManager->getBasisType(); }


IDagorEd2Engine::BasisType DagorEdAppWindow::getGizmoBasisTypeForMode(ModeType tp)
{
  return ged.tbManager->getGizmoBasisTypeForMode(tp);
}


IDagorEd2Engine::ModeType DagorEdAppWindow::getGizmoModeType() { return gizmoEH->getGizmoType(); }


IDagorEd2Engine::CenterType DagorEdAppWindow::getGizmoCenterType() { return ged.tbManager->getCenterType(); }


bool DagorEdAppWindow::isGizmoOperationStarted() const { return gizmoEH->isStarted(); }


void DagorEdAppWindow::setViewportZnearZfar(real zn, real zf) { ged.setZnearZfar(zn, zf); }


IGenViewportWnd *DagorEdAppWindow::screenToViewport(int &x, int &y) { return ged.screenToViewport(x, y); }


bool DagorEdAppWindow::clientToWorldTrace(IGenViewportWnd *vp, int x, int y, Point3 &world, real maxdist, Point3 *out_norm)
{
  Point3 campos, camdir;
  vp->clientToWorld(Point2(x, y), campos, camdir);

  real t = maxdist;

  bool success = false;

  if (out_norm)
  {
    if (DagorPhys::trace_ray_static(campos, camdir, t, *out_norm))
    {
      world = campos + camdir * t;
      success = true;
    }
  }
  else
  {
    if (DagorPhys::trace_ray_static(campos, camdir, t))
    {
      world = campos + camdir * t;
      success = true;
    }
  }

  Point3 planeNorm;
  if (traceWaterPlane(campos, camdir, t, &planeNorm))
  {
    Point3 p = campos + camdir * t;
    if (!success || (planeNorm.y > 0.f && p.y > world.y) || (planeNorm.y < 0.f && p.y < world.y))
    {
      world = p;
      if (out_norm)
        *out_norm = planeNorm;
    }
    success = true;
  }

  if (!success)
  {
    if (traceZeroLevelPlane(campos, camdir, t, out_norm))
    {
      world = campos + camdir * t;
      success = true;
    }
  }

  return success;
}


bool DagorEdAppWindow::screenToWorldTrace(int x, int y, Point3 &world, real maxdist, Point3 *out_norm)
{
  IGenViewportWnd *vp = screenToViewport(x, y);
  if (vp)
    return clientToWorldTrace(vp, x, y, world, maxdist, out_norm);

  return false;
}


//==============================================================================
void DagorEdAppWindow::setupColliderParams(int mode, const BBox3 &area) { DagorPhys::setup_collider_params(mode, area); }


//==============================================================================
bool DagorEdAppWindow::traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm, bool use_zero_plane)
{
  bool clip;

  if (out_norm)
    clip = DagorPhys::trace_ray_static(src, dir, dist, *out_norm);
  else
    clip = DagorPhys::trace_ray_static(src, dir, dist);

  if (clip)
    return true;
  else if (use_zero_plane)
    return traceZeroLevelPlane(src, dir, dist, out_norm);

  return false;
}


//==============================================================================
float DagorEdAppWindow::clipCapsuleStatic(const Capsule &c, Point3 &cap_pt, Point3 &world_pt)
{
  return DagorPhys::clip_capsule_static((Capsule &)c, cap_pt, world_pt);
}


//==============================================================================
void DagorEdAppWindow::registerCustomCollider(IDagorEdCustomCollider *coll) const { ::register_custom_collider(coll); }


//==============================================================================
void DagorEdAppWindow::unregisterCustomCollider(IDagorEdCustomCollider *coll) const { ::unregister_custom_collider(coll); }


//==============================================================================
void DagorEdAppWindow::enableCustomShadow(const char *name) const { ::enable_custom_shadow(name); }


//==============================================================================
void DagorEdAppWindow::disableCustomShadow(const char *name) const { ::disable_custom_shadow(name); }


//==============================================================================
void DagorEdAppWindow::enableCustomCollider(const char *name) const { ::enable_custom_collider(name); }


//==============================================================================
void DagorEdAppWindow::disableCustomCollider(const char *name) const { ::disable_custom_collider(name); }

//==============================================================================
void DagorEdAppWindow::setEnabledColliders(const Tab<String> &names) const { ::set_enabled_colliders(names); }


//==============================================================================
void DagorEdAppWindow::getEnabledColliders(Tab<String> &names) const { ::get_enabled_colliders(names); }

//==============================================================================
bool DagorEdAppWindow::isCustomShadowEnabled(const IDagorEdCustomCollider *collider) const
{
  return ::is_custom_shadow_enabled(collider);
}


//==============================================================================
int DagorEdAppWindow::getCustomCollidersCount() const { return ::get_custom_colliders_count(); }


//==============================================================================
IDagorEdCustomCollider *DagorEdAppWindow::getCustomCollider(int idx) const { return ::get_custom_collider(idx); }


//==============================================================================
bool DagorEdAppWindow::fillCustomCollidersList(PropPanel::ContainerPropertyControl &panel, const char *grp_caption, int grp_pid,
  int collider_pid, bool shadow, bool open_grp = false) const
{
  return ::fill_custom_colliders_list(panel, grp_caption, grp_pid, collider_pid, shadow, open_grp);
}


//==============================================================================
bool DagorEdAppWindow::onPPColliderCheck(int pid, const PropPanel::ContainerPropertyControl &panel, int collider_pid,
  bool shadow) const
{
  return ::on_pp_collider_check(pid, panel, collider_pid, shadow);
}


//==============================================================================
bool DagorEdAppWindow::getUseOnlyVisibleColliders() const { return DagorPhys::use_only_visible_colliders; }


//==============================================================================
void DagorEdAppWindow::setUseOnlyVisibleColliders(bool use) { DagorPhys::use_only_visible_colliders = use; }


//==============================================================================
dag::ConstSpan<IDagorEdCustomCollider *> DagorEdAppWindow::loadColliders(const DataBlock &blk, unsigned &out_filter_mask,
  const char *blk_name) const
{
  static Tab<IDagorEdCustomCollider *> list(tmpmem);

  list.clear();

  const DataBlock *collidersBlk = blk.getBlockByName(blk_name);

  unsigned old_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, 0x7FFFFFFF);

  if (collidersBlk)
  {
    if (collidersBlk->getBool("applyFilters", false))
      IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, 0);

    for (int i = 0; i < collidersBlk->paramCount(); ++i)
    {
      if (collidersBlk->getParamType(i) != DataBlock::TYPE_STRING)
        continue;

      const char *collName = collidersBlk->getStr(i);
      if (collName && *collName)
      {
        IDagorEdCustomCollider *c = NULL;
        for (int i = getCustomCollidersCount() - 1; i >= 0; i--)
          if (stricmp(getCustomCollider(i)->getColliderName(), collName) == 0)
          {
            c = getCustomCollider(i);
            break;
          }

        if (c)
          list.push_back(c);
        else
        {
          IGenEditorPlugin *p = DAGORED2->getPluginByMenuName(collName);

          IObjEntityFilter *f = p ? p->queryInterface<IObjEntityFilter>() : NULL;
          if (f && f->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION))
            f->applyFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION);
          else if (f)
            DAEDITOR3.conError("filter: %s doesnt support Collision filtering", collName);
          else
            DAEDITOR3.conError("unknown collider: %s", collName);
        }
      }
    }
  }

  out_filter_mask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, old_mask);
  return list;
}


void DagorEdAppWindow::setColliders(dag::ConstSpan<IDagorEdCustomCollider *> c, unsigned filt_mask) const
{
  set_custom_colliders(c, filt_mask);
}
void DagorEdAppWindow::restoreEditorColliders() const { restore_editor_colliders(); }


dag::ConstSpan<IDagorEdCustomCollider *> DagorEdAppWindow::getCurColliders(unsigned &out_filt_mask) const
{
  return get_current_colliders(out_filt_mask);
}
void DagorEdAppWindow::saveColliders(DataBlock &blk, dag::ConstSpan<IDagorEdCustomCollider *> coll, unsigned filter_mask,
  bool save_filters) const
{
  FastNameMap map;
  for (int i = 0; i < coll.size(); i++)
    map.addNameId(coll[i]->getColliderName());
  iterate_names(map, [&](int, const char *name) { blk.addStr("plugin", name); });

  if (save_filters)
  {
    map.reset();

    unsigned old_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
    DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, filter_mask);

    for (int i = plugin.size() - 1; i >= 0; i--)
    {
      IGenEditorPlugin *p = plugin[i].p;
      IObjEntityFilter *filter = p->queryInterface<IObjEntityFilter>();

      if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_COLLISION) &&
          filter->isFilteringActive(IObjEntityFilter::STMASK_TYPE_COLLISION))
        map.addNameId(p->getMenuCommandName());
    }

    DAEDITOR3.setEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION, old_mask);

    blk.addBool("applyFilters", save_filters);
    iterate_names(map, [&](int, const char *name) { blk.addStr("plugin", name); });
  }
}


//==============================================================================
static bool trace_xz_plane(float plane_y, const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm)
{
  if (fabs(dir.y - plane_y) < 0.0001f)
    return false;
  real t = (plane_y - src.y) / dir.y;
  if (t > 0.f && t < dist)
  {
    dist = t;
    if (out_norm)
    {
      if (dir.y < 0.f)
        *out_norm = Point3(0.f, 1.f, 0.f);
      else
        *out_norm = Point3(0.f, -1.f, 0.f);
    }
    return true;
  }
  return false;
}

bool DagorEdAppWindow::traceZeroLevelPlane(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm)
{
  return trace_xz_plane(0.f, src, dir, dist, out_norm);
}

bool DagorEdAppWindow::traceWaterPlane(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm)
{
  if (!waterService && !noWaterService)
  {
    waterService = queryEditorInterface<IWaterService>();
    noWaterService = !waterService;
  }

  if (waterService)
    return trace_xz_plane(waterService->get_level(), src, dir, dist, out_norm);
  else
    return false;
}


void DagorEdAppWindow::setViewportCameraViewProjection(unsigned viewport_no, TMatrix &view, real fov)
{
  if (viewport_no >= ged.getViewportCount())
    return;

  ged.getViewport(viewport_no)->setCameraViewProjection(view, fov);
}


void DagorEdAppWindow::switchCamera(unsigned from, unsigned to)
{
  for (unsigned int viewportNo = 0; viewportNo < ged.getViewportCount(); viewportNo++)
    ged.getViewport(viewportNo)->switchCamera(from, to);
}


void DagorEdAppWindow::setViewportCustomCameras(ICustomCameras *customCameras)
{
  for (unsigned int viewportNo = 0; viewportNo < ged.getViewportCount(); viewportNo++)
    ged.getViewport(viewportNo)->setCustomCameras(customCameras);
}


//==============================================================================
bool DagorEdAppWindow::getSelectionBox(BBox3 &box)
{
  box.setempty();
  if (curPluginId == -1)
    return false;

  if (plugin[curPluginId].p)
    return plugin[curPluginId].p->getSelectionBox(box);

  return false;
}

/*
//==============================================================================
Point3 DagorEdAppWindow::getStatusBarPos()
{
  const IGenEditorPlugin* cur = curPlugin();
  const IGenViewportWnd* viewport = ged.getActiveViewport();

  Point3 defPos = Point3(0, 0, 0);
  if (viewport)
  {
    TMatrix tm;
    viewport->getCameraTransform(tm);
    defPos = tm.getcol(3);
  }

  if (!cur)
    return defPos;

  Point3 pos;

  if (!cur->getStatusBarPos(pos))
    return defPos;

  return pos;
}
*/

//==============================================================================
void DagorEdAppWindow::setViewportCameraMode(unsigned int viewport_no, bool camera_mode)
{
  ged.getViewport(viewport_no)->setCameraMode(camera_mode);
}


//==============================================================================
void DagorEdAppWindow::setGizmo(IGizmoClient *gc, ModeType type)
{
  if (gizmoEH->isStarted() && !gc)
    gizmoEH->handleKeyPress(NULL, wingw::V_ESC, 0);

  // set a new gizmoclient
  ged.tbManager->setGizmoClient(gc, type);

  if (gc && gizmoEH->getGizmoClient() == gc)
  {
    gizmoEH->setGizmoType(type);
    repaint();
    return;
  }

  if (gizmoEH->getGizmoClient())
    gizmoEH->getGizmoClient()->release();

  gizmoEH->setGizmoType(type);
  gizmoEH->setGizmoClient(gc);
  gizmoEH->zeroOverAndSelected();

  ged.setEH(appEH);
  repaint();
}


//==============================================================================
void DagorEdAppWindow::startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (gizmoEH->getGizmoClient() && gizmoEH->getGizmoType() == MODE_None)
  {
    IGenEventHandler *eh = ged.curEH;
    ged.curEH = NULL;
    gizmoEH->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
    ged.curEH = eh;
  }
}


int DagorEdAppWindow::plugin_cmp_render_order(const Plugin *p1, const Plugin *p2)
{
  int diff = p1->p->getRenderOrder() - p2->p->getRenderOrder();
  if (diff)
    return diff;
  return strcmp(p1->p->getInternalName(), p2->p->getInternalName());
}


int DagorEdAppWindow::plugin_cmp_build_order(IGenEditorPlugin *const *p1, IGenEditorPlugin *const *p2)
{
  int ford0 = plugin_forced_export_order.getNameId((*p1)->getMenuCommandName());
  int ford1 = plugin_forced_export_order.getNameId((*p2)->getMenuCommandName());
  if (ford0 < 0)
    ford0 = 100000;
  if (ford1 < 0)
    ford1 = 100000;

  if (int diff = ford0 - ford1)
    return diff;
  if (int diff = (*p1)->getBuildOrder() - (*p2)->getBuildOrder())
    return diff;
  return strcmp((*p1)->getInternalName(), (*p2)->getInternalName());
}


//==============================================================================
void DagorEdAppWindow::sortPlugins()
{
  IGenEditorPlugin *p = curPlugin();

  sort(plugin, &plugin_cmp_render_order);
  if (p)
    curPluginId = findPlugin(p);
}


//==============================================================================
int DagorEdAppWindow::findPlugin(IGenEditorPlugin *p)
{
  for (int i = plugin.size() - 1; i >= 0; --i)
    if (plugin[i].p == p)
      return i;
  return -1;
}


//==============================================================================
void DagorEdAppWindow::showUiCursor(bool vis) { cursor.visible = vis; }


void DagorEdAppWindow::setUiCursorPos(const Point3 &pos, const Point3 *norm)
{
  cursor.pt = pos;
  if (norm)
    cursor.norm = *norm;
}


void DagorEdAppWindow::getUiCursorPos(Point3 &pos, Point3 &norm)
{
  pos = cursor.pt;
  norm = cursor.norm;
}


void DagorEdAppWindow::setUiCursorTex(TEXTUREID tex_id) { cursor.texId = tex_id; }


void DagorEdAppWindow::setUiCursorProps(float size, bool always_xz)
{
  cursor.size = size;
  cursor.xz_based = always_xz;
}

void DagorEdAppWindow::screenshotRender() { queryEditorInterface<IDynRenderService>()->renderScreenshot(); }

void DagorEdAppWindow::actObjects(float dt)
{
  cpujobs::release_done_jobs();

  PropPanel::process_message_queue();

  ged.act(dt);
  ViewportWindow *activeViewportWindow = ged.getActiveViewport();

  /*
  if (gridDlg)
  {
    bool update = false;
    gridDlg->act(update, activeViewportWindow ? ged.getCurrentViewportId() : -1);

    if (update)
      invalidateViewportCache();
  }
  */

  services_act(dt);
  for (int i = 0; i < plugin.size(); ++i)
    plugin[i].p->actObjects(dt);

  static float timeToTrackFiles = 0;
  timeToTrackFiles -= dt;
  if (timeToTrackFiles < 0)
  {
    IFileChangeTracker *tracker = queryEditorInterface<IFileChangeTracker>();
    if (tracker)
    {
      tracker->lazyCheckOnAct();
      timeToTrackFiles = 0.1;
    }
    else
      timeToTrackFiles = 10.0;
    regular_update_interface_de3();
  }

  /*
  if (cursorLast.visible != cursor.visible || ::memcmp(&cursorLast, &cursor, sizeof(cursor)))
    repaint();
  */
}


//==============================================================================
#include <libTools/renderViewports/cachedViewports.h>
#include <libTools/renderViewports/renderViewport.h>
extern CachedRenderViewports *ec_cached_viewports;

void DagorEdAppWindow::beforeRenderObjects()
{
  ddsx::tex_pack2_perform_delayed_data_loading();
  ViewportWindow *vpw = ged.getRenderViewport();
  if (!vpw)
  {
    ViewportWindow *vpa = ged.getCurrentViewport();
    if (vpa)
      for (int i = ec_cached_viewports->viewportsCount() - 1; i >= 0; i--)
        if (ec_cached_viewports->getViewportUserData(i) == vpa)
        {
          ec_cached_viewports->getViewport(i)->setViewProjTms();
          break;
        }
  }

  if (vpw) //== services currently doesn't require pre-frame-render update
    services_before_render();

  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->getVisible())
      plugin[i].p->beforeRenderObjects(vpw);

  if (vpw && shouldUpdateViewports)
  {
    shouldUpdateViewports = false;
    ged.redrawClientRect();
  }
}


//==============================================================================
void DagorEdAppWindow::renderObjects()
{
  ViewportWindow *vpw = ged.getRenderViewport();

  if (!vpw)
  {
    return;
  }

  services_render();
  if ((!isOrthogonalPreview() && !isRenderingOrtScreenshot()) || mScrData.renderObjects)
    for (int i = 0; i < plugin.size(); ++i)
    {
      if (plugin[i].p->getVisible())
      {
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_OBJECT);

        plugin[i].p->renderObjects();
      }
    }

  DAGORED2->spawnEvent(HUID_PostRenderObjects, NULL);

  renderGrid();
  renderOrtogonalCells();
}


void DagorEdAppWindow::onBeforeReset3dDevice()
{
  for (int i = 0; i < plugin.size(); ++i)
    plugin[i].p->onBeforeReset3dDevice();
}


bool DagorEdAppWindow::onDropFiles(const dag::Vector<String> &files)
{
  if (startupDlgShown)
    return startupDlgShown->onDropFiles(files);

  ViewportWindow *viewport = ged.getCurrentViewport();
  return viewport && viewport->onDropFiles(files);
}

//==================================================================================================
void DagorEdAppWindow::renderGrid()
{
  Tab<Point3> pt(tmpmem);
  Tab<Point3> dirs(tmpmem);

  Point3 center;

  const int gridConersNum = 4;
  const int gridDirectionsNum = 2;
  const real fakeGridSize = 200.f;
  const real perspectiveZoom = 600.f;

  pt.resize(gridConersNum);
  dirs.resize(gridDirectionsNum);

  TMatrix camera;
  ViewportWindow *vpw = ged.getRenderViewport();

  if (vpw->isOrthogonal() && grid.getUseInfiniteGrid())
    return;

  vpw->clientRectToWorld(pt.data(), dirs.data(), grid.getStep() * fakeGridSize);
  vpw->getCameraTransform(camera);

  float dh = fabs(camera.getcol(3).y - grid.getGridHeight()) + 1;
  grid.render(pt.data(), dirs.data(), fabs(vpw->isOrthogonal() ? vpw->getOrthogonalZoom() : perspectiveZoom / dh),
    ged.findViewportIndex(vpw));
}


//==============================================================================
void DagorEdAppWindow::renderTransObjects()
{
  ViewportWindow *vpw = ged.getRenderViewport();
  if (!vpw)
    return;

  services_render_trans();
  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->getVisible())
      plugin[i].p->renderTransObjects();

  /*
  if (cursor.visible)
  {
    // draw creation cursor
    if (drb)
    {
      drb->clearBuf();
      drb->drawQuad(cursor.pt + Point3( cursor.size,0,-cursor.size),
                    cursor.pt + Point3( cursor.size,0, cursor.size),
                    cursor.pt + Point3(-cursor.size,0, cursor.size),
                    cursor.pt + Point3(-cursor.size,0,-cursor.size),
                    E3DCOLOR(255,255,0,255));
      drb->flushToBuffer(cursor.texId);
      drb->flush();
    }
  }
  */

  cursorLast = cursor;

  if (de3_show_tex_helper.get())
  {
    int w = 1, h = 1;
    EDITORCORE->getRenderViewport()->getViewportSize(w, h);
    de3_show_tex_helper->setTargetSize(Point2(w, h));
    de3_show_tex_helper->render();
  }
}


//==============================================================================
void DagorEdAppWindow::showStats()
{
  StaticGeometryContainer visGeom;
  StaticGeometryContainer enviGeom;
  StaticGeometryContainer collisGeomGame;
  StaticGeometryContainer collisGeomEditor;
  int i;

  console->startProgress();
  console->setTotal(plugin.size());
  console->setActionDesc("Gathering geometry to analyze...");

  int oldMask = IObjEntityFilter::getSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT);
  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, 0xFFFFFFFF);

  for (i = 0; i < plugin.size(); ++i)
  {
    console->incDone();

    if (!plugin[i].p)
      continue;

    IGatherStaticGeometry *geom = plugin[i].p->queryInterface<IGatherStaticGeometry>();

    if (!geom)
      continue;

    console->setActionDesc("%s plugin...", plugin[i].p->getMenuCommandName());

    geom->gatherStaticVisualGeometry(visGeom);
    geom->gatherStaticEnviGeometry(enviGeom);
    geom->gatherStaticCollisionGeomGame(collisGeomGame);
    geom->gatherStaticCollisionGeomEditor(collisGeomEditor);
  }

  IObjEntityFilter::setSubTypeMask(IObjEntityFilter::STMASK_TYPE_EXPORT, oldMask);

  const unsigned visFaces = ::get_face_count(visGeom);
  const unsigned enviFaces = ::get_face_count(enviGeom);
  const unsigned clipGameFaces = ::get_face_count(collisGeomGame);
  const unsigned clipEditFaces = ::get_face_count(collisGeomEditor);

  Tab<String> texFnames(tmpmem);
  ::get_textures(visGeom, texFnames);
  ::get_textures(enviGeom, texFnames);
  ::get_textures(collisGeomGame, texFnames);
  ::get_textures(collisGeomEditor, texFnames);

  unsigned texSize = 0;

  for (i = 0; i < texFnames.size(); ++i)
  {
    file_ptr_t file = ::df_open(TextureMetaData::decodeFileName(texFnames[i]), DF_READ);
    if (file)
    {
      texSize += ::df_length(file);
      ::df_close(file);
    }
    else
      console->addMessage(ILogWriter::WARNING, "missing tex: %s", texFnames[i].str());
  }

  console->endProgress();

  wingw::message_box(0, "Level stats",
    "Visual geometry faces count:\t\t%u\n"
    "Environment geometry faces count:\t%u\n"
    "Game collision geometry faces count:\t%u\n"
    "Editor collision geometry faces count:\t%u\n\n"
    "Approximately textures memory size:\t%s (%u bytes)\n"
    "Textures count:\t\t\t%i",
    visFaces, enviFaces, clipGameFaces, clipEditFaces, (const char *)::bytes_to_mb(texSize), texSize, texFnames.size());
}


// disable all plugins render
//==============================================================================
void DagorEdAppWindow::disablePluginsRender()
{
  for (int i = 0; i < plugin.size(); ++i)
  {
    if (plugin[i].p)
    {
      if (plugin[i].data)
        plugin[i].data->oldVisible = plugin[i].p->getVisible();

      plugin[i].p->setVisible(false);
    }
  }
}


// enable plugins render, which could be rendered before call disablePluginsRender
//==============================================================================
void DagorEdAppWindow::enablePluginsRender()
{
  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p && plugin[i].data)
      plugin[i].p->setVisible(plugin[i].data->oldVisible);
}


//==============================================================================
void DagorEdAppWindow::getSortedListForBuild(Tab<IGenEditorPlugin *> &list, Tab<bool *> &export_list, Tab<bool *> &extern_list)
{
  list.clear();
  export_list.clear();
  extern_list.clear();

  for (int i = 0; i < plugin.size(); ++i)
    if (plugin[i].p->queryInterfacePtr(HUID_IBinaryDataBuilder))
      list.push_back(plugin[i].p);

  sort(list, &plugin_cmp_build_order);

  export_list.resize(list.size());
  extern_list.resize(list.size());

  for (int j = 0; j < list.size(); j++)
    for (int i = 0; i < plugin.size(); i++)
      if (plugin[i].p == list[j])
      {
        export_list[j] = &plugin[i].data->doExport;
        extern_list[j] = &plugin[i].data->externalExport;
      }
}

void DagorEdAppWindow::getFiltersList(Tab<IGenEditorPlugin *> &list, Tab<bool *> &use)
{
  list.clear();
  use.clear();

  for (int i = plugin.size() - 1; i >= 0; i--)
  {
    IObjEntityFilter *filter = plugin[i].p->queryInterface<IObjEntityFilter>();
    if (filter && filter->allowFiltering(IObjEntityFilter::STMASK_TYPE_EXPORT))
    {
      list.push_back(plugin[i].p);
      use.push_back(&plugin[i].data->doExportFilter);
    }
  }
}

//==============================================================================
void DagorEdAppWindow::beginBrushPaint() { brushEH->activate(); }


//==============================================================================
bool DagorEdAppWindow::isBrushPainting() const { return brushEH->isActive(); }

//==============================================================================
void DagorEdAppWindow::renderBrush() { brushEH->renderBrush(); }


//==============================================================================
void DagorEdAppWindow::setBrush(Brush *brush) { brushEH->setBrush(brush); }

//==============================================================================
Brush *DagorEdAppWindow::getBrush() const { return brushEH->getBrush(); }


//==============================================================================
void DagorEdAppWindow::endBrushPaint() { brushEH->deactivate(); }

//==============================================================================
void DagorEdAppWindow::onTabSelChange(int id)
{
  id -= TAB_BASE_ID;
  if (id >= 0 || id < plugin.size())
    switchToPlugin(id);
}

/*

void DagorEdAppWindow::checkLibrariesChange()
{
}
*/

bool DagorEdAppWindow::forceSaveProject() { return saveProject(sceneFname); }


bool DagorEdAppWindow::reloadProject() { return loadProject(sceneFname); }
