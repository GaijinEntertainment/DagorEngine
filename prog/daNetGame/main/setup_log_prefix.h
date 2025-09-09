// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC
#include "net/dedicated.h"
#include "version.h"
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>

#if _TARGET_PC_LINUX | _TARGET_APPLE
#include <unistd.h>
#include <aio.h>
#endif

static void init_early() // called from within main(), but before log system init and DagorWinMainInit
{
  if (dgs_get_argv("dumpversion")) // print version of app & exit
  {
    printf("%s\n", get_exe_version_str());
    fflush(stdout);
    _exit(0); // don't call static dtors
  }
#if _TARGET_PC_LINUX
#ifdef __SANITIZE_THREAD__ // use big idle timeout to prevent exit of AIO threads which prevents TSAN suppressions to work
  aioinit aioi{20 /* aio_threads */, 64 /* aio_num */, 0, 0, 0, 0, 600 /* aio_idle_time*/, 0};
#else
  aioinit aioi{20 /* aio_threads */, 64 /* aio_num */, 0, 0, 0, 0, 30 /* aio_idle_time*/, 0};
#endif
  aio_init(&aioi);
#endif
}

#if DAGOR_DBGLEVEL > 0
#define __DEBUG_MODERN_PREFIX dng_get_log_prefix(false)
#define __DEBUG_DYNAMIC_LOGSYS
#define __DEBUG_CLASSIC_LOGSYS_FILEPATH dng_get_log_prefix(false)
#define __DEBUG_USE_CLASSIC_LOGSYS()    (dedicated::is_dedicated() || dgs_get_argv("stdout"))
#else
#define __DEBUG_FILEPATH dng_get_log_prefix(true)
#endif

static const char *get_log_dir() { return ".game_logs"; }

static String dng_get_log_prefix(bool do_crypt)
{
  init_early(); // side effect

  String prefix;
  if (::dgs_get_argv("stdout"))
  {
    prefix = "*";
    return prefix;
  }
  else
  {
    prefix = dedicated::setup_log();
    if (!prefix.empty())
      return prefix;
  }
  prefix.printf(0, "%s/", get_log_dir());
  if (!::dgs_get_argv("devMode") && do_crypt)
    prefix += "#";
  return prefix;
}
#endif
