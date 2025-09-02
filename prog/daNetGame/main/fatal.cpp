// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fatal.h"

#include <stdio.h>
#include <stdlib.h> // atexit/_set_purecall_handler/_set_abort_behavior()

#include <EASTL/internal/config.h>
#include <EASTL/algorithm.h>

#include <breakpad/binder.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_unicode.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_miscApi.h>
#include <squirrel.h>
#include <sqstdaux.h> // sqstd_printcallstack
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_localization.h>

#include "version.h"
#include "net/net.h" // net_stop
#include "main/settings.h"
#include "main/storeApiEvents.h"
#include <ecs/core/entityManager.h>
#include "game/gameScripts.h"
#include "ui/userUi.h"
#include "ui/overlay.h"
#include "main/main.h"

#include <statsd/statsd.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_gpuConfig.h>
#include "main/gameProjConfig.h"

#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif

#if _TARGET_XBOX
extern "C" void __ud2(void);
#pragma intrinsic(__ud2)
#endif

static void notify_user(const breakpad::CrashInfo &ci)
{
  if (ci.isManual) // Do not notify user twice
    return;

#if !_TARGET_APPLE
  if (dgs_report_fatal_error)
    dgs_report_fatal_error("FATAL ERROR", ci.expression, ci.callstack);
#endif
}

#if DAGOR_DBGLEVEL != 0
static bool mbox_user_allows_abort(const char *msg, const char *call_stack)
{
  if (dgs_execute_quiet)
    return true; // In quiet mode assume, we should always abort

  char buf[4096];
  snprintf(buf, sizeof(buf), "%s\n%s", msg, call_stack);
  buf[sizeof(buf) - 1] = 0;
  ScopeDetachAllWndComponents wndCompsGuard; // stop handling windows input events during fatal message box
  int result = os_message_box(buf, "GAME FATAL ERROR", GUI_MB_ABORT_RETRY_IGNORE | GUI_MB_ICON_ERROR | GUI_MB_NATIVE_DLG);
  debug("%s result = %d", __FUNCTION__, result);
  if (result == GUI_MB_BUTTON_2) // RETRY
    G_DEBUG_BREAK;
  return result == GUI_MB_BUTTON_1 || result == GUI_MB_FAIL;
}
#endif

static inline const char *classify_fatal_type(const char *msg, const char *file)
{
  auto strstrPred = [&](const char *s) { return strstr(msg, s) != NULL; };
  static const char *HangSignatures[] = {"Freeze detected"};
  if (eastl::any_of(eastl::begin(HangSignatures), eastl::end(HangSignatures), strstrPred))
    return "Hang";
  // We intentionally don't differentiate between video & system memory OOMs since it's kind of hard to do (line is somewhat blurry)
  // Note: OOMs also might be due to corrupted content (attempt to allocate huge nonsensical values)
  static const char *OOMSignatures[] = {
    "ot enough memory", // Both "Not enough memory to alloc..." & "zstd error -64 (Allocation error : not enough memory)"
    "OUTOFMEMORY", "8007000E",
#if _TARGET_C1 | _TARGET_C1

#endif
  };
  if (eastl::any_of(eastl::begin(OOMSignatures), eastl::end(OOMSignatures), strstrPred))
    return "OOM";
  static const char *ccSignatures[] = {"zstd error",
    "zlib error", // bin blk (BB[zZ])
    "Corrupt file", "no GRP2 label", "Can't open GameResPack", "Can't open level",
    "Error loading file" // FMOD_ERR_FILE_BAD
    "read error",
    "seek error", // LFileGeneralLoadCB
    "LoadException"};
  if (eastl::any_of(eastl::begin(ccSignatures), eastl::end(ccSignatures), strstrPred))
    return "ContentCorruption";
#if _TARGET_PC_WIN || _TARGET_XBOX
  const char *driverPath = "engine\\drv\\drv3d";
#else
  const char *driverPath = "engine/drv/drv3d";
#endif
  if (file && strstr(file, driverPath)) // Note: callstack is missing/meaningless in release so there is no point for checking it
    return "D3D";
  return "GenericFatal";
}

static void enable_breakpad()
{
  breakpad::Product p;
  clear_and_shrink(p.commandLine); // forbid restarting as it doesn't work with some anti-cheat libs
  p.name = gameproj::game_telemetry_name();
  p.version = get_exe_version_str();

  breakpad::Configuration cfg;
  cfg.userAgent = gameproj::game_telemetry_name();
  cfg.notify = notify_user;
  cfg.preprocess = [](breakpad::CrashInfo &ci) { ci.name = !ci.isManual ? "Exception" : classify_fatal_type(ci.expression, ci.file); };
  // This is called right after spawn of bpreport process (which is stalled in user message box asking about submission)
  cfg.hook = [](const breakpad::CrashInfo &ci) {
    flush_debug_file();
    if (ci.isManual && strcmp(ci.name, "ContentCorruption") == 0)
    {
      if (g_entity_mgr)
        g_entity_mgr->broadcastEventImmediate(StoreApiReportContentCorruption{});
      commit_settings([](DataBlock &blk) { blk.setInt("forcedLauncher", /*force*/ 2); });
    }
  };

  breakpad::init(eastl::move(p), eastl::move(cfg));
  if (breakpad::is_enabled())
    atexit([] { breakpad::shutdown(); });
}

static void statsd_report_fatal(const char *fatal_type)
{
  if (strcmp(fatal_type, "D3D") != 0)
    statsd::counter("fatal", 1, {{"type", fatal_type}});
  else
  {
    // Note: driver_version, error_code, etc... can't be used here due high cardinality (backend restriction)
    statsd::counter("fatal", 1,
      {{"type", fatal_type}, {"d3d_driver", d3d::get_driver_name()},
        {"gpu_vendor", d3d_get_vendor_name(d3d_get_gpu_cfg().primaryVendor)}});
  }
}

static void breakpad_dump(const char *expr, const char *call_stack, const char *file, int line)
{
  if (!breakpad::is_enabled())
    return;

  breakpad::CrashInfo info;
  info.expression = expr;
  info.callstack = call_stack;
  info.file = file;
  info.line = line;
  info.isManual = true;
  if (strstr(expr, "Freeze detected"))
    info.type = breakpad::CrashInfo::BPAD_CRASHTYPE_FREEZE;

  breakpad::dump(info);
}

void fatal::force_immediate_dump(bool freeze)
{
  if (auto cfg = breakpad::get_configuration())
  {
    // Note: don't call any hooks/cbs
    cfg->preprocess = nullptr;
    cfg->hook = nullptr;
    breakpad::CrashInfo cinfo;
    if (freeze)
      cinfo.type = breakpad::CrashInfo::BPAD_CRASHTYPE_FREEZE;
    breakpad::dump(cinfo);
  }
  else
  {
#if defined(_MSC_VER) && !defined(__clang__)
    __debugbreak();
#else
    __builtin_trap();
#endif
  }
}

inline void dump_sqvm_callstacks(const char *call_stack)
{
#if DAGOR_DBGLEVEL > 0 // This code obviously won't work if exe symbols not loaded
  if (!call_stack || !(strstr(call_stack, "SQVM") || strstr(call_stack, "Sqrat") || strstr(call_stack, "sq_call")))
    return;
  // Dump every vm since it's not trivial to detect in which vm fatal was called from.
  HSQUIRRELVM vms[] = {
    is_main_thread() ? gamescripts::get_vm() : nullptr,
    user_ui::get_vm(), // Do it even in non main thread since darg might be rendered/updated not in main thread.
    is_main_thread() ? overlay_ui::get_vm() : nullptr,
  };
  for (HSQUIRRELVM vm : vms)
  {
    if (vm && SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
    {
      const char *cs = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &cs)));
      clogmessage(_MAKE4C('SQRL'), "%s", cs);
      sq_pop(vm, 1);
    }
  }
#else
  G_UNUSED(call_stack);
#endif
}

#define PROCESS_FATAL_ON_INIT_VIDEO _TARGET_PC_WIN

#if PROCESS_FATAL_ON_INIT_VIDEO
void send_gpu_net_event(const char *event_name, GpuVendor video_vendor, const uint32_t *drv_ver);
static void on_video_error_fatal_action()
{
  if (dgs_execute_quiet)
    return;
  send_gpu_net_event("Unsupported_driver", GpuVendor::UNKNOWN, nullptr);
  const char *addressPrefix = get_localized_text("url/knowledgebase", "https://support.gaijin.net/hc/search?&query=");
  String address(1024, "%s%X", addressPrefix, 0x8111000B);
  String text(1024, "Visit <a href=\"%s\">%s</a>", address.str(), address.str());
  ScopeDetachAllWndComponents wndCompsGuard; // stop handling windows input events during fatal message box
  os_message_box(text, get_localized_text("msgbox/critical_error_header"), GUI_MB_OK | GUI_MB_ICON_ERROR | GUI_MB_FOREGROUND);
  _exit(1);
}
#endif

void dump_shader_statistics();

static bool fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  dump_sqvm_callstacks(call_stack);
  dump_shader_statistics();

#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::setCustomKey("fatal_source", String(-1, "%s:%d", file, line).c_str());
#endif

  if (::dgs_get_argv("fatals_to_stderr"))
  {
    fprintf(stderr, "FATAL ERROR:\n%s:%d:\n%s\n%s", file, line, msg, call_stack);
    fflush(stderr);
  }

#if PROCESS_FATAL_ON_INIT_VIDEO
  if (strstr(msg, "Error initializing video"))
    on_video_error_fatal_action();
#endif

  // This covers both PC (regadless of crash reporter used) & consoles
  statsd_report_fatal(classify_fatal_type(msg, file));

#if DAGOR_DBGLEVEL != 0
  bool shall_crash = mbox_user_allows_abort(msg, call_stack);
  if (shall_crash)
    net_stop();
  debug("User %sallows abort", shall_crash ? "" : "dis");
#else
  constexpr bool shall_crash = true;
  if (shall_crash)
    net_stop();
  if (dgs_report_fatal_error)
    dgs_report_fatal_error("FATAL ERROR", msg, call_stack);
#endif
  if (!shall_crash) // skip
    return false;

  breakpad_dump(msg, call_stack, file, line);

#if _TARGET_PC_LINUX | _TARGET_C1 | _TARGET_C2
  fflush(NULL);
  // At this point we want to generate core dump.
  // Intentionally not using lib functions (like abort()) to avoid depending on symbols of glibc during postmortem debugging.
#if defined(__i386__) || defined(__x86_64__)
  __asm__ volatile("int $0x03");
#else
  __builtin_trap();
#endif
#elif _TARGET_XBOX
  __ud2();
#else
  terminate_process(1);
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4702) // unreachable code
#endif
  return false; // silence the compiler
}

#if _TARGET_PC_WIN
static void __cdecl fatal_on_pure_virtual_call() { DAG_FATAL("Pure virtual function call!"); }
#endif

#if EASTL_ASSERT_ENABLED
static void handle_eastl_assertion_failure(const char *expr, void * /*ctx*/)
{
  if (dgs_assertion_handler(false, "", 0, "", expr, nullptr, nullptr, 0))
    G_DEBUG_BREAK;
}
#endif


namespace fatal
{

void init()
{
  ::dgs_fatal_handler = fatal_handler;
#if EASTL_ASSERT_ENABLED
  eastl::SetAssertionFailureFunction(&handle_eastl_assertion_failure, NULL);
#endif

#if _TARGET_PC_WIN
  if (dgs_execute_quiet)
    _set_abort_behavior(0, ~0u); // don't show any message boxes from within abort() in quiet mode
  _set_purecall_handler(fatal_on_pure_virtual_call);
#endif // _TARGET_PC_WIN

  if (!dgs_get_argv("noeh"))
    enable_breakpad();
}


void set_language() { breakpad::set_locale(language_to_locale_code(get_current_language())); }


void set_product_title(const char *name) { breakpad::set_product_title(name); }

void set_d3d_driver_data(const char *driver, const char *vendor) { breakpad::set_d3d_driver_data(driver, vendor); }

} // namespace fatal
