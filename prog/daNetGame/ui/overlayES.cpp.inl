// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "camera/sceneCam.h"
#include "overlay.h"
#include "input/inputControls.h"
#include "input/uiInput.h"
#include "main/app.h"
#include "main/scriptDebug.h"
#include "main/vromfs.h"
#include "main/appProfile.h"
#include "main/main.h"
#include "main/settings.h"
#include <quirrel/sqSysInfo/sqSysInfo.h>
#include <quirrel/yupfile_parse/yupfile_parse.h>
#include "main/watchdog.h"
#include "net/netBindSq.h"
#include "net/authEvents.h"
#include "ui/uiBindings.h"
#include "ui/loadingUi.h"
#include "game/dasEvents.h"
#include "game/gameEvents.h"

#include <daRg/dag_events.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_browser.h>
#include <daECS/net/msgSink.h>
#include <daECS/net/dasEvents.h>
#include <ecs/core/entityManager.h>
#include <ecs/scripts/scripts.h>
#include <ecs/scripts/netBindSq.h>
#include <ecs/scripts/dasEs.h>
#include <ecs/scripts/netsqevent.h>
#include <gui/dag_stdGuiRender.h>
#include <ioSys/dag_dataBlock.h>
#include <quirrel/sqConsole/sqConsole.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_startupTex.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_delayedAction.h>
#include <EASTL/unique_ptr.h>
#include <main/circuit.h>
#include <forceFeedback/forceFeedback.h>
#include <sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include <quirrel/bindQuirrelEx/autoCleanup.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <quirrel/http/sqHttpClient.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqJwt/sqJwt.h>
#include <quirrel/clientLog/clientLog.h>
#include <quirrel/sqForceFeedback/sqForceFeedback.h>
#include <quirrel/sqStatsd/sqStatsd.h>
#include <quirrel/base64/base64.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqWebBrowser/sqWebBrowser.h>
#include <quirrel/nestdb/nestdb.h>
#include <ui/uiVideoMode.h>
#include <eventLog/errorLog.h>
#if _TARGET_C1 | _TARGET_C2


#elif _TARGET_GDK
#include <quirrel/gdk/main.h>
#endif
#include <quirrel/sqDataCache/datacache.h>
#if _TARGET_C3

#elif _TARGET_ANDROID
#include <platform/android/android_bind.h>
#elif _TARGET_IOS
#include <platform/ios/ios_bind.h>
#endif
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_autoFuncProf.h>
#include <fpsProfiler/dag_perfRating.h>
#include "main/gameLoad.h"

#include <soundSystem/quirrel/sqSoundSystem.h>

#include <daEditorE/editorCommon/inGameEditor.h>

SQInteger get_thread_id_func();

ECS_REGISTER_EVENT(EventScriptUiInitNetworkServices);
ECS_REGISTER_EVENT(EventScriptUiTermNetworkServices);
ECS_REGISTER_EVENT(EventScriptUiUpdate)
ECS_REGISTER_EVENT(EventScriptUiBeforeEventbusShutdown, bool /*quit*/)
ECS_REGISTER_EVENT(EventScriptUiShutdown, bool /*quit*/)

namespace overlay_ui
{

static eastl::unique_ptr<darg::IGuiScene> dargScene;


static const char *get_overlay_ui_script_fn()
{
  const char *fn = ::dgs_get_settings()->getBlockByNameEx("ui")->getStr("overlay_ui_script", nullptr);
  if (!fn)
    logerr("ui{ overlay_ui_script:t= not set, overlay_ui VM will not be started");
  return (fn && !*fn) ? nullptr : fn;
}

static struct GuiSceneCb : public darg::IGuiSceneCallback
{
  virtual void onScriptError(darg::IGuiScene * /*scene*/, const char *err_msg) override
  {
    G_UNUSED(err_msg);
#if DAGOR_DBGLEVEL == 0
    event_log::ErrorLogSendParams params;
    params.attach_game_log = true;
    params.meta["vm"] = "overlay_ui";
    params.collection = sceneload::is_user_game_mod() ? "mods_client_errorlog" : "sqassert";
    event_log::send_error_log(err_msg, params);
#endif
  }

  virtual void onDismissError() override {}

  virtual void onShutdownScript(HSQUIRRELVM vm) override
  {
    sq::cleanup_unreg_native_api(vm);
    bindquirrel::http_client_on_vm_shutdown(vm);
  }

  virtual void onToggleInteractive(int iflags)
  {
    uiinput::mask_dainput_pointers(iflags, int(uiinput::DaInputMaskLayers::OverlayUiPointers));

    // Need the code below to allow implementing User UI scripts in Overlay UI VM
    // Assuming there are no two scenes using "HUD" action set at the same time
    // This function is essentially the same as onToggleInteractive in HUD VM
    bool active = (iflags & (IF_MOUSE | IF_KEYBOARD)) != 0;
    dainput::action_set_handle_t s = dainput::get_action_set_handle("HUD");
    if (s != dainput::BAD_ACTION_SET_HANDLE)
      dainput::activate_action_set(s, active);
  }

  virtual bool useInputConsumersCallback() override { return true; }

  virtual void onInputConsumersChange(
    dag::ConstSpan<darg::HotkeyButton> buttons, dag::ConstSpan<String> event_hotkeys, bool consume_text_input) override
  {
    bool isDaInputInited = dainput::get_action_sets_count() > 0;
    if (!isDaInputInited) // handle UI scene cleanup after dainput was shut down
      return;

    uiinput::mask_dainput_buttons(buttons, consume_text_input, int(uiinput::DaInputMaskLayers::OverlayUiButtons));

    // Enable processing of events specified in UI scene layout
    Tab<dainput::action_handle_t> actions(framemem_ptr());
    actions.reserve(event_hotkeys.size());
    for (const String &s : event_hotkeys)
    {
      dainput::action_handle_t a = dainput::get_action_handle(s, dainput::TYPEGRP_DIGITAL);
      if (a != dainput::BAD_ACTION_HANDLE)
        actions.push_back(a);
    }

    dainput::action_set_handle_t set = dainput::setup_action_set("UI.overlay.dynamic", actions);
    dainput::activate_action_set(set, actions.size() > 0);
  }
} gui_scene_cb;


static void bind_overlay_ui_script_apis(SqModules *moduleMgr, HSQUIRRELVM vm)
{
#if DAGOR_DBGLEVEL > 0
  sq_enabledebuginfo(vm, true);
  sq_enablevartrace(vm, true);
  sq_set_thread_id_function(vm, get_thread_id_func);
  dag_sq_debuggers.initDebugger(SQ_DEBUGGER_OVERLAY_UI_SCRIPTS, moduleMgr, "Overlay UI Scripts");
  scriptprofile::register_profiler_module(vm, moduleMgr);
  const bool varTraceOn = sq_isvartracesupported();
#else
  const bool varTraceOn = false;
#endif
  Sqrat::Table(Sqrat::ConstTable(vm)).SetValue("VAR_TRACE_ENABLED", varTraceOn);

  sqeventbus::bind(moduleMgr, "overlay_ui", sqeventbus::ProcessingMode::MANUAL_PUMP);

  moduleMgr->registerIoLib();
  moduleMgr->registerSystemLib();
  moduleMgr->registerDateTimeLib();
  moduleMgr->registerDebugLib();

  bindquirrel::sqrat_bind_dagor_math(moduleMgr);
  bindquirrel::sqrat_bind_dagor_logsys(moduleMgr);
  bindquirrel::sqrat_bind_datablock(moduleMgr);
  bindquirrel::bind_dagor_time(moduleMgr);
  bindquirrel::register_iso8601_time(moduleMgr);
  bindquirrel::register_platform_module(moduleMgr);
  bindquirrel::bind_dagor_workcycle(moduleMgr, /*auto_update*/ false, "overlay_ui");
  bindquirrel::register_dagor_system(moduleMgr);
  bindquirrel::register_dagor_shell(moduleMgr);
  bindquirrel::register_dagor_clipboard(moduleMgr);
  bindquirrel::bind_http_client(moduleMgr);
  bindquirrel::bind_json_api(moduleMgr);
  bindquirrel::bind_statsd(moduleMgr);

  watchdog::bind_sq(moduleMgr);

#if _TARGET_GDK
  bindquirrel::gdk::bind_sq(moduleMgr);
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_C3

#elif _TARGET_ANDROID
  android::bind_sq(moduleMgr);
#elif _TARGET_IOS | _TARGET_TVOS
  ios_sq::bind_sq(moduleMgr);
#endif

  bindquirrel::register_reg_exp(moduleMgr);
  bindquirrel::register_utf8(moduleMgr);
  bindquirrel::register_dagor_localization_module(moduleMgr);
  bindquirrel::register_dagor_fs_module(moduleMgr);
  bindquirrel::register_random(moduleMgr);
  bindquirrel::register_hash(moduleMgr);
  bindquirrel::bind_jwt(moduleMgr);
  controls::bind_script_api(moduleMgr);
  bindquirrel::bind_webbrowser(moduleMgr);

  bindquirrel::bind_sysinfo(moduleMgr);

  bindquirrel::register_yupfile_module(moduleMgr);

#if _TARGET_C1 | _TARGET_C2

#endif
  clientlog::bind_script(moduleMgr, "overlay_ui");
  sq::auto_bind_native_api(moduleMgr, sq::VM_INTERNAL_UI);
  register_net_sqevent(moduleMgr);
  net::register_dasevents(moduleMgr);
  ecs::sq::bind_net(moduleMgr);
  net::bind_danetgame_net(moduleMgr);
  bind_dascript::bind_das_events(moduleMgr);
  bind_dascript::bind_das(moduleMgr);
  darg::bind_browser_behavior(moduleMgr);
  ui::videomode::bind_script(moduleMgr);
  ui::bind_ui_behaviors(moduleMgr);
  bindquirrel::bind_base64_utils(moduleMgr);

  nestdb::bind_api(moduleMgr);

  loading_ui::bind(moduleMgr);

  ::register_editor_script(moduleMgr);

  bindquirrel::bind_datacache(moduleMgr);
}


void rebind_das()
{
  if (dargScene)
  {
    SqModules *moduleMgr = dargScene->getModuleMgr();
    bind_dascript::bind_das_events(moduleMgr);
    bind_dascript::bind_das(moduleMgr);
  }
}


void init_network_services()
{
  fps_profile::initPerfProfile();

#if _TARGET_GDK
  bindquirrel::gdk::init();
#endif
  g_entity_mgr->broadcastEventImmediate(EventScriptUiInitNetworkServices());
}

void init_ui()
{
  TIME_PROFILE(overlay_ui_init_ui);

  const char *script_fn = get_overlay_ui_script_fn();
  if (!script_fn)
    return;

  dargScene.reset(darg::create_gui_scene());
  dargScene->setCallback(&gui_scene_cb);
  dargScene->setSceneErrorRenderMode(darg::SEI_WARNING_ICON);

  HSQUIRRELVM vm = dargScene->getScriptVM();
  SqModules *moduleMgr = dargScene->getModuleMgr();

  G_ASSERT(::get_current_thread_id() == ::get_main_thread_id());
  sq_limitthreadaccess(vm, ::get_current_thread_id());

  bindquirrel::apply_compiler_options_from_game_settings(moduleMgr);

  ecs_register_sq_binding(moduleMgr, true, true);

  bind_overlay_ui_script_apis(moduleMgr, vm);

  create_script_console_processor(moduleMgr, "overlay_ui.exec");

  debug("running scripted scene %s", script_fn);
  start_es_loading();
  dargScene->runScriptScene(script_fn);
  end_es_loading();
  debug("run scripted scene. done");

  ecs::EntityId curCam = get_cur_cam_entity();
  if (g_entity_mgr->has(curCam, ECS_HASH("camera__input_enabled")))
    g_entity_mgr->set(curCam, ECS_HASH("camera__input_enabled"), false);

  dainput::register_darg_scene(0, dargScene.get());

  g_entity_mgr->broadcastEvent(EventScriptReloaded());
}

ECS_AFTER(on_gameapp_started_es)
static inline void overlay_ui_init_on_appstart_es_event_handler(const EventOnGameAppStarted &)
{
  delayed_call([]() // DA to execute this after actual scene load
    {
      bool doconnect = false;
#if DAGOR_DBGLEVEL > 0
      if (dgs_get_argv("connect")) // do we really need it?
        doconnect = true;
#endif
      if (!(app_profile::get().disableRemoteNetServices || app_profile::get().devMode || doconnect))
        overlay_ui::init_network_services();
      overlay_ui::init_ui();
    });
}

void shutdown_ui(bool quit)
{
  if (!dargScene)
    return;

  g_entity_mgr->broadcastEventImmediate(EventScriptUiBeforeEventbusShutdown{quit});

  HSQUIRRELVM vm = dargScene->getScriptVM();

  sqeventbus::process_events(vm);                      // flush queue, ensure all events are processed
  dargScene->callScriptHandlers(/*is_shutdown*/ true); // Ditto (before ecs vm shutdown)
  sqeventbus::unbind(vm);

  destroy_script_console_processor(vm);

  bindquirrel::cleanup_dagor_workcycle_module(vm);
  bindquirrel::clear_logerr_interceptors(vm);
  shutdown_ecs_sq_script(vm);
#if _TARGET_C1 | _TARGET_C2

#endif
  scriptprofile::shutdown(vm);

#if _TARGET_C3

#endif
  g_entity_mgr->broadcastEventImmediate(EventScriptUiShutdown{quit});

  sound::sqapi::release_vm(vm); // Note: this is also implicitly pulls 'bind_sound'

#if DAGOR_DBGLEVEL > 0
  dag_sq_debuggers.shutdownDebugger(SQ_DEBUGGER_OVERLAY_UI_SCRIPTS);
#endif
  // cleanup scene after all elements referncing inner VM are processed

  dainput::unregister_darg_scene(0);

  dargScene.reset();

  if (quit)
    bindquirrel::shutdown_datacache();
}


void shutdown_network_services()
{
  if (app_profile::get().disableRemoteNetServices)
    return;
  g_entity_mgr->broadcastEventImmediate(EventScriptUiTermNetworkServices());
#if _TARGET_GDK
  bindquirrel::gdk::shutdown();
#endif
}

void update()
{
  TIME_PROFILE(overlay_ui_update);

  g_entity_mgr->broadcastEventImmediate(EventScriptUiUpdate());

  if (dargScene)
  {
    {
      TIME_PROFILE(sq_timers);
      bindquirrel::dagor_workcycle_frame_update(dargScene->getScriptVM());
    }

    sqeventbus::process_events(dargScene->getScriptVM());
  }
}

darg::IGuiScene *gui_scene() { return dargScene.get(); }

SQVM *get_vm() { return dargScene ? dargScene->getScriptVM() : nullptr; }


static void do_reload_scripts(bool full_reinit)
{
  if (dargScene)
  {
    AutoFuncProfT<AFP_MSEC> _prof(
      full_reinit ? "[RELOAD] overlay_ui::do_reload_scripts(full) %d msec" : "[RELOAD] overlay_ui::do_reload_scripts(hot) %d msec");

    debug("reloading overlay_ui script");
    if (full_reinit)
    {
      debug("hard reload begin ==>");
      shutdown_ui(false);
      init_ui();
      debug("<== hard reload end");
    }
    else
    {
      HSQUIRRELVM vm = dargScene->getScriptVM();
      g_entity_mgr->broadcastEvent(EventScriptBeforeReload());
      start_es_loading();
      clear_script_console_processor(vm);
      sqeventbus::clear_on_reload(vm);
      bindquirrel::clear_workcycle_actions(vm);
      dargScene->reloadScript(get_overlay_ui_script_fn());
      end_es_loading();
      debug("done");

      g_entity_mgr->broadcastEvent(EventScriptReloaded());
    }
  }
}

static updater::Version current_overlay_ui_version;

void set_overlay_ui_version(updater::Version version) { current_overlay_ui_version = version; }

updater::Version get_overlay_ui_version()
{
  if (!current_overlay_ui_version)
    current_overlay_ui_version = get_game_build_version();

  return current_overlay_ui_version;
}

void reload_scripts(bool full_reinit)
{
  const updater::Version wasVersion = get_overlay_ui_version();
  const updater::Version curVersion = max(wasVersion, get_updated_game_version());
  set_overlay_ui_version(curVersion);
  debug("Reload overlay_ui: %s -> %s", wasVersion.to_string(), curVersion.to_string());

  do_reload_scripts(full_reinit);
}


} // namespace overlay_ui
