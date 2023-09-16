#include <pathFinder/tileCache.h>
#include <pathFinder/tileCacheRI.h>
#include <pathFinder/pathFinder.h>

namespace pathfinder
{
static dtTileCache *tileCache = nullptr;

void tilecache_init(dtTileCache *tc, const ska::flat_hash_set<uint32_t> &obstacle_res_name_hashes)
{
  G_UNUSED(obstacle_res_name_hashes);
  tileCache = tc;
}

void tilecache_stop() {}

bool tilecache_is_loaded() { return false; }

void tilecache_sync() {}

void tilecache_cleanup()
{
  dtFreeTileCache(tileCache);
  tileCache = nullptr;
}

bool tilecache_is_inside(const BBox3 &) { return true; }

void tilecache_restart() {}

void tilecache_start(tile_check_cb_t tile_check_cb, const char *obstacle_settings_path)
{
  G_UNUSED(tile_check_cb);
  G_UNUSED(obstacle_settings_path);
}

void tilecache_start_add_ri(tile_check_cb_t tile_check_cb, rendinst::riex_handle_t riex_handle)
{
  G_UNUSED(tile_check_cb);
  G_UNUSED(riex_handle);
}

void tilecache_update(float dt) { G_UNUSED(dt); }

void tilecache_render_debug(const Frustum *frustum) { G_UNUSED(frustum); }

obstacle_handle_t tilecache_obstacle_add(const TMatrix &tm, const BBox3 &oobb, const Point2 &padding, bool skip_rebuild, bool sync)
{
  G_UNUSED(tm);
  G_UNUSED(oobb);
  G_UNUSED(padding);
  G_UNUSED(skip_rebuild);
  G_UNUSED(sync);
  return 0;
}

void rebuildNavMesh_init() {}

void rebuildNavMesh_setup(const char *, float) {}

void rebuildNavMesh_addBBox(const BBox3 &) {}

bool rebuildNavMesh_update(bool) { return false; }

int rebuildNavMesh_getProgress() { return 0; }

int rebuildNavMesh_getTotalTiles() { return 0; }

bool rebuildNavMesh_saveToFile(const char *) { return false; }

void rebuildNavMesh_close() {}

uint32_t patchedNavMesh_getFileSizeAndNumTiles(const char *, int &) { return 0u; }

bool patchedNavMesh_loadFromFile(const char *, dtTileCache *, uint8_t *, ska::flat_hash_set<uint32_t> &) { return false; }

void clear_removed_rebuild_tile_cache_tiles() { return; }

const Tab<uint32_t> &get_removed_rebuild_tile_cache_tiles()
{
  G_ASSERT(0);

  static Tab<uint32_t> temp;
  return temp;
}

void tilecache_obstacle_move(obstacle_handle_t obstacle_handle, const TMatrix &tm, const BBox3 &oobb, const Point2 &padding, bool sync)
{
  G_UNUSED(obstacle_handle);
  G_UNUSED(tm);
  G_UNUSED(oobb);
  G_UNUSED(padding);
  G_UNUSED(sync);
}

bool tilecache_obstacle_remove(obstacle_handle_t obstacle_handle, bool sync)
{
  G_UNUSED(obstacle_handle);
  G_UNUSED(sync);
  return false;
}

void tilecache_ri_init_obstacles(const char *obstacle_settings_path, obstacle_paddings_t &obstacle_paddings)
{
  G_UNUSED(obstacle_settings_path);
  G_UNUSED(obstacle_paddings);
}

void tilecache_ri_start(const ska::flat_hash_set<uint32_t> &res_name_hashes, float cell_size, float walkable_climb,
  const obstacle_paddings_t &obstacle_paddings, const Point2 &padding, tile_check_cb_t tile_check_cb)
{
  G_UNUSED(res_name_hashes);
  G_UNUSED(cell_size);
  G_UNUSED(walkable_climb);
  G_UNUSED(obstacle_paddings);
  G_UNUSED(padding);
  G_UNUSED(tile_check_cb);
}

void tilecache_ri_start_add(const ska::flat_hash_set<uint32_t> &res_name_hashes, float walkable_climb,
  const obstacle_paddings_t &obstacle_paddings, rendinst::riex_handle_t handle, tile_check_cb_t tile_check_cb)
{
  G_UNUSED(res_name_hashes);
  G_UNUSED(walkable_climb);
  G_UNUSED(obstacle_paddings);
  G_UNUSED(handle);
  G_UNUSED(tile_check_cb);
}

void tilecache_ri_update(float dt) { G_UNUSED(dt); }

void tilecache_ri_stop() {}

void tilecache_ri_walk_obstacles(ri_obstacle_cb_t ri_obstacle_cb) { G_UNUSED(ri_obstacle_cb); }

void tilecache_ri_enable_obstacle(rendinst::riex_handle_t ri_handle, bool enable, bool sync)
{
  G_UNUSED(ri_handle);
  G_UNUSED(enable);
  G_UNUSED(sync);
}

bool tilecache_ri_obstacle_add(rendinst::riex_handle_t ri_handle, const BBox3 &oobb_inflate, const Point2 &padding, bool sync)
{
  G_UNUSED(ri_handle);
  G_UNUSED(oobb_inflate);
  G_UNUSED(padding);
  G_UNUSED(sync);
  return false;
}

bool tilecache_ri_obstacle_remove(rendinst::riex_handle_t ri_handle, bool sync)
{
  G_UNUSED(ri_handle);
  G_UNUSED(sync);
  return false;
}
} // namespace pathfinder