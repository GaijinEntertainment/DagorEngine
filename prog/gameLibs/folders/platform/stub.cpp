// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "internal.h"

namespace folders
{
namespace internal
{

void platform_initialize(const char *) {}

String get_exe_dir() { return {}; }
String get_game_dir() { return {}; }
String get_gamedata_dir() { return {}; }
String get_temp_dir() { return {}; }
String get_local_appdata_dir() { return {}; }
String get_common_appdata_dir() { return {}; }
String get_downloads_dir() { return {}; }

} // namespace internal
} // namespace folders
