// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/unordered_map.h>

struct VromLoadInfo;
namespace net
{
struct ConnectParams;
}

namespace sceneload
{
struct GamePackage
{
  struct VromInfo
  {
    eastl::string mount;
    eastl::string vromFile;
    bool mountAsPack = false;
  };

  eastl::vector<eastl::string> basePaths;
  eastl::vector<VromInfo> addonVroms;
  eastl::vector<eastl::string> addons;
  eastl::vector<eastl::string> rawAddons;
  DataBlock gameSettings;

  eastl::string gameName;
  eastl::string sceneName;
  eastl::string levelBlkPath;
  eastl::vector<eastl::string> importScenes;
  eastl::unordered_map<eastl::string, uint32_t> chunksMap;

  eastl::string ugmContentID;
  eastl::string ugmContentMount;
  eastl::vector<VromInfo> ugmVroms;
  eastl::vector<eastl::string> ugmAddons;
};

struct UserGameModeInfo
{
  int version = 0;
  eastl::string modName;
  eastl::string author;
};

struct UserGameModeContext
{
  eastl::string modId;
  eastl::vector<eastl::string> modsVroms;
  eastl::vector<eastl::string> modsPackVroms;
  eastl::vector<eastl::string> modsAddons;
};

int scan_local_games(DataBlock &games);

extern bool unload_in_progress;
const GamePackage &get_current_game();
bool is_user_game_mod();
const UserGameModeInfo &get_user_mode_info();
void unload_current_game();

GamePackage load_game_package();
void load_package_files(const GamePackage &game_info, bool load_game_res = true);
void setup_base_resources(const GamePackage &game_info, dag::ConstSpan<VromLoadInfo> extra_vroms = {}, bool load_game_res = true);

// Note: Entities that created from scene with non zero import depth isn't saved in editor
bool load_game_scene(const char *scene_path, int import_depth = 1);

void switch_scene(eastl::string_view scene, eastl::vector<eastl::string> &&import_scenes, UserGameModeContext &&ugm_ctx = {});
void switch_scene_and_update(eastl::string_view scene);
void connect_to_session(net::ConnectParams &&connect_params, UserGameModeContext &&ugm_ctx = {});
bool is_load_in_progress();
bool is_scene_switch_in_progress();

void set_scene_blk_path(const char *lblk_path);
void apply_scene_level(eastl::string &&name, eastl::string &&lblk_path);
} // namespace sceneload
