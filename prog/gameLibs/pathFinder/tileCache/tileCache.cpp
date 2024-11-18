// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheRI.h>
#include <pathFinder/tileRICommon.h>
#include <pathFinder/tileCacheUtil.h>
#include <pathFinder/pathFinder.h>
#include <detourCommon.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <debug/dag_debug3d.h>
#include <math/dag_mathUtils.h>
#include <util/dag_console.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/string.h>

namespace pathfinder
{
enum UpdateType
{
  UPT_NORMAL = 0,
  UPT_UPTODATE = 1,
  UPT_UNDER_PRESSURE = 2,
  UPT_COUNT = 3
};

static const float TC_UPDATE_TIMEOUT[UPT_COUNT] = {0.15f, 0.5f, 0.05f};
static const float TC_MAX_FLUSH_COUNT = 32;

struct Obstacle
{
  dtObstacleRef ref;
};

struct PendingAdd
{
  Point3 c;
  Point3 ext;
  float angY;
  bool block;
};

using ObstaclesToAddMap = ska::flat_hash_map<obstacle_handle_t, PendingAdd>;

dtTileCache *tileCache = nullptr;
static tile_check_cb_t tileCacheCheckCb = nullptr;
static eastl::string tileCacheObstacleSettingsPath;
static ska::flat_hash_set<uint32_t> tileObstacleResNameHashes;
static obstacle_handle_t nextHandle = 0;
static float tileCacheUpdateT = 0.0f;
static int tileCacheUpdateCnt[UPT_COUNT];
// Can't just call tileCache->add/removeObstacle all the time, detour's queue is limited,
// need to keep our own.
static ObstaclesToAddMap obstaclesToAdd;
static ska::flat_hash_set<dtObstacleRef> obstaclesToRemove;

static ska::flat_hash_map<obstacle_handle_t, Obstacle> handle2obstacle;

void tilecache_init(dtTileCache *tc, const ska::flat_hash_set<uint32_t> &obstacle_res_name_hashes)
{
  tilecache_cleanup();
  tileCache = tc;
  tileObstacleResNameHashes = obstacle_res_name_hashes;
}

bool tilecache_is_working()
{
  return (tileCache && !tileCache->isUpToDate()) || !obstaclesToAdd.empty() || !obstaclesToRemove.empty();
}

bool tilecache_is_blocking(rendinst::riex_handle_t riex_handle) { return tilecache_ri_is_blocking(riex_handle); }

bool tilecache_is_loaded() { return tileCache && nextHandle; }

bool tilecache_is_inside(const BBox3 &box)
{
  if (!tileCacheCheckCb)
    return true;
  return tileCacheCheckCb(box);
}

void tilecache_cleanup()
{
  tilecache_stop();
  dtFreeTileCache(tileCache);
  tileCache = nullptr;
  tileObstacleResNameHashes.clear();
  obstaclesToAdd.clear();
  obstaclesToRemove.clear();
}

void tilecache_restart() { tilecache_start(tileCacheCheckCb, tileCacheObstacleSettingsPath.c_str()); }

void tilecache_start(tile_check_cb_t tile_check_cb, const char *obstacle_settings_path)
{
  if (!tileCache || nextHandle)
    return;

  tileCacheCheckCb = tile_check_cb;
  tileCacheObstacleSettingsPath = obstacle_settings_path;

  nextHandle = 1;
  for (int i = 0; i < UPT_COUNT; ++i)
    tileCacheUpdateCnt[i] = 0;

  rendinst::obstacle_settings_t obstaclesSettings;
  rendinst::load_obstacle_settings(obstacle_settings_path, obstaclesSettings);

  tilecache_ri_start(tileObstacleResNameHashes, tileCache->getParams()->cs, tileCache->getParams()->walkableClimb, obstaclesSettings,
    Point2(tileCache->getParams()->walkableRadius, tileCache->getParams()->walkableHeight), tile_check_cb);
}

void tilecache_start_add_ri(tile_check_cb_t tile_check_cb, rendinst::riex_handle_t riex_handle)
{
  if (!tileCache || !nextHandle)
    return;

  tilecache_ri_start_add(tileObstacleResNameHashes, tileCache->getParams()->walkableClimb, riex_handle, tile_check_cb);
}

void tilecache_stop()
{
  if (!tileCache || !nextHandle)
    return;
  tilecache_ri_stop();
  nextHandle = 0;
  handle2obstacle.clear();
}

static bool tilecache_step()
{
  // 'update' updates at most 1 tile, this is pretty fast, about 1-2ms with tileSize=64 and cellSize=0.125.
  bool upToDate = false;
  bool wasUpToDate = tileCache->isUpToDate();
  tileCache->update(0.0f /* dt currently isn't used by the detour library */, getNavMeshPtr(), &upToDate);
  bool res = upToDate && obstaclesToAdd.empty() && obstaclesToRemove.empty();
  UpdateType upt;
  if (upToDate)
  {
    // Not that much changes are coming, we can accumulate for much longer period.
    upt = UPT_UPTODATE;
    if (!wasUpToDate)
      ++tileCacheUpdateCnt[upt];
  }
  else if (!obstaclesToAdd.empty() || !obstaclesToRemove.empty())
  {
    // Tile cache is under pressure, detour's internal fast queues have been overflowed
    // and we're using our own, need to process them faster.
    upt = UPT_UNDER_PRESSURE;
    ++tileCacheUpdateCnt[upt];
  }
  else
  {
    // Normal operation, changes are coming, but there're not that many, we can accumulate
    // more.
    upt = UPT_NORMAL;
    ++tileCacheUpdateCnt[upt];
  }
  tileCacheUpdateT = TC_UPDATE_TIMEOUT[upt];
  // flush more pending-to-add/remove obstacles, if any.
  int cnt = 0;
  while (!obstaclesToAdd.empty() && (cnt++ < TC_MAX_FLUSH_COUNT))
  {
    auto it = obstaclesToAdd.begin();
    dtObstacleRef ref;
    dtStatus status = tileCache->addBoxObstacle(&it->second.c.x, &it->second.ext.x, it->second.angY, &ref);
    if (!dtStatusFailed(status))
    {
      handle2obstacle[it->first].ref = ref;
      dtTileCacheObstacle *ob = const_cast<dtTileCacheObstacle *>(tileCache->getObstacleByRef(ref));
      ob->areaId = it->second.block ? pathfinder::POLYAREA_BLOCKED : pathfinder::POLYAREA_OBSTACLE;
    }
    else if (!dtStatusDetail(status, DT_BUFFER_TOO_SMALL))
    {
      logerr("failed to add navmesh obstacle!");
    }
    else
      break;
    obstaclesToAdd.erase(obstaclesToAdd.begin());
  }
  while (!obstaclesToRemove.empty() && (cnt++ < TC_MAX_FLUSH_COUNT))
  {
    dtObstacleRef ref = *obstaclesToRemove.begin();
    if (dtStatusFailed(tileCache->removeObstacle(ref)))
      break;
    obstaclesToRemove.erase(obstaclesToRemove.begin());
  }
  return res;
}

void tilecache_update(float dt)
{
  if (!tileCache || !nextHandle)
    return;
  tilecache_ri_update(dt);
  tileCacheUpdateT -= dt;
  if (tileCacheUpdateT > 0.0f)
    return;
  tilecache_step();
}

void tilecache_sync()
{
  if (!tileCache || !nextHandle)
    return;
  uint64_t reft = ref_time_ticks();
  while (!tilecache_step()) {}
  int us = get_time_usec(reft);
  if (us > 2000000)
    logwarn("tilecache_sync took %.3f sec", (float)us / 1000000);
}

static obstacle_handle_t obstacle_add_impl(obstacle_handle_t handle, const Point3 &c, const Point3 &ext, float angY, bool block,
  bool sync, dtObstacleRef &ref)
{
  bool upToDate = false;
  ref = 0;
  while (true)
  {
    dtStatus status = tileCache->addBoxObstacle(&c.x, &ext.x, angY, &ref);
    if (!dtStatusFailed(status))
    {
      if (!handle)
      {
        handle = nextHandle++;
        G_ASSERT(handle > 0);
      }
      break;
    }
    if (!dtStatusDetail(status, DT_BUFFER_TOO_SMALL) || upToDate)
    {
      logerr("failed to add navmesh obstacle!");
      handle = 0;
      ref = 0;
      break;
    }
    if (!sync)
    {
      if (!handle)
      {
        handle = nextHandle++;
        G_ASSERT(handle > 0);
      }
      ref = 0;
      obstaclesToAdd.insert(ObstaclesToAddMap::value_type(handle, {c, ext, angY, block}));
      break;
    }
    pathfinder::tileCache->update(0.0f, getNavMeshPtr(), &upToDate);
  }
  return handle;
}

static bool obstacle_remove_impl(const Obstacle &obstacle, bool sync)
{
  G_ASSERT(obstacle.ref);
  bool upToDate = false;
  bool res = false;
  while (true)
  {
    if (!dtStatusFailed(tileCache->removeObstacle(obstacle.ref)))
    {
      res = true;
      break;
    }
    if (upToDate)
    {
      logerr("failed to remove navmesh obstacle!");
      break;
    }
    if (!sync)
    {
      obstaclesToRemove.insert(obstacle.ref);
      res = true;
      break;
    }
    pathfinder::tileCache->update(0.0f, getNavMeshPtr(), &upToDate);
  }
  return res;
}

obstacle_handle_t tilecache_obstacle_add(const Point3 &c, const Point3 &ext, float angY, bool block, bool skip_rebuild, bool sync)
{
  if (!tileCache || !nextHandle)
    return 0;

  dtObstacleRef ref = 0;
  obstacle_handle_t handle = obstacle_add_impl(0, c, ext, angY, sync, block, ref);
  if (handle)
    handle2obstacle[handle].ref = ref;
  if (ref)
  {
    dtTileCacheObstacle *ob = const_cast<dtTileCacheObstacle *>(tileCache->getObstacleByRef(ref));
    if (skip_rebuild)
    {
      // Drop the "obstacle add" request, just fill in affected tiles and set state to "processed".
      tileCache->decNumReqs();
      ob->state = DT_OBSTACLE_PROCESSED;
      float bmin[3], bmax[3];
      tileCache->getObstacleBounds(ob, bmin, bmax);
      int ntouched = 0;
      tileCache->queryTiles(bmin, bmax, ob->touched, &ntouched, DT_MAX_TOUCHED_TILES);
      if (ntouched == DT_MAX_TOUCHED_TILES)
      {
#if _TARGET_PC || DAGOR_DBGLEVEL <= 0
        constexpr int ll = LOGLEVEL_ERR;
#else
        constexpr int ll = LOGLEVEL_WARN; // Dev run of PC server mission/scene?
#endif
        logmessage(ll, "Maximum number of touched tiles (%d) has been reached? pos(%@) ext(%@)", DT_MAX_TOUCHED_TILES, c, ext);
      }
      ob->ntouched = (unsigned char)ntouched;
    }
    else
      ob->areaId = block ? pathfinder::POLYAREA_BLOCKED : pathfinder::POLYAREA_OBSTACLE;
  }
  return handle;
}

obstacle_handle_t tilecache_obstacle_add(const TMatrix &tm, const BBox3 &oobb, const Point2 &padding, bool block, bool skip_rebuild,
  bool sync)
{
  if (!tileCache || !nextHandle)
    return 0;
  Point3 c, ext;
  float angY;
  tilecache_calc_obstacle_pos(tm, oobb, tileCache->getParams()->cs, padding, c, ext, angY);

  return tilecache_obstacle_add(c, ext, angY, block, skip_rebuild, sync);
}

void tilecache_obstacle_move(obstacle_handle_t obstacle_handle, const TMatrix &tm, const BBox3 &oobb, const Point2 &padding,
  bool block, bool sync)
{
  if (!tileCache || !nextHandle)
    return;
  auto it = handle2obstacle.find(obstacle_handle);
  if (it == handle2obstacle.end())
    return;
  Point3 c, ext;
  float angY;
  tilecache_calc_obstacle_pos(tm, oobb, tileCache->getParams()->cs, padding, c, ext, angY);
  if (!it->second.ref)
  {
    auto jt = obstaclesToAdd.find(obstacle_handle);
    if (jt != obstaclesToAdd.end())
    {
      jt->second.c = c;
      jt->second.ext = ext;
      jt->second.angY = angY;
      jt->second.block = block;
    }
    return;
  }
  if (dtTileCacheObstacle *ob = const_cast<dtTileCacheObstacle *>(tileCache->getObstacleByRef(it->second.ref)))
  {
    Point3 oldC(ob->orientedBox.center[0], ob->orientedBox.center[1], ob->orientedBox.center[2]);
    Point3 oldExt(ob->orientedBox.halfExtents[0], ob->orientedBox.halfExtents[1], ob->orientedBox.halfExtents[2]);
    float oldAngY = ob->orientedBox.angle;
    float posThresholdSq = 0.15f * 0.15f;
    const float angThreshold = DegToRad(5.0f);
    if (((c - oldC).lengthSq() <= posThresholdSq) && ((ext - oldExt).lengthSq() <= posThresholdSq) &&
        (fabs(angle_diff(angY, oldAngY)) <= angThreshold))
    {
      // Obstacle diff is within thresholds, don't update.
      return;
    }
  }
  else
    logerr("failed to dereference obstacle ref!");
  // Important! First add, then remove, i.e. if we have an overloaded situation when
  // everything is queued it's better to add new things first and then removed old ones,
  // otherwise we might end up in situation when there's no obstacle at all for some
  // number of frames.
  dtObstacleRef ref = 0;
  obstacle_add_impl(it->first, c, ext, angY, block, sync, ref);
  obstacle_remove_impl(it->second, sync);
  it->second.ref = ref;
  if (ref)
  {
    dtTileCacheObstacle *ob = const_cast<dtTileCacheObstacle *>(tileCache->getObstacleByRef(ref));
    ob->areaId = block ? pathfinder::POLYAREA_BLOCKED : pathfinder::POLYAREA_OBSTACLE;
  }
}

bool tilecache_obstacle_remove(obstacle_handle_t obstacle_handle, bool sync)
{
  if (!tileCache || !nextHandle)
    return false;
  auto it = handle2obstacle.find(obstacle_handle);
  if (it == handle2obstacle.end())
    return false;
  if (!it->second.ref)
  {
    handle2obstacle.erase(it);
    obstaclesToAdd.erase(it->first);
    return true;
  }
  bool res = obstacle_remove_impl(it->second, sync);
  if (res)
    handle2obstacle.erase(it);
  return res;
}

void tilecache_render_debug(const Frustum *frustum)
{
  (void)frustum;
  if (!tileCache || !nextHandle)
    return;
  for (int i = 0; i < tileCache->getObstacleCount(); ++i)
  {
    const dtTileCacheObstacle *obs = tileCache->getObstacle(i);
    if ((obs->state == DT_OBSTACLE_EMPTY) || (obs->state == DT_OBSTACLE_PROCESSING))
      continue;
    Point3 c(obs->orientedBox.center[0], obs->orientedBox.center[1], obs->orientedBox.center[2]);
    Point3 ax(obs->orientedBox.halfExtents[0] * 2.0f, 0.0f, 0.0f);
    Point3 ay(0.0f, obs->orientedBox.halfExtents[1] * 2.0f, 0.0f);
    Point3 az(0.0f, 0.0f, obs->orientedBox.halfExtents[2] * 2.0f);
    TMatrix tm = rotyTM(-obs->orientedBox.angle);
    ax = tm * ax;
    az = tm * az;
    draw_cached_debug_box(c - (ax + ay + az) * 0.5f, ax, ay, az, E3DCOLOR_MAKE(0, 255, 0, 255));
  }
}
} // namespace pathfinder

static bool tilecache_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("tilecache", "debug_perf", 1, 2)
  {
    if (argc < 2)
      console::print("Usage: tilecache.debug_perf [num_obstacles]");
    else
    {
      // Note! This will remove obstacles, but will leave handle intact, use for
      // perf debug only!
      int numObstacles = atoi(argv[1]);
      for (auto it = pathfinder::handle2obstacle.begin(); it != pathfinder::handle2obstacle.end(); ++it)
      {
        if (!it->second.ref)
          continue;
        if (numObstacles-- <= 0)
          break;
        if (dtStatusFailed(pathfinder::tileCache->removeObstacle(it->second.ref)))
          console::print_d("removeObstacle failed");
        else
          it->second.ref = 0;
      }
      bool upToDate = false;
      float minUpdTime = 1000.0f;
      float maxUpdTime = 0.0f;
      while (!upToDate)
      {
        int64_t startTime = ref_time_ticks();
        pathfinder::tileCache->update(0.0f, pathfinder::getNavMeshPtr(), &upToDate);
        int resTime = get_time_usec(startTime);
        float dt = resTime / 1000.0f;
        console::print_d("tileCache update in %.3f ms", dt);
        minUpdTime = min(minUpdTime, dt);
        maxUpdTime = max(maxUpdTime, dt);
      }
      console::print_d("tileCache update min = %.3f ms, max = %.4f ms", minUpdTime, maxUpdTime);
    }
  }
  CONSOLE_CHECK_NAME("tilecache", "stats", 1, 1)
  {
    logerr("tilecache.stats: updates[NORMAL] = %d, updates[UPTODATE] = %d, updates[UNDER_PRESSURE] = %d",
      pathfinder::tileCacheUpdateCnt[pathfinder::UPT_NORMAL], pathfinder::tileCacheUpdateCnt[pathfinder::UPT_UPTODATE],
      pathfinder::tileCacheUpdateCnt[pathfinder::UPT_UNDER_PRESSURE]);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(tilecache_console_handler);
