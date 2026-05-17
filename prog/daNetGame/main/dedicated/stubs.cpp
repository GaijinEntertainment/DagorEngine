// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point3.h>

#if DAGOR_DBGLEVEL > 0

void start_local_profile_server() {}

#endif

void pull_client_das() {}

namespace ballistics
{
float get_terrain_elevation(const Point3 & /*pt*/, Point3 & /*out_normal*/) { return 0.f; }
} // namespace ballistics

#if _TARGET_PC_MACOSX && !DAGOR_HOSTED_INTERNAL_SERVER // startup for dedicated server for macOS
#define __DAGOR_OVERRIDE_DEFAULT_ROOT_RELATIVE_PATH ".."
#include "main/setup_log_prefix.h"
#define __DEBUG_FILEPATH dng_get_log_prefix(false)
#include <startup/dag_mainCon.inc.cpp>
extern void DagorWinMainInit(int, bool);
extern int DagorWinMain(int, bool);
int DagorWinMain(bool debugmode)
{
  DagorWinMainInit(0, debugmode);
  return DagorWinMain(0, debugmode);
}
#endif
