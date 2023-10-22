#include "scriptBindings.h"

#include <sqModules/sqModules.h>

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <bindQuirrelEx/autoBind.h>
#include <quirrel/http/sqHttpClient.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/base64/base64.h>
#if HAS_MATCHING_MODULE
#include <quirrel/matchingModule/matchingModule.h>
#endif
#include <quirrel/nestdb/nestdb.h>
#include <quirrel/udp/udp.h>

#include <util/dag_delayedAction.h>

#include "vr.h"
#include "main.h"
#include "screenshotMetaInfoLoader.h"
#include "gamelib/sound.h"
#include "gamelib/input.h"


extern void reload_scripts(bool full_reinit);

static auto delayed_reload_scripts = make_delayed_func(reload_scripts);


void bind_dargbox_script_api(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();

  module_mgr->registerSystemLib();
  module_mgr->registerIoLib();
  module_mgr->registerDateTimeLib();

  bindquirrel::register_dagor_localization_module(module_mgr);
  bindquirrel::register_dagor_shell(module_mgr);
  bindquirrel::bind_dagor_workcycle(module_mgr, true);
  bindquirrel::sqrat_bind_dagor_logsys(module_mgr);
  bindquirrel::sqrat_bind_dagor_math(module_mgr);
  bindquirrel::sqrat_bind_datablock(module_mgr);
  bindquirrel::register_random(module_mgr);
  bindquirrel::register_hash(module_mgr);
  bindquirrel::bind_http_client(module_mgr);
  bindquirrel::bind_base64_utils(module_mgr);
  bindquirrel::register_dagor_fs_module(module_mgr);
  bindquirrel::bind_dagor_time(module_mgr);
  bindquirrel::register_iso8601_time(module_mgr);
  bindquirrel::register_dagor_system(module_mgr);
  bindquirrel::register_dagor_clipboard(module_mgr);
  bindquirrel::bind_json_api(module_mgr);
  bindquirrel::register_reg_exp(module_mgr);
  bindquirrel::register_utf8(module_mgr);
  bindquirrel::register_platform_module(module_mgr);
#if HAS_MATCHING_MODULE
  bindquirrel::matching_module::bind(module_mgr, get_io_events_poll());
#endif
  bindquirrel::udp::bind(module_mgr);

  sqeventbus::bind(module_mgr, "ui", sqeventbus::ProcessingMode::IMMEDIATE);
  gamelib::sound::bind_script(module_mgr);
  gamelib::input::bind_script(module_mgr);

  nestdb::init();
  nestdb::bind_api(module_mgr);

  sq::auto_bind_native_api(module_mgr, sq::VM_UI);

  Sqrat::Table sqDargbox(vm);
  sqDargbox.Func("reload_scripts", [](bool full_reinit) { delayed_reload_scripts(full_reinit); });
  sqDargbox.Func("get_meta_info_from_screenshot", get_meta_info_from_screenshot);
  module_mgr->addNativeModule("dargbox", sqDargbox);

  Sqrat::Table sqVR(vm);
  sqVR.Func("is_in_vr", []() { return vr::is_configured(); });
  module_mgr->addNativeModule("daRg.vr", sqVR);
}

void unbind_dargbox_script_api(HSQUIRRELVM vm)
{
#if HAS_MATCHING_MODULE
  bindquirrel::matching_module::cleanup();
#endif
  bindquirrel::http_client_wait_active_requests();
  bindquirrel::http_client_on_vm_shutdown(vm);
  bindquirrel::cleanup_dagor_workcycle_module(vm);
  sqeventbus::unbind(vm);

  nestdb::shutdown();
}
