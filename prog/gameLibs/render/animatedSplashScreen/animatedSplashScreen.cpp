// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animated_splash_screen_api.h>
#include <splash_graph_api.h>

#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/random/dag_random.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_texPackMgr2.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <util/dag_watchdog.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <osApiWrappers/dag_spinlock.h>
#include <render/noiseTex.h>
#include <render/hdrRender.h>
#include <3d/dag_createTex.h>
#include <osApiWrappers/dag_direct.h>
#include <drv/3d/dag_variableRateShading.h>
#include <render/antialiasing.h>
#include <render/shaderCacheWarmup/shaderCacheWarmup.h>
#include <EASTL/fixed_vector.h>
#include <dag/dag_vectorMap.h>
#include <EASTL/algorithm.h>
#include <util/dag_hash.h>

CONSOLE_BOOL_VAL("render", splashScreenFullRes, false);
CONSOLE_BOOL_VAL("app", useTitleLogoForSplash, false);
CONSOLE_FLOAT_VAL("app", splashDtSkipThreshold, 10);
// the splash thread draws a frame no more often than this: 33 is about 30 fps,
// 16 about 60
CONSOLE_INT_VAL("app", splashFrameMsec, 33, 1, 1000);
// a stop waits out the thread's sleep, so it sleeps in slices this long at most
static constexpr uint32_t SPLASH_SLEEP_SLICE_MSEC = 33;

extern bool grs_draw_wire;

static float splash_time = 0;
static float slow_exit_time = 0;
static float rush_exit_time = 0;
static int slow_exit_latched = 0;
// the host asks from its own thread while the splash thread may be drawing
static volatile int rush_exit_latched = 0;
static volatile int slow_exit_requested = 0, rush_exit_requested = 0;
// one atomic word: the graph can vanish
static dag::AtomicPod<SplashExitSeconds> scene_exit_seconds{SplashExitSeconds()};
static volatile int scene_opens_onto_game = 0;
static float splash_seed = 0.f;
static eastl::string splash_screen_name;
// the host loop and the platform exit path both read it
static volatile int splash_started = 0;
static PostFxRenderer loadingSplash;
// a scene with <graphDir>/<scene>.graph.blk draws through the graph instead of loadingSplash
static eastl::unique_ptr<SplashGraph> splash_graph;
// hosts restart the splash per loading phase, and a scene whose graph failed
// is not retried in this process: a load failure repeats every time, and a
// create failure retried each phase would only repeat its logerr for a splash.
// One session logs it, the graph and the fallback logerr both, and the rest
// skip the graph quietly. A key is dir plus the resolved output shader name,
// which carries the scene and its HDR variant, so one broken scene does not
// stand in for another, nor one variant for the other
static eastl::string live_graph_key;
// where this session's graph came from: the only fact the latch rule needs
enum class GraphSource
{
  None,
  File,
  Copy
};
static GraphSource live_graph_source = GraphSource::None;
static eastl::fixed_vector<eastl::string, 4, /*bEnableOverflow*/ true> failed_graph_keys;
static void latch_failed_graph()
{
  if (eastl::find(failed_graph_keys.begin(), failed_graph_keys.end(), live_graph_key) == failed_graph_keys.end())
    failed_graph_keys.push_back(live_graph_key);
}
// loadingSplash, splash_graph, the keys, the legacy counters and the fallback
// list: start() writes them before SplashThread::start(), stop() after the
// join, and in between only a draw does, and a draw reads no settings block.
// Two threads may draw in a session, the splash thread and the caller through
// animated_splash_screen_draw, serialized by the d3d::GpuAutoLock each holds
// across its whole draw, like the register_render pair below
static int legacy_frame_no = 0;
static float legacy_prev_time = 0;
static Texture *halfTargetTex = nullptr;
static BaseTexture *titleLogo = nullptr;
static int last_splash_draw_msec = 0;
static int paper_white_nits = 200;
static bool is_encoding = true;
// host-owned, cleared after its own draw
static bool over_game = false;

static void (*splash_render_func)(int w, int h, void *arg) = nullptr;
static void *splash_render_func_arg = nullptr;
static bool splash_render_func_exclusive = false;

#define ANIMATED_SPLASH_SCREEN_CONST_LIST                            \
  VAR(loading_splash_noise_64_tex_l8, false)                         \
  VAR(loading_splash_noise_128_tex_hash, false)                      \
  VAR(loading_splash_iGlobalTime_iResolution_iPaperWhiteNits, false) \
  VAR(loading_splash_iSeed, true)                                    \
  VAR(loading_splash_title_logo, true)                               \
  VAR(loading_splash_exit, true)                                     \
  VAR(loading_splash_iFrame_history_iPrevTime, true)                 \
  VAR(loading_splash_node_params, true)                              \
  VAR(loading_splash_linear_clamp, true)                             \
  VAR(loading_splash_point_clamp, true)
#define VAR(a, opt) static ShaderVariableInfo a##_const_no{#a "_const_no", opt};
ANIMATED_SPLASH_SCREEN_CONST_LIST
#undef VAR

SplashExitSeconds animated_splash_screen_exit_seconds() { return scene_exit_seconds.load(dag::mo::acquire); }

bool animated_splash_screen_opens_onto_game() { return interlocked_acquire_load(scene_opens_onto_game) != 0; }

// zero outside a session: both of its transitions write it
static void clear_exit_state()
{
  slow_exit_time = rush_exit_time = 0;
  slow_exit_latched = 0;
  interlocked_release_store(rush_exit_latched, 0);
  interlocked_release_store(slow_exit_requested, 0);
  interlocked_release_store(rush_exit_requested, 0);
}

void animated_splash_screen_begin_slow_exit() { interlocked_release_store(slow_exit_requested, 1); }
void animated_splash_screen_begin_rush_exit() { interlocked_release_store(rush_exit_requested, 1); }

bool animated_splash_screen_exit_done()
{
  // no session to wait for, so a host asking is already past it, as the stub says
  if (!is_animated_splash_screen_started())
    return true;
  const SplashExitSeconds sec = scene_exit_seconds.load(dag::mo::acquire);
  return splash_exit_pacing(splash_time, slow_exit_time, rush_exit_time, sec.slow, sec.rush).portal >= 1.f;
}

// an unlatched request counts too
bool animated_splash_screen_rush_under_way()
{
  return interlocked_acquire_load(rush_exit_latched) || interlocked_acquire_load(rush_exit_requested);
}

void animated_splash_screen_set_over_game(bool draw_over_game) { over_game = draw_over_game; }

static volatile int allow_watchdog_kick = 0;
void animated_splash_screen_allow_watchdog_kick(bool allow) { interlocked_release_store(allow_watchdog_kick, allow ? 1 : 0); }

// Contract: register/unregister must be called under the d3d ownership lock
// (d3d::GpuAutoLock). Every animated_splash_screen_draw caller holds that lock
// for the whole frame including the callback, so the lock both publishes the
// func/arg pair atomically and guarantees that after an unregister returns no
// callback is in flight, making it safe to destroy the callback context.
void animated_splash_screen_register_render(void (*render_func)(int w, int h, void *arg), void *arg, bool exclusive)
{
  splash_render_func = render_func;
  splash_render_func_arg = arg;
  splash_render_func_exclusive = exclusive;
}

void load_title_logo()
{
  if (!useTitleLogoForSplash.get() || titleLogo)
    return;

  int w = 0, h = 0;
  d3d::get_render_target_size(w, h, nullptr, 0);
  if (w <= 0 || h <= 0)
  {
    w = 1280;
    h = 720;
  }

#if _TARGET_ANDROID
  const char *path = "asset://title_logo.svg";
#else
  const char *path = "ui/title_logo.svg";
#endif
  eastl::string fpath;
  // use half size always
  fpath.sprintf("%s:%u:%u:K", path, w / 2, h / 2);

  if (!dd_file_exists(path))
    // silently wait while engine systems initialize
    return;

  titleLogo = ::create_texture(fpath.c_str(), TEXCF_RGB | TEXCF_ABEST, 0, false, nullptr);
  if (!titleLogo)
    // silently wait while engine systems initialize
    return;

  if (loading_splash_title_logo_const_no < 0)
    logerr("shader variable %s is not present in shaders dump", loading_splash_title_logo_const_no.getName());
}

// Owned copies: pointers into dgs_get_settings() expire on vrom reload.
using LoadingScreenNames = eastl::fixed_vector<eastl::string, 16, /*bEnableOverflow*/ true>;
// resolved at start on the caller thread: the settings block is not the splash
// thread's to read, and a graph that drops mid-session falls back from here
static LoadingScreenNames fallback_names;

static void gather_loading_screens(const DataBlock *screens, LoadingScreenNames &out_names)
{
  for (int i = 0; i < screens->paramCount(); i++)
    if (screens->getParamType(i) == DataBlock::TYPE_STRING && strcmp(screens->getParamName(i), "screen") == 0)
    {
      const char *name = screens->getStr(i);
      if (*name)
        out_names.emplace_back(name);
      else
        debug("[splash] empty loading screen entry in screens{}, skipped");
    }
}

// The one mode vocabulary: these constants drive both the reserved-name
// check and the mode dispatch in resolve_loading_screen_name, so adding a
// mode word here reserves it in the same edit. A screen whose name equals
// a mode word is unaddressable by name (the mode word wins in the schema).
static constexpr uint32_t MODE_DEFAULT_H = "default"_h;
static constexpr uint32_t MODE_RANDOM_H = "random"_h;
static constexpr uint32_t MODE_EXACT_H = "exact"_h;
static constexpr uint32_t MODE_CHOSEN_H = "chosen"_h;

static bool is_reserved_screen_name(const char *name)
{
  const uint32_t h = str_hash_fnv1(name);
  return h == MODE_DEFAULT_H || h == MODE_RANDOM_H || h == MODE_EXACT_H || h == MODE_CHOSEN_H;
}

static int find_loading_screen(const LoadingScreenNames &names, const char *name)
{
  if (!name || !*name || is_reserved_screen_name(name))
    return -1;
  for (int i = 0; i < (int)names.size(); i++)
    if (names[i] == name)
      return i;
  return -1;
}

// Built-in fallback when the config never names a scene; renders only where
// the host's shader list compiles loading/rgbTriangle.dshl (every in-tree
// host either names its scene or compiles the one it names).
static const char *default_loading_screen_name() { return "rgbTriangle"; }

// the config default when listed, else the first listed screen
// (gather_fallback_scene_names logs an unlisted default, once per start)
static int default_loading_screen_id(const LoadingScreenNames &names, const char *def)
{
  const int id = find_loading_screen(names, def);
  return id >= 0 ? id : 0;
}

// Owned copy: the caller's block may be a script-side temporary. Empty means
// no override: every lookup falls through to settings.blk anyway.
// Script callers may run on a UI worker thread while the resolver runs on the
// main thread, so the block is guarded for its whole read in the resolver.
static OSSpinlock splash_config_lock;
static DataBlock splash_config_override; // guarded by splash_config_lock
// a changed config must win over the sub-2s "same visual session" rule
static int splash_config_changed = 0; // guarded by splash_config_lock, like the block itself

void animated_splash_screen_set_config(const DataBlock *cfg)
{
  // copy outside the lock: the locked work is a compare and a move, the old
  // block is freed after the scope
  DataBlock copy;
  if (cfg)
    copy = *cfg;
  const bool isOverride = !copy.isEmpty();
  bool changed = false;
  {
    OSSpinlockScopedLock lock(splash_config_lock);
    // repeating the active config would restart the visual session for the same
    // scene, so only a different one re-resolves. Content equality, not
    // resolution: a block differing only in a key the resolver never reads
    // still counts. nullptr and an empty block are one state, equal to none
    changed = copy != splash_config_override;
    if (changed)
    {
      splash_config_override = eastl::move(copy);
      splash_config_changed = 1;
    }
  }
  if (changed)
    debug("[splash] loading screen config %s", isOverride ? "set at runtime" : "back to settings.blk");
}

// Graph blks the game carries for the window before their vrom is mounted. The
// file wins, so a copy left here never hides a newer one. Written before the
// first start and read at start, both on the caller thread: no lock needed.
static dag::VectorMap<eastl::string, DataBlock> provided_graphs;

void animated_splash_screen_set_graph(const char *scene_name, const DataBlock *blk)
{
  G_ASSERT(is_main_thread()); // the registry is unguarded, and a start reads it here
  if (!scene_name || !*scene_name)
    return;
  // an empty block drops the copy, as it clears the config in set_config:
  // keeping one would only fail the next start's load_from_blk
  if (blk && !blk->isEmpty())
    provided_graphs[scene_name] = *blk;
  else
    provided_graphs.erase(scene_name);
}

static const DataBlock *find_provided_graph(const char *scene_name)
{
  // find_as: a plain find would build an eastl::string key on every start
  const auto it = provided_graphs.find_as(scene_name, eastl::less<>());
  return it != provided_graphs.end() ? &it->second : nullptr;
}

// Per-key layering: the runtime config wins where it has the key, settings.blk
// fills the rest (the user's mode/exact/chosen survive a list-only config).
struct LoadingScreenConfig
{
  const DataBlock *over = nullptr;
  const DataBlock *base = nullptr;

  const char *getStr(const char *key, const char *def) const
  {
    return over->findParam(key) >= 0 ? over->getStr(key, def) : base->getStr(key, def);
  }
  // never null: an absent block reads as an empty one, so callers need no guard
  const DataBlock *getBlock(const char *key) const
  {
    const DataBlock *b = over->getBlockByName(key);
    return b ? b : base->getBlockByNameEx(key);
  }
};

// out_over owns the override copy the returned view points into, so it must
// outlive the view. The changed flag is read and cleared inside the copy's lock
// scope, which the setter also holds across both its writes: so one start sees
// one config, and a set that lands after the scope keeps its flag for the next
// start. The settings read stays outside, a spinlock must not span it.
static LoadingScreenConfig snapshot_loading_screen_config(DataBlock &out_over, bool &inout_reresolve)
{
  {
    OSSpinlockScopedLock lock(splash_config_lock);
    if (splash_config_changed)
    {
      inout_reresolve = true;
      splash_config_changed = 0;
    }
    out_over = splash_config_override;
  }
  LoadingScreenConfig cfg;
  cfg.base = dgs_get_settings()->getBlockByNameEx("loadingScreen"); // the shared empty block when absent
  cfg.over = &out_over;                                             // an empty override falls through to base by itself
  return cfg;
}

// Resolves the scene name for one splash session, honoring the full
// loadingScreen contract (screens{}, default, mode=default|random|exact
// |chosen, exact and chosen sub-configs). Reserved-name screens are
// filterable through find_loading_screen; the mode words always win.
// Returns an owned string: the sources live in a reloadable DataBlock.
static eastl::string resolve_loading_screen_name(const LoadingScreenConfig &cfg)
{
  LoadingScreenNames names;
  gather_loading_screens(cfg.getBlock("screens"), names);
  const char *def = cfg.getStr("default", "");
  if (names.empty())
  {
    if (const char *mode = cfg.getStr("mode", nullptr))
      debug("[splash] loadingScreen mode '%s' ignored: screens{} lists no scenes", mode);
    return eastl::string(*def ? def : default_loading_screen_name());
  }

  int defaultId = default_loading_screen_id(names, def);

  const char *mode = cfg.getStr("mode", "default");
  const uint32_t modeH = str_hash_fnv1(mode);
  int picked = -1;
  if (modeH == MODE_RANDOM_H)
    picked = grnd() % names.size();
  else if (modeH == MODE_EXACT_H)
  {
    const char *exact = cfg.getStr("exact", "");
    picked = find_loading_screen(names, exact);
    if (picked < 0)
      debug("[splash] exact loading screen '%s' is not listed, using default", exact);
  }
  else if (modeH == MODE_CHOSEN_H)
  {
    const DataBlock *chosen = cfg.getBlock("chosen");
    eastl::fixed_vector<int, 16, /*bEnableOverflow*/ true> chosenIds;
    for (int i = 0; i < chosen->paramCount(); i++)
    {
      if (chosen->getParamType(i) != DataBlock::TYPE_BOOL || !chosen->getBool(i))
        continue;
      const char *pname = chosen->getParamName(i);
      int idx = find_loading_screen(names, pname);
      if (idx >= 0)
        chosenIds.push_back(idx);
      else
        debug("[splash] chosen loading screen '%s' is not listed, ignored", pname);
    }
    if (!chosenIds.empty())
      picked = chosenIds[grnd() % chosenIds.size()];
    else
      debug("[splash] chosen mode with no listed chosen screens, using default");
  }
  else if (modeH != MODE_DEFAULT_H)
    debug("[splash] unknown loading screen mode '%s', using default", mode);
  if (picked < 0)
    picked = defaultId;

  debug("[splash] loading screen '%s' (mode=%s, %s)", names[picked].c_str(), mode,
    cfg.over->isEmpty() ? "settings.blk" : "runtime config");
  return eastl::move(names[picked]);
}

// Names the per-scene HDR variant used for this frame. do_encode=false forces
// variant 4 (pre-encoded SDR-in-HDR pipe); otherwise variant matches HdrOutputMode.
static eastl::string build_shader_name(const char *scene_name, bool do_encode)
{
  const int variant = do_encode ? static_cast<int>(hdrrender::get_hdr_output_mode()) : 4;
  return variant > 0 ? eastl::string(eastl::string::CtorSprintf{}, "loading_splash_%s_%d", scene_name, variant)
                     : eastl::string(eastl::string::CtorSprintf{}, "loading_splash_%s", scene_name);
}

// The scenes tried when the resolved one cannot draw, in the order
// resolve_loading_screen_name falls back: its default (default_loading_screen_id),
// the first listed screen, the built-in default
static void gather_fallback_scene_names(const LoadingScreenConfig &cfg, LoadingScreenNames &out_names)
{
  const char *def = cfg.getStr("default", "");
  LoadingScreenNames names;
  gather_loading_screens(cfg.getBlock("screens"), names);
  if (names.empty())
  {
    if (*def)
      out_names.push_back(eastl::string(def));
  }
  else
  {
    const int defaultId = default_loading_screen_id(names, def);
    if (*def && names[defaultId] != def)
      debug("[splash] default loading screen '%s' is not listed, using '%s'", def, names[0].c_str());
    out_names.push_back(names[defaultId]);
    if (defaultId != 0)
      out_names.push_back(names[0]);
  }
  out_names.push_back(eastl::string(default_loading_screen_name()));
}

// The single-shader path: the scene's own shader, else the first fallback
// scene that has one. A graph scene has no shader of its own (its composite
// is a family of its own, see SplashGraph::load), so it lands on a fallback.
// log_failure = false when the caller already logged this scene's failure
// in an earlier session, so a broken scene names its fallback once per run
static void init_single_shader(const char *scene_name, bool do_encode, bool log_failure = true)
{
  loadingSplash.init(build_shader_name(scene_name, do_encode).c_str(), /*is_optional*/ true);
  if (loadingSplash.getMat())
    return;
  for (const eastl::string &name : fallback_names)
  {
    loadingSplash.init(build_shader_name(name.c_str(), do_encode).c_str(), /*is_optional*/ true);
    if (loadingSplash.getMat())
    {
      if (log_failure)
        logerr("[splash] loading screen '%s' cannot draw, drawing '%s'", scene_name, name.c_str());
      return;
    }
  }
  if (log_failure)
    logerr("[splash] loading screen '%s' cannot draw, and no fallback scene has a shader", scene_name);
}

// every graph change republishes them
static void publish_exit_seconds()
{
  SplashExitSeconds v;
  if (splash_graph)
  {
    v.slow = splash_graph->slowExitSeconds();
    v.rush = splash_graph->rushExitSeconds();
  }
  scene_exit_seconds.store(v, dag::mo::release);
  interlocked_release_store(scene_opens_onto_game, splash_graph && splash_graph->opensOntoGame() ? 1 : 0);
}

// A graph that cannot serve gives way to the single shader. Only a file
// failure latches the scene, because reading the same file again fails the same
// way. A scene with no blk did not fail (the legacy path), and a carried copy
// failure says nothing about the file a later vrom mount serves
static void drop_graph_for_single_shader(bool do_encode, bool log_failure)
{
  splash_graph.reset();
  publish_exit_seconds();
  if (live_graph_source == GraphSource::File)
    latch_failed_graph();
  init_single_shader(splash_screen_name.c_str(), do_encode, log_failure);
}

void animated_splash_screen_start(bool do_encode)
{
  if (dgs_get_settings()->getBool("skipSplashScreenAnimation", false))
    return;

  debug("[splash] animated_splash_screen_start: splash_started=%d", splash_started);
  G_ASSERT(is_main_thread()); // every caller, the SplashThread constructor included, runs here
  ddsx::set_streaming_mode(ddsx::MultiDecoders);
  if (splash_started)
    return;

  splashDtSkipThreshold.set(dgs_get_settings()->getReal("splashDtSkipThreshold", 10.0f));
  splashFrameMsec.set(dgs_get_settings()->getInt("splashFrameMsec", 33));

  useTitleLogoForSplash.set(dgs_get_settings()->getBool("useTitleLogoForSplash", false));
#if _TARGET_PC
  splashScreenFullRes.set(dgs_get_settings()->getBool("splashScreenFullRes", false));
#endif

  init_and_get_l8_64_noise();
  init_and_get_hash_128_noise();
  load_title_logo();

  const uint32_t lastDraw = (uint32_t)interlocked_relaxed_load(last_splash_draw_msec);
  // one snapshot per start, and it decides the re-resolve too: the scene and its
  // fallback chain must come from the config the decision was made on
  bool reresolve = !lastDraw || get_time_msec() > lastDraw + 2000;
  DataBlock over;
  const LoadingScreenConfig cfg = snapshot_loading_screen_config(over, reresolve);
  if (reresolve)
  {
    splash_screen_name = resolve_loading_screen_name(cfg);
    splash_time = 0;
    splash_seed = (grnd() % 10000) * 0.01f;
    legacy_frame_no = 0;
  }
  clear_exit_state(); // a kept visual session still re-arms

  is_encoding = do_encode;
  // stop() cleared the material and the graph, so a start always re-inits
  // here, including a same-name restart; splash_screen_name may persist
  // across a sub-2s stop/start to keep one visual session
  const char *graphDir = dgs_get_settings()->getBlockByNameEx("loadingScreen")->getStr("graphDir", nullptr);
  fallback_names.clear();
  gather_fallback_scene_names(cfg, fallback_names);
  // the resolved output shader name carries the scene and its HDR variant
  const eastl::string shaderName = build_shader_name(splash_screen_name.c_str(), do_encode);
  live_graph_key.sprintf("%s|%s", graphDir ? graphDir : "", shaderName.c_str());
  const bool firstTry = eastl::find(failed_graph_keys.begin(), failed_graph_keys.end(), live_graph_key) == failed_graph_keys.end();
  bool blkPresent = false;
  live_graph_source = GraphSource::None;
  if (firstTry)
  {
    splash_graph = SplashGraph::load(graphDir, splash_screen_name.c_str(), shaderName.c_str(), &blkPresent);
    if (blkPresent)
      live_graph_source = GraphSource::File;
    // the carried copy stands in only where no file exists: a broken file is a
    // failure to report, not something the copy should paper over. !splash_graph
    // is redundant while load returns null whenever blkPresent is false, and is
    // kept so a drift there cannot overwrite a graph that did load
    else if (!splash_graph)
      if (const DataBlock *provided = find_provided_graph(splash_screen_name.c_str()))
      {
        splash_graph = SplashGraph::load_from_blk(*provided, /*src_name*/ nullptr, splash_screen_name.c_str(), shaderName.c_str());
        live_graph_source = GraphSource::Copy;
      }
  }
  if (!splash_graph)
    drop_graph_for_single_shader(do_encode, /*log_failure*/ firstTry);
  else
    publish_exit_seconds();
  interlocked_release_store(splash_started, 1);

  paper_white_nits = dgs_get_settings()->getBlockByName("video")->getInt("paperWhiteNits", paper_white_nits);

  ShaderElement::invalidate_cached_state_block();
}

bool is_animated_splash_screen_encoding() { return is_encoding; }

void animated_splash_screen_stop()
{
  debug("[splash] animated_splash_screen_stop: splash_started=%d", splash_started);
  if (!splash_started)
    return;
  if (is_animated_splash_screen_in_thread())
    return stop_animated_splash_screen_in_thread();

  splash_graph.reset();
  publish_exit_seconds();
  loadingSplash.clear();
  del_d3dres(halfTargetTex);
  del_d3dres(titleLogo);
  release_l8_64_noise();
  release_hash_128_noise();
  interlocked_release_store(splash_started, 0);
  clear_exit_state(); // a live session skips re-arming
  ddsx::set_streaming_mode(ddsx::BackgroundSerial);
}

// the host's overlay draws over the presented splash, in the output target
static void call_splash_render_func()
{
  if (!splash_render_func)
    return;
  int w = 0, h = 0;
  d3d::get_target_size(w, h);
  splash_render_func(w, h, splash_render_func_arg);
}

struct ShaderCacheWarmupProgressInfo
{
  uint32_t completed = 0;
  uint32_t total = 0;
};

// dedicated function to make it easier to implement alternate ways of obtaining the counters
static eastl::optional<ShaderCacheWarmupProgressInfo> get_shader_cache_warmup_progress()
{
  static ShaderCacheWarmupProgressInfo prevResult;
  if (d3d::get_driver_code().is(d3d::dx12 && !d3d::anyXbox))
  {
    const uint32_t currentQueueLength = d3d::driver_command(Drv3dCommand::GET_PIPELINE_COMPILATION_QUEUE_LENGTH);
    if (currentQueueLength > 0)
    {
      prevResult.total = max(prevResult.total, currentQueueLength);
      prevResult.completed = max(prevResult.completed, prevResult.total - currentQueueLength);
      return prevResult;
    }
    else
    {
      prevResult = {};
    }
  }
  else
  {
    if (shadercache::get_warmup_status(prevResult.completed, prevResult.total))
    {
      return prevResult;
    }
  }
  return {};
}

static void draw_shader_cache_warmup_progress()
{
  if (auto shaderCacheProgress = get_shader_cache_warmup_progress(); shaderCacheProgress)
  {
    shadercache::draw_warmup_status(shaderCacheProgress->completed, shaderCacheProgress->total);
  }
}

// Graph scenes skip the half-res target and the VRS 2x2 trick: coarse shading
// fights sub-pixel jitter, and internal resolution is the graph's own scale:r.
// False when the graph failed this frame and is gone: the caller then draws
// the same frame through the single-shader path.
static bool draw_graph_frame()
{
  load_title_logo();
  if (!splash_render_func_exclusive)
  {
    SplashGraph::Frame frame;
    frame.time = splash_time;
    frame.seed = splash_seed;
    frame.slowExitTime = slow_exit_time;
    frame.rushExitTime = rush_exit_time;
    frame.paperWhiteNits = float(paper_white_nits);
    frame.titleLogo = titleLogo;
    frame.overGame = over_game;
    if (!splash_graph->draw(frame))
    {
      // the graph logged why; the scene goes on through a single shader. A file
      // failure repeats, so it latches; a carried copy's does not stand for the
      // file that is not readable yet, so that one leaves the key free
      drop_graph_for_single_shader(is_encoding, /*log_failure*/ true);
      return false;
    }
  }

  draw_shader_cache_warmup_progress();
  call_splash_render_func();
  return true;
}

void animated_splash_screen_draw()
{
  uint32_t currentTimeMs = get_time_msec();
  float splash_dt = (currentTimeMs - interlocked_exchange(last_splash_draw_msec, currentTimeMs)) * 1e-3;
  if (::grs_draw_wire)
    d3d::setwire(0);

  // cut out laggy frames to avoid fast progress on animation
  splash_time += min(splashDtSkipThreshold.get(), splash_dt);

  // one snapshot: a store between two loads would stamp a rush with no slow
  const bool rushAsked = interlocked_acquire_load(rush_exit_requested) != 0;
  // old scenes read the slow lane only
  if (!slow_exit_latched && (interlocked_acquire_load(slow_exit_requested) || rushAsked))
  {
    slow_exit_latched = 1;
    slow_exit_time = splash_exit_stamp(splash_time);
  }
  if (!rush_exit_latched && rushAsked)
  {
    // the time first, so a reader that sees the latch sees a stamp
    rush_exit_time = splash_exit_stamp(splash_time);
    interlocked_release_store(rush_exit_latched, 1);
  }

  if (splash_graph && draw_graph_frame())
    return;
  if (!loadingSplash.getElem())
  {
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    return;
  }

  {
    int w = 0, h = 0;
    d3d::get_target_size(w, h);

    bool loadingHalfRes = !splashScreenFullRes;
    bool loadingHalfResThroughVrs = d3d::get_driver_desc().caps.hasVariableRateShading && loadingHalfRes;
    load_title_logo();
    if (splash_render_func_exclusive || loadingHalfResThroughVrs || hdrrender::get_hdr_output_mode() == HdrOutputMode::HDR10_AND_SDR)
      loadingHalfRes = false;

    if (loadingHalfRes && !halfTargetTex)
    {
      uint32_t rtFmt = TEXFMT_R11G11B10F;
      if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET) || hdrrender::is_hdr_enabled())
        rtFmt = TEXFMT_A16B16G16R16F;
      halfTargetTex = d3d::create_tex(NULL, w / 2, h / 2, rtFmt | TEXCF_RTARGET, 1, "splash_half_target");
      if (halfTargetTex)
        debug("[splash] created splash_half_target on demand");
      else
        logerr("Failed to create RT <splash_half_target>");
    }

    Driver3dRenderTarget outputRt;
    Texture *halfResRt = loadingHalfRes ? halfTargetTex : nullptr;
    if (halfResRt)
    {
      d3d::get_render_target(outputRt);
      d3d::set_render_target({}, DepthAccess::RW, {{halfResRt, 0, 0}});
    }

    d3d::get_target_size(w, h);
    splash_slots::set_time_const(STAGE_PS, loading_splash_iGlobalTime_iResolution_iPaperWhiteNits_const_no.get_int(), splash_time, w,
      h, float(paper_white_nits));
    if (bool(loading_splash_iSeed_const_no))
    {
      float seedParams[4] = {splash_seed, 0.f, 0.f, 0.f};
      d3d::set_ps_const(loading_splash_iSeed_const_no.get_int(), seedParams, 1);
    }
    // no graph blk, so no declared durations
    if (bool(loading_splash_exit_const_no))
      splash_slots::set_exit_const(STAGE_PS, loading_splash_exit_const_no.get_int(), slow_exit_time, rush_exit_time, 0.f, 0.f);
    splash_slots::bind_single_shader_slots(STAGE_PS, splash_slots::reg_or_none(loading_splash_iFrame_history_iPrevTime_const_no),
      splash_slots::reg_or_none(loading_splash_node_params_const_no), splash_slots::reg_or_none(loading_splash_linear_clamp_const_no),
      splash_slots::reg_or_none(loading_splash_point_clamp_const_no), legacy_frame_no, legacy_prev_time, splash_time);
    const SharedTexWithShaderVar &noise64 = init_and_get_l8_64_noise();
    const SharedTexWithShaderVar &noise128 = init_and_get_hash_128_noise();
    d3d::SamplerHandle smp = d3d::request_sampler({});
    d3d::set_tex(STAGE_PS, loading_splash_noise_64_tex_l8_const_no.get_int(), noise64.getTex2D());
    d3d::set_sampler(STAGE_PS, loading_splash_noise_64_tex_l8_const_no.get_int(), smp);
    d3d::set_tex(STAGE_PS, loading_splash_noise_128_tex_hash_const_no.get_int(), noise128.getTex2D());
    d3d::set_sampler(STAGE_PS, loading_splash_noise_128_tex_hash_const_no.get_int(), smp);
    if (titleLogo)
    {
      const int logoReg = loading_splash_title_logo_const_no.get_int();
      d3d::set_tex(STAGE_PS, logoReg, titleLogo);
      // a dump that pins the logo on a fixed clamp register reads it through
      // that sampler, as the graph does: a border sampler there would replace
      // the clamp every scene input shares
      const bool onClamp = (bool(loading_splash_linear_clamp_const_no) && logoReg == loading_splash_linear_clamp_const_no.get_int()) ||
                           (bool(loading_splash_point_clamp_const_no) && logoReg == loading_splash_point_clamp_const_no.get_int());
      if (!onClamp)
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
        d3d::set_sampler(STAGE_PS, logoReg, d3d::request_sampler(smpInfo));
      }
    }
    if (!splash_render_func_exclusive)
    {
      if (loadingHalfResThroughVrs)
        d3d::set_variable_rate_shading(2, 2);

      loadingSplash.getElem()->setProgram(0);
      d3d::setvsrc(0, 0, 0);
      d3d::draw(PRIM_TRILIST, 0, 1);
      // c3 counts drawn frames: an exclusive callback frame draws no scene
      legacy_frame_no++;
      legacy_prev_time = splash_time;

      if (loadingHalfResThroughVrs)
        d3d::set_variable_rate_shading(1, 1);
    }

    draw_shader_cache_warmup_progress();

    if (halfResRt)
    {
      // outputRt.color[0].tex == nullptr may either mean the backbuffer or no texture is bound as the target.
      // Make sure it's the backbuffer by checking the usage. (d3d::stretch_rect(*, d3d::get_backbuffer_tex()) will stretch to the
      // backbuffer)
      G_ASSERT(outputRt.isColorUsed(0));
      halfResRt->texmiplevel(0, 0);
      d3d::stretch_rect(halfResRt, outputRt.getColor(0).tex);
      halfResRt->texmiplevel(-1, -1);
      d3d::set_render_target(outputRt);
    }

    // After the half-res resolve: the callback draws over the presented
    // splash, in the full-res output target, so a UI laid out in screen
    // space renders correctly regardless of the splash internal resolution
    call_splash_render_func();

    release_l8_64_noise();
    release_hash_128_noise();
  }
}

bool is_animated_splash_screen_started() { return interlocked_acquire_load(splash_started) != 0; }

static void splash_render()
{
  d3d::GpuAutoLock gpuLock;
  d3d::set_render_target();

  d3d::clearview(CLEAR_TARGET, 0, 0, 0);
  animated_splash_screen_draw();

  d3d::update_screen();
}

class SplashThread;

static eastl::unique_ptr<SplashThread> splash_thread = nullptr;
d3d::BeforeWindowDestroyedCookie *splash_cookie = nullptr;
#if _TARGET_SCARLETT
static bool splash_vsync_was_enabled = false;
#endif

class SplashThread final : public DaThread
{
  void execute() override
  {
    while (!interlocked_acquire_load(this->terminating))
    {
      const uint32_t lastDraw = (uint32_t)interlocked_relaxed_load(last_splash_draw_msec);
      const uint32_t frameMsec = uint32_t(splashFrameMsec.get());
      if (get_time_msec() > lastDraw + frameMsec)
      {
        if (!d3d::is_in_device_reset_now())
          splash_render();
        if (interlocked_acquire_load(allow_watchdog_kick))
          watchdog_kick();
      }
      sleep_msec(min(frameMsec - clamp(get_time_msec() - lastDraw, 0u, frameMsec), SPLASH_SLEEP_SLICE_MSEC));
    }
  }

public:
  SplashThread() :
    DaThread("SplashRenderThread",
#if _TARGET_PC_WIN // For Fraps and other 3rd parties that hook d3d present
      256 << 10,
#else
      128 << 10,
#endif
      // we increase priority by one step to match main thread priority
      cpujobs::DEFAULT_THREAD_PRIORITY - cpujobs::THREAD_PRIORITY_LOWER_STEP, WORKER_THREADS_AFFINITY_MASK)
  {
    animated_splash_screen_start();
  }
};

void start_animated_splash_screen_in_thread()
{
  if (dgs_get_settings()->getBool("skipSplashScreenAnimation", false) ||
      dgs_get_settings()->getBool("skipSplashScreenAnimationInThread", false))
    return;

  // else two threads draw one scene
  if (splash_started && !splash_thread)
    animated_splash_screen_stop();

  render::antialiasing::enable_frame_generation(false);

#if _TARGET_PC_WIN
  if (is_main_thread())
  {
    // render single splash frame when called from main thread in full-screen
    bool was_started = is_animated_splash_screen_started();
    if (!was_started)
      animated_splash_screen_start();
    splash_render();
    if (!was_started)
      animated_splash_screen_stop();
  }
#endif
  debug("[splash] start_animated_splash_screen_in_thread: splash_thread=%p splash_started=%d", splash_thread.get(), splash_started);
  if (!splash_thread)
  {
    // It's enough to register only once and don't unregister, because the stop_animated_splash_screen_in_thread can be called anyway
    if (!splash_cookie)
      splash_cookie = d3d::register_before_window_destroyed_callback(stop_animated_splash_screen_in_thread);
#if _TARGET_SCARLETT
    splash_vsync_was_enabled = d3d::get_vsync_enabled();
    d3d::enable_vsync(false);
#endif
    splash_thread.reset(new SplashThread());
    splash_thread->start();
  }
}

// after it, that state is the caller's
static void join_splash_thread()
{
  if (!splash_thread)
    return;
  splash_thread->terminate(true, -1);
  splash_thread.reset();
#if _TARGET_SCARLETT
  d3d::enable_vsync(splash_vsync_was_enabled);
#endif
}

void stop_animated_splash_screen_in_thread()
{
  debug("[splash] stop_animated_splash_screen_in_thread: splash_thread=%p splash_started=%d", splash_thread.get(), splash_started);
  // a kept scene has no thread
  join_splash_thread();
  animated_splash_screen_stop();
}

void stop_animated_splash_screen_thread_keep_scene()
{
  G_ASSERT(is_main_thread());
  join_splash_thread();
  const bool rushUnderWay = animated_splash_screen_rush_under_way();
  debug("[splash] stop_animated_splash_screen_thread_keep_scene: rush_under_way=%d splash_started=%d", rushUnderWay, splash_started);
  if (!rushUnderWay)
    animated_splash_screen_stop();
}

bool is_animated_splash_screen_in_thread() { return splash_thread.get() != nullptr; }
