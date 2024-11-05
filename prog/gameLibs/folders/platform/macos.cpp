// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "internal.h"

#include <mach-o/dyld.h>
#include <stdlib.h>
#include <unistd.h>
#include <debug/dag_debug.h>

namespace folders
{
namespace internal
{
static String game_name;

void platform_initialize(const char *app_name) { game_name = app_name; }

String get_exe_dir()
{
  String dir;
  uint32_t buf_size = MAX_PATH;
  dir.resize(buf_size);
  if (_NSGetExecutablePath(dir.data(), &buf_size) < 0)
  {
    dir.resize(buf_size);
    if (_NSGetExecutablePath(dir.data(), &buf_size) < 0)
    {
      logerr("Could not allocate enough memory to get executable path. ");
      clear_and_shrink(dir);
    }
  }
  dir.updateSz();
  truncate_exe_dir(dir);
  return dir;
}

String get_game_dir()
{
  String dir;
  get_current_work_dir(dir);
  return dir;
}

String get_gamedata_dir()
{
  G_ASSERT_RETURN(!game_name.empty(), {});
  String dir;
  dir.printf(260, "%s/%s/%s", getenv("HOME"), "My Games", game_name);
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
