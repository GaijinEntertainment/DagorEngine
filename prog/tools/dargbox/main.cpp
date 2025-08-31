// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>

#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <startup/dag_startupTex.h>
#include <startup/dag_fatalHandler.inc.cpp>

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiPointingData.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_picMgr.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_texPackMgr2.h>
#include <fx/dag_commonFx.h>
#include <gui/dag_guiStartup.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <memory/dag_framemem.h>
#include <gui/dag_baseCursor.h>
#include <debug/dag_debug.h>
#include <debug/dag_visualErrLog.h>
#include <perfMon/dag_cpuFreq.h>
#include <hid_mouse/api_wrappers.h>
#include "main.h"
#include "joystick.h"
#include "drawFps.h"
#include <webvromfs/webvromfs.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <json/json.h>
#include "initScript.h"
#include "vr.h"
#include <quirrel/udp/udp.h>

#include "setupLogsDir.h"

#if _TARGET_PC_WIN
#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_winMain.inc.cpp>
#elif _TARGET_PC_LINUX
#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_linuxMain.inc.cpp>
#elif _TARGET_ANDROID
#include <startup/dag_androidMain.inc.cpp>
#elif _TARGET_C3






#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
#include <startup/dag_xboxOneMain.inc.cpp>
#endif

static InitOnDemand<DrawFpsGuiMgr> gui_mgr;
static InitOnDemand<WebVromfsDataCache> webVromfs;

struct MountVromfsRec
{
  VirtualRomFsPack *vd;
  SimpleString fn;
  SimpleString mountPath;

  MountVromfsRec() : vd(NULL) {}
  ~MountVromfsRec()
  {
    remove_vromfs(vd);
    close_vromfs_pack(vd, inimem);
    vd = NULL;
  }
};
DAG_DECLARE_RELOCATABLE(MountVromfsRec);
static Tab<MountVromfsRec> mnt_vromfs(inimem);

static bool mount_vrom(const char *fn, const char *mnt_path)
{
  if (!dd_file_exist(fn))
    return false;

  int idx = -1;
  for (int i = 0; i < mnt_vromfs.size(); i++)
    if (dd_stricmp(mnt_vromfs[i].fn, fn) == 0)
    {
      idx = i;
      break;
    }
  if (idx < 0)
    idx = append_items(mnt_vromfs, 1);
  else
  {
    remove_vromfs(mnt_vromfs[idx].vd);
    close_vromfs_pack(mnt_vromfs[idx].vd, inimem);
  }

  MountVromfsRec *rec = &mnt_vromfs[idx];
  rec->vd = open_vromfs_pack(fn, inimem);
  if (!rec->vd)
  {
    erase_items(mnt_vromfs, idx, 1);
    return false;
  }

  rec->fn = fn;
  rec->mountPath = mnt_path;
  add_vromfs(rec->vd, false, rec->mountPath);
  debug("  mounted %s to %s", fn, rec->mountPath.str());
  return true;
}
static void unmount_vrom(const char *fn)
{
  for (int i = 0; i < mnt_vromfs.size(); i++)
    if (dd_stricmp(mnt_vromfs[i].fn, fn) == 0)
    {
      erase_items(mnt_vromfs, i, 1);
      return;
    }
}

void DagorWinMainInit(int, bool) {}

static void post_shutdown_handler()
{
  dargbox_app_shutdown();
  bindquirrel::udp::shutdown();
  httprequests::shutdown_async();
  clear_and_shrink(mnt_vromfs);

  vr::destroy();

  Tab<VirtualRomFsData *> vd;
  VromReadHandle::lock.lockRead();
  vd = vromfs_get_entries_unsafe();
  VromReadHandle::lock.unlockRead();
  for (int i = 0; i < vd.size(); i++)
  {
    remove_vromfs(vd[i]);
    inimem->free(vd[i]);
  }
  clear_and_shrink(vd);
  register_url_tex_load_factory(NULL, NULL);
  if (webVromfs)
  {
    webVromfs->term();
    webVromfs.demandDestroy();
  }

  shutdown_game(RESTART_UI);
  PictureManager::release();
  DEBUG_CTX("shutdown!");
  shutdown_game(RESTART_INPUT);
  shutdown_game(RESTART_ALL);
  cpujobs::term(false, 0);

#if DAGOR_DBGLEVEL > 0
  const DataBlock *debugBlk = dgs_get_settings()->getBlockByNameEx("debug");
  bool doFatalOnLogerrOnExit = debugBlk->getBool("fatalOnLogerrOnExit", true);
  if (doFatalOnLogerrOnExit)
    visual_err_log_check_any();
#endif
}


static struct Reset3DCallback : public IDrv3DResetCB
{
  void beforeReset(bool full_reset) override { PictureManager::before_d3d_reset(); }
  void afterReset(bool full_reset) override
  {
    debug("Reset3DCallback::afterReset");
    debug_flush(false);
    if (full_reset)
    {
      PictureManager::after_d3d_reset();
      ddsx::reload_active_textures(0);
    }
    StdGuiRender::after_device_reset();
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    debug_flush(false);
  }
} drv_3d_reset_cb;


static void dagor_main(int nCmdShow)
{
#if _TARGET_PC
  ::measure_cpu_freq();
#elif _TARGET_C3






#endif

#if _TARGET_PC_LINUX
  char cwdbuf[DAGOR_MAX_PATH] = {0};
  if (getcwd(cwdbuf, sizeof(cwdbuf)) != nullptr)
  {
    dd_append_slash_c(cwdbuf);
    dd_remove_base_path("");
    dd_add_base_path(cwdbuf);
  }
  else
  {
    logwarn("failed to get current work directory");
  }
#endif

#if _TARGET_ANDROID
  {
    static char android_mount_path[DAGOR_MAX_PATH] = "";
    const char *vromsFileNames[] = {"android.vromfs.bin", "dargbox.vromfs.bin"};
    for (const char *vromFileName : vromsFileNames)
    {
      VirtualRomFsData *vfsys = load_vromfs_from_asset(vromFileName, inimem);
      if (vfsys)
      {
        android_mount_path[0] = 0;
        add_vromfs(vfsys, true, android_mount_path);
      }
      else
        logerr("failed to load vromfs: %s", vromFileName);
    }
  }
#else
  if (!::dgs_get_argv("nobasevrom"))
  {
    VirtualRomFsData *vfbase = load_vromfs_dump("dargbox.vromfs.bin", inimem);
    static char main_mount_path[DAGOR_MAX_PATH] = "";
    if (vfbase)
      add_vromfs(vfbase, true, main_mount_path);
  }
#endif
  //::dgs_dont_use_cpu_in_background = true;
  ::dgs_post_shutdown_handler = post_shutdown_handler;
  dagor_install_dev_fatal_handler(NULL);


#if _TARGET_ANDROID
  const char *settingsBlk = "settings-android.blk";
#else
  const char *settingsBlk = "settings.blk";
#endif

  dgs_load_settings_blk(true, settingsBlk);

  dgs_load_game_params_blk("params.blk");

  cpujobs::init();

  run_init_script("earlyInitScript");

  int nid = ::dgs_get_settings()->getNameId("addpath");
  for (int i = 0; i < ::dgs_get_settings()->paramCount(); i++)
    if (::dgs_get_settings()->getParamType(i) == DataBlock::TYPE_STRING && ::dgs_get_settings()->getParamNameId(i) == nid)
    {
      String buf(0, "%s/", ::dgs_get_settings()->getStr(i));
      dd_simplify_fname_c(buf);
      if (dd_add_base_path(buf))
        DEBUG_CTX("addpath: %s", buf.str());
    }


  bool useAddonVromSrc = false;
  if (const DataBlock *debugBlk = dgs_get_settings()->getBlockByName("debug"))
    useAddonVromSrc = debugBlk->getBool("useAddonVromSrc", false);

  if (const DataBlock *mnt = ::dgs_get_settings()->getBlockByName("mountPoints"))
  {
    dblk::iterate_child_blocks(*mnt, [p = useAddonVromSrc ? "forSource" : "forVromfs"](const DataBlock &b) {
      dd_set_named_mount_path(b.getBlockName(), b.getStr(p));
    });
    dd_dump_named_mounts();
  }


  {
    DataBlock cfg;
    if (dd_file_exist("dargbox.config.blk"))
      dblk::load(cfg, "dargbox.config.blk", dblk::ReadFlag::ROBUST);
    debug_print_datablock("dargbox.config.blk", &cfg);
    dgs_apply_config_blk(cfg, /* force_apply_all */ true, /* store_cfg_copy */ false);
  }

  vromfs_first_priority = false;
  nid = ::dgs_get_settings()->getNameId("vromfs");
  for (int i = 0; i < ::dgs_get_settings()->paramCount(); i++)
    if (::dgs_get_settings()->getParamType(i) == DataBlock::TYPE_STRING && ::dgs_get_settings()->getParamNameId(i) == nid)
    {
      const char *fn = ::dgs_get_settings()->getStr(i);
      if (VirtualRomFsData *d = load_vromfs_dump(fn, midmem))
        add_vromfs(d);
      else
        LOGERR_CTX("failed to load vromfs: %s", fn);
    }

  nid = ::dgs_get_settings()->getNameId("basevromfs");
  for (int i = 0; i < ::dgs_get_settings()->blockCount(); i++)
  {
    const DataBlock *blk = ::dgs_get_settings()->getBlock(i);
    if (blk->getBlockNameId() != nid)
      continue;
    for (int j = 0; j < blk->paramCount(); j++)
      if (blk->getParamType(j) == DataBlock::TYPE_STRING)
      {
        const char *fn = blk->getStr(j);
#if _TARGET_ANDROID
        VirtualRomFsData *d = load_vromfs_from_asset(fn, midmem);
#else
        VirtualRomFsData *d = load_vromfs_dump(fn, midmem);
#endif
        if (d)
          add_vromfs(d);
        else
          LOGERR_CTX("failed to load vromfs: %s", fn);
      }
  }

  nid = ::dgs_get_settings()->getNameId("mount");
  for (int i = 0; i < ::dgs_get_settings()->blockCount(); i++)
    if (::dgs_get_settings()->getBlock(i)->getBlockNameId() == nid)
      mount_vrom(::dgs_get_settings()->getBlock(i)->getStr("vromfs"), ::dgs_get_settings()->getBlock(i)->getStr("mnt"));


  bindquirrel::udp::init();
  run_init_script("initScript");

  bool drawFps = ::dgs_get_settings()->getBool("drawFps", false);


  webVromfs.demandInit();
  ::register_url_tex_load_factory(&WebVromfsDataCache::vromfs_get_file_data, webVromfs.get());
  ::register_common_game_tex_factories();

  register_dds_tex_create_factory();

  register_jpeg_tex_load_factory();
  register_tga_tex_load_factory();
  register_png_tex_load_factory();
  register_avif_tex_load_factory();
  register_svg_tex_load_factory();
  register_any_tex_load_factory();

  register_loadable_tex_create_factory();

  ::register_lottie_tex_load_factory();

  visual_err_log_setup(false);

  ::dagor_init_video("DagorWClass", nCmdShow, NULL, "Loading...", "dargbox");

#if _TARGET_PC
  ::dagor_init_keyboard_win();
  ::dagor_init_mouse_win();
#elif _TARGET_IOS | _TARGET_TVOS | _TARGET_ANDROID | _TARGET_C3
  ::dagor_init_keyboard_win();
  ::dagor_init_mouse_win();
#endif

  ::dargbox_init_joystick();
  HumanInput::stg_pnt.touchScreen = ::dgs_get_settings()->getBool("touchScreen", false);
  HumanInput::stg_pnt.emuTouchScreenWithMouse = ::dgs_get_settings()->getBool("emuTouchScreenWithMouse", false);

  ::startup_game(RESTART_ALL);
  shaders_register_console(true);

  startup_shaders(::dgs_get_settings()->getStr("shaders", "shaders/game"));

  ::startup_game(RESTART_ALL);

  ::dagor_common_startup();

  int maxTexCount = ::dgs_get_settings()->getBlockByNameEx("video")->getInt("maxTexCount", 8192);
  enable_tex_mgr_mt(true, maxTexCount);

  const char *webcacheFolder = "webcache";
#if _TARGET_C3

#endif
  if (webVromfs)
  {
    DataBlock params;
    params.addBlock("baseUrls")->addStr("url", "http://localhost:8000");
    webVromfs->init(params, webcacheFolder);
  }

  DataBlock picMgrBlkDef;
  picMgrBlkDef.setBool("createAsyncLoadJobMgr", true);
  picMgrBlkDef.setBool("dynAtlasLazyAllocDef", true);
  picMgrBlkDef.setBool("fatalOnPicLoadFailed", false);
  PictureManager::init(::dgs_get_settings()->getBlockByNameEx("picMgr", &picMgrBlkDef));

  int quads = dgs_get_settings()->getInt("guiBufQuads", 8192);
  debug("GUI buffers: %d quads", quads);
  StdGuiRender::init_dynamic_buffers(quads);
  ::startup_gui_base(::dgs_get_settings()->getStr("fonts", "fonts/fonts.blk"));
  ::startup_game(RESTART_ALL);

  gui_mgr.demandInit();
  ::dagor_gui_manager = gui_mgr;

  ::dgs_draw_fps = drawFps;

  const DataBlock &blkFx = *dgs_get_settings()->getBlockByNameEx("effects");

  ::set_gameres_sys_ver(2);
  if (blkFx.getStr("res", NULL))
  {
    ShaderGlobal::enableAutoBlockChange(true);
    ::register_effect_gameres_factory();
    ::register_all_common_fx_factories();

    ::load_res_packs_from_list(blkFx.getStr("res"));
  }

#if _TARGET_PC
  ::win32_set_window_title("dargbox");
#endif

  ::set_3d_device_reset_callback(&drv_3d_reset_cb);
  dargbox_app_init();
}
int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  dagor_main(nCmdShow);
  ::dagor_reset_spent_work_time();

#if _TARGET_APPLE
  // Dagor hides cursor, we should show it back
  mouse_api_hide_cursor(false);
#endif

  for (;;) // infinite cycle
    ::dagor_work_cycle();
  return dargbox_get_exit_code(); // Not sure if we will ever get here, but just in case
}

#include <startup/dag_leakDetector.inc.cpp>
