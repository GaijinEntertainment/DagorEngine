// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>

#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <startup/dag_winCommons.h>
#include <startup/dag_restart.h>
#include <startup/dag_addBasePathDef.h>
#include <workCycle/dag_startupModules.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_logSys.h>
#include <util/dag_string.h>


static String *logerr_capture_target = nullptr;

void set_logerr_capture(String *target) { logerr_capture_target = target; }

static void fail_test_on_unexpected_err(void *, LogMessage &msg)
{
  String s;
  if (msg.ctxFile)
    s.printf(0, "[E] %s,%d: ", msg.ctxFile, msg.ctxLine);
  else
    s.printf(0, "[E] ");
  s.avprintf(0, msg.fmt, (const DagorSafeArg *)msg.arg, msg.anum);

  if (logerr_capture_target)
  {
    *logerr_capture_target += s;
    return;
  }

  FAIL(s);
}

#define CUSTOM_UNITTEST_CODE                                             \
  static DagorSettingsBlkHolder stgBlkHolder;                            \
  dgs_init_argv(argc, argv);                                             \
  set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE)); \
  dagor_change_root_directory();                                         \
  dagor_init_base_path();                                                \
  dgs_load_settings_blk(false, "../../settings.blk");                    \
  cpujobs::init();                                                       \
  d3d::init_driver();                                                    \
  ::dagor_init_video("metronomeTests", 0, nullptr, "");                  \
  ::startup_game(RESTART_ALL);                                           \
  debug_add_log_handler(&fail_test_on_unexpected_err, nullptr, {.levelMask = LOGMASK_ERRORS});

#define CUSTOM_UNITTEST_SHUTDOWN_CODE \
  d3d::release_driver();              \
  cpujobs::term(true, 1000);

#include <unittest/mainCatch2.inc.cpp>
