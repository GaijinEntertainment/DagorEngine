// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../DebugLevel.h"
#include "defines.h"

#include <debug/dag_logSys.h>

// stubs for the linker
extern "C" const char *dagor_exe_build_date = "*";
extern "C" const char *dagor_exe_build_time = "*";

// Stubs, @TODO: enable settings passing
DebugLevel hlslDebugLevel = DebugLevel::NONE;
bool hlslEmbedSource = false;
bool enableBindless = false;

static int debug_log_cb_stub(int, const char *, const void *, int, const char *, int) { return 0; /* false */ }

DLL_EXPORT void init_settings(debug_log_callback_t debug_log_cb)
{
  debug_set_log_callback(debug_log_cb ? debug_log_cb : &debug_log_cb_stub);
}