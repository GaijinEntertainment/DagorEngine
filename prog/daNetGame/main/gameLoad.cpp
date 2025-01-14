// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameLoad.h"
#include <EASTL/optional.h>
#include <EASTL/numeric.h>
#include <daGame/timers.h>
#include <ui/overlay.h>
#include "app.h"
#include "camera/sceneCam.h"
#include <consoleKeyBindings/consoleKeyBindings.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "game/gameScripts.h"
#include "game/player.h"
#include "input/globInput.h"
#include "input/inputControls.h"
#include "main/appProfile.h"
#include "main/ecsUtils.h"
#include "main/level.h"
#include "main/physMat.h"
#include "main/settings.h"
// #include "main/skies.h"
#include "main/vromfs.h"
#include "main/watchdog.h"
#include "net/net.h"
#include "net/replay.h"
#include "net/dedicated.h"
#include <daECS/net/netEvents.h>
#include <daECS/net/msgSink.h>
#include "net/userid.h"
#include "render/renderer.h"
#include "render/hdrRender.h"
#include "sound/dngSound.h"
#include "ui/userUi.h"
#include "ui/uiShared.h"

#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <util/dag_delayedAction.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <ioSys/dag_findFiles.h>
#include <ecs/core/entityManager.h>
#include <ecs/scene/scene.h>
#include <ecs/scripts/dascripts.h>
#include <daScript/daScript.h>
#include <daScript/ast/ast_serializer.h>
#include <ecs/anim/slotAttach.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <daECS/net/connid.h>
#include <daECS/io/blk.h>
#include <ecs/scripts/scripts.h>
#include <bindQuirrelEx/autoBind.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <sqrat.h>
#include <sqstdblob.h>
#include "main.h"
#include "render/renderSettings.h"
#include "render/animatedSplashScreen.h"
#include <gui/dag_visualLog.h>
#include <forceFeedback/forceFeedback.h>
#include <3d/dag_picMgr.h>
#include "main/circuit.h"
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
#include <gdk/package.h>
#endif
#include <statsd/statsd.h>
#include "net/netMsg.h"
#include <syncVroms/syncVroms.h>
#include <contentUpdater/fsUtils.h>
#include <contentUpdater/binaryCache.h>
#include <util/dag_console.h>
#include <ecs/scripts/dasEs.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_cpuJobs.h>
#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif
#include "rapidJsonUtils/rapidJsonUtils.h"
#include <json/value.h>
#include <atomic>
#include <osApiWrappers/basePath.h>
#include <osApiWrappers/fs_hlp.h>
#include <drv/3d/dag_renderTarget.h>
#include "updaterEvents.h"

extern void (*get_memcollect_cur_thread_cb())(); // FIXME: move to engine header (dag_memBase.h)

namespace gamescripts
{
const char *get_serialized_data_filename(char buf[DAGOR_MAX_PATH]);
uint32_t initialize_deserializer(const char *a);
} // namespace gamescripts
namespace bind_dascript
{
extern bool serializationReading;
extern bool enableSerialization;
extern bool debugSerialization;
extern das::unique_ptr<das::AstSerializer> initSerializer;
extern das::unique_ptr<das::AstSerializer> initDeserializer;
} // namespace bind_dascript

struct MemCollectJob final : public cpujobs::IJob
{
  void (*memcollect_cur_thread)();
  void doJob() override { memcollect_cur_thread(); }
};
DAG_DECLARE_RELOCATABLE(MemCollectJob);

static void mem_collect_threads(bool loading_job_only = false)
{
  void (*memcollect_cur_thread)() = get_memcollect_cur_thread_cb();
  if (!memcollect_cur_thread)
    return;
  memcollect_cur_thread(); // Main's thread goes first to collect abandoned segments
  int nw = loading_job_only ? 0 : threadpool::get_num_workers();
  dag::RelocatableFixedVector<MemCollectJob, 8, true, framemem_allocator> jobs(nw + 1);
  for (int i = 0; i < jobs.size(); ++i)
  {
    jobs[i].memcollect_cur_thread = memcollect_cur_thread;
    if (i == 0)
      cpujobs::add_job(ecs::get_common_loading_job_mgr(), &jobs[0], false, /*need_release*/ false);
    else
      threadpool::add(&jobs[i], threadpool::PRIO_DEFAULT, /*wake*/ false);
  }
  if (nw)
    threadpool::wake_up_all();
  spin_wait([&] { return cpujobs::is_job_running(ecs::get_common_loading_job_mgr(), &jobs[0]); });
  for (int i = 1; i < jobs.size(); ++i)
    threadpool::wait(&jobs[i]);
}

extern void init_component_replication_filters();

extern void dng_load_localization();

void add_search_path(const char *path_name)
{
  if (!dd_add_base_path(path_name, false))
    logerr("dd_add_base_path(%s) failed", path_name);
#if _TARGET_C1 || _TARGET_C2 || _TARGET_C3 || _TARGET_ANDROID
  if (const char *pname = df_get_real_folder_name(path_name))
  {
    if (pname != path_name && strcmp(pname, path_name) != 0)
      dd_add_base_path(pname, false);
  }
  else
    logwarn("real path for \"%s\" is not found, skipping additional dd_add_base_path", path_name);
#endif
}
void del_search_path(const char *path_name)
{
#if _TARGET_C1 || _TARGET_C2 || _TARGET_C3 || _TARGET_ANDROID
  if (const char *pname = df_get_real_folder_name(path_name))
  {
    if (pname != path_name && strcmp(pname, path_name) != 0)
      dd_remove_base_path(pname);
  }
#endif
  dd_remove_base_path(path_name);
}

namespace sceneload
{

static eastl::vector<eastl::string> current_basePaths;
static GamePackage current_game;
static std::atomic<bool> switch_scene_flag(false);
static UserGameModeInfo user_game_mode_info;
bool unload_in_progress = false;

const UserGameModeInfo &get_user_mode_info()
{
  static UserGameModeInfo empty;
  return is_user_game_mod() ? user_game_mode_info : empty;
}

bool is_user_game_mod() { return get_current_game().ugmContentID != ""; }

const GamePackage &get_current_game() { return current_game; }

void set_scene_blk_path(const char *lblk_path) { current_game.levelBlkPath = lblk_path; }

static void load_common_game_client_scenes(bool (*load_scene_cb)(const char *, int) = nullptr);
void apply_scene_level(eastl::string &&name, eastl::string &&lblk_path)
{
  current_game.sceneName = eastl::move(name);
  current_game.levelBlkPath = eastl::move(lblk_path);

  // Before loading common game scenes which might request game resources which might interfere with this settings setup
  DataBlock levelBlk;
  if (!current_game.levelBlkPath.empty() && dblk::load(levelBlk, current_game.levelBlkPath.c_str(), dblk::ReadFlag::ROBUST_IN_REL))
    prepare_united_vdata_setup(&levelBlk);

  load_common_game_client_scenes();
}

static String expand_path(const char *p)
{
  String path(0, "%s/", p);
  dd_simplify_fname_c(path);
  path.shrink_to_fit();
  return path;
}

void setup_base_resources(const GamePackage &game_info, dag::ConstSpan<VromLoadInfo> extra_vroms, bool load_game_res)
{
  const auto appendRawAddon = [](const eastl::string &rawAddon) {
    String mnt(0, "%s/", rawAddon);
    debug("scanning raw res from %s", mnt.c_str());
    ::scan_for_game_resources(mnt, true /*scan_subdirs*/, true /*scan_dxp*/, true /*allow_override*/, true /*scan_vromfs*/);
    gameres_append_desc(gameres_rendinst_desc, mnt + "riDesc.bin", rawAddon.c_str(), true);
    gameres_append_desc(gameres_dynmodel_desc, mnt + "dynModelDesc.bin", rawAddon.c_str(), true);
  };
  apply_vrom_list_difference(*game_info.gameSettings.getBlockByNameEx("vromList"), extra_vroms);
  if (load_game_res)
  {
    for (const eastl::string &addon : game_info.addons)
    {
      TIME_PROFILE(load_res_package);
      load_res_package(addon.c_str());
      // process inputs to avoid application not responding status (we load on main thread in blocking fashion)
      ::dagor_idle_cycle();
    }
    if (app_profile::get().devMode)
      for (const eastl::string &rawAddon : game_info.rawAddons) // we use this for CDK only, rework to some commandline or options
      {
        TIME_PROFILE(append_raw_res);
        appendRawAddon(rawAddon);
      }
    if (!game_info.ugmContentID.empty())
    {
      for (const eastl::string &ugmAddon : game_info.ugmAddons)
      {
        TIME_PROFILE(append_raw_res);
        appendRawAddon(ugmAddon);
      }
    }
    gameres_final_optimize_desc(gameres_rendinst_desc, "riDesc");
    gameres_final_optimize_desc(gameres_dynmodel_desc, "dynModelDesc");
  }
  watchdog_kick();
}

void load_package_files(const GamePackage &game_info, bool load_game_res)
{
  if (current_basePaths == game_info.basePaths)
    debug("base paths are the same (%d paths), skip reinit", current_basePaths.size());
  else
  {
    for (auto const &path : current_basePaths)
    {
      debug("REMOVE base path: %s", path.c_str());
      del_search_path(path.c_str());
    }
    for (auto const &path : game_info.basePaths)
    {
      debug("ADD base path: %s", path.c_str());
      add_search_path(path.c_str());
    }
    current_basePaths = game_info.basePaths;
  }

  Tab<VromLoadInfo> extraVroms(framemem_ptr());
  extraVroms.reserve((int)game_info.addonVroms.size() + 1);
  for (const GamePackage::VromInfo &vromInfo : game_info.addonVroms)
    extraVroms.push_back(VromLoadInfo{vromInfo.vromFile.c_str(), vromInfo.mount.c_str(), vromInfo.mountAsPack, true,
      vromInfo.mountAsPack ? ReqVromSign::No : ReqVromSign::Yes});

  for (const GamePackage::VromInfo &vrom : game_info.ugmVroms)
    extraVroms.push_back(
      VromLoadInfo{vrom.vromFile.c_str(), vrom.mount.c_str(), vrom.mountAsPack, false /*optional*/, ReqVromSign::No});
  setup_base_resources(game_info, extraVroms, load_game_res);
}

GamePackage load_game_package()
{
  GamePackage gameInfo;
  gameInfo.gameName = get_game_name();
  String settingsFn(0, "content/%s/gamedata/%s.settings.blk", gameInfo.gameName, gameInfo.gameName);
  prepare_before_loading_game_settings_blk(gameInfo.gameName.c_str());
  gameInfo.gameSettings.load(settingsFn);
  restore_after_loading_game_settings_blk();

  String overrideFn = get_config_filename(gameInfo.gameName.c_str());
  DataBlock cfg;
  if (dd_file_exist(overrideFn))
  {
    dblk::load(cfg, overrideFn, dblk::ReadFlag::ROBUST);
    debug("Load override config %s", overrideFn);
  }
  dgs_apply_changes_to_config(cfg, true);
  dgs_apply_config_blk_ex(gameInfo.gameSettings, cfg, false, false);

  app_profile::apply_settings_blk(gameInfo.gameSettings);

  String consoleBindsFn(0, "%s.console_binds.blk", gameInfo.gameName);
  console_keybindings::set_binds_file_path(consoleBindsFn);
  console_keybindings::load_binds_from_file();

  bool useAddonVromSrc = false;
#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC || _TARGET_C3)
  if (const DataBlock *debugBlk = dgs_get_settings()->getBlockByName("debug"))
  {
    if (debugBlk->getBool("useAddonVromSrc", false))
    {
      useAddonVromSrc = true;
      debug("useAddonVromSrc if appliable");
    }
  }
#endif

  if (const DataBlock *mnt = gameInfo.gameSettings.getBlockByName("mountPoints"))
  {
    dblk::iterate_child_blocks(*mnt, [p = useAddonVromSrc ? "forSource" : "forVromfs"](const DataBlock &b) {
      dd_set_named_mount_path(b.getBlockName(), b.getStr(p));
    });
    dd_dump_named_mounts();
  }

  if (const DataBlock *addons = gameInfo.gameSettings.getBlockByName("addonBasePath"))
  {
    int addonNid = addons->getNameId("addon");
    for (int i = 0; i < addons->blockCount(); ++i)
    {
      const DataBlock *addonBlk = addons->getBlock(i);
      if (addonBlk->getBlockNameId() != addonNid)
        continue;
      const char *folder = addonBlk->getStr("folder", nullptr);
      String path = expand_path(".");
      if (folder != nullptr)
      {
        path = expand_path(folder);
        if (eastl::find(gameInfo.basePaths.begin(), gameInfo.basePaths.end(), path.str()) == gameInfo.basePaths.end())
        {
          gameInfo.basePaths.push_back(path.str());
          debug("[VROM] addon: %s, path:%s", folder, path);
        }
      }
      const char *src = addonBlk->getStr("src", nullptr);
#if DAGOR_DBGLEVEL > 0
      if (src != nullptr)
        debug("[VROM] src path: %s", src);
#endif
      String vrom_fn(0, "%s%s.vromfs.bin", path, dd_get_fname(folder));

      const char *explicit_vrom_fn = addonBlk->getStr("vrom", nullptr);
      if (explicit_vrom_fn != nullptr)
        vrom_fn = explicit_vrom_fn;

      bool vrom_optional = addonBlk->getBool("vromOptional", addonBlk->paramExists("vromed") ? !addonBlk->getBool("vromed") : false);
      vrom_optional = addonBlk->getBool(dedicated::is_dedicated() ? "vromOptionalForServer" : "vromOptionalForClient", vrom_optional);
      if (const bool isHarmonizationVrom = addonBlk->getBool("harmonization", false))
      {
        // we dont care about harmonization on the server itself right now
        const bool isHarmRequired = !dedicated::is_dedicated() && ::dgs_get_settings()->getBool("harmonizationRequired", false);
        const bool isHarmEnabled = ::dgs_get_settings()->getBool("harmonizationEnabled", false);

        if (isHarmRequired)
          vrom_optional = false;
        else if (isHarmEnabled)
          vrom_optional = true;
        // don't load the vrom if no harmonization is on
        else
          continue;
      }

      if (dd_file_exists(vrom_fn) && (!useAddonVromSrc || src == nullptr))
      {
        bool vpack = addonBlk->getBool("mountAsPack", false);
        debug("[%s] use addon vroms %s", vpack ? "VPACK" : "VROM", vrom_fn);
        gameInfo.addonVroms.push_back({path.str(), vrom_fn.str(), vpack});
      }
      else if (useAddonVromSrc && src != nullptr)
      {
#if DAGOR_DBGLEVEL > 0
        for (int i = addonBlk->paramCount() - 1, nid = addonBlk->getNameId("src"); i >= 0; i--)
        {
          if (addonBlk->getParamNameId(i) == nid || strncmp(addonBlk->getParamName(i), "src", 3) == 0)
          {
            String path = expand_path(expand_path(addonBlk->getStr(i)));
            if (eastl::find(gameInfo.basePaths.begin(), gameInfo.basePaths.end(), path.str()) == gameInfo.basePaths.end())
            {
              gameInfo.basePaths.push_back(path.str());
              debug("[VROM] use addon source folder: %s, cause useAddonVromSrc specified", path);
            }
          }
        }
#endif
      }
      else if (!dd_file_exists(vrom_fn) && !useAddonVromSrc && !vrom_optional)
      {
#if DAGOR_DBGLEVEL > 0
        G_ASSERTF(0, "vrom %s required but doesn't exist", vrom_fn);
#else
        DAG_FATAL("vrom %s required but doesn't exist", vrom_fn);
#endif
      }
    }
  }

#if _TARGET_XBOX | _TARGET_C1 | _TARGET_C2
  if (const DataBlock *chunksMapping = gameInfo.gameSettings.getBlockByName("chunksMapping"))
  {
    for (int i = 0, nid = chunksMapping->getNameId("chunk"); i < chunksMapping->blockCount(); ++i)
    {
      const DataBlock *chunkBlk = chunksMapping->getBlock(i);
      if (chunkBlk->getBlockNameId() == nid)
      {
        const char *addonFolder = chunkBlk->getStr("folder", nullptr);
#if _TARGET_XBOX
        static constexpr uint32_t XBOX_LAUNCH_CHUNK_ID = 1001;
        uint32_t chunkId = chunkBlk->getInt("id", XBOX_LAUNCH_CHUNK_ID);
        if (chunkId == XBOX_LAUNCH_CHUNK_ID)
          continue;
#elif _TARGET_C1 | _TARGET_C2




#endif
        if (addonFolder)
          gameInfo.chunksMap.emplace(addonFolder, chunkId);
      }
    }
    debug("Loaded %u entries into chunks map", gameInfo.chunksMap.size());
  }
  else
    logwarn("Chunks mapping wasn't found");
#endif // end #if _TARGET_XBOX | _TARGET_C1 | _TARGET_C2

  auto initAddons = [&](const char *blockName, eastl::vector<eastl::string> &buffer) {
    if (const DataBlock *addons = gameInfo.gameSettings.getBlockByName(blockName))
      for (int i = 0, nid = addons->getNameId("folder"); i < addons->paramCount(); i++)
        if (addons->getParamNameId(i) == nid || strncmp(addons->getParamName(i), "folder", 6) == 0)
        {
          String addon_name(addons->getStr(i));
          if (addon_name.find('{'))
          {
            // replace known keywords
            addon_name.replaceAll("{SHADER_SUFFIX}", node_based_shader_current_platform_suffix());

            // skip loading packages with unresolved keywords
            if (addon_name.find('{'))
            {
              logerr("skip unsupported package %s", addon_name);
              continue;
            }
          }
#if _TARGET_XBOX | _TARGET_C1 | _TARGET_C2
          eastl::unordered_map<eastl::string, uint32_t>::iterator it = gameInfo.chunksMap.find(addon_name.c_str());
          if (it != gameInfo.chunksMap.end())
          {
            uint32_t chunkId = it->second;
#if _TARGET_XBOX
            bool chunkAvailable = gdk::is_game_chunk_available(chunkId);
#elif _TARGET_C1 | _TARGET_C2


#endif
            if (!chunkAvailable)
            {
              logwarn("Chunk %u with folder %s is not available. Skipping.", chunkId, addon_name.c_str());
              continue;
            }
          }
#endif // end _TARGET_XBOX | _TARGET_C1 | _TARGET_C2
          buffer.push_back((addon_name + "/res").str());
        }
  };

  initAddons("addons", gameInfo.addons);
  initAddons("rawAddons", gameInfo.rawAddons);
  return gameInfo;
}

void unload_current_game()
{
  if (current_game.gameName.empty())
    return;

  unload_in_progress = true;
  g_entity_mgr->broadcastEventImmediate(EventOnGameUnloadStart());
  hdrrender::shutdown();
  user_ui::term();
  destroy_glob_input();
  reset_all_cameras();
  if (ecs::g_scenes)
    ecs::g_scenes->clearScene();
  ecs::SceneManager::resetEntityCreationCounter();

  if (!current_game.ugmContentMount.empty())
  {
    gamescripts::unload_user_game_mode_scripts(current_game.ugmContentMount.c_str());
    gamescripts::unload_user_game_mode_scripts("%ugm/");
  }

  // Explicitly flush delayed actions that might hold sq callbacks (which in turn might need entities, not empty tdb, etc...)
  perform_delayed_actions();

  net_on_before_emgr_clear();

  {
    debug("clear g_entity_mgr");
    // TODO: rewrite these timer clears to ESes that handles EventEntityManager{Before,After}Clear
    game::g_timers_mgr->clearAllTimers();
    g_entity_mgr->clear();
    game::g_timers_mgr->clearAllTimers();
  }

  dngsound::flush_commands();
  PictureManager::prepare_to_release(false);
  term_game(); // after EM clear as delayed events might depend on global resources owned by this (e.g. props_registry)
  term_renderer_per_game();
  net_destroy(); // after EM clear as entities might hold references to resources owned by network (e.g. connections, g_time, etc...)
  controls::destroy(); // After EM clear as events might ref daInput actions which cleared here

  g_entity_mgr->broadcastEventImmediate(EventOnGameUnloadEnd());

  ::reset_required_res_list_restriction();
  ::free_unused_game_resources();
  ::reset_game_resources();
  gameres_rendinst_desc.reset();
  gameres_dynmodel_desc.reset();
  prepare_united_vdata_setup(nullptr); // Reset to default

  current_game.gameSettings.resetAndReleaseRoNameMap();

  dd_set_named_mount_path("ugm", nullptr);
  for (int i = 0, sz = current_game.ugmAddons.size(); i < sz; ++i)
  {
    const String mountName(0, "ugm%i", i);
    dd_set_named_mount_path(mountName.c_str(), nullptr);
  }

  current_game = GamePackage();

  mem_collect_threads();

  start_editor_reload();

  unload_in_progress = false;
}

// Note: only entities created with import depth 0 added to scene
static bool load_game_scene(const DataBlock &scene_blk, const char *scene_path, bool res, int import_depth)
{
  if (res)
  {
    if (!ecs::g_scenes && has_in_game_editor())
      ecs::g_scenes.demandInit();
    ecs::SceneManager::loadScene(ecs::g_scenes.get(), scene_blk, scene_path, &ecs::SceneManager::entityCreationCounter, import_depth);
  }
  return res;
}

bool load_game_scene(const char *scene_path, int import_depth)
{
  DataBlock blk;
  // not ROBUST in dev to see syntax errors
  return load_game_scene(blk, scene_path, dblk::load(blk, scene_path, dblk::ReadFlag::ROBUST_IN_REL), import_depth);
}

static void load_common_game_scenes()
{
  const DataBlock &blk = *::dgs_get_game_params()->getBlockByNameEx("commonScenes");
  for (int i = 0; i < blk.paramCount(); ++i)
    if (!load_game_scene(blk.getStr(i)))
      logerr("Failed to load scene from '%s'", blk.getStr(i));
}

static void load_common_game_client_scenes(bool (*load_scene_cb)(const char *, int))
{
  if (!load_scene_cb)
    load_scene_cb = [](const char *scene_path, int) {
      DataBlock blk;
      if (dblk::load(blk, scene_path, dblk::ReadFlag::ROBUST_IN_REL))
        return ecs::create_entities_blk(*g_entity_mgr, blk, scene_path), true;
      return false;
    };
  ecs::SceneManager::CounterGuard cntg; // Protects against sending event on first empty scene
  const DataBlock &blk = *::dgs_get_game_params()->getBlockByNameEx("commonClientScenes");
  for (int i = 0; i < blk.paramCount(); ++i)
    if (!load_scene_cb(blk.getStr(i), /*import_depth*/ 1))
      logerr("Failed to load client scene from '%s'", blk.getStr(i));
}

static void load_import_scenes(const eastl::vector<eastl::string> &imports)
{
  for (const eastl::string &name : imports)
    if (!load_game_scene(name.c_str()))
      logerr("Failed to load scene from '%s'", name.c_str());
}

static void load_scene(const char *name, const eastl::vector<eastl::string> &imports)
{
  DataBlock sceneBlk;
  bool sceneLoaded = false;
  if (*name) // Could be empty on client
  {
    // UGM scenes could be missing on client
    auto sflg = strncmp(name, "%ugm/", sizeof("%ugm/") - 1) == 0 ? dblk::ReadFlag::ROBUST : dblk::ReadFlag::ROBUST_IN_REL;
    sceneLoaded = dblk::load(sceneBlk, name, sflg);
    // Before any entity creation (including the ones from common scenes) as it might request resources
    apply_united_vdata_settings(sceneLoaded ? &sceneBlk : nullptr);
  }
  if (app_profile::get().haveGraphicsWindow && (is_server() || !app_profile::get().replay.playFile.empty()))
    load_common_game_client_scenes(&load_game_scene); // Otherwise it will be loaded on connect to server (see `apply_scene_level`)
  if (!is_server() || !app_profile::get().replay.playFile.empty())
    return;

  da_editor4_setup_scene(df_get_real_name(name));
  debug("scene selected <%s>", name);

  {
    ecs::SceneManager::CounterGuard cntg; // Protects against sending event on first empty scene
    load_common_game_scenes();
    load_import_scenes(imports);
    if (DAGOR_UNLIKELY(!load_game_scene(sceneBlk, name, sceneLoaded, /*import_depth*/ 0)))
    {
      G_ASSERT_LOG(sceneLoaded, "Failed to load scene from '%s'", name);
      exit_game("Failed to load scene");
    }
  }

  // should it be really to be placed here?
  // TODO: add is_local_game flag to network module
  if (app_profile::get().haveGraphicsWindow && is_server())
  {
    g_entity_mgr->tick(); // make sure that scene entities are actually created (i.e. not just scheduled for creation)

    if (!game::get_local_player()) // initial load
      g_entity_mgr->broadcastEventImmediate(
        EventOnClientConnected(net::INVALID_CONNECTION_ID, net::get_user_id(), net::get_user_name(),
          /*group_id*/ net::get_user_id(), /*flags*/ 0, /*pltf_uid*/ "", get_platform_string_id(), TEAM_UNASSIGNED,
          app_profile::get().appId));
    else // restart
      g_entity_mgr->broadcastEventImmediate(ServerCreatePlayersEntities{});
  }
}

static bool read_ugm_manifest(const char *vrom_fn, eastl::string &out_content_id, eastl::string *out_manifest, FastNameMap *out_flist)
{
  VirtualRomFsData *vrom = ::load_vromfs_dump(vrom_fn, inimem, /*do not check mod vrom sign*/ nullptr);
  bool read_ok = false;
  if (vrom && vrom->files.map.size())
    if (const char *sp = strchr(vrom->files.map[0], '/'))
    {
      size_t prefix_len = sp - vrom->files.map[0] + 1;
      out_content_id = String::mk_sub_str(vrom->files.map[0], sp);
      if (out_manifest)
        *out_manifest = "";
      read_ok = true;

      for (int i = 0; i < vrom->files.map.size(); i++)
        if (i > 0 && strncmp(vrom->files.map[i], vrom->files.map[0], prefix_len) != 0)
        {
          logwarn("ugm: %s: unexpected path in vrom: %s", vrom_fn, vrom->files.map[i]);
          read_ok = false;
        }
        else
        {
          if (out_flist)
            out_flist->addNameId(vrom->files.map[i] + prefix_len);
          if (out_manifest && strcmp(vrom->files.map[i] + prefix_len, "manifest.json") == 0)
            *out_manifest = eastl::string(vrom->data[i].data(), vrom->data[i].data() + vrom->data[i].size());
        }
    }

  if (vrom)
    memfree(vrom, inimem);
  return read_ok;
}

static void load_mod_from_source(GamePackage &game_info, const UserGameModeContext &mods_ctx)
{
  if (!mods_ctx.modsVroms.empty())
    logerr("Mod in offline mode can't load extra vroms");
  eastl::string mount_path;
  mount_path.sprintf("userGameMods/%s", mods_ctx.modId.c_str());
  dd_set_named_mount_path("ugm", mount_path.c_str());
  game_info.ugmContentMount = "%ugm/";

  eastl::string contentBlkPath;
  contentBlkPath.sprintf("%s/content.blk", mount_path.c_str());
  if (dd_file_exist(contentBlkPath.c_str()))
  {
    DataBlock contentBlk(contentBlkPath.c_str(), framemem_ptr());
    if (contentBlk.isValid())
    {
      const uint32_t mountBlkBlockCount = contentBlk.blockCount();
      game_info.ugmAddons.reserve(mountBlkBlockCount);
      for (uint32_t i = 0; i < mountBlkBlockCount; ++i)
      {
        const DataBlock *mountBlk = contentBlk.getBlock(i);
        if (!strcmp(mountBlk->getBlockName(), "addon"))
        {
          const char *contentPath = mountBlk->getByName<const char *>("path", nullptr);
          if (!contentPath)
            continue;
          else if (is_path_abs(contentPath))
          {
            if (dd_dir_exist(contentPath))
              game_info.ugmAddons.emplace_back(contentPath);
            else
              logerr("Directory '%s' with content doesn't exist, in file '%s'", contentPath, contentBlkPath.c_str());
          }
          else
          {
            eastl::string contentMountPath;
            contentMountPath.sprintf("%s/%s", mount_path.c_str(), contentPath);
            const bool foundPath = iterate_base_paths_simplify(contentMountPath.c_str(), [&game_info](const char *fname) {
              const bool dirExists = check_dir_exists(fname);
              if (dirExists)
                game_info.ugmAddons.emplace_back(fname);
              return dirExists;
            });
            if (!foundPath)
              logerr("Directory '%s' with content doesn't exist, in file '%s'", contentPath, contentBlkPath.c_str());
          }
        }
      }
    }
    else
      logerr("Failed to load '%s'", mount_path.c_str());
  }
}

static void load_mod_from_vrom(GamePackage &game_info, const UserGameModeContext &mods_ctx)
{
  game_info.ugmContentMount = "%ugm/";
  game_info.ugmVroms.reserve(mods_ctx.modsVroms.size() + mods_ctx.modsPackVroms.size() + mods_ctx.modsAddons.size());

  dd_set_named_mount_path("ugm", "userGameModsV");
  for (const eastl::string &vrom : mods_ctx.modsVroms)
  {
    GamePackage::VromInfo vromInfo{};
    vromInfo.mount = "%ugm/";
    vromInfo.vromFile = vrom;
    dd_resolve_named_mount_inplace(vromInfo.mount);
    game_info.ugmVroms.emplace_back(std::move(vromInfo));
  }

  for (const eastl::string &vrom : mods_ctx.modsPackVroms)
  {
    GamePackage::VromInfo vromInfo;
    vromInfo.mount = "%ugm/";
    vromInfo.vromFile = vrom;
    vromInfo.mountAsPack = true;
    dd_resolve_named_mount_inplace(vromInfo.mount);
    game_info.ugmVroms.emplace_back(std::move(vromInfo));
  }

  // UGC ADDONS DISABLED UNTIL CERTIFICATION ESTABLISHED FOR CONSOLES
  game_info.ugmAddons.clear();
  // game_info.ugmAddons.reserve(mods_ctx.modsAddons.size());
  // for (int i = 0, sz = mods_ctx.modsAddons.size(); i < sz; ++i)
  //{
  //   const String mountName(0, "ugm%i", i);
  //   const String pathTo(0, "userGameModsV%i", i);
  //   GamePackage::VromInfo vromInfo;
  //   vromInfo.mount.sprintf("%%%s/", mountName.c_str());
  //   vromInfo.vromFile = mods_ctx.modsAddons[i];
  //   vromInfo.mountAsPack = true;
  //   game_info.ugmAddons.emplace_back(vromInfo.mount);
  //   dd_set_named_mount_path(mountName.c_str(), pathTo.c_str());
  //   dd_resolve_named_mount_inplace(vromInfo.mount);
  //   game_info.ugmVroms.emplace_back(std::move(vromInfo));
  // }
}

static void load_scene_impl(const eastl::string_view &scene_name,
  eastl::vector<eastl::string> &&import_scenes,
  net::ConnectParams &&connect_params,
  const UserGameModeContext &mods_ctx)
{
  TIME_PROFILE(load_scene_impl)
  debug("load_scene_impl");
#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::AppState appState("load_scene_impl");
#endif

  GamePackage gameInfo = load_game_package();
  gameInfo.sceneName = eastl::string(scene_name.begin(), scene_name.end());
  gameInfo.importScenes = eastl::move(import_scenes);
  if (!mods_ctx.modId.empty())
  {
    gameInfo.ugmContentID = mods_ctx.modId;
    if (app_profile::get().devMode) // try load mod from source
      load_mod_from_source(gameInfo, mods_ctx);
    else if (!mods_ctx.modsVroms.empty() || !mods_ctx.modsAddons.empty() || !mods_ctx.modsPackVroms.empty())
      load_mod_from_vrom(gameInfo, mods_ctx);

    if (auto modeInfo = jsonutils::get_ptr<rapidjson::Value::ConstObject>(get_matching_invite_data(), "mode_info"))
    {
      user_game_mode_info.author = jsonutils::get_or(*modeInfo, "modAuthor", "");
      user_game_mode_info.modName = jsonutils::get_or(*modeInfo, "modName", "");
      user_game_mode_info.version = jsonutils::get_or(*modeInfo, "modVersion", -1);
    }
  }

  for (const auto &vrom : gameInfo.ugmVroms)
    debug("ugm vrom mount '%s' = '%s'", dd_get_named_mount_by_path(vrom.mount.c_str()), vrom.mount.c_str());
  if (gameInfo.ugmContentMount.empty())
    dd_set_named_mount_path("ugm", nullptr);
  else
  {
    debug("  -> gameInfo.sceneName=%s [ %s = %s ]", gameInfo.sceneName.c_str(), "%ugm", dd_get_named_mount_path("ugm"));
    dd_resolve_named_mount_inplace(gameInfo.ugmContentMount);
  }

  // load_package_files can be long, so we need to call idle cycle to make app responsive
  ::dagor_idle_cycle();
  {
    TIME_PROFILE(load_package_files)

    load_package_files(gameInfo);
    current_game = eastl::move(gameInfo);
  }
  ::dagor_idle_cycle();

  load_gameparams(current_game.gameName.c_str());
  dng_load_localization();

  set_timespeed(1.f);
  bool hasServerUrls = !connect_params.serverUrls.empty();
  if (!app_profile::get().devMode && (hasServerUrls || !app_profile::get().replay.playFile.empty()))
    net_init_late_client(eastl::move(connect_params));

  init_renderer_per_game();
  init_game();

  physmat::init();

  game::init_players(); // needed for get_controlled_hero()/get_local_player()
  {
    if (is_true_net_server())
      init_component_replication_filters(); // we only init for net server
    eastl::string ugm_entities_import_fn;
    eastl::string ugm_entities_es_order_fn;
    if (!current_game.ugmContentMount.empty())
    {
      ugm_entities_es_order_fn = "%ugm/es_order.blk";
      ugm_entities_import_fn = "#%ugm/entities.blk";
      if (!dd_file_exist(ugm_entities_es_order_fn.c_str()))
        ugm_entities_es_order_fn.clear();
      if (!dd_file_exist(ugm_entities_import_fn.c_str() + 1))
        ugm_entities_import_fn.clear();
    }
    ecs_set_global_tags_context(current_game.gameName.c_str(), // requires world-renderer
      ugm_entities_es_order_fn.empty() ? nullptr : ugm_entities_es_order_fn.c_str());

    gamescripts::das_load_ecs_templates();
    debug("load_ecs_templates for game '%s'", current_game.gameName.c_str());

    // load_ecs_templates can be long, so we need to call idle cycle to make app responsive
    ::dagor_idle_cycle();
    load_ecs_templates(ugm_entities_import_fn.c_str());
    ::dagor_idle_cycle();
  }

  const char *anim_attachements_fn = "gamedata/attachment_slots.blk";
  if (dd_file_exists(anim_attachements_fn))
    anim::init_attachements(DataBlock(anim_attachements_fn));

  net_init_late_server(); // on server net_init must be called after templates had been loaded and before scene is starting to load

  if (current_game.sceneName.empty() && !hasServerUrls)
  {
    const char *sceneNm =
      dgs_get_argv("scene", dgs_get_settings()->getStr("scene", current_game.gameSettings.getStr("scene", nullptr)));
    if (!sceneNm && is_server())
      DAG_FATAL("scene:t= is not specified neither in *.config.blk, *.patch or in commandline\n");
    else if (sceneNm)
      current_game.sceneName = sceneNm;
  }

  // Disable script loading in ugm from now
  // To restore this functionality uncomment these rows
  // if (!current_game.ugmContentMount.empty())
  //   gamescripts::run_user_game_mode_entry_scripts("%ugm/entry.das", "%ugm/entry.nut");

  // Here resides things that needed loaded templates in order to be inited
  eastl::string ugm_input_cfg_fn;
  if (!current_game.ugmContentMount.empty())
  {
    ugm_input_cfg_fn = "%ugm/controlsConfig.blk";
    if (!dd_file_exist(ugm_input_cfg_fn.c_str()))
      ugm_input_cfg_fn.clear();
  }
  controls::init_control(current_game.gameName.c_str(), ugm_input_cfg_fn.empty() ? nullptr : ugm_input_cfg_fn.c_str());
  init_glob_input();

  bool fsrEnabled = false;
  // We have to check for fsr here, bacause otherwise hdrrender and gamma_pass will be created without
  // uav usage support and if worldRenderer is not recrated (this is behaviour for all scenes except first loaded),
  // textures will have incorrect flags for fsr
  const char *quality = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("fsr", nullptr);
  if (quality && strcmp(quality, "off") != 0)
    fsrEnabled = true;

  // ui init can be long, process inputs to avoid application not responding status
  ::dagor_idle_cycle();
  uishared::init();
  user_ui::init();
  ::dagor_idle_cycle();

  int width, height;
  d3d::get_render_target_size(width, height, 0, 0);
  hdrrender::init(width, height, true, fsrEnabled);

  g_entity_mgr->broadcastEventImmediate(
    EventOnBeforeSceneLoad(current_game.sceneName.c_str(), current_game.importScenes, current_game.gameSettings));

  load_scene(current_game.sceneName.c_str(), current_game.importScenes);
  net_replay_rewind();
}

static void prepare_to_switch_scene()
{
  watchdog::change_mode(watchdog::LOADING);
  visuallog::reset(true);
  force_feedback::rumble::reset();
  set_fps_limit(30);
  start_animated_splash_screen_in_thread();
  animated_splash_screen_allow_watchdog_kick(false);
  g_entity_mgr->tick(); // create/destroy delayed entities
  unload_current_game();
  animated_splash_screen_allow_watchdog_kick(true);
}

struct ReloadDasModulesJob final : public cpujobs::IJob
{
  das::daScriptEnvironment *bound;
  explicit ReloadDasModulesJob(das::daScriptEnvironment *bound_) : bound(bound_) {}
  void doJob() override
  {
    das::daScriptEnvironment::bound = bound;
    int rt = gamescripts::get_game_das_reload_threads();
    if (rt)
      gamescripts::set_load_threads_num(eastl::exchange(rt, gamescripts::get_load_threads_num()));
    gamescripts::reload_das_modules();
    if (rt)
      gamescripts::set_load_threads_num(rt); // restore
    das::daScriptEnvironment::bound = nullptr;
  }

  void releaseJob() override
  {
    bind_dascript::main_thread_post_load();
    g_entity_mgr->broadcastEvent(EventDaScriptReloaded());
    switch_scene_flag = false;
    delete this;
    mem_collect_threads(/*loading_job_only*/ true);
  }
};


void on_apply_sync_vroms_diffs_msg(const net::IMessage *_msg)
{
  const ApplySyncVromsDiffs *msg = _msg->cast<ApplySyncVromsDiffs>();
  G_ASSERT_RETURN(msg, );

  bool shouldReloadVroms = false;
  updater::Version maxVersion;

  reset_vroms_changed_state();

  syncvroms::read_sync_vroms_diffs(msg->get<0>(), [&](const syncvroms::VromDiff &diff) {
    const char *vromName = syncvroms::get_base_vrom_name(diff.baseVromHash);
    if (!vromName)
    {
      logerr("[SyncVroms]: Cannot find base vrom name with hash '%s'", diff.baseVromHash.c_str());
      return;
    }

    const char *vromPath = get_vrom_path_by_name(vromName);
    if (!vromPath)
    {
      logerr("[SyncVroms]: Cannot find base vrom path '%s' with hash '%s'", vromName, diff.baseVromHash.c_str());
      return;
    }

    const eastl::string saveVromTo{updater::fs::join_path({updater::binarycache::get_cache_folder().c_str(), vromPath})};
    const VromfsCompression &compr = create_vromfs_compression(saveVromTo.c_str());

    if (syncvroms::apply_vrom_diff(diff, saveVromTo.c_str(), compr))
    {
      shouldReloadVroms = true;
      mark_vrom_as_changed_by_name(vromName);
    }

    debug("[SyncVroms]: Version: %s", updater::Version{get_vromfs_dump_version(saveVromTo.c_str())}.to_string());
    maxVersion = max(maxVersion, updater::Version{get_vromfs_dump_version(saveVromTo.c_str())});
  });

  if (shouldReloadVroms)
  {
    maxVersion = max(maxVersion, updater::binarycache::get_cache_version());
    debug("[SyncVroms]: Applied diffs version: %s", maxVersion.to_string());
    updater::binarycache::set_cache_version(updater::binarycache::get_cache_folder().c_str(), maxVersion);

    const int remountedCount = remount_changed_vroms();
    debug("[SyncVroms]: Remounted %d vroms after diffs has been applied", remountedCount);

    const bool isOk = reload_ecs_templates([](const char *name, ecs::EntityManager::UpdateTemplateResult result) {
      switch (result)
      {
        case ecs::EntityManager::UpdateTemplateResult::Same: break;
        case ecs::EntityManager::UpdateTemplateResult::Added: debug("[SyncVroms]: %s added", name); break;
        case ecs::EntityManager::UpdateTemplateResult::Updated: debug("[SyncVroms]: %s updated", name); break;
        case ecs::EntityManager::UpdateTemplateResult::Removed: debug("[SyncVroms]: %s removed", name); break;
        case ecs::EntityManager::UpdateTemplateResult::InvalidName: debug("[SyncVroms]: %s wrong name", name); break;
        case ecs::EntityManager::UpdateTemplateResult::RemoveHasEntities:
          logerr("[SyncVroms]: %s has entities and cant be removed", name);
          break;
        case ecs::EntityManager::UpdateTemplateResult::DifferentTag:
          logerr("[SyncVroms]: %s is registered with different tag and can't be updated", name);
          break;
        default: logerr("[SyncVroms]: Unknown error %d for template '%s'", (int)result, name); break;
      }
    });

    if (!isOk)
      statsd::counter("syncvroms.error", 1, {"error", "reload_ecs_templates"});

    debug("[SyncVroms]: Templates has been reloaded: %s", isOk ? "success" : "fail");
  }

  G_VERIFY(send_net_msg(net::get_msg_sink(), SyncVromsDone()) > 0);
}

static void switch_scene_and_apply_update(eastl::string_view scene)
{
  switch_scene_flag = true;

  prepare_to_switch_scene();

  const DataBlock *circuitConf = circuit::get_conf();
  const bool debugEnableDataReload = !(::dgs_get_settings()->getBlockByNameEx("debug")->getBool("disableDataReloadOnUpdate", false));
  const bool disableAlwaysReloadData = ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("disableAlwaysReloadData", false);
  const bool alwaysReloadDas = !disableAlwaysReloadData && circuitConf && circuitConf->getBool("alwaysReloadDas", false);
  const bool alwaysReloadGameSq = !disableAlwaysReloadData && circuitConf && circuitConf->getBool("alwaysReloadGameSq", false);
  const bool alwaysReloadOverlay = !disableAlwaysReloadData && circuitConf && circuitConf->getBool("alwaysReloadOverlay", false);

  bool serializationRequested = false;
#if defined(DAGOR_THREAD_SANITIZER)
  serializationRequested = false;
#endif

  // Current game's version with actualy mounted vroms
  const updater::Version updatedVersion = get_updated_game_version();

  // Downloaded version. New version is ready to use.
  const updater::Version cacheVersion = updater::binarycache::get_cache_version();

  bool updater_is_running = false;
  g_entity_mgr->broadcastEventImmediate(CheckVromfsUpdaterRunningEvent{&updater_is_running});
  const bool couldReload = debugEnableDataReload && !updater_is_running;
  const bool isRemoteVromsDifferent = cacheVersion && cacheVersion != updatedVersion;
  const bool isNewVromsAvailable = couldReload && isRemoteVromsDifferent;

  debug("Try to remount vroms to the version: %s -> %s", updatedVersion.to_string(), cacheVersion.to_string());

  if (isNewVromsAvailable)
  {
    statsd::counter("gameload.remount_all_vroms");

    debug("Remount vroms due to vroms update %s -> %s", updatedVersion.to_string(), cacheVersion.to_string());
    remount_all_vroms();
  }
  else
    debug("Vroms are up-to-date %s == %s", updatedVersion.to_string(), cacheVersion.to_string());

  load_scene_impl(scene, {}, {}, {});

  // Use game's version after vroms has been remounted.
  // Remount of vroms might change game's version.
  const updater::Version currentVersion = get_updated_game_version();

  const updater::Version overlayVersion = overlay_ui::get_overlay_ui_version();
  const updater::Version sqVersion = gamescripts::get_sq_version();
  const updater::Version dasVersion = gamescripts::get_das_version();

  uint32_t serializedVersion = 0;
  char tmpbuf[DAGOR_MAX_PATH];
  G_VERIFY(gamescripts::get_serialized_data_filename(tmpbuf) == tmpbuf);
  const bool shouldUseSerializer = serializationRequested && !isRemoteVromsDifferent &&
                                   (serializedVersion = gamescripts::initialize_deserializer(tmpbuf)) != -1 &&
                                   (serializedVersion != 0 || bind_dascript::debugSerialization);
  const bool shouldReloadOverlay = couldReload && (alwaysReloadOverlay || overlayVersion != currentVersion);
  const bool shouldReloadGameSq = couldReload && (alwaysReloadGameSq || sqVersion != currentVersion);
  const bool shouldReloadDas = couldReload && (alwaysReloadDas || dasVersion != currentVersion);

  debug("Try to reload scripts to version %s:", currentVersion.to_string());
  debug("  serialization: %s", shouldUseSerializer ? "enabled" : "disabled");
  debug("  [serializationRequested: %d, isRemoteVromsDifferent: %d, serializedVersion: %lu]", serializationRequested,
    isRemoteVromsDifferent, serializedVersion);
  debug("  overlay_ui: %s", overlayVersion.to_string());
  debug("  sq: %s", sqVersion.to_string());
  debug("  das: %s", dasVersion.to_string());

  if (shouldUseSerializer)
  {
    bind_dascript::enableSerialization = true;
    bind_dascript::serializationReading = true;
  }

  if (!shouldReloadOverlay && !shouldReloadGameSq && !shouldReloadDas)
  {
    debug("Scripts are up-to-date %s == %s; alwaysReloadOverlay: %@;  alwaysReloadGameSq: %@;  alwaysReloadDas: %@; "
          "debugEnableDataReload: %@",
      currentVersion.to_string(), cacheVersion.to_string(), alwaysReloadOverlay, alwaysReloadGameSq, alwaysReloadDas,
      debugEnableDataReload);
    switch_scene_flag = false;
  }
  else
  {
    const DataBlock *reloadBlk = ::dgs_get_settings()->getBlockByNameEx("embeddedUpdater")->getBlockByNameEx("reload");
    // read parameters before settings datablock may be modified (and pointer invalidated)
    const bool reloadOverlayUi = reloadBlk->getBool("overlay_ui", false);
    const bool reloadDascript = reloadBlk->getBool("gameDas", false);
    const bool reloadGameSq = reloadBlk->getBool("gameSq", false);

    if (reloadOverlayUi && shouldReloadOverlay)
    {
      statsd::counter("gameload.reload_scripts", 1, {"reload", "overlay_ui"});
      overlay_ui::reload_scripts(true);
    }
    else
      debug("Overlay UI has't been reloaded");

    if (reloadDascript && shouldReloadDas)
    {
      statsd::counter("gameload.reload_scripts", 1, {"reload", "das"});

      gamescripts::unload_all_das_scripts_without_debug_agents();
      // game_init.das is very small and contains only events, load it synchronously before sq
      debug("Reload synchronously game_init.das.");
      gamescripts::reload_das_init();
      end_es_loading(); // call end_es_loading to reset systems order
    }

    if (reloadGameSq && shouldReloadGameSq)
    {
      statsd::counter("gameload.reload_scripts", 1, {"reload", "sq"});
      gamescripts::reload_sq_modules(gamescripts::ReloadMode::SILENT_FULL_RELOAD);
    }
    else
      debug("Game scripts (quirrel) has't been reloaded");

    if (reloadDascript && shouldReloadDas)
    {
      debug("Start a job to reload das modules.");

      G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), new ReloadDasModulesJob(das::daScriptEnvironment::bound)));
    }
    else
    {
      debug("Game scripts (daScript) has't been reloaded");
      switch_scene_flag = false;
    }
  }
}

bool is_load_in_progress() { return switch_scene_flag || is_level_loading(); }

bool is_scene_switch_in_progress() { return switch_scene_flag; }

struct SwitchSceneDelayedAction : public DelayedAction
{
  eastl::string scene;
  eastl::vector<eastl::string> importScenes;
  UserGameModeContext ugmCtx;
  bool precondition() override { return !is_load_in_progress(); } // can't switch game during active loading
  void performAction() override
  {
    debug("switch current scene to '%.*s' (+%d imports)", (int)scene.size(), scene.data(), importScenes.size());
    switch_scene_flag = true;
    prepare_to_switch_scene();
    load_scene_impl(scene, eastl::move(importScenes), {}, ugmCtx);
    switch_scene_flag = false;
  }
};

struct SwitchSceneAndUpdateDelayedAction final : SwitchSceneDelayedAction
{
  void performAction() override { switch_scene_and_apply_update(scene); }
};

void switch_scene(eastl::string_view scene, eastl::vector<eastl::string> &&import_scenes, UserGameModeContext &&ugm_ctx)
{
  debug("switch_scene(%s)", scene);
  auto act = new SwitchSceneDelayedAction{};
  act->scene = scene;
  act->importScenes = eastl::move(import_scenes);
  act->ugmCtx = std::move(ugm_ctx);
  add_delayed_action(act);
}

void switch_scene_and_update(eastl::string_view scene)
{
  debug("switch_scene_and_update(%s)", scene);
  auto act = new SwitchSceneAndUpdateDelayedAction{};
  act->scene = scene;
  add_delayed_action(act);
}

void connect_to_session(net::ConnectParams &&connect_params, UserGameModeContext &&ugmCtx)
{
  struct ConnectToSessionDelayedAction : public DelayedAction
  {
    net::ConnectParams connectParams;
    UserGameModeContext ugmCtx;
    bool precondition() override { return !is_load_in_progress(); } // can't switch game during active loading
    void performAction() override
    {
      debug("connect to session (switch current scene), server_urls=[%s]",
        eastl::accumulate(connectParams.serverUrls.begin(), connectParams.serverUrls.end(), eastl::string{},
          [](eastl::string &a, const eastl::string &s) {
            a += !s.empty() ? s : "\"\"";
            a += ',';
            return a;
          })
          .c_str());

      switch_scene_flag = true;
      prepare_to_switch_scene();
      load_scene_impl({}, {}, eastl::move(connectParams), ugmCtx);
      switch_scene_flag = false;
    }
  };
  auto act = new ConnectToSessionDelayedAction{};
  act->connectParams = eastl::move(connect_params);
  act->ugmCtx = std::move(ugmCtx);
  add_delayed_action(act);
};

static SQInteger request_ugm_manifest_sq(HSQUIRRELVM vm)
{
  const char *vrom_fn = nullptr;
  const char *event_id = nullptr;
  sq_getstring(vm, 2, &vrom_fn);
  sq_getstring(vm, 3, &event_id);

  struct ReadUgmManifestJob final : public cpujobs::IJob
  {
    eastl::string vromFn, contentId, manifestJsonStr, eventId;
    FastNameMap flist;
    bool result = false;

    ReadUgmManifestJob(const char *vrom, const char *ev) : vromFn(vrom), eventId(ev) {}
    void doJob() override { result = read_ugm_manifest(vromFn.c_str(), contentId, &manifestJsonStr, &flist); }

    void releaseJob() override
    {
      Json::Value data;
      data["result"] = result;
      data["vromfs"] = vromFn.c_str();
      data["contentId"] = contentId.c_str();
      data["manifestStr"] = manifestJsonStr.c_str();
      data["sceneBlkFound"] = (flist.getNameId("scene.blk") >= 0);
      Json::Value files(Json::arrayValue);
      iterate_names(flist, [&](int, const char *name) {
        files.append(name);
        debug(name);
      });
      data["modFiles"] = files;
      sqeventbus::send_event(eventId.c_str(), data);
      delete this;
    }
  };
  G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), new ReadUgmManifestJob(vrom_fn, event_id)));
  return 0;
}

} // namespace sceneload

static SQInteger register_virtual_file_sq(HSQUIRRELVM vm)
{
  const char *mount_dir = nullptr;
  const char *rel_fname = nullptr;
  SQUserPointer blobData = nullptr;
  sq_getstring(vm, 2, &mount_dir);
  sq_getstring(vm, 3, &rel_fname);
  if (sq_gettype(vm, 4) != OT_INSTANCE || !SQ_SUCCEEDED(sqstd_getblob(vm, 4, &blobData)))
  {
    logerr("SQ: %s requires (str, str, BLOB) args", "register_blob_as_virtual_file");
    sq_pushbool(vm, false);
    return 1;
  }
  sq_pushbool(vm, register_data_as_single_file_in_virtual_vrom(dag::ConstSpan<char>((char *)blobData, sqstd_getblobsize(vm, 4)),
                    mount_dir, rel_fname));
  return 1;
}
static SQInteger unregister_virtual_file_sq(HSQUIRRELVM vm)
{
  const char *mount_dir = nullptr;
  const char *rel_fname = nullptr;
  sq_getstring(vm, 2, &mount_dir);
  sq_getstring(vm, 3, &rel_fname);
  sq_pushbool(vm, unregister_data_as_single_file_in_virtual_vrom(mount_dir, rel_fname));
  return 1;
}

static bool check_tex_exists(const char *tex_nm) { return get_managed_texture_id(tex_nm) != BAD_TEXTUREID; }

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_sceneload, "game_load", sq::VM_ALL)
{
  Sqrat::Table tbl(vm);
  tbl //
    .SquirrelFunc("request_ugm_manifest", sceneload::request_ugm_manifest_sq, 3, ".ss")
    .SquirrelFunc("register_blob_as_virtual_file", register_virtual_file_sq, 4, ".ssx")
    .SquirrelFunc("unregister_virtual_file", unregister_virtual_file_sq, 3, ".ss")
    .Func("check_tex_exists", check_tex_exists)
    /**/;
  return tbl;
}
