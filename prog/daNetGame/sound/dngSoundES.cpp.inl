// Copyright (C) Gaijin Games KFT.  All rights reserved.

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
#include <atomic>

#include "sound/dngSound.h"

#define SNDCTRL_IS_MAIN_THREAD G_ASSERT(is_main_thread())

static WinCritSec g_preset_cs;
#define SNDSYS_PRESET_BLOCK WinAutoLock presetLock(g_preset_cs);

ECS_REGISTER_EVENT(EventOnSoundPresetLoaded);
ECS_REGISTER_EVENT(EventSoundDrawDebug);

namespace dngsound
{
namespace cvars
{
static CONSOLE_BOOL_VAL("snd", draw_debug, false);
static CONSOLE_BOOL_VAL("snd", mute, false);
} // namespace cvars

static bool g_master_preset_loaded = false;
static sndsys::str_hash_t g_master_preset_name_hash = SND_HASH("");
static constexpr float g_mute_speed = 1.5f;
static float g_mute_vol = FLT_EPSILON;

using vca_name_t = eastl::fixed_string<char, 16>;
static eastl::vector<vca_name_t> g_vca_names;
static int g_muteable_vca_index = -1;

static std::atomic_bool g_is_record_list_changed = ATOMIC_VAR_INIT(false);
static std::atomic_bool g_is_output_list_changed = ATOMIC_VAR_INIT(false);

static void async_record_list_changed_cb() { g_is_record_list_changed = true; }
static void async_output_list_changed_cb() { g_is_output_list_changed = true; }
static void async_device_lost_cb() { g_is_output_list_changed = true; }

static void preset_loaded_update_cb(sndsys::str_hash_t preset_name_hash, bool is_loaded)
{
  if (preset_name_hash == g_master_preset_name_hash)
  {
    SNDSYS_PRESET_BLOCK;
    g_master_preset_loaded = is_loaded;

    if (g_master_preset_loaded)
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
}

static bool have_sound(const DataBlock &blk)
{
#if DAGOR_DBGLEVEL > 0
  if (dgs_get_argv("nosound"))
    return false;
#endif
  return blk.getBool("haveSound", true);
}

static constexpr sndsys::banks::ProhibitedBankDesc all_prohibited_bank_descs[] = {
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


void init()
{
  SNDCTRL_IS_MAIN_THREAD;
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
      logerr("load sound settings '%s' failed", soundSettingsPath);
    }
  }
  const DataBlock &blk = *soundBlk;
  g_master_preset_loaded = false;

  const bool haveSound = have_sound(blk);

#if DAGOR_DBGLEVEL > 0
  if (haveSound && dgs_get_argv("snddbg"))
    cvars::draw_debug.set(true);
#endif

  if (haveSound)
  {
    const DataBlock &vcaBlk = *blk.getBlockByNameEx("vca");
    g_vca_names.reserve(vcaBlk.blockCount());
    for (int i = 0; i < vcaBlk.blockCount(); ++i)
      g_vca_names.push_back(vcaBlk.getBlock(i)->getBlockName());

    g_muteable_vca_index = -1;
    const char *muteableVca = blk.getStr("muteableVca", "MASTER");
    if (muteableVca && *muteableVca)
    {
      const auto muteableVcaIt = eastl::find(g_vca_names.begin(), g_vca_names.end(), muteableVca);
      g_muteable_vca_index = muteableVcaIt - g_vca_names.begin();
      if (g_muteable_vca_index >= g_vca_names.size())
      {
        g_muteable_vca_index = -1;
        logerr("no vca '%s' in block vca{}", muteableVca);
      }
    }

    if (sndsys::init(blk))
    {
      sndsys::dsp::init(blk);

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

      sndsys::banks::init(blk, make_span_const(all_prohibited_bank_descs));

      const char *preset = sndsys::banks::get_master_preset();
      g_master_preset_name_hash = SND_HASH_SLOW(preset);

#if DAGOR_DBGLEVEL > 0
      if (blk.getBool("debugLoadMasterPreset", true))
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
  SNDCTRL_IS_MAIN_THREAD;
  g_master_preset_loaded = false;
  if (sndsys::is_inited())
  {
    sndsys::dsp::close();
    sndsys::shutdown();
  }
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
  const bool isMute = (!::dgs_app_active || cvars::mute.get()) && !cvars::draw_debug.get();
  if (isMute && g_mute_vol > 0.f)
  {
    SNDSYS_PRESET_BLOCK;
    if (g_master_preset_loaded)
    {
      g_mute_vol = max(0.f, g_mute_vol - dt * g_mute_speed);
      apply_volume(g_muteable_vca_index, get_vol_blk());
    }
  }
  else if (!isMute && g_mute_vol < 1.f)
  {
    SNDSYS_PRESET_BLOCK;
    if (g_master_preset_loaded)
    {
      g_mute_vol = 1.f;
      apply_volume(g_muteable_vca_index, get_vol_blk());
    }
  }
}

void apply_config_volumes()
{
  SNDSYS_PRESET_BLOCK;
  if (!g_master_preset_loaded)
    return;
  const DataBlock &blkVol = get_vol_blk();
  for (int i = 0; i < g_vca_names.size(); ++i)
    apply_volume(i, blkVol);
}

void debug_draw()
{
  SNDCTRL_IS_MAIN_THREAD;
  if (cvars::draw_debug.get())
    sndsys::debug_draw();
}

void reset_listener()
{
  SNDCTRL_IS_MAIN_THREAD;
  sndsys::reset_3d_listener();
}

void flush_commands()
{
  SNDCTRL_IS_MAIN_THREAD;
  sndsys::flush_commands();
}

void update(float dt)
{
  TIME_PROFILE(dngsound_update);

  SNDCTRL_IS_MAIN_THREAD;

  update_master_volume_for_app_in_background(dt);

  const TMatrix listenerTm = get_cam_itm();
  if (listenerTm != TMatrix::IDENT)
    sndsys::update_listener(dt, listenerTm);
  else
    sndsys::reset_3d_listener();

  sndsys::set_time_speed(get_timespeed());

  // compensatory update if sndsys was not updated [from start_async_game_tasks] for some long time
  // to update from loading screen or when ParallelUpdateFrameDelayed is not working
  // to prevent possible command buffer overflow or missing [ui] sound right after scene was loaded
  sndsys::lazy_update();

  // should move this to UI using ecs Event
  if (g_is_record_list_changed)
  {
    g_is_record_list_changed = false;
    sound::sqapi::on_record_devices_list_changed();
  }

  // should move this to UI using ecs Event
  if (g_is_output_list_changed)
  {
    g_is_output_list_changed = false;
    sound::sqapi::on_output_devices_list_changed();
  }
}

} // namespace dngsound

using namespace dngsound;

ECS_TAG(sound, render, dev)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_NO_ORDER
static void sound_draw_debug_es(const ecs::UpdateStageInfoRenderDebug &)
{
  if (cvars::draw_debug.get())
    g_entity_mgr->broadcastEventImmediate(EventSoundDrawDebug());
}

// - expected correct order: -
// dngsound::update(main thread)
// sound_begin_update_es(ParallelUpdateFrameDelayed)
// game code(das)
// sound_end_update_es(ParallelUpdateFrameDelayed)

ECS_TAG(sound)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_AFTER(after_net_phys_sync) // may need some more proper points
static void sound_begin_update_es(const ParallelUpdateFrameDelayed &evt) { sndsys::begin_update(evt.dt); }


ECS_TAG(sound)
ECS_REQUIRE(ecs::Tag msg_sink)
ECS_AFTER(sound_begin_update_es)
static void sound_end_update_es(const ParallelUpdateFrameDelayed &evt) { sndsys::end_update(evt.dt); }
