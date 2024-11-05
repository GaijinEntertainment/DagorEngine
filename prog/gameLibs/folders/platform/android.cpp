// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "internal.h"
#include <stdlib.h>
#include <unistd.h>

#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>

namespace folders
{
namespace internal
{
void platform_initialize(const char *app_name) { G_UNUSED(app_name); }

String get_exe_dir() { return {}; }

String get_game_dir() { return String(dagor_android_external_path ? dagor_android_external_path : ""); }

String get_gamedata_dir()
{
  String dir;
  dir.printf(260, "%s/user", dagor_android_internal_path);
  return dir;
}

String get_temp_dir() { return String(dagor_android_internal_path); }

String get_local_appdata_dir() { return get_gamedata_dir(); }

String get_common_appdata_dir() { return get_gamedata_dir(); }

String get_downloads_dir()
{
  String gameData = get_gamedata_dir();
  String dir;
  dir.printf(0, "%s/%s", gameData, "downloads");
  return dir;
}
} // namespace internal
} // namespace folders
