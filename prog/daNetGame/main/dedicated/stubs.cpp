// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0

void start_local_profile_server() {}

#endif

void pull_client_das() {}

#if _TARGET_PC_MACOSX // startup for dedicated server for macOS
#define __DEBUG_FILEPATH                            dng_get_log_prefix()
#define __DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH ".."

#include "net/dedicated.h"
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>

static String dng_get_log_prefix()
{
  if (::dgs_get_argv("stdout"))
    return String("*");
  String prefix(dedicated::setup_log());
  if (!prefix.empty())
    return prefix;
  return String(".log/");
}

#include <startup/dag_mainCon.inc.cpp>
extern void DagorWinMainInit(int, bool);
extern int DagorWinMain(int, bool);
int DagorWinMain(bool debugmode)
{
  DagorWinMainInit(0, debugmode);
  return DagorWinMain(0, debugmode);
}
#endif
