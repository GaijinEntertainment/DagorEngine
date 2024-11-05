// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <folders/folders.h>
#include "platform/internal.h"
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_lookup.h>
#include <util/dag_baseDef.h>
#include <debug/dag_debug.h>

namespace folders
{
static eastl::unordered_map<eastl::string, eastl::string> storage;
static bool initialized = false;

using PatternFN = String(void);

static const char *patterns[] = {"exedir", "gamedir", "gamedatadir", "tempdir", "localappdata", "commonappdata", "downloads"};

static PatternFN *pFuncs[] = {
  &get_exe_dir, &get_game_dir, &get_gamedata_dir, &get_temp_dir, &get_local_appdata_dir, &get_common_appdata_dir, &get_downloads_dir};

static void ensure_existence(const char *path)
{
  if (!dd_dir_exists(path))
  {
    debug("Folders: Creating %s", path);
    dd_mkdir(path);
  }
}

void initialize(const char *app_name)
{
  static_assert(countof(patterns) == countof(pFuncs), "Check patterns/pFuncs count");
  internal::platform_initialize(app_name);
  // store default folders for using these locations in the script via app.get_dir (calls get_path)
  int count = countof(patterns);
  for (int i = 0; i < count; i++)
  {
    storage.emplace(patterns[i], pFuncs[i]().c_str());
  }
  initialized = true;
}

void load_custom_folders(const DataBlock &cfg)
{
  G_ASSERT_RETURN(initialized, );

  for (int i = 0; i < cfg.paramCount(); ++i)
  {
    if (cfg.getParamType(i) != DataBlock::TYPE_STRING)
      continue;
    const char *name = cfg.getParamName(i);
    const char *value = cfg.getStr(i);
    add_location(name, value);
  }
}

static void replace_all_occurences(eastl::string &str, const eastl::string &pattern, const eastl::string &repl)
{
  eastl_size_t index = 0;
  while (true)
  {
    index = str.find(pattern, index);
    if (index == eastl::string::npos)
      break;
    str.replace(index, pattern.length(), repl);
    index += repl.length();
  }
}

static eastl::string replace_pattern(const eastl::string &str)
{
  eastl_size_t startIdx = str.find('{');
  eastl_size_t endIdx = str.find('}');
  G_ASSERT(startIdx < endIdx && endIdx != eastl::string::npos);

  eastl::string result = str.substr(0, startIdx);
  eastl::string key = str.substr(startIdx + 1, endIdx - startIdx - 1);
  key.make_lower();

  int fnIdx = lup(key.c_str(), patterns, countof(patterns));
  G_ASSERTF_RETURN(fnIdx != -1, {}, "Failed to find substitution function %s", key.c_str());

  result += pFuncs[fnIdx]().c_str();
  result += str.substr(endIdx + 1);

  replace_all_occurences(result, "//", "/");
  replace_all_occurences(result, "\\/", "/");

  return result;
}

void add_location(const char *name, const char *pattern, bool create)
{
  eastl::string path = replace_pattern(pattern);
  storage.emplace(name, path);
  debug("Added location: %s -> %s, create flag: %d", name, path.c_str(), create);
  if (create)
  {
    bool result = dd_mkdir(path.c_str());
    G_ASSERTF(result, "Failed to create directory <%s>. Maybe location is not writable", path.c_str());
  }
}

void remove_location(const char *name) { storage.erase(name); }

const char *get_path(const char *name, const char *def)
{
  const auto iterator = storage.find(name);
  if (iterator != storage.end())
    return iterator->second.c_str();
  return def;
}

String get_exe_dir()
{
  // No need to ensure existence of this path
  return internal::get_exe_dir();
}

String get_game_dir()
{
  // No need to ensure existence of this path
  return internal::get_game_dir();
}

String get_gamedata_dir()
{
  String path = internal::get_gamedata_dir();
  ensure_existence(path.c_str());
  return path;
}

String get_temp_dir()
{
  String path = internal::get_temp_dir();
  ensure_existence(path.c_str());
  return path;
}

String get_local_appdata_dir()
{
  String path = internal::get_local_appdata_dir();
  ensure_existence(path.c_str());
  return path;
}

String get_common_appdata_dir()
{
  String path = internal::get_common_appdata_dir();
  ensure_existence(path.c_str());
  return path;
}

String get_downloads_dir()
{
  String path = internal::get_downloads_dir();
  ensure_existence(path.c_str());
  return path;
}
} // namespace folders
