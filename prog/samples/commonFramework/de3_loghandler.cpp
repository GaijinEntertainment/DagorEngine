// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_loghandler.h"

#if DAGOR_DBGLEVEL > 0

#include <debug/dag_logSys.h>
#include <debug/dag_assert.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <stdio.h>

static int dev_log_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  String s;
  if (ctx_file)
    s.printf(0, "[%c] %s,%d: ", lev_tag == LOGLEVEL_ERR ? 'E' : 'F', ctx_file, ctx_line);
  else
    s.printf(0, "[%c] ", lev_tag == LOGLEVEL_ERR ? 'E' : 'F');
  s.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);

  if (lev_tag == LOGLEVEL_ERR && ::dgs_get_argv("logerr_to_stderr"))
  {
    fprintf(stderr, "%s\n", s.str());
    fflush(stderr);
  }
  return 1;
}

void dagor_install_dev_log_handler_callback() { debug_set_log_callback(&dev_log_callback); }

#else

void dagor_install_dev_log_handler_callback() {}

#endif
