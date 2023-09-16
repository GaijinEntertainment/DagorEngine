//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_string.h>

class DataBlock;

namespace folders
{
void initialize(const char *app_name);
void load_custom_folders(const DataBlock &cfg);
void add_location(const char *name, const char *pattern, bool create = true);
void remove_location(const char *name);
const char *get_path(const char *location_name, const char *def = nullptr);

String get_exe_dir();
String get_game_dir();
String get_temp_dir();
String get_gamedata_dir();
String get_local_appdata_dir();
String get_common_appdata_dir();
String get_downloads_dir();
} // namespace folders
