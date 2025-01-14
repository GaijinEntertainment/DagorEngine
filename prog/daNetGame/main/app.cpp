// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "app.h"
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_picMgr.h>
#include <3d/dag_lowLatency.h>
#include <asyncHTTPClient/asyncHTTPClient.h>
#include <debug/dag_logSys.h>
#include <debug/dag_textMarks.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <generic/dag_carray.h>
#include <generic/dag_initOnDemand.h>
#include <gui/dag_guiStartup.h>
#include <gui/dag_visualLog.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_threads.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_delayedAction.h>
#include <util/dag_localization.h>
#include <util/dag_threadPool.h>
#include <util/dag_fileMd5Validate.h>
#include <supp/dag_cpuControl.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <rendInst/rendInstGen.h>
#include <EASTL/tuple.h>
#include <daGame/timers.h>
#include <ecs/core/entityManager.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <ecs/rendInst/riExtra.h>
#include <daECS/scene/scene.h>
#include <daECS/core/updateStage.h>
#include <ecs/scripts/scripts.h>
#include <ecs/delayedAct/actInThread.h>
#include <propsRegistry/propsRegistry.h>
#include <webvromfs/webvromfs.h>
#include <folders/folders.h>
#include <statsd/statsd.h>
#include <util/dag_convar.h>
#include <gui/dag_imgui.h>
#include <daScript/daScript.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/nestdb/nestdb.h>
#include <perfMon/dag_memoryReport.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <math/random/dag_random.h>

#include "appProfile.h"
#include "camTrack/camTrack.h"
#include "camera/sceneCam.h"
#include <daEditorE/editorCommon/inGameEditor.h>
#include <webui/shaderEditors.h>
#include "ui/overlay.h"
#include "game/gameEvents.h"
#include "game/dasEvents.h"
#include "game/gameLauncher.h"
#include "game/gameScripts.h"
#include "game/player.h"
#include "game/riDestr.h"
#include "input/inputControls.h"
#include "input/uiInput.h"
#include "input/inputEvents.h"
#include "main/main.h"
#include "main/circuit.h"
#include "main/console.h"
#include "main/ecsUtils.h"
#include "main/gameLoad.h"
#include "main/level.h"
#include "main/settings.h"
#include "main/vromfs.h"
#include "net/dedicated.h"
#include "net/net.h"
#include "net/time.h"
#include "net/authEvents.h"
#include "phys/netPhys.h"
#include "phys/physUtils.h"
#include "phys/gridCollision.h"
#include "render/renderer.h"
#include "render/screencap.h"
#include <render/cinematicMode.h>
#include "render/animatedSplashScreen.h"
#include "sound/dngSound.h"
#include "sound_net/registerSoundNetProps.h"
#include "ui/userUi.h"
#include "ui/uiShared.h"
#include "ui/uiRender.h"
#include "main/browser/webBrowser.h"
#include "main/storeApiEvents.h"
#include <render/tdrGpu.h>
#include <render/androidScreenRotation.h>
#include <shaders/dag_rendInstRes.h>

#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/rendinstCollision.h>
#include <gamePhys/phys/rendinstFloating.h>
#include <phys/dag_physics.h>

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_GDK
#include <gdk/main.h>
#elif _TARGET_C3


#endif
#include <stdlib.h>

#include <syncVroms/syncVroms.h>

#include <debug/dag_memReport.h>
#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif

#define DEF_INPUT_EVENT ECS_REGISTER_EVENT
DEF_INPUT_EVENTS
#undef DEF_INPUT_EVENT


#if _TARGET_PC_WIN || _TARGET_XBOX
#include <windows.h> // SetThreadAffinityMask
#endif

#include <ecs/os/window.h>

extern void wait_start_fx_job_done(bool finish_async_update = false);

extern const float DM_DEFAULT_BSPHERE_RAD = 2.15f; // Max empirically measured distance to root (climbing onto something pose) is ~2.14


CONSOLE_FLOAT_VAL("mem", log_memreport_interval, 0);
CONSOLE_BOOL_VAL("mem", memreport_sys, true);
CONSOLE_BOOL_VAL("mem", memreport_gpu, true);

static InitOnDemand<WebVromfsDataCache> webVromfs;
extern void unload_localization();
extern void update_float_exceptions();

namespace game_scene
{
double total_time = 0.0;
static float timeSpeed = 1.f;
bool parallel_logic_mode = false;
static String scenePath;
static TMatrix cur_view_itm;

void updateInput(float rtDt, float dt, float cur_time)
{
  TIME_PROFILE(updateInput);
  controls::process_input(rtDt);
  g_entity_mgr->broadcastEventImmediate(UpdateStageUpdateInput(cur_time, dt));
  // In order to send controls to server in start of the frame, update phys input as early as possible
  phys_enqueue_controls(cur_time);
}

void update(float dt, float real_dt, float cur_time)
{
  TIME_PROFILE(update);
  total_time += dt;

  {
#if !_TARGET_PC
    TIME_PROFILE_DEV(pltf_update);
#endif
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_GDK
    gdk::update();
#elif _TARGET_C3

#endif
  }

  g_entity_mgr->broadcastEventImmediate(NetAuthEventOnUpdate{});

  dump_periodic_gpu_info();

  // This is only needed for cases when render/draw_scene isn't called (e.g. fullscreen & alt+tab on PC)
  // some fx can started on creation events or on update stages.
  wait_start_fx_job_done(/*finish_async_update*/ true);

  g_entity_mgr->tick();

  imgui_update();

  {
    if (is_level_loaded())
      g_entity_mgr->update(ecs::UpdateStageInfoAct(dt, cur_time));
    if (parallel_logic_mode)
    {
      if (dedicated::is_dedicated())
        updateInput(real_dt, dt, cur_time); // we must update input on dedicated for bots and other input logic!
      net_update();
    }
    if (is_level_loaded())
    {
      if (dedicated::is_dedicated())
        g_entity_mgr->broadcastEventImmediate(ParallelUpdateFrameDelayed(dt, cur_time));
      net_send_phys_snapshots(cur_time, dt); // after all phys/anim updates
      ridestr::update(dt, calc_active_camera_globtm());
    }
  }
  g_entity_mgr->broadcastEventImmediate(UpdateStageGameLogic(dt, cur_time));
  if (get_world_renderer() && !sceneload::is_load_in_progress())
  {
    TIME_PROFILE(get_world_renderer__update);
    get_world_renderer()->update(dt, real_dt, get_cam_itm());
  }
  g_entity_mgr->broadcastEventImmediate(EventOnGameUpdateAfterRenderer(dt, cur_time));

  if (is_level_loaded())
  {
    TIME_PROFILE(rendinst__update);
    Tab<Point3> points(framemem_ptr());
    if (get_world_renderer())
      points.assign(1, get_cam_itm().getcol(3));
    if (is_server())
      game::gather_players_poi(points);
    rendinst::scheduleRIGenPrepare(points);
    rendinst::updateRIGen(dt);
    dacoll::update_ri_instances(dt);
  }

  uishared::update();
  uiinput::update_joystick_input();
#if _TARGET_C3

#endif
}

eastl::tuple<float, float, float> updateTime() // [rtDt, dt, curTime]
{
  float rtDt = ::dagor_game_act_time;
  float rtDtNoSmoothing = ::dagor_game_act_time;
  if (const CinematicMode::Settings *cms = get_cinematic_mode_settings())
  {
    if (is_recording())
    {
      G_ASSERT(cms->videoSettings.fps);
      const float fps = cms->videoSettings.fps ? float(cms->videoSettings.fps) : 60.f;
      rtDt = rtDtNoSmoothing = 1.0 / fps;
    }
  }
  else if (screencap::fixed_act_rate() >= 0.f)
    rtDt = screencap::fixed_act_rate();
  else
    d3d::driver_command(Drv3dCommand::GET_GPU_FRAME_TIME, &rtDt, &rtDt); // Choose the smoothest dt.

  double curTime = get_time_mgr().advance(rtDtNoSmoothing * timeSpeed, /*out*/ rtDt);
  float dt = rtDt * timeSpeed;
  return eastl::make_tuple(rtDt, dt, curTime);
}

static void act_scene()
{
  if (log_memreport_interval.get() > 0)
    memreport::dump_memory_usage_report(int(log_memreport_interval.get() * 1000.f), memreport_sys.get(), memreport_gpu.get());
  if (sceneload::is_scene_switch_in_progress())
    return;

  gamescripts::update_deferred();

  uint64_t startTime = profile_ref_ticks();
  if (gamescripts::reload_changed())
    visuallog::logmsg(String(0, "Some das scripts were reloaded in %dms", profile_time_usec(startTime) / 1000).c_str());

  gamescripts::collect_das_garbage();

  float rtDt, dt, curTime;
  eastl::tie(rtDt, dt, curTime) = updateTime();

  if (parallel_logic_mode)
  {
    {
      TIME_PROFILE(game_timers_act);
      update_ecs_sq_timers(dt, rtDt);
      game::g_timers_mgr->act(dt);
    }

    camtrack::update_record(curTime, cur_view_itm);
    update(dt, rtDt, curTime); // input and netUpdate in parallel mode called between updateStageInfoAct and ParallelUpdateFrameDelayed
  }
  else
  {
    updateInput(rtDt, dt, curTime); // as close to frame start as possible

    {
      TIME_PROFILE(game_timers_act);
      update_ecs_sq_timers(dt, rtDt);
      game::g_timers_mgr->act(dt);
    }

    net_update();
    camtrack::update_record(curTime, cur_view_itm);
    update(dt, rtDt, curTime);
  }

#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
  if (ShaderGraphRecompiler *compiler = ShaderGraphRecompiler::getInstance())
    compiler->update(rtDt);
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT

  get_da_editor4().act(dt);

  dngsound::update(dt);

  g_entity_mgr->broadcastEventImmediate(OnStoreApiUpdate{});

  overlay_ui::update();
  dedicated::update();
  {
    TIME_PROFILE(visuallog_act);
    visuallog::act(rtDt);
  }
}

void before_draw_scene(int realtime_elapsed_usec, float gametime_elapsed_sec)
{
  ::before_draw_scene(realtime_elapsed_usec, gametime_elapsed_sec, timeSpeed, get_cur_cam_entity(), cur_view_itm);
  if (is_level_loaded())
  {
    dacoll::get_phys_world()->fetchSimRes(true);
#ifndef USE_BULLET_PHYSICS // We still need update (even with zero dt) for broadphase quadtree update
    if (dedicated::is_dedicated())
      dacoll::get_phys_world()->simulate(0.f);
#endif
  }
}

void draw_scene() { ::draw_scene(cur_view_itm); }

void on_scene_deselected()
{
  cpujobs::reset_job_queue(ecs::get_common_loading_job_mgr(), true);

  int64_t cpuRefTime = ref_time_ticks();
  while (cpujobs::is_job_manager_busy(ecs::get_common_loading_job_mgr()))
  {
    sleep_msec(100);
    perform_delayed_actions();
    const int loopTimeMsec = get_time_usec(cpuRefTime) / 1000;
    if (loopTimeMsec > 5 * 1000)
    {
      G_ASSERT_LOG(0, "busy loop takes too long time %d", loopTimeMsec);
      break;
    }
  }

  close_world_renderer();
}
} // namespace game_scene

static int loading_job_mgr_id = -1;

void load_res_package(const char *folder, bool optional)
{
  String mnt0(0, "%s/", folder);
  String mntP(0, "content/patch/%s", mnt0);
  bool base_pkg_loaded = false;
  bool load_tex = !dgs_get_settings()->getBlockByNameEx("texStreaming")->getBool("disableTexScan", false) && have_renderer();

  gameres_append_desc(gameres_rendinst_desc, mnt0 + "riDesc.bin", folder);
  gameres_append_desc(gameres_dynmodel_desc, mnt0 + "dynModelDesc.bin", folder);

  {
    RAIIVrom vrom(mnt0 + "grp_hdr.vromfs.bin", optional, tmpmem, true, mnt0.str());
    if (vrom.isLoaded())
    {
      debug("loading res pkg: %s", mnt0);
      if (strstr(folder, "/tomoe"))
      {
        const bool do_harmonization = ::dgs_get_settings()->getBool("harmonizationRequired", false) ||
                                      ::dgs_get_settings()->getBool("harmonizationEnabled", false);

        DataBlock res_list;
        dblk::load(res_list, mnt0 + "respacks.blk");
        if (DataBlock *b = res_list.getBlockByName("ddsxTexPacks"))
          for (int i = b->paramCount() - 1, nid = b->getNameId("pack"); i >= 0; i--)
            if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
              if (strstr(b->getStr(i), do_harmonization ? "_orig.dxp.bin" : "_harm.dxp.bin"))
                b->removeParam(i);
        ::load_res_packs_from_list_blk(res_list, mnt0 + "respacks.blk", true, load_tex, mnt0);
      }
      else
        ::load_res_packs_from_list(mnt0 + "respacks.blk", true, load_tex, mnt0);
      base_pkg_loaded = true;
    }
    else
      logwarn("missing res pkg: %s", mnt0);
  }

  if (base_pkg_loaded)
  {
    gameres_patch_desc(gameres_rendinst_desc, mntP + "riDesc.bin", folder, mnt0 + "riDesc.bin");
    gameres_patch_desc(gameres_dynmodel_desc, mntP + "dynModelDesc.bin", folder, mnt0 + "dynModelDesc.bin");

    RAIIVrom vrom(mntP + "grp_hdr.vromfs.bin", true, framemem_ptr(), true, mntP);
    if (vrom.isLoaded())
    {
      debug("loading res pkg(patch): %s", folder);
      ::load_res_packs_patch(mntP + "respacks.blk", mnt0 + "grp_hdr.vromfs.bin", true, load_tex);
    }
  }
}

void init_loading_job_manager()
{
  G_ASSERT(loading_job_mgr_id < 0 || loading_job_mgr_id == cpujobs::COREID_IMMEDIATE);
#if (_TARGET_C1 || _TARGET_PC_LINUX) && DAGOR_DBGLEVEL > 0
  constexpr int stackSize = 512 << 10; // debug mode has huge stack heaps
#else
  constexpr int stackSize = (128 + 64) << 10;
#endif
  loading_job_mgr_id = cpujobs::create_virtual_job_manager(stackSize, WORKER_THREADS_AFFINITY_MASK, "LoadingJobMgr");
  if (DAGOR_UNLIKELY(loading_job_mgr_id < 0))
    DAG_FATAL("Can't start LoadingJobMgr");
  register_job_manager_requiring_shaders_bindump(loading_job_mgr_id);
  ecs::set_common_loading_job_mgr(loading_job_mgr_id);
}

void set_timespeed(float ts) { game_scene::timeSpeed = ts; }

float get_timespeed() { return game_scene::timeSpeed; }

static float lastTimeSpeed = 1.0f;
void toggle_pause()
{
  if (game_scene::timeSpeed == 0.f)
    game_scene::timeSpeed = lastTimeSpeed;
  else
  {
    lastTimeSpeed = game_scene::timeSpeed;
    game_scene::timeSpeed = 0.f;
  }
}

static void send_exit_metrics()
{
  int mins_from_start = get_time_msec() / 1000 / 60;
  if (mins_from_start <= 1)
    statsd::counter("app.exit_queued", 1, {"time_from_start", "1min"}); // only login?
  else if (mins_from_start <= 5)
    statsd::counter("app.exit_queued", 1, {"time_from_start", "5min"}); // less than one average battle. rage quit?
  else if (mins_from_start <= 30)
    statsd::counter("app.exit_queued", 1, {"time_from_start", "30min"}); // probably 0-3 average battles
  else if (mins_from_start <= 120)
    statsd::counter("app.exit_queued", 1, {"time_from_start", "120min"}); // more than average game session
  else
    statsd::counter("app.exit_queued", 1, {"time_from_start", "huge"}); // just for everyone else

  gamelauncher::send_exit_metrics();
}

static void send_dedicated_exit_metrics()
{
  if (dedicated::is_dynamic_tickrate_enabled())
  {
    uint32_t logBucket = get_log2i(dedicated::number_of_tickrate_changes + 1);
    statsd::counter("tickrate.changes", 1, {"logBucket", eastl::to_string(logBucket).c_str()});
  }
}

static void init_main_thread(void *);
static void init_threads(const DataBlock &cfg)
{
  int coreCount = cpujobs::get_core_count();
  // Note: according to measurements of current threadpool & ecs parallel for implementation there is (almost)
  // no perfomance gain with more then 6 work threads (together with main thread this corresponds to 7/8 cores
  // of typical consumer Intel CCU or one Ryzen's CCX)
  int maxWorkThreads = cfg.getInt("maxWorkThreads", 6);
  int num_workers = eastl::max(cfg.getInt("numWorkThreads", eastl::min(coreCount - /*main*/ 1, maxWorkThreads)), 0);
  int stack_size = cfg.getInt("workThreadsStackSize", 192 << 10);
#if DAGOR_DBGLEVEL > 1 || defined(__SANITIZE_THREAD__)
  stack_size *= 2;
#endif
  threadpool::init(num_workers, 2048, stack_size);
  debug("threadpool inited for %d workers", num_workers);
  add_delayed_callback(init_main_thread, nullptr);
}

#if _TARGET_C1

#endif
static void init_main_thread(void *)
{
#if _TARGET_PC_WIN || _TARGET_XBOX
  const DataBlock &cfg = *dgs_get_settings()->getBlockByNameEx("debug");
  G_ASSERT(is_main_thread());
  // boost main thread priority in order to overrule loading threads (resources, textures, etc...)
  // we increase main thread priority by one step. We only support two priorities: high and low
  if (cfg.getBool("boostMainThreadPriority", true))
    DaThread::applyThisThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);

  if (cfg.getBool("pinMainThreadToCore", true))
    G_VERIFY(SetThreadAffinityMask(GetCurrentThread(), MAIN_THREAD_AFFINITY));
#elif _TARGET_C1


#endif
  if (is_float_exceptions_enabled())
    update_float_exceptions();
}


static bool app_is_post_shutdown_state = false;
bool dng_is_app_terminated() { return app_is_post_shutdown_state; }

void app_close()
{
  app_is_post_shutdown_state = true;
  // should be called before quirrel VMs termination
  sqeventbus::send_event("app.shutdown");
  // since both overlay_ui and user_ui use sqeventbus::ProcessingMode::MANUAL_PUMP we need to push event
  if (auto *vm = overlay_ui::get_vm())
    sqeventbus::process_events(vm);
  if (auto *vm = user_ui::get_vm())
    sqeventbus::process_events(vm);

  da_profiler::sync_stop_sampling();
  memoryreport::stop_report();

  if (!dedicated::is_dedicated())
  {
    send_exit_metrics();
    android_screen_rotation::shutdown();
  }
  else
    send_dedicated_exit_metrics();
#if _TARGET_PC_WIN
  if (!is_level_loaded())
  {
    net_stop();
    _exit(0);
  }
#endif
  ecs_os::cleanup_window_handler();
  g_entity_mgr->broadcastEventImmediate(OnStoreApiTerm{});

  PictureManager::prepare_to_release();
  gamescripts::shutdown_before_ecs();
  sceneload::unload_current_game();
  net_destroy(/*final=*/true);

  browserutils::shutdown_web_browser();

  controls::global_destroy();

  user_ui::term();
  overlay_ui::shutdown_ui(true);
  overlay_ui::shutdown_network_services();
  uishared::term();

  imgui_shutdown();
  term_da_editor4(); // before ecs destroy since it's depends on entity manager

  gamescripts::shutdown();
  g_entity_mgr->broadcastEventImmediate(EventOnGameShutdown());

  ecs::g_scenes.demandDestroy();
  g_entity_mgr->clear();
  destroy_world_renderer();

  dagor_select_game_scene(NULL);
  unmount_all_vroms();
  unregister_all_virtual_vrom();
  if (ecs::get_common_loading_job_mgr() != cpujobs::COREID_IMMEDIATE)
  {
    cpujobs::destroy_virtual_job_manager(loading_job_mgr_id, true);
    ecs::set_common_loading_job_mgr(-1);
  }
  dacoll::term_collision_world();
  threadpool::shutdown();
  propsreg::clear_registry();
  shutdown_debug_text_marks();
  game::g_timers_mgr.demandDestroy();

  dngsound::close();
  unload_localization();

  dedicated::shutdown();
  PictureManager::release();
  if (webVromfs)
  {
    webVromfs->term();
    webVromfs.demandDestroy();
  }
  screencap::term();
  console::term_ingame();
  nestdb::shutdown();
  g_entity_mgr->broadcastEventImmediate(NetAuthEventOnTerm{});
  httprequests::shutdown_async();
  syncvroms::shutdown();
  g_entity_mgr.demandDestroy();
  ecs::clear_component_manager_registry();
}

#include <ioSys/dag_memIo.h>
static void dump_config_blk(const DataBlock &configBlk)
{
#if DAGOR_DBGLEVEL > 0 || _TARGET_PC // We can't get full logs from consoles in release anyway so doesn't bother writing it
  static const char *excludeConfigBlkDumpNames[] = {
    "embeddedUpdater" // contains private yutags for dev circuits
  };
  DataBlock dumpBlk;
  dumpBlk.setFrom(&configBlk);
  for (const char *exludeBlockName : excludeConfigBlkDumpNames)
    dumpBlk.removeBlock(exludeBlockName);
  DynamicMemGeneralSaveCB dump(tmpmem);
  dumpBlk.saveToTextStream(dump);
  dump.writeInt(0);
  logmessage(_MAKE4C('SPAM'), "%s:\n%s", "config", dump.data());
#else
  G_UNUSED(configBlk);
#endif
}

static void init_auth()
{
  if (app_profile::get().devMode || app_profile::get().disableRemoteNetServices)
    return;
  // Read settings.blk from Launcher for "distr" (if any)
  const char *gameDistr = "";
  DataBlock conf;

  static const char *conf_fn = "settings.blk";
  if (::dd_file_exists(conf_fn))
  {
    FullFileLoadCB crd(conf_fn, DF_READ | DF_IGNORE_MISSING);
    Tab<char> blk_content;
    bool overrides_detected = false;
    blk_content.resize(::df_length(crd.fileHandle) + 1);
    crd.read(blk_content.data(), blk_content.size() - 1);
    blk_content.back() = 0;

    String tmp_str, tmp_str2;
    for (const char *p0 = blk_content.data(), *p = strstr(p0, "\"@override:"); p; p = strstr(p0, "\"@override:"))
    {
      overrides_detected = true;
      const char *pe = strchr(p, '\n');
      if (!pe)
        pe = blk_content.data() + blk_content.size() - 1;
      else if (*(pe - 1) == '\r')
        pe--;

      tmp_str.setSubStr(p + 11, pe);
      tmp_str2.printf(0, "\"@delete-all:%s; \"%s", tmp_str, tmp_str);

      size_t idx = p - blk_content.data();
      insert_items(blk_content, idx, tmp_str2.length() - (pe - p));
      memcpy(&blk_content[idx], tmp_str2.data(), tmp_str2.length());
      p0 = blk_content.data() + idx + tmp_str2.length();
    }

    if (dblk::load_text(conf, blk_content, dblk::ReadFlag::ROBUST_IN_REL, conf_fn))
    {
      gameDistr = conf.getStr("distr", "");
      if (overrides_detected)
      {
        dblk::save_to_text_file(conf, conf_fn);
        logwarn("@override: was found in %s; config is preprocessed and saved", conf_fn);
      }
    }
    else
      logwarn("failed to load text BLK from %s%s", conf_fn, overrides_detected ? ", overrides detected" : "");
  }

  g_entity_mgr->broadcastEventImmediate(NetAuthEventOnInit{::get_game_name(), gameDistr, app_profile::get().appId});
}

extern void register_vsync_plugin();

void app_start()
{
#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::AppState appState("app_start");
#endif

  const DataBlock &settings = *dgs_get_settings();
  dump_config_blk(settings);
  const DataBlock &debugSettings = *settings.getBlockByNameEx("debug");

  execute_delayed_action_on_main_thread(make_delayed_action([]() { controls::global_init(); }));
  execute_delayed_action_on_main_thread(make_delayed_action([]() { g_entity_mgr->broadcastEventImmediate(EventOnGameInit()); }));
  register_vsync_plugin();
  console::init_ingame();
  screencap::init(&settings);

  httprequests::InitAsyncParams initHttpAsyncParams;
  initHttpAsyncParams.verboseDebug = debugSettings.getBool("httpClientVerbose", false);
  httprequests::init_async(initHttpAsyncParams);
  init_auth();

  ::dgs_limit_fps = settings.getBool("limitFps", false);
  dagor_set_game_act_rate(settings.getInt("actRate", -60));
  if (!dedicated::is_dedicated())
    init_threads(debugSettings);

  nestdb::init();
  uishared::init_early();
  uirender::init();

  sceneload::GamePackage gameInfo = sceneload::load_game_package();
  sceneload::load_package_files(gameInfo, false);
  load_gameparams(get_game_name());

  if (!dedicated::is_dedicated())
  {
    ::startup_gui_base("ui/fonts.blk");
    ::startup_game(RESTART_ALL);
    if (settings.getBool("useWebVromfs", false))
    {
      webVromfs.demandInit();
      ::register_url_tex_load_factory(&WebVromfsDataCache::vromfs_get_file_data, webVromfs.get());
      DataBlock params;
      params.addBlock("baseUrls")->addStr("url", "http://localhost:8000");
      webVromfs->init(params, folders::get_path("webcache", "webcache"));
    }
    android_screen_rotation::init();
  }

  init_world_renderer();

  execute_delayed_action_on_main_thread(make_delayed_action([]() { dngsound::init(); }));

  soundnet::register_props();

#if DAGOR_DBGLEVEL == 0
  if (app_profile::get().devMode)
#endif
    init_da_editor4();

  if (!dedicated::is_dedicated())
    init_loading_job_manager();

  game::g_timers_mgr.demandInit();
  if (dedicated::is_dedicated())
  {
    float selfDestructTime = dgs_get_game_params()->getBlockByNameEx("timers")->getReal("sessionSelfDestructTimeSec", 6.f * 3600.f);
    if (selfDestructTime > 0.f)
    {
      game::Timer timer;
      game::g_timers_mgr->setTimer(
        timer, []() { exit_game("session self destruct timer"); }, selfDestructTime);
      timer.release(); // fire & forget
    }
  }

  {
    using namespace dacoll;
    auto collInitFlags = InitFlags::Default & ~InitFlags::ForceUpdateAABB;
    if (!app_profile::get().haveGraphicsWindow)
      collInitFlags &= ~InitFlags::EnableDebugDraw;
    if (dedicated::is_dedicated())
      collInitFlags |= InitFlags::SingleThreaded;
    init_collision_world(collInitFlags, 0.1f);
    set_add_instances_to_world(true);
    // TTL must be grater than ttl for ragdolls in order to process ragdoll collision with RI correctly
    set_ttl_for_collision_instances(dedicated::is_dedicated() ? 0.5f : 35.f);
    set_trace_game_objects_cb(grid_trace_main_entities);
    set_gather_game_objects_on_ray_cb(grid_process_main_entities_collision_objects_on_ray);
  }

  {
    d3d::GpuAutoLock gpuLock;
    d3d::setpersp(Driver3dPerspective(1, 1, 0.1, 100, 0, 0));
  }

  browserutils::init_web_browser();

  syncvroms::init_base_vroms();

  static class GameScene : public DagorGameScene
  {
  public:
    virtual void actScene() override
    {
      game_scene::act_scene();
      if (is_animated_splash_screen_in_thread() && !sceneload::is_load_in_progress())
      {
        debug("[splash] auto stop due to is_load_in_progress()=%d", sceneload::is_load_in_progress());
        stop_animated_splash_screen_in_thread();
      }
    }
    virtual void drawScene() override
    {
      if (is_animated_splash_screen_in_thread())
      {
        animated_splash_screen_draw(d3d::get_backbuffer_tex());
        if (imgui_get_state() != ImGuiState::OFF)
          imgui_endframe();
        return;
      }
      game_scene::draw_scene();
      android_screen_rotation::onFrameEnd();
    }
    virtual void sceneDeselected(DagorGameScene *) override { game_scene::on_scene_deselected(); }
    virtual void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec) override
    {
      if (is_animated_splash_screen_in_thread())
        return;
      game_scene::before_draw_scene(realtime_elapsed_usec, gametime_elapsed_sec);
    }
  } dagor_game_scene;

  game_scene::parallel_logic_mode = ::dgs_get_settings()->getBool("parallel_logic_mode", false);
  dagor_select_game_scene(&dagor_game_scene);

  G_VERIFY(gamescripts::init());

  gamescripts::run();

  delayed_call([] {
    gamescripts::main_thread_post_load();
    g_entity_mgr->broadcastEventImmediate(EventOnGameAppStarted());
  });

  // this DA will be executed after actual scene load
  delayed_call([]() {
#if DAGOR_DBGLEVEL > 0
    if (::dgs_get_settings()->getBlockByNameEx("debug")->getBool("imguiStartWithOverlay", false))
      imgui_request_state_change(ImGuiState::OVERLAY);
#endif

    ecs_os::init_window_handler();

    memoryreport::init();
    memoryreport::start_report();
  });
}

static int (*defaultRIGenLodSkip)(const char *name, bool has_impostors, int total_lods);

int customRIGenLodSkip(const char *name, bool, int total_lods)
{
  if (total_lods <= 2)
    return 0;

  return defaultRIGenLodSkip(name, false, total_lods);
}

void init_game()
{
  bool useSimplifiedRi = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("riSimplifiedRender", false);
  bool shouldInitGpuObjects = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("gpuObjects", true);
  rendinst::initRIGen(app_profile::get().haveGraphicsWindow, 8 * 8 + 8, -1.f, dacoll::register_collision_cb,
    dacoll::unregister_collision_cb, -1, -1.f, useSimplifiedRi, shouldInitGpuObjects);
  if (useSimplifiedRi)
  {
    defaultRIGenLodSkip = RenderableInstanceLodsResource::get_skip_first_lods_count;
    RenderableInstanceLodsResource::get_skip_first_lods_count = customRIGenLodSkip;
  }

  rendinst::init_phys();
  ridestr::init(app_profile::get().haveGraphicsWindow);
}

void term_game()
{
  ridestr::shutdown();
  rendinst::termRIGen();
  propsreg::cleanup_props();
  dacoll::clear_collision_world();
  cleanup_phys();
  gamescripts::collect_garbage();
  g_entity_mgr->broadcastEventImmediate(EventOnGameTerm());
}
