//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <pathFinder/pathFinder.h>
#include <pathFinder/tileRICommon.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <rendInst/riexHandle.h>

namespace pathfinder
{
struct RiObstacle
{
  obstacle_handle_t obstacle_handle = 0;
  bool dynamic = false;
};

void tilecache_ri_start(const ska::flat_hash_set<uint32_t> &res_name_hashes, float cell_size, float walkable_climb,
  const rendinst::obstacle_settings_t &obstacle_settings, const Point2 &padding, tile_check_cb_t tile_check_cb);
void tilecache_ri_start_add(const ska::flat_hash_set<uint32_t> &res_name_hashes, float walkable_climb, rendinst::riex_handle_t handle,
  tile_check_cb_t tile_check_cb);
bool tilecache_ri_is_blocking(rendinst::riex_handle_t handle);
void tilecache_ri_update(float dt);
void tilecache_ri_stop();
}; // namespace pathfinder
