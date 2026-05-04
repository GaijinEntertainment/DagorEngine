// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/tileCacheRI.h>
#include <pathFinder/tileCacheUtil.h>
#include <pathFinder/pathFinder.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/riexHashMap.h>
#include <math/dag_math3d.h>
#include <math/dag_mathAng.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_delayedAction.h>
#include <EASTL/sort.h>

namespace pathfinder
{
RiexHashMap<RiObstacle> riHandle2obstacle;
static Point2 riPadding;
static float riCellSize;
static float riWalkableClimb;
static tile_check_cb_t riTileCheckCb;
// Note: assumed to be immutable after `tilecache_ri_start` started (accessed in thread)
static ska::flat_hash_set<uint32_t> riResourceIds;

static rendinst::obstacle_settings_t riObstacleSettings;

static float timeSinceStarted = 0.f;
static float startAddedExtraTimer = 0.f;
static int startAddedExtraCount = 0;

static void on_ri_invalidate_cb(rendinst::riex_handle_t handle)
{
  auto it = riHandle2obstacle.find(handle);
  if (it == riHandle2obstacle.end())
    return;
  tilecache_obstacle_remove(it->second.obstacle_handle);
  riHandle2obstacle.erase(it);
}

bool tilecache_ri_is_blocking(rendinst::riex_handle_t handle)
{
  const int resIdx = rendinst::handle_to_ri_type(handle);
  const auto setup = riObstacleSettings.findRiEx(resIdx);
  if (!setup || !setup->overrideType)
    return false;
  return setup->overrideTypeValue == 1;
}

void tilecache_log_obstacle(rendinst::riex_handle_t handle);
void tilecache_log_final();

static void tilecache_ri_start_try_add_obstacle(rendinst::riex_handle_t handle, float walkable_climb, float horPadding,
  tile_check_cb_t tile_check_cb)
{
  rendinst::RendInstDesc desc(handle);
  TMatrix tm = rendinst::getRIGenMatrix(desc);
  BBox3 oobb = rendinst::getRIGenBBox(desc);

  if (tile_check_cb && !tile_check_cb(tm * oobb))
    return;

  Point3 c, ext;
  float angY;
  // We need to get unpadded obstacle aabb size, so we pass zero padding.
  tilecache_calc_obstacle_pos(tm, oobb, riCellSize, ZERO<Point2>(), c, ext, angY);
  if (tilecache_ri_obstacle_too_low(ext.y * 2.0f, walkable_climb))
    return;

  if (riObstacleSettings.logObstacles)
    tilecache_log_obstacle(handle);

  const bool block = tilecache_ri_is_blocking(handle);
  riHandle2obstacle[handle].obstacle_handle = tilecache_obstacle_add(tm, oobb, Point2(horPadding, riPadding.y), block, true);
  // WARNING: Do not try to remove these added obstacles right after adding them here to get rid of
  //          already "non-obstacle" obstacles, because such removal is really slow and requires to
  //          fully rebuild navmesh tile. Instead make level designers to re-export level.
}

static float calc_obstacle_hor_padding(uint32_t res_idx)
{
  const auto setup = riObstacleSettings.findRiEx(res_idx);
  return setup && setup->overridePadding ? setup->overridePaddingValue : riPadding.x;
}

static void tilecache_ri_start_try_add_obstacle(rendinst::riex_handle_t handle)
{
  if (!riHandle2obstacle.count(handle))
  {
    const float horPadding = calc_obstacle_hor_padding(rendinst::handle_to_ri_type(handle));
    tilecache_ri_start_try_add_obstacle(handle, riWalkableClimb, horPadding, riTileCheckCb);
  }
}

static void on_ri_extra_added_from_gen_data_in_thread(rendinst::riex_handle_t handle)
{
  if (riResourceIds.count(rendinst::handle_to_ri_type(handle)))
  {
    if constexpr (sizeof(void *) >= sizeof(handle))
      add_delayed_callback([](void *p) { tilecache_ri_start_try_add_obstacle((rendinst::riex_handle_t)p); }, (void *)handle);
    else
      delayed_call([=] { tilecache_ri_start_try_add_obstacle(handle); });
  }
}

void tilecache_ri_start(const ska::flat_hash_set<uint32_t> &res_name_hashes, float cell_size, float walkable_climb,
  const rendinst::obstacle_settings_t &obstacle_settings, const Point2 &padding, tile_check_cb_t tile_check_cb)
{
  riObstacleSettings = obstacle_settings;

  riPadding = padding;
  riCellSize = cell_size;
  riWalkableClimb = walkable_climb;
  riTileCheckCb = tile_check_cb;

  Tab<rendinst::riex_handle_t> handles(framemem_ptr());
  rendinst::iterateRIExtraMap([&](int resIdx, const char *riex_name) {
    if (!res_name_hashes.count(str_hash_fnv1(riex_name)))
      return;
    riResourceIds.insert(resIdx); // In case if it would be loaded later
    handles.clear();
    if (rendinst::getRiGenExtraInstances(handles, resIdx))
    {
      const float horPadding = calc_obstacle_hor_padding(resIdx);
      for (rendinst::riex_handle_t handle : handles)
        tilecache_ri_start_try_add_obstacle(handle, walkable_climb, horPadding, tile_check_cb);
    }
  });

  debug("tile cache: %d stationary obstacles registered (%d/%d/%d hashes/resIds/riExtra)", riHandle2obstacle.size(),
    res_name_hashes.size(), riResourceIds.size(), rendinst::getRIExtraMapSize());

  G_VERIFY(!rendinst::setRiExtraAddedFromGenDataCb(on_ri_extra_added_from_gen_data_in_thread)); // No prev CBs supported
  rendinst::registerRIGenExtraInvalidateHandleCb(on_ri_invalidate_cb);

  timeSinceStarted = 0.f;
  startAddedExtraCount = 0;
}

void tilecache_ri_start_add(const ska::flat_hash_set<uint32_t> &res_name_hashes, float walkable_climb, rendinst::riex_handle_t handle,
  tile_check_cb_t tile_check_cb)
{
  if (riHandle2obstacle.count(handle) > 0)
    return;

  const uint32_t resIdx = rendinst::handle_to_ri_type(handle);
  const char *resName = rendinst::getRIGenExtraName(resIdx);
  if (!resName || res_name_hashes.count(str_hash_fnv1(resName)) <= 0)
    return;

  const float horPadding = calc_obstacle_hor_padding(resIdx);

  const auto wasCount = riHandle2obstacle.size();

  tilecache_ri_start_try_add_obstacle(handle, walkable_climb, horPadding, tile_check_cb);

  const auto newCount = riHandle2obstacle.size();
  if (newCount > wasCount)
  {
    startAddedExtraTimer = (timeSinceStarted < 30.f) ? 5.f : 30.f;
    startAddedExtraCount += (int)(newCount - wasCount);
  }
}

void tilecache_ri_update(float dt)
{
  timeSinceStarted += dt;
  if (startAddedExtraCount <= 0)
    return;
  startAddedExtraTimer -= dt;
  if (startAddedExtraTimer < 0.f)
  {
    debug("tile cache: %d added stationary obstacles registered", startAddedExtraCount);
    startAddedExtraTimer = 0.f;
    startAddedExtraCount = 0;
  }
}

void tilecache_ri_stop()
{
  if (riObstacleSettings.logObstacles)
    tilecache_log_final();

  // Warn: potentially `riGenJobId` might still be running (reads `riResourceIds`)
  if (auto pcb = rendinst::setRiExtraAddedFromGenDataCb(nullptr); pcb && pcb != &on_ri_extra_added_from_gen_data_in_thread)
    rendinst::setRiExtraAddedFromGenDataCb(pcb);

  G_VERIFY(rendinst::unregisterRIGenExtraInvalidateHandleCb(on_ri_invalidate_cb));
  riHandle2obstacle.clear();
  riResourceIds.clear();

  timeSinceStarted = 0.f;
  startAddedExtraCount = 0;
}

void tilecache_ri_walk_obstacles(ri_obstacle_cb_t ri_obstacle_cb)
{
  for (const auto &it : riHandle2obstacle)
  {
    if (it.second.dynamic)
      continue;
    rendinst::RendInstDesc desc(it.first);
    Point3 c, ext;
    float angY;
    tilecache_calc_obstacle_pos(rendinst::getRIGenMatrix(desc), rendinst::getRIGenBBox(desc), riCellSize, riPadding, c, ext, angY);
    TMatrix tm = rotyTM(-angY);
    tm.setcol(3, c);
    ri_obstacle_cb(it.first, tm, BBox3(-ext, ext));
  }
}

void tilecache_ri_enable_obstacle(rendinst::riex_handle_t ri_handle, bool enable, bool sync)
{
  auto it = riHandle2obstacle.find(ri_handle);
  if (it == riHandle2obstacle.end())
    return;
  if ((it->second.obstacle_handle != 0) == enable)
    return;
  if (it->second.dynamic)
    return;
  if (enable)
  {
    rendinst::RendInstDesc desc(ri_handle);
    const bool block = tilecache_ri_is_blocking(ri_handle);
    it->second.obstacle_handle =
      tilecache_obstacle_add(rendinst::getRIGenMatrix(desc), rendinst::getRIGenBBox(desc), riPadding, block, false, sync);
  }
  else if (tilecache_obstacle_remove(it->second.obstacle_handle, sync))
    it->second.obstacle_handle = 0;
}

bool tilecache_ri_obstacle_add(rendinst::riex_handle_t ri_handle, const BBox3 &oobb_inflate, const Point2 &padding, bool sync)
{
  if (riHandle2obstacle.count(ri_handle) > 0)
    return false;
  if (!rendinst::isRiGenExtraValid(ri_handle))
    return false;
  if (!tilecache_is_loaded())
    return true;
  rendinst::RendInstDesc desc(ri_handle);
  RiObstacle obstacle;
  obstacle.dynamic = true;
  BBox3 oobb = rendinst::getRIGenBBox(desc);
  oobb.lim[0] += oobb_inflate.lim[0];
  oobb.lim[1] += oobb_inflate.lim[1];
  const bool block = tilecache_ri_is_blocking(ri_handle);
  obstacle.obstacle_handle = tilecache_obstacle_add(rendinst::getRIGenMatrix(desc), oobb, padding, block, false, sync);
  if (obstacle.obstacle_handle)
    riHandle2obstacle[ri_handle] = obstacle;
  return obstacle.obstacle_handle != 0;
}

bool tilecache_ri_obstacle_remove(rendinst::riex_handle_t ri_handle, bool sync)
{
  auto it = riHandle2obstacle.find(ri_handle);
  if ((it == riHandle2obstacle.end()) || !it->second.dynamic)
    return false;
  if (!tilecache_is_loaded())
    return true;
  bool res = tilecache_obstacle_remove(it->second.obstacle_handle, sync);
  if (res)
    riHandle2obstacle.erase(it);
  return res;
}

void tilecache_log_obstacle(rendinst::riex_handle_t handle)
{
  const uint32_t poolIdx = rendinst::handle_to_ri_type(handle);
  const char *riName = rendinst::getRIGenExtraName(poolIdx);
  riObstacleSettings.logCounts[poolIdx] += 1;
  riObstacleSettings.logNames[poolIdx] = riName;
  ++riObstacleSettings.logNumAddedObstacles;
}

void tilecache_log_final()
{
  eastl::vector<uint32_t> poolIdxs;
  eastl::vector<int> counts;
  eastl::vector<int> order;
  for (auto &it : riObstacleSettings.logCounts)
  {
    poolIdxs.push_back(it.first);
    counts.push_back(it.second);
    order.push_back((int)order.size());
  }
  eastl::sort(order.begin(), order.end(), [&](int a, int b) { return counts[a] > counts[b]; });
  logdbg("Total obstacles: %d", riObstacleSettings.logNumAddedObstacles);
  for (int i = 0; i < (int)order.size(); ++i)
  {
    uint32_t poolIdx = poolIdxs[order[i]];
    // NOTE: rendinst::getRIGenExtraName(poolIdx) may be invalid here (returns <null>)
    const char *riName = riObstacleSettings.logNames[poolIdx].c_str();
    logdbg("Sorted obstacle: %-50s num: %d", riName, counts[order[i]]);
  }
}
} // namespace pathfinder
