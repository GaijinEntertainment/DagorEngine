// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <pathFinder/pathFinder.h>

namespace pathfinder
{

void clear(bool) {}
bool loadNavMesh(IGenLoad &, NavMeshType, tile_check_cb_t, const char *) { return {}; }
void initWeights(const DataBlock *) {}
bool isLoaded() { return {}; }
FindPathResult findPath(Tab<Point3> &, FindRequest &, float, float, const CustomNav *) { return {}; }
FindPathResult findPath(const Point3 &, const Point3 &, Tab<Point3> &, float, float, float, const CustomNav *,
  const dag::Vector<Point2> &, int, int)
{
  return {};
}
FindPathResult findPath(const Point3 &, const Point3 &, Tab<Point3> &, const Point3 &, float, float, const CustomNav *, int, int)
{
  return {};
}
bool check_path(FindRequest &, float, float) { return {}; }
bool check_path(const Point3 &, const Point3 &, const Point3 &, float, float, const CustomNav *, int, int) { return {}; }
bool check_path(dtNavMeshQuery *, FindRequest &, const NavParams &, float, float, const CustomNav *) { return {}; }
bool check_path(dtNavMeshQuery *, const Point3 &, const Point3 &, const Point3 &, const NavParams &, float, float, const CustomNav *,
  int, int)
{
  return {};
}
float calc_approx_path_length(FindRequest &, float, float) { return {}; }
float calc_approx_path_length(const Point3 &, const Point3 &, const Point3 &, float, float, int, int) { return {}; }
float calc_approx_path_length(dtNavMeshQuery *, FindRequest &, const NavParams &, float, float) { return {}; }
bool traceray_navmesh(const Point3 &, const Point3 &, const Point3 &, Point3 &, const CustomNav *) { return {}; }
bool traceray_navmesh(const Point3 &, const Point3 &, const Point3 &, Point3 &, dtPolyRef &, const CustomNav *) { return {}; }
bool traceray_navmesh(dtNavMeshQuery *, const Point3 &, const Point3 &, const Point3 &, const NavParams &, Point3 &, const CustomNav *)
{
  return {};
}
bool traceray_navmesh(dtNavMeshQuery *, const Point3 &, const Point3 &, const Point3 &, const NavParams &, Point3 &, dtPolyRef &,
  const CustomNav *)
{
  return {};
}
bool project_to_nearest_navmesh_point_ref(Point3 &, const Point3 &, dtPolyRef &) { return {}; }
bool project_to_nearest_navmesh_point(Point3 &, const Point3 &, const CustomNav *) { return {}; }
bool project_to_nearest_navmesh_point(Point3 &, float, const CustomNav *) { return {}; }
bool project_to_nearest_navmesh_point_no_obstacles(Point3 &, const Point3 &, const CustomNav *) { return {}; }
bool project_to_nearest_navmesh_point_ex(int, Point3 &, float, const CustomNav *) { return {}; }
bool project_to_nearest_navmesh_point_ex(int, Point3 &, const Point3 &, const CustomNav *) { return {}; }
bool project_to_nearest_navmesh_point_no_obstacles_ex(int, Point3 &, const Point3 &, const CustomNav *) { return {}; }
bool navmesh_point_has_obstacle(const Point3 &, const CustomNav *) { return {}; }
bool query_navmesh_projections(const Point3 &, const Point3 &, Tab<Point3> &, int, const CustomNav *) { return {}; }
bool query_navmesh_projections(const Point3 &, const Point3 &, Tab<Point3> &, Tab<dtPolyRef> &, int, const CustomNav *) { return {}; }
float get_distance_to_wall(const Point3 &, float, float, const CustomNav *) { return {}; }
float get_distance_to_wall(const Point3 &, float, float, Point3 &, const CustomNav *) { return {}; }
bool find_random_point_around_circle(const Point3 &, float, Point3 &, const CustomNav *) { return {}; }
bool find_random_points_around_circle(const Point3 &, float, int, Tab<Point3> &) { return {}; }
bool find_random_point_inside_circle(const Point3 &, float, float, Point3 &) { return {}; }

bool is_on_same_polygon(const Point3 &, const Point3 &, const CustomNav *) { return {}; }

void renderDebug(const Frustum *, int, bool) {}
void setPathForDebug(dag::ConstSpan<Point3>) {}

void init_path_corridor(dtPathCorridor &) {}
FindPathResult set_path_corridor(dtPathCorridor &, const CorridorInput &, const CustomNav *) { return {}; }
FindPathResult set_curved_path_corridor(dtPathCorridor &, const CorridorInput &, float, float, const CustomNav *) { return {}; }
FindCornersResult find_corridor_corners(dtPathCorridor &, Tab<Point3> &, int, const CustomNav *) { return {}; }
bool set_corridor_agent_position(dtPathCorridor &, const Point3 &, dag::ConstSpan<Point2> *, const CustomNav *) { return {}; }
bool set_corridor_target(dtPathCorridor &, const Point3 &, dag::ConstSpan<Point2> *, const CustomNav *) { return {}; }

bool optimize_corridor_path(dtPathCorridor &, const FindRequest &, bool, const CustomNav *) { return {}; }
bool optimize_corridor_path(dtPathCorridor &, const Point3 &, bool, const CustomNav *) { return {}; }
bool get_poly_flags(dtPolyRef, unsigned short &) { return {}; }
bool set_poly_flags(dtPolyRef, unsigned short) { return {}; }
bool get_poly_area(dtPolyRef, unsigned char &) { return {}; }
bool set_poly_area(dtPolyRef, unsigned char) { return {}; }
bool corridor_moveOverOffmeshConnection(dtPathCorridor &, dtPolyRef, dtPolyRef &, dtPolyRef &, Point3 &, Point3 &) { return {}; }
int move_over_offmesh_link_in_corridor(dtPathCorridor &, const Point3 &, const Point3 &, const FindCornersResult &, Tab<Point3> &,
  int &, Point3 &, Point3 &, const CustomNav *)
{
  return {};
}
bool is_corridor_valid(dtPathCorridor &, const CustomNav *) { return {}; }

void draw_corridor_path(const dtPathCorridor &) {}
bool move_along_surface(FindRequest &) { return {}; }

bool navmesh_is_valid_poly_ref(dtPolyRef) { return {}; }
bool is_valid_poly_ref(const FindRequest &) { return {}; }
bool reload_nav_mesh(int) { return {}; }

bool get_triangle_by_pos(const Point3 &, NavMeshTriangle &, float, int, int, const CustomNav *, float) { return {}; }
bool get_triangle_by_poly(const Point3 &, const dtPolyRef, NavMeshTriangle &) { return {}; }
bool get_triangle_by_poly(const dtPolyRef, NavMeshTriangle &) { return {}; }
bool find_nearest_triangle_by_pos(const Point3 &, const dtPolyRef, float, NavMeshTriangle &) { return {}; }
int squash_jumplinks(const TMatrix &, const BBox3 &) { return {}; }
bool find_polys_in_circle(dag::Vector<dtPolyRef, framemem_allocator> &, const Point3 &, float, float) { return {}; }
void clear_nav_mesh(int, bool) {}
bool load_nav_mesh_ex(int, const char *, IGenLoad &, NavMeshType, tile_check_cb_t, const char *) { return {}; }
void init_weights_ex(int, const DataBlock *) {}
bool is_loaded_ex(int) { return {}; }
FindPathResult find_path_ex(int, Tab<Point3> &, FindRequest &, float, float, const CustomNav *) { return {}; }
FindPathResult find_path_ex(int, const Point3 &, const Point3 &, Tab<Point3> &, float, float, float, const CustomNav *,
  const dag::Vector<Point2> &, int, int)
{
  return {};
}
FindPathResult find_path_ex(int, const Point3 &, const Point3 &, Tab<Point3> &, const Point3 &, float, float, const CustomNav *, int,
  int)
{
  return {};
}
bool check_path_ex(int, FindRequest &, float, float) { return {}; }
bool check_path_ex(int, const Point3 &, const Point3 &, const Point3 &, float, float, const CustomNav *, int, int) { return {}; }
bool get_triangle_by_pos_ex(int, const Point3 &, NavMeshTriangle &, float, int, int, const CustomNav *, float) { return {}; }
bool set_poly_flags_ex(int, dtPolyRef, unsigned short) { return {}; }
bool get_poly_flags_ex(int, dtPolyRef, unsigned short &) { return {}; }
bool find_polys_in_circle_ex(int, dag::Vector<dtPolyRef, framemem_allocator> &, const Point3 &, float, float) { return {}; }
FindPathResult set_path_corridor_ex(int, dtPathCorridor &, const CorridorInput &, const CustomNav *) { return {}; }
bool set_corridor_agent_position_ex(int, dtPathCorridor &, const Point3 &, dag::ConstSpan<Point2> *, const CustomNav *) { return {}; }
FindCornersResult find_corridor_corners_ex(int, dtPathCorridor &, Tab<Point3> &, int, const CustomNav *) { return {}; }
const char *get_nav_mesh_kind(int) { return {}; }
dtNavMesh *get_nav_mesh_ptr(int) { return {}; }
dtNavMesh *getNavMeshPtr() { return {}; }

void tilecache_restart() {}
void tilecache_start(tile_check_cb_t, const char *) {}
void tilecache_start_add_ri(tile_check_cb_t, rendinst::riex_handle_t) {}
void tilecache_start_ladders(const scene::TiledScene *) {}
bool tilecache_is_blocking(rendinst::riex_handle_t) { return {}; }
bool tilecache_is_working() { return {}; }
bool tilecache_is_loaded() { return {}; }
bool tilecache_is_inside(const BBox3 &) { return {}; }
void tilecache_sync() {}
void tilecache_update(float) {}
void tilecache_stop() {}

void tilecache_disable_tile_remove_cb() {}
void tilecache_set_tile_remove_cb(tile_remove_cb_t) {}

void tilecache_disable_dynamic_jump_links() {}
void tilecache_disable_dynamic_ladder_links() {}
bool tilecache_is_dynamic_ladder_links_enabled() { return {}; }
obstacle_handle_t tilecache_obstacle_add(const TMatrix &, const BBox3 &, const Point2 &, bool, bool, bool) { return {}; }
obstacle_handle_t tilecache_obstacle_add(const Point3 &, const Point3 &, float, bool, bool, bool) { return {}; }

void rebuildNavMesh_init() {}
void rebuildNavMesh_setup(const char *, float) {}
void rebuildNavMesh_setup(const char *, const Point2 &) {}
void rebuildNavMesh_addBBox(const BBox3 &) {}
bool rebuildNavMesh_update(bool) { return {}; }
int rebuildNavMesh_getProgress() { return {}; }
int rebuildNavMesh_getTotalTiles() { return {}; }
bool rebuildNavMesh_saveToFile(const char *) { return {}; }
void rebuildNavMesh_close() {}

uint32_t patchedNavMesh_getFileSizeAndNumTiles(const char *, int &) { return {}; }
bool patchedNavMesh_loadFromFile(const char *, dtTileCache *, uint8_t *, ska::flat_hash_set<uint32_t> &) { return {}; }

const Tab<uint32_t> &get_removed_tile_cache_tiles()
{
  Tab<uint32_t> *t = nullptr;
  return *t;
}
void clear_removed_tile_cache_tiles() {}

const Tab<uint32_t> &get_removed_rebuild_tile_cache_tiles()
{
  Tab<uint32_t> *t = nullptr;
  return *t;
}
void clear_removed_rebuild_tile_cache_tiles() {}
void tilecache_obstacle_move(obstacle_handle_t, const TMatrix &, const BBox3 &, const Point2 &, bool, bool) {}
bool tilecache_obstacle_remove(obstacle_handle_t, bool) { return {}; }

void tilecache_ri_walk_obstacles(ri_obstacle_cb_t) {}
void tilecache_ri_enable_obstacle(rendinst::riex_handle_t, bool, bool) {}

bool tilecache_ri_obstacle_add(rendinst::riex_handle_t, const BBox3 &, const Point2 &, bool) { return {}; }
bool tilecache_ri_obstacle_remove(rendinst::riex_handle_t, bool) { return {}; }
void mark_polygons_lower(float, unsigned char) {}
void mark_polygons_upper(float, unsigned char) {}

void change_navpolys_flags_in_poly(int, const Point2 *, int, unsigned short, unsigned short, unsigned short, unsigned short, int,
  float, eastl::fixed_function<sizeof(void *) * 4, uint8_t(uint8_t, uint16_t)>)
{}
void change_navpolys_flags_in_box(int, const BBox2 &, unsigned short, unsigned short, unsigned short, unsigned short, int, bool,
  eastl::fixed_function<sizeof(void *) * 4, uint8_t(uint8_t, uint16_t)>)
{}
void change_navpolys_flags_all(int, unsigned short, unsigned short, unsigned short, unsigned short, int) {}

bool find_nearest_ladder(const Point3 &, float, Point3 &, Point3 &, Point3 &) { return {}; }

int get_poly_count(int) { return {}; }
bool store_mesh_state(int, Tab<PolyState> *) { return {}; }
bool restore_mesh_state(int, const Tab<PolyState> *) { return {}; }
bool is_stored_state_valid(int) { return {}; }
Tab<PolyState> &get_internal_stored_state_buffer(int)
{
  Tab<PolyState> *state = nullptr;
  return *state;
}
bool update_walkability(int, const Tab<TMatrix> &, const Tab<TMatrix> &) { return {}; }


}; // namespace pathfinder
