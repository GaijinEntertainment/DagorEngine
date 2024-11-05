// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <osApiWrappers/dag_pathDelim.h>
#include <osApiWrappers/dag_unicode.h>

namespace folders
{
namespace internal
{
void platform_initialize(const char *app_name);

String get_exe_dir();
String get_game_dir();
String get_gamedata_dir();
String get_temp_dir();
String get_local_appdata_dir();
String get_common_appdata_dir();
String get_downloads_dir();

void get_current_work_dir(String &dir);
void truncate_exe_dir(String &dir);
} // namespace internal
} // namespace folders
