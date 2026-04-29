// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>

#include <startup/dag_globalSettings.h>
#include <startup/dag_loadSettings.h>
#include <startup/dag_winCommons.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_logSys.h>
#include <util/dag_string.h>


static debug_log_callback_t original_log_callback = nullptr;
static int fail_test_on_err(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (original_log_callback)
    original_log_callback(lev_tag, fmt, arg, anum, ctx_file, ctx_line);

  if (lev_tag > LOGLEVEL_ERR)
    return 1;

  String s;
  if (ctx_file)
    s.printf(0, "[E] %s,%d: ", ctx_file, ctx_line);
  else
    s.printf(0, "[E] ");
  s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);

  FAIL(s);

  return 1;
}

#define CUSTOM_UNITTEST_CODE                                             \
  static DagorSettingsBlkHolder stgBlkHolder;                            \
  dgs_load_settings_blk();                                               \
  d3d::init_driver();                                                    \
  set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE)); \
  original_log_callback = debug_set_log_callback(&fail_test_on_err);

#include <unittest/mainCatch2.inc.cpp>
