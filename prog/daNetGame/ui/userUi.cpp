// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "userUi.h"
#include "uiBindings.h"
#include "uiRender.h"
#include "main/watchdog.h"
#include "main/circuit.h"
#include "net/netBindSq.h"

#include "input/inputControls.h"
#include "input/uiInput.h"

#include "render/world/dargPanelAnchorResolve.h"

#include <daRg/dag_guiScene.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_element.h>
#include <daRg/dag_behavior.h>
#include <daRg/dag_browser.h>
#include <gui/dag_stdGuiRender.h>
#include <3d/dag_picMgr.h>

#include <gui/dag_visualLog.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_oaHashNameMap.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

#include <memory/dag_framemem.h>

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_threadPool.h>
#include <util/dag_delayedAction.h>

#include <sqrat.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/autoCleanup.h>
#include <sqModules/sqModules.h>
#include <quirrel/http/sqHttpClient.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqConsole/sqConsole.h>
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_ANDROID
#include <platform/android/android_bind.h>
#elif _TARGET_IOS
#include <platform/ios/ios_bind.h>
#endif
#include <quirrel/clientLog/clientLog.h>
#include <quirrel/base64/base64.h>
#include <quirrel/sqJwt/sqJwt.h>
#include <quirrel/sqWebBrowser/sqWebBrowser.h>
#include <quirrel/nestdb/nestdb.h>

#include "uiVideoMode.h"

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/scripts/scripts.h>
#include <ecs/scripts/dasEs.h>
#include <ecs/scripts/netBindSq.h>
#include <ecs/scripts/netsqevent.h>
#include <daECS/net/dasEvents.h>

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiJoystick.h>
#include <startup/dag_inpDevClsDrv.h>

#include "uiEvents.h"
#include "game/gameEvents.h"
#include "game/player.h"
#include "main/app.h"
#include "main/scriptDebug.h"

#include <daEditorE/editorCommon/inGameEditor.h>
#include <daEditorE/editorCommon/entityEditor.h>

#include <quirrel/sqEventBus/sqEventBus.h>
#include "camera/sceneCam.h"
#include <eventLog/errorLog.h>
#include <daInput/input_api.h>

#include <gui/dag_imgui.h>
#include <imgui/imguiInput.h>

#include <soundSystem/quirrel/sqSoundSystem.h>

#include <render/dargPanels.h>
#include "main/gameLoad.h"
#include "loadingUi.h"

SQInteger get_thread_id_func();

#define UI_ECS_EVENT ECS_REGISTER_EVENT
UI_ECS_EVENTS
#undef UI_ECS_EVENT


PULL_CONSOLE_PROC(darg_console_handler)
PULL_CONSOLE_PROC(ui_console_handler)

namespace user_ui
{

eastl::unique_ptr<darg::IGuiScene> gui_scene;


static const char *get_ui_script_fn()
{
  const char *fn = ::dgs_get_settings()->getStr("user_ui_start_script", nullptr);
  if (!fn)
    debug("user_ui_start_script:t= not set, UI VM will not be started");
  return (fn && !*fn) ? nullptr : fn;
}


static struct GuiSceneCb : public darg::IGuiSceneCallback
{
  virtual void onScriptError(darg::IGuiScene * /*scene*/, const char *err_msg) override { G_UNUSED(err_msg); }

  virtual void onDismissError() override
  {
    delayed_call([]() { reload_user_ui_script(true); });
  }

  virtual void onToggleInteractive(int iflags) override
  {
    uiinput::mask_dainput_pointers(iflags, int(uiinput::DaInputMaskLayers::UserUiPointers));

    // User UI actions
    bool active = (iflags & (IF_MOUSE | IF_KEYBOARD)) != 0;
    dainput::activate_action_set(dainput::get_action_set_handle("HUD"), active);
  }

  virtual void onShutdownScript(HSQUIRRELVM vm) override
  {
    sq::cleanup_unreg_native_api(vm);
    bindquirrel::http_client_on_vm_shutdown(vm);
  }

  virtual bool useInputConsumersCallback() override { return true; }
  virtual void onInputConsumersChange(
    dag::ConstSpan<darg::HotkeyButton> device_buttons, dag::ConstSpan<String> event_hotkeys, bool consume_text_input) override
  {
    uiinput::mask_dainput_buttons(device_buttons, consume_text_input, int(uiinput::DaInputMaskLayers::UserUiButtons));

    Tab<dainput::action_handle_t> actions(framemem_ptr());
    actions.reserve(event_hotkeys.size());
    for (const String &s : event_hotkeys)
    {
      dainput::action_handle_t a = dainput::get_action_handle(s, dainput::TYPEGRP_DIGITAL);
      if (a != dainput::BAD_ACTION_HANDLE)
        actions.push_back(a);
    }
    dainput::action_set_handle_t set = dainput::setup_action_set("UI.dynamic", actions);
    dainput::activate_action_set(set, actions.size() > 0);
  }
} gui_scene_cb;


void reload_user_ui_script(bool full_reinit)
{
  if (gui_scene)
  {
    DEBUG_CTX("Reloading User UI script");
    if (full_reinit)
      init();
    else
    {
      HSQUIRRELVM vm = gui_scene->getScriptVM();
      clear_script_console_processor(vm);
      sqeventbus::clear_on_reload(vm);
      bindquirrel::clear_workcycle_actions(vm);

      start_es_loading();
      gui_scene->reloadScript(get_ui_script_fn());
      end_es_loading();
    }

    // require scripts from UI scripts loader
    // We want that es from that sctipts to receive EventScriptReloaded event too
    g_entity_mgr->broadcastEventImmediate(CmdRequireUIScripts{});
    g_entity_mgr->broadcastEvent(EventScriptReloaded());
  }
}

static void do_bind_das(SqModules *moduleMgr)
{
  bind_dascript::bind_das_events(moduleMgr);

  // This is disabled for security reasons, but may be returned later under a permission system
  // bind_dascript::bind_das(moduleMgr);
}


void rebind_das()
{
  if (gui_scene)
    do_bind_das(gui_scene->getModuleMgr());
}


static void bind_ui_script_apis(SqModules *moduleMgr)
{
  sq::auto_bind_native_api(moduleMgr, sq::VM_USER_UI);
  net::register_dasevents(moduleMgr);
  ecs::sq::bind_net(moduleMgr);
  do_bind_das(moduleMgr);
  ui::videomode::bind_script(moduleMgr);

  bindquirrel::register_dagor_shell(moduleMgr);
  bindquirrel::sqrat_bind_dagor_math(moduleMgr);
  bindquirrel::sqrat_bind_dagor_logsys(moduleMgr);
  bindquirrel::bind_dagor_time(moduleMgr);
  bindquirrel::register_iso8601_time(moduleMgr);
  bindquirrel::register_platform_module(moduleMgr);
  bindquirrel::bind_dagor_workcycle(moduleMgr, true, "user_ui");
  bindquirrel::register_dagor_localization_module(moduleMgr);
  bindquirrel::register_random(moduleMgr);
  bindquirrel::register_hash(moduleMgr);
  bindquirrel::register_reg_exp(moduleMgr);
  bindquirrel::register_utf8(moduleMgr);
  controls::bind_script_api(moduleMgr);
  bindquirrel::sqrat_bind_datablock(moduleMgr);
  bindquirrel::bind_json_api(moduleMgr);
  bindquirrel::bind_base64_utils(moduleMgr);
  bindquirrel::bind_jwt(moduleMgr);

  // bindings below BETTER have permissions and\or whitelisting to enable them in mods.
  // But we can treat all mods as they are using it (not too dangerous, just warning needed)

  register_net_sqevent(moduleMgr);                                                         // Permissions here should be better to have
  net::bind_danetgame_net(moduleMgr);                                                      // better add permissions here
  ::ecs_register_sq_binding(moduleMgr, /*createSystems*/ true, /*createFactories*/ false); // rw components should be disabled here

  // bindings below SHOULD have permissions and\or whitelisting to enable them
  //  we alse need whitelisting for eventbus in settings.blk, or eventbus as well as nestdb shouldnt be used
  //   nestdb::bind_api(moduleMgr);//We 100% need whitelisting here, and in the same time we need it
  //   bindquirrel::bind_http_client(moduleMgr); //We need permissions, whitelisting here (or both)
  //   bindquirrel::register_dagor_clipboard(moduleMgr); //should be whitelisted to enable, or not used
  //   bindquirrel::register_dagor_fs_module(moduleMgr); //We need permissions, whitelisting or vroms only here (or everything)
  ui::bind_ui_behaviors(moduleMgr);
}


void init()
{
  TIME_PROFILE(ui_init);
  term();

  g_entity_mgr->tick(); // perform delayed user ui entity deletion

  const char *scriptFn = get_ui_script_fn();
  if (!scriptFn)
    return;

  if (!::dd_file_exist(scriptFn))
  {
    LOGERR_CTX("User UI start script %s was not found", scriptFn);
    G_ASSERTF(0, "User UI start script %s was not found", scriptFn);
    return;
  }

  gui_scene.reset(darg::create_gui_scene());
  gui_scene->setCallback(&gui_scene_cb);

  gui_scene->setSceneErrorRenderMode(darg::SEI_WARNING_ICON);

  HSQUIRRELVM vm = gui_scene->getScriptVM();
  sq_limitthreadaccess(vm, ::get_main_thread_id());
  SqModules *moduleMgr = gui_scene->getModuleMgr();

  sqeventbus::bind(moduleMgr, "ui", sqeventbus::ProcessingMode::MANUAL_PUMP); // whitelist!!

  bindquirrel::apply_compiler_options_from_game_settings(moduleMgr);

#if DAGOR_DBGLEVEL > 0
  sq_enabledebuginfo(vm, true);
  sq_enablevartrace(vm, true);
  sq_set_thread_id_function(vm, get_thread_id_func);
  dag_sq_debuggers.initDebugger(SQ_DEBUGGER_DARG_SCRIPTS, moduleMgr, "DNG daRg Scripts");
  scriptprofile::register_profiler_module(vm, gui_scene->getModuleMgr());

  const bool varTraceOn = sq_isvartracesupported();
#else
  const bool varTraceOn = false;
#endif

  Sqrat::Table(Sqrat::ConstTable(vm)).SetValue("VAR_TRACE_ENABLED", varTraceOn);

  moduleMgr->registerSystemLib();
  moduleMgr->registerDateTimeLib();
  moduleMgr->registerDebugLib();
  moduleMgr->registerIoLib();

  create_script_console_processor(moduleMgr, "user_ui.exec");

  bind_ui_script_apis(moduleMgr);

  start_es_loading();
  gui_scene->runScriptScene(scriptFn);
  end_es_loading();

  dainput::register_darg_scene(1, gui_scene.get());
}

void term()
{
  if (!gui_scene.get())
    return;

  HSQUIRRELVM vm = gui_scene->getScriptVM();

  sqeventbus::process_events(vm); // flush queue, ensure all events are processed
  sqeventbus::unbind(vm);

  sound::sqapi::release_vm(vm); // Note: this is also implicitly pulls 'bind_sound'

  shutdown_ecs_sq_script(vm);
  bindquirrel::cleanup_dagor_workcycle_module(vm);
  scriptprofile::shutdown(vm);

#if DAGOR_DBGLEVEL > 0
  dag_sq_debuggers.shutdownDebugger(SQ_DEBUGGER_DARG_SCRIPTS);
#endif

  destroy_script_console_processor(vm);
  bindquirrel::clear_logerr_interceptors(vm);

  uirender::wait_ui_render_job_done();

  dainput::unregister_darg_scene(1);

  auto guiScene = eastl::move(gui_scene);
  guiScene.reset();
}


HSQUIRRELVM get_vm() { return gui_scene ? gui_scene->getScriptVM() : nullptr; }


void before_render()
{
  if (gui_scene)
  {
    TIME_PROFILE(ui_before_render);
    sqeventbus::process_events(gui_scene->getScriptVM());
  }
}


} // namespace user_ui
