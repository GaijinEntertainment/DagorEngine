// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "internal.h"

#include <osApiWrappers/dag_direct.h>
#include <stdlib.h>
#include <unistd.h>

namespace folders
{
namespace internal
{
static String game_name;

void platform_initialize(const char *app_name) { game_name = app_name; }

String get_exe_dir()
{
  String dir;
  dir.resize(MAX_PATH);
  ssize_t len = readlink("/proc/self/exe", dir.data(), MAX_PATH);
  if (len == MAX_PATH) // The path was most-likely truncated
  {
    dir.resize(MAX_PATH * 2);
    len = readlink("/proc/self/exe", dir.data(), MAX_PATH);
  }
  if (len == -1 || len == MAX_PATH * 2)
    clear_and_shrink(dir); // It is either empty or incorrect
  else
  {
    dir[int(len)] = '\0';
    dir.updateSz();
  }
  truncate_exe_dir(dir);
  return dir;
}

String get_gamedata_dir()
{
  G_ASSERT(!game_name.empty());
  String dir;
  const char *xdgDocumentsDir = getenv("XDG_CONFIG_HOME");
  const char *homeDir = getenv("HOME");
  if (xdgDocumentsDir)
    dir.printf(260, "%s/%s", xdgDocumentsDir, game_name);
  else if (homeDir)
    dir.printf(260, "%s/%s/%s", homeDir, ".config", game_name);
  else
  {
    String fallback;
    get_current_work_dir(fallback);
    dir.printf(260, "%s/%s/%s", fallback, ".config", game_name);
  }
  return dir;
}

String get_game_dir()
{
  String dir;
  get_current_work_dir(dir);
  return dir;
}

String get_temp_dir() { return String("/tmp/"); }

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
