//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <pathFinder/pathFinder.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <rendInst/riexHandle.h>

namespace pathfinder
{
struct RiObstacle
{
  obstacle_handle_t obstacle_handle = 0;
  bool dynamic = false;
};

typedef ska::flat_hash_map<uint32_t, float> obstacle_paddings_t;

void tilecache_ri_init_obstacles(const char *obstacle_settings_path, obstacle_paddings_t &obstacle_paddings);
void tilecache_ri_start(const ska::flat_hash_set<uint32_t> &res_name_hashes, float cell_size, float walkable_climb,
  const obstacle_paddings_t &obstacle_paddings, const Point2 &padding, tile_check_cb_t tile_check_cb);
void tilecache_ri_start_add(const ska::flat_hash_set<uint32_t> &res_name_hashes, float walkable_climb,
  const obstacle_paddings_t &obstacle_paddings, rendinst::riex_handle_t handle, tile_check_cb_t tile_check_cb);
void tilecache_ri_update(float dt);
void tilecache_ri_stop();
}; // namespace pathfinder
