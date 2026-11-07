// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_loghandler.h"

#if DAGOR_DBGLEVEL > 0

#include <debug/dag_logSys.h>
#include <debug/dag_assert.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <stdio.h>

static void dev_log_callback(void *, LogMessage &msg)
{
  if (!::dgs_get_argv("logerr_to_stderr"))
    return;

  String s;
  if (msg.ctxFile)
    s.printf(0, "[E] %s,%d: ", msg.ctxFile, msg.ctxLine);
  else
    s.printf(0, "[E] ");
  s.avprintf(0, msg.fmt, (const DagorSafeArg *)msg.arg, msg.anum);

  fprintf(stderr, "%s\n", s.str());
  fflush(stderr);
}

void dagor_install_dev_log_handler_callback() { debug_add_log_handler(&dev_log_callback, nullptr, {.levelMask = LOGMASK_ERR}); }

#else

void dagor_install_dev_log_handler_callback() {}

#endif
