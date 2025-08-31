// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include <objbase.h>
#endif

#include <osApiWrappers/dag_critSec.h>
#include <EASTL/fixed_string.h>
#include <EASTL/vector.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include "camera/sceneCam.h"
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <propsRegistry/commonPropsRegistry.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/debug.h>
#include <soundSystem/banks.h>
#include <soundSystem/dsp.h>
#include <soundSystem/quirrel/sqSoundSystem.h>
#include <perfMon/dag_statDrv.h>
#include <daRg/soundSystem/uiSoundSystem.h>
#include "actionSoundProps.h"
#include <math/dag_mathUtils.h>
#include <ecs/delayedAct/actInThread.h>

#include <daECS/core/updateStage.h>
#include <ecs/core/entityManager.h>
#include "sound/ecsEvents.h"
#include "camera/sceneCam.h"
#include "main/app.h"
#include <osApiWrappers/dag_atomic_types.h>

#include <main/gameLoad.h>
#include "main/level.h"
#include "sound/dngSound.h"

#define DNGSND_IS_MAIN_THREAD G_ASSERT(is_main_thread())

static WinCritSec g_preset_cs;
#define SNDSYS_PRESET_BLOCK WinAutoLock presetLock(g_preset_cs);

ECS_REGISTER_EVENT(EventOnSoundPresetLoaded);
ECS_REGISTER_EVENT(EventOnMasterSoundPresetLoaded);
ECS_REGISTER_EVENT(EventSoundDrawDebug);

namespace dngsound
{
namespace cvars
{
static CONSOLE_BOOL_VAL("snd", debug, false);
static CONSOLE_BOOL_VAL("snd", mute, false);
} // namespace cvars

static bool g_master_preset_loaded = false;
static sndsys::str_hash_t g_master_preset_name_hash = SND_HASH("");
static constexpr float g_mute_speed = 1.5f;
static float g_mute_vol = FLT_EPSILON;

using vca_name_t = eastl::fixed_string<char, 16>;
static eastl::vector<vca_name_t> g_vca_names;
static int g_muteable_vca_index = -1;

static dag::AtomicInteger<bool> g_is_record_list_changed = false;
static dag::AtomicInteger<bool> g_is_output_list_changed = false;

static void async_record_list_changed_cb() { g_is_record_list_changed.store(true); }
static void async_output_list_changed_cb() { g_is_output_list_changed.store(true); }
static void async_device_lost_cb() { g_is_output_list_changed.store(true); }

static bool is_master_preset_loaded() { return g_master_preset_loaded; }
static void set_master_preset_loaded(bool is_loaded) { g_master_preset_loaded = is_loaded; }
static void preset_loaded_update_cb(sndsys::str_hash_t preset_name_hash, bool is_loaded)
{
  if (preset_name_hash == g_master_preset_name_hash)
  {
    SNDSYS_PRESET_BLOCK;
    set_master_preset_loaded(is_loaded);

    if (is_loaded)
    {
      sndsys::lock_channel_group("", true); // currently used to change pitch, see comment above sndsys::lock_channel_group() {}

      sndsys::dsp::apply();

      apply_config_volumes();
    }
  }

  EventOnSoundPresetLoaded evt(preset_name_hash, is_loaded);
  if (is_loaded)
    g_entity_mgr->broadcastEvent(eastl::move(evt));
  else
    g_entity_mgr->broadcastEventImmediate(eastl::move(evt));

  if (preset_name_hash == g_master_preset_name_hash)
  {
    EventOnMasterSoundPresetLoaded evt(is_loaded);
    if (is_loaded)
      g_entity_mgr->broadcastEvent(eastl::move(evt));
    else
      g_entity_mgr->broadcastEventImmediate(eastl::move(evt));
  }
}

static bool have_sound(const DataBlock &sndblk)
{
#if DAGOR_DBGLEVEL > 0
  if (dgs_get_argv("nosound"))
    return false;
#endif
  return sndblk.getBool("enabled", true);
}

static void debug_init_event_failed_cb(const char *event_path, const char *fmod_error_message, bool as_oneshot)
{
  if (!is_master_preset_loaded())
  {
    if (!as_oneshot)
      sndsys::debug_trace_err_once("init event '%s' failed (and master preset not loaded): '%s'", event_path, fmod_error_message);
  }
  else if (as_oneshot)
    sndsys::debug_trace_err_once("init event '%s' (as oneshot) failed: '%s'", event_path, fmod_error_message);
  else
    sndsys::debug_trace_err_once("init event '%s' failed: '%s'", event_path, fmod_error_message);
}

// hardcoded list of obsolete bank mods that cause terrifying, unstable issue inside the fmod resource system
// open this hell portal only if this names are completely gone
static constexpr sndsys::banks::ProhibitedBankDesc all_prohibited_bank_descs[] = {
  // {name, size, hash, mod label}
  // put same file together to hash it only once
  {"cmn_fx_impact.assets", 23694400ll, 17706345508862987374ull, "mod v6"},
  {"cmn_fx_impact.assets", 24161728ll, 6362643752110213450ull, "soundpack 3.5"},
  {"cmn_sfx_effect.assets", 6852448ll, 6120978199567167517ull, "soundpack 3.5"},
  {"ww2_weapon_antivehicle.assets", 46748416ll, 16566567175214684850ull, "soundpack 3.5"},
  {"ww2_weapon_mgun.assets", 62332896ll, 3842154586005375549ull, "mod v6"},
  {"ww2_weapon_mgun.assets", 54365728ll, 10398093140192776151ull, "soundpack 3.5"},
  {"ww2_weapon_pistol.assets", 11957152ll, 7181721626584471908ull, "mod v6"},
  {"ww2_weapon_pistol.assets", 10158592ll, 11391380321154668338ull, "soundpack 3.5"},
  {"ww2_weapon_rifle.assets", 204998080ll, 14231951712570022395ull, "soundpack 3.5"},
  {"ww2_weapon_rifle.assets", 278471552ll, 13094680411969053016ull, "mod v6"},
  {"ww2_weapon_cannon.assets", 22690336ll, 833878350069481598ull, "mod v6"},
  {"ww2_weapon_mgun_mounted.assets", 14466112ll, 7692226828054102393ull, "mod v6"},
};

#if _TARGET_PC_WIN
static bool g_com_initialized = false;
static void com_initialize()
{
  const HRESULT hres = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hres))
    g_com_initialized = true;
  else
    logerr("[dngSound] Failed to initialize COM library. Error code = %0x", hres);
}
static void com_uninitialize()
{
  if (g_com_initialized)
    CoUninitialize();
  g_com_initialized = false;
}
#else
static void com_initialize() {}
static void com_uninitialize() {}
#endif

void init()
{
  DNGSND_IS_MAIN_THREAD;
  if (sndsys::is_inited())
    return;

  DataBlock soundSettingsBlk; //(framemem_ptr());
  const DataBlock *soundBlk = ::dgs_get_settings()->getBlockByNameEx("sound");
  if (::dgs_get_settings()->paramExists("soundSettingsBlk"))
  {
    const char *soundSettingsPath = ::dgs_get_settings()->getStr("soundSettingsBlk", "");
    if (dblk::load(soundSettingsBlk, soundSettingsPath))
    {
      merge_data_block(soundSettingsBlk, *soundBlk); // also apply from argv
      soundBlk = &soundSettingsBlk;
    }
    else
    {
      logerr("[dngSound] Load sound settings '%s' failed", soundSettingsPath);
    }
  }
  const DataBlock &sndblk = *soundBlk;
  set_master_preset_loaded(false);

  sndsys::set_debug_init_event_failed_cb(&debug_init_event_failed_cb);

  const bool haveSound = have_sound(sndblk);

#if DAGOR_DBGLEVEL > 0
  if (haveSound)
  {
    if (dgs_get_argv("snddbg") || sndblk.getBool("debug", false))
      cvars::debug.set(true);
  }
#endif

  if (haveSound)
  {
    const DataBlock &vcaBlk = *sndblk.getBlockByNameEx("vca");
    g_vca_names.reserve(vcaBlk.blockCount());
    for (int i = 0; i < vcaBlk.blockCount(); ++i)
      g_vca_names.push_back(vcaBlk.getBlock(i)->getBlockName());

    g_muteable_vca_index = -1;
    const char *muteableVca = sndblk.getStr("muteableVca", "MASTER");
    if (muteableVca && *muteableVca)
    {
      const auto muteableVcaIt = eastl::find(g_vca_names.begin(), g_vca_names.end(), muteableVca);
      g_muteable_vca_index = muteableVcaIt - g_vca_names.begin();
      if (g_muteable_vca_index >= g_vca_names.size())
      {
        g_muteable_vca_index = -1;
        logerr("[dngSound] No vca '%s' in block vca{}", muteableVca);
      }
    }

    com_initialize();

    if (sndsys::init(sndblk))
    {
      sndsys::dsp::init(sndblk);

      sndsys::banks::set_preset_loaded_cb(&preset_loaded_update_cb);

      sndsys::banks::set_err_cb([](const char *sndsys_message, const char *fmod_error_message, const char *bank_path, bool is_mod) {
        if (!is_mod)
        {
          DAG_FATAL("[SNDSYS] %s \"%s\" \"%s\"", sndsys_message, bank_path, fmod_error_message);
        }
        else
        {
          debug("[SNDSYS] %s \"%s\" \"%s\"", sndsys_message, bank_path, fmod_error_message);
        }
      });

      sndsys::banks::init(sndblk, make_span_const(all_prohibited_bank_descs));

      const char *preset = sndsys::banks::get_master_preset();
      g_master_preset_name_hash = SND_HASH_SLOW(preset);

#if DAGOR_DBGLEVEL > 0
      if (sndblk.getBool("debugLoadMasterPreset", true))
        sndsys::banks::enable(preset);
#else
      sndsys::banks::enable(preset);
#endif
      sndsys::set_device_changed_async_callbacks(async_record_list_changed_cb, async_output_list_changed_cb, async_device_lost_cb);

      sndsys::CallbackType ctype;
      ctype.device_lost = true;
      ctype.device_list_changed = true;
      ctype.record_list_changed = true;
      sndsys::set_system_callbacks(ctype);
    }
  }

  sound::ActionProps::register_props_class();

  darg::init_soundsystem_ui_player();
}

void close()
{
  DNGSND_IS_MAIN_THREAD;
  set_master_preset_loaded(false);
  if (sndsys::is_inited())
  {
    sndsys::dsp::close();
    sndsys::shutdown();
  }
  com_uninitialize();
}

static inline const DataBlock &get_vol_blk() { return *::dgs_get_settings()->getBlockByNameEx("sound")->getBlockByNameEx("volume"); }
static inline void apply_volume(int vca_id, const DataBlock &blk_vol)
{
  float vol = blk_vol.getReal(g_vca_names[vca_id].c_str(), 1.f);
  if (vca_id == g_muteable_vca_index)
    vol *= ssmooth(g_mute_vol);
  sndsys::set_volume(g_vca_names[vca_id].c_str(), vol);
}

static void update_master_volume_for_app_in_background(float dt)
{
  if (g_muteable_vca_index == -1)
    return;
  const bool isMute = (!::dgs_app_active || cvars::mute.get()) && !cvars::debug.get();
  if (isMute && g_mute_vol > 0.f)
  {
    SNDSYS_PRESET_BLOCK;
    if (is_master_preset_loaded())
    {
      g_mute_vol = max(0.f, g_mute_vol - dt * g_mute_speed);
      apply_volume(g_muteable_vca_index, get_vol_blk());
    }
  }
  else if (!isMute && g_mute_vol < 1.f)
  {
    SNDSYS_PRESET_BLOCK;
    if (is_master_preset_loaded())
    {
      g_mute_vol = 1.f;
      apply_volume(g_muteable_vca_index, get_vol_blk());
    }
  }
}

void apply_config_volumes()
{
  SNDSYS_PRESET_BLOCK;
  if (!is_master_preset_loaded())
    return;
  const DataBlock &blkVol = get_vol_blk();
  for (int i = 0; i < g_vca_names.size(); ++i)
    apply_volume(i, blkVol);
}

void debug_draw()
{
  DNGSND_IS_MAIN_THREAD;
  if (cvars::debug.get())
    sndsys::debug_draw();
}

void reset_listener()
{
  DNGSND_IS_MAIN_THREAD;
  sndsys::reset_3d_listener();
}

void flush_commands()
{
  DNGSND_IS_MAIN_THREAD;
  sndsys::flush_commands();
}

static uint32_t g_update_frame_idx = 0;
// reduce update frequency to shorten FMOD command list
static constexpr int g_update_listener_interval = 15; // pow(2, N) - 1
static constexpr float g_update_listener_dist_threshold_sq = sqr(0.5f);
static constexpr float g_update_listener_dot_threshold = 0.98f;
static bool g_listener_valid = false;
static Point3 g_listener_pos = {};
static Point3 g_listener_side = {};
static Point3 g_listener_up = {};
static bool g_is_previous_user_listener_valid = false;

void update(float dt)
{
  TIME_PROFILE(dngsound_update);

  DNGSND_IS_MAIN_THREAD;

  update_master_volume_for_app_in_background(dt);

  sndsys::set_time_speed(get_timespeed());

  // compensatory update if sndsys was not updated [from start_async_game_tasks] for some long time
  // to update from loading screen or when ParallelUpdateFrameDelayed is not working
  // to prevent possible command buffer overflow or missing [ui] sound right after scene was loaded
  sndsys::lazy_update();

  if (g_is_record_list_changed.load())
  {
    g_is_record_list_changed.store(false);
    sound::sqapi::on_record_devices_list_changed();
  }

  if (g_is_output_list_changed.load())
  {
    g_is_output_list_changed.store(false);
    sound::sqapi::on_output_devices_list_changed();
  }

  ++g_update_frame_idx;
}

} // namespace dngsound

using namespace dngsound;

ECS_TAG(sound, dev)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_ON_EVENT(EventLevelLoaded)
static void dng_sound_set_cur_scene_name_es(const ecs::Event &)
{
  sndsys::debug_set_cur_scene_name(sceneload::get_current_game().sceneName.c_str());
}

ECS_TAG(sound, render, dev)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_NO_ORDER
static void dng_sound_debug_draw_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (cvars::debug.get())
    g_entity_mgr->broadcastEventImmediate(EventSoundDrawDebug());
}

// - expected correct order: -
// dngsound::update(main thread)
// sound_begin_update_es(ParallelUpdateFrameDelayed)
// game code(das)
// sound_end_update_es(ParallelUpdateFrameDelayed)

template <typename Callable>
static void dng_sound_listener_ecs_query(Callable c);

ECS_TAG(sound)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_AFTER(after_net_phys_sync) // may need some better points
static void sound_begin_update_es(const ParallelUpdateFrameDelayed &evt)
{
  TMatrix userListenerTm;
  bool userListenerValid = false;

  dng_sound_listener_ecs_query([&](ECS_REQUIRE(ecs::Tag dngSoundListener) bool dng_sound_listener_enabled, const TMatrix &transform) {
    if (dng_sound_listener_enabled)
    {
      userListenerValid = true;
      userListenerTm = transform;
    }
  });

  const TMatrix listenerTm = userListenerValid ? userListenerTm : get_cam_itm();

  const bool isListenerValid = listenerTm != TMatrix::IDENT;

  if (g_listener_valid != isListenerValid || g_is_previous_user_listener_valid != userListenerValid ||
      !(g_update_frame_idx & g_update_listener_interval) ||
      lengthSq(g_listener_pos - listenerTm.getcol(3)) > g_update_listener_dist_threshold_sq ||
      dot(g_listener_side, listenerTm.getcol(0)) < g_update_listener_dot_threshold ||
      dot(g_listener_up, listenerTm.getcol(1)) < g_update_listener_dot_threshold)
  {
    g_listener_pos = listenerTm.getcol(3);
    g_listener_side = listenerTm.getcol(0);
    g_listener_up = listenerTm.getcol(1);
    g_listener_valid = isListenerValid;

    if (isListenerValid)
    {
      if (g_is_previous_user_listener_valid != userListenerValid)
        sndsys::reset_3d_listener();
      sndsys::update_listener(evt.dt, listenerTm);
    }
    else
      sndsys::reset_3d_listener();
    g_is_previous_user_listener_valid = userListenerValid;
  }

  sndsys::begin_update(evt.dt);
}


ECS_TAG(sound)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_AFTER(sound_begin_update_es)
static void sound_end_update_es(const ParallelUpdateFrameDelayed &evt) { sndsys::end_update(evt.dt); }
