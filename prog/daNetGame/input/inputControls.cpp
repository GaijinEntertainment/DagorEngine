// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_string.h>
#include <generic/dag_initOnDemand.h>

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiVrInput.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h> // d3d::get_target_size
#include "forceFeedback/forceFeedback.h"

#include <sqrat.h>
#include <quirrel_json/quirrel_json.h>
#include <sqModules/sqModules.h>
#include <ecs/scripts/sqBindEvent.h>
#include <bindQuirrelEx/autoBind.h>

#include <ecs/core/entityManager.h>
#include <ecs/input/input.h>
#include <ecs/input/message.h>

#include <daInput/input_api.h>
#include <daInput/config_api.h>
#include <folders/folders.h>
#include "main/onlineStorageBlk.h"
#include "inputControls.h"

#include "uiInput.h"
#include "main/settings.h"
#include "main/main.h"
#include <drv/dag_vr.h>
#include <util/dag_delayedAction.h>

#if _TARGET_C1 | _TARGET_C2

#endif

namespace dainput
{
class IMap;
}

namespace controls
{
static eastl::unique_ptr<uiinput::MouseKbdHandler> ui_input_handler_val;
static String user_setup_fname;

static bool input_controls_were_reset = false;
static bool controls_were_reset() { return input_controls_were_reset; }
static void clear_controls_were_reset() { input_controls_were_reset = false; }

SensScale sens_scale;

void SensScale::writeTo(DataBlock &blk)
{
  blk.setReal("humanAiming", humanAiming);
  blk.setReal("humanTpsCam", humanTpsCam);
  blk.setReal("humanFpsCam", humanFpsCam);
  blk.setReal("vehicleCam", vehicleCam);
  blk.setReal("planeCam", planeCam);
}

void SensScale::readFrom(const DataBlock &blk)
{
  humanAiming = blk.getReal("humanAiming", 1.0);
  humanTpsCam = blk.getReal("humanTpsCam", 1.0);
  humanFpsCam = blk.getReal("humanFpsCam", 1.0);
  vehicleCam = blk.getReal("vehicleCam", 1.0);
  planeCam = blk.getReal("planeCam", 1.0);
}

class OnlineStorageControlsExchange : public online_storage::DataExchangeBLK
{
protected:
  const char *getDataTypeName() const override { return "GBT_GENERAL_MACHINE_CONTROLS"; }
  bool applyGeneralBlk(const DataBlock &container) const override;
  bool fillGeneralBlk(DataBlock &container) const override;

public:
  static DataBlock loadedConfigOnInit;
  static void loadConfig();
};
DataBlock OnlineStorageControlsExchange::loadedConfigOnInit;

void global_init()
{
  HumanInput::stg_pnt.allowEmulatedLMB = false;    // disable xbox gamepad click emulation
  HumanInput::stg_pnt.emuMouseWithTouchPad = true; // use PS4 touchpad in legacy mode for now
  ui_input_handler_val.reset(new uiinput::MouseKbdHandler);
  eastl::unique_ptr<online_storage::DataExchangeBLK> exchange(new OnlineStorageControlsExchange);
  g_entity_mgr->broadcastEventImmediate(OnlineStorageAddExchangeEvent{&exchange});
}

void global_destroy()
{
  ui_input_handler_val.reset();
  force_feedback::rumble::shutdown();
  ecs::term_hid_drivers();
  dainput::term_user_config();
}

void init_drivers()
{
  dgs_inpdev_allow_rawinput = ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("enableRawInput", true);
  dgs_inpdev_disable_acessibility_shortcuts =
    ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("disableAccessibilityShortcuts", true);

  HumanInput::stg_joy.disableVirtualControls = true;
  int pollThreadDelayMsec = ::dgs_get_settings()->getBlockByNameEx("debug")->getInt("pollThreadDelayMsec", 8); // Legacy
  const DataBlock *inputBlk = ::dgs_get_settings()->getBlockByNameEx("input");
  pollThreadDelayMsec = inputBlk->getInt("pollThreadDelayMsec", pollThreadDelayMsec);
  int hidDriversInit = inputBlk->getInt("hidDriversInit", ecs::InitDeviceType::Default);
  if (hidDriversInit <= 0)
    hidDriversInit = int(ecs::InitDeviceType::All) + hidDriversInit;
  execute_delayed_action_on_main_thread(make_delayed_action([&]() { ecs::init_hid_drivers(pollThreadDelayMsec, hidDriversInit); }));
  if (global_cls_drv_pnt)
  {
    // fixme: should be window
    int w, h;
    d3d::get_screen_size(w, h);
    global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, w, h);
  }
  dainput::reapply_stick_dead_zones();
}

void destroy() { ecs::term_input(); }

static bool load_config(const char *game)
{
#if _TARGET_PC
  user_setup_fname = String(260, "user/%s.setup.blk", game);
#else
#if _TARGET_C1 | _TARGET_C2


#else
  user_setup_fname = String(260, "%s/%s.setup.blk", folders::get_gamedata_dir(), game);
#endif
#endif

  debug("InputControls: Loading input config from %s", user_setup_fname);
  DataBlock cfg;
  if (dd_file_exists(user_setup_fname))
    cfg.load(user_setup_fname);
  dainput::load_user_config(cfg);
  sens_scale.readFrom(dainput::get_user_props());
  return dainput::get_actions_binding_columns() > 0;
}

static void save_config()
{
  debug("InputControls: save input config");

  OnlineStorageControlsExchange::loadedConfigOnInit.clearData();
  sens_scale.writeTo(dainput::get_user_props());
  dainput::save_user_config(OnlineStorageControlsExchange::loadedConfigOnInit);

  bool save_success = false;
  g_entity_mgr->broadcastEventImmediate(SaveToOnlineStorageEvent{true, &save_success});
  if (!save_success)
  {
    debug("InputControls: save input config by legacy way");
    G_ASSERTF(!user_setup_fname.empty(), "User config path not specified", 0);
#if _TARGET_C1 | _TARGET_C2

#endif

    DataBlock cfg;
    sens_scale.writeTo(dainput::get_user_props());
    dainput::save_user_config(cfg);
    dd_mkpath(user_setup_fname);
    cfg.saveToTextFile(user_setup_fname);
  }
}


static void reset_to_default()
{
  dainput::reset_user_config_to_currest_preset();
  sens_scale.readFrom(dainput::get_user_props());
}

static void restore_saved_config()
{
  if (!OnlineStorageControlsExchange::loadedConfigOnInit.isEmpty())
  {
    debug("InputControls: Restore saved input config from online storage");
    OnlineStorageControlsExchange::loadConfig();
    return;
  }

  debug("InputControls: Restore saved input config from legacy file");
  load_config(get_game_name());
}


bool OnlineStorageControlsExchange::applyGeneralBlk(const DataBlock &container) const
{
  if (container.isEmpty() && ::dgs_get_settings()->getInt("launchCnt", 0) > 1)
    input_controls_were_reset = true;

  loadedConfigOnInit = container;

  // compatibility for previous layout
  if (!loadedConfigOnInit.paramExists("preset"))
    loadedConfigOnInit.setStr("preset", dainput::get_default_preset_prefix());
  if (const DataBlock *b2 = container.getBlockByName("sens_scale"))
  {
    loadedConfigOnInit.removeBlock("sens_scale");
    loadedConfigOnInit.addNewBlock(b2, "props");
  }

  loadConfig();
  return true;
}
bool OnlineStorageControlsExchange::fillGeneralBlk(DataBlock &container) const
{
  container = loadedConfigOnInit;
  return true;
}

void OnlineStorageControlsExchange::loadConfig()
{
  dainput::load_user_config(loadedConfigOnInit);
  sens_scale.readFrom(dainput::get_user_props());
}


void init_control(const char *game, const char *user_game_mode_input_cfg_fn)
{
  String gameControlsPath(0, "content/%s/config/controlsConfig.blk", game);
  String platformControlsPath(0, "content/%s/config/controlsConfig~%s.blk", game, get_platform_string_id());
  if (dd_file_exists(platformControlsPath))
    gameControlsPath = platformControlsPath;
  else if (!dd_file_exists(gameControlsPath))
    gameControlsPath = "config/controlsConfig.blk";
  if (!dd_file_exists(gameControlsPath))
  {
    logwarn("input controls config not found: content/%s/config/controlsConfig.blk or %s or %s", game, platformControlsPath,
      gameControlsPath);
    return;
  }
  debug("reading input config: %s", gameControlsPath);
  DataBlock gameControlsBlk(gameControlsPath);

  dainput::set_invariant_preset_complementary_config(DataBlock::emptyBlock);
  if (user_game_mode_input_cfg_fn)
  {
    DataBlock add_cfg;
    if (dblk::load(add_cfg, user_game_mode_input_cfg_fn, dblk::ReadFlag::ROBUST))
    {
      dblk::iterate_child_blocks(*add_cfg.getBlockByNameEx("actionSets"),
        [&dest = *gameControlsBlk.addBlock("actionSets")](const DataBlock &b) { dest.addNewBlock(&b); });

      dblk::iterate_child_blocks(*add_cfg.getBlockByNameEx("actionSetsOrder"),
        [&dest = *gameControlsBlk.addBlock("actionSetsOrder")](const DataBlock &b) { dest.addNewBlock(&b); });

      dainput::set_invariant_preset_complementary_config(*add_cfg.getBlockByNameEx("defaultPreset"));
    }
  }

  if (dgs_get_settings()->getBlockByNameEx("gameplay")->getBool("enableVR", false))
  {
    debug("input: registering VR=%p to daInput", VRDevice::getInput());
    dainput::register_dev5_vr(VRDevice::getInput());
  }
  else
    dainput::register_dev5_vr(nullptr);
  ecs::init_input(gameControlsBlk);
  dainput::dump_action_sets();

  String gameDefaultPresetPrefix = String(0, "content/%s/config/%s.default", game, game);

  if (dd_file_exists(String(0, "%s.c0.preset.blk", gameDefaultPresetPrefix)))
    dainput::set_default_preset_prefix(gameDefaultPresetPrefix);
  else
    dainput::set_default_preset_prefix("config/default");

  String platformSpecificPresetPrefix = String(0, "%s~%s", dainput::get_default_preset_prefix(), get_platform_string_id());

  if (dd_file_exists(String(0, "%s.c0.preset.blk", platformSpecificPresetPrefix)))
    dainput::set_default_preset_prefix(String(0, "%s", platformSpecificPresetPrefix));

  const DataBlock *inputBlk = ::dgs_get_settings()->getBlockByNameEx("input");
  bool enableGyroByDefault = inputBlk->getBool("enableGyroByDefault", false);

  if (enableGyroByDefault)
  {
    String gyroPresetPrefix = String(0, "content/%s/config/%s.gyro~%s", game, game, get_platform_string_id());
    if (dd_file_exists(String(0, "%s.c0.preset.blk", gyroPresetPrefix)))
      dainput::set_default_preset_prefix(String(0, "%s", gyroPresetPrefix));
  }

  debug("default input bindings preset: %s, %d columns", dainput::get_default_preset_prefix(),
    dainput::get_default_preset_column_count());

  if (OnlineStorageControlsExchange::loadedConfigOnInit.isEmpty())
  {
    debug("InputControls: Loading input config from legacy file");
    load_config(game);
  }
  else
  {
    debug("InputControls: Loading input config from online storage");
    OnlineStorageControlsExchange::loadConfig();
  }

  dainput::activate_action_set(dainput::get_action_set_handle("BackgroundHUD"));
  dainput::activate_action_set(dainput::get_action_set_handle("Globals"));
  dainput::activate_action_set(dainput::get_action_set_handle("HUDBase"));
  dainput::activate_action_set(dainput::get_action_set_handle("Human"));
  // dainput::dump_action_bindings();
}


void process_input(double /*dt*/)
{
  TIME_PROFILE(controls_process_input);
  if (::global_cls_drv_joy)
  {
#if !(_TARGET_C1 | _TARGET_C2) // createPS4CompositeJoystickClassDriver() requires manual refreshDeviceList
    if (::global_cls_drv_joy != ::global_cls_composite_drv_joy)
#endif
    {
      if (::global_cls_drv_joy->isDeviceConfigChanged())
      {
        WinAutoLock joyGuard(global_cls_drv_update_cs);
        ::global_cls_drv_joy->refreshDeviceList();
        dainput::register_dev3_gpad(global_cls_drv_joy->getDevice(0));
        dainput::reapply_stick_dead_zones();

        for (int i = 0; i < ::global_cls_drv_joy->getDeviceCount(); i++)
        {
          if (HumanInput::IGenJoystick *joy = ::global_cls_drv_joy->getDevice(i))
          {
            joy->enableSensorsTiltCorrection(true);
            joy->reportGyroSensorsAngDelta(true);
          }
        }
      }
    }
  }

  dainput::process_actions_tick();
}


void bind_script_api(SqModules *moduleMgr)
{
  dainput::bind_sq_api(moduleMgr);

  ///@class control/SensScale
  Sqrat::Class<SensScale> sqSensScale(moduleMgr->getVM(), "SensScale");
  sqSensScale //
    .Var("humanAiming", &SensScale::humanAiming)
    .Var("humanTpsCam", &SensScale::humanTpsCam)
    .Var("humanFpsCam", &SensScale::humanFpsCam)
    .Var("vehicleCam", &SensScale::vehicleCam)
    .Var("planeCam", &SensScale::planeCam)
    /**/;

  ///@module control
  Sqrat::Table controlTable(moduleMgr->getVM());
  controlTable //
    .Func("save_config", save_config)
    .Func("reset_to_default", reset_to_default)
    .Func("restore_saved_config", restore_saved_config)
    .Func("controls_were_reset", controls_were_reset)
    .Func("clear_controls_were_reset", clear_controls_were_reset)
    .Func("get_sens_scale", [] { return &sens_scale; })
    /**/;
  moduleMgr->addNativeModule("control", controlTable);
}
} // namespace controls

SQ_DEF_AUTO_BINDING_MODULE(bind_dainput2_events, "dainput2_events")
{
  return ecs::sq::EventsBind<
#define DAINPUT_ECS_EVENT(x, ...) x,
    DAINPUT_ECS_EVENTS
#undef DAINPUT_ECS_EVENT
      ecs::sq::term_event_t>::bindall(vm);
}
