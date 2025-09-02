//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/riexHandle.h>
#include <ioSys/dag_baseIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <math/dag_frustum.h>
#include <memory/dag_framemem.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_function.h>
#include <dag/dag_vector.h>
#include <generic/dag_smallTab.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class dtPathCorridor;
class dtNavMeshQuery;
class dtNavMesh;
class DataBlock;
class dtTileCache;
typedef uint64_t dtPolyRef;

namespace scene
{
class TiledScene;
}

namespace pathfinder
{
enum NavMeshID
{
  NM_MAIN,
  NM_EXT_1,
  NMS_COUNT
};

class CustomNav;

using tile_check_cb_t = eastl::fixed_function<2 * sizeof(void *), bool(const BBox3 &)>;
using tile_remove_cb_t = eastl::fixed_function<4 * sizeof(void *), void(uint32_t tile_id, int tx, int ty, int tlayer)>;
using ri_obstacle_cb_t =
  eastl::fixed_function<4 * sizeof(void *), void(rendinst::riex_handle_t ri_handle, const TMatrix &tm, const BBox3 &oobb)>;

struct FindCornersResult
{
  eastl::vector<unsigned char, framemem_allocator> cornerFlags;
  eastl::vector<uint64_t, framemem_allocator> cornerPolys;
};

struct FindRequest
{
  Point3 start;
  Point3 end;
  int includeFlags;
  int excludeFlags;
  Point3 extents;
  float maxJumpUpHeight;

  int numPolys;

  dtPolyRef startPoly;
  dtPolyRef endPoly;

  dag::Vector<Point2> areasCost;
};

struct NavParams
{
  bool enable_jumps = false;
  float max_path_weight = FLT_MAX; // no limit
  float jump_down_threshold = 0.4f;
  float jump_down_weight = 20.0f;
  float jump_over_weight = 20.0f;
  float jump_weight = 40.0f;
};

struct PolyState
{
  unsigned short flags;
  unsigned char areaAndType;
};

enum FindPathResult : unsigned
{
  FPR_FAILED = 0,
  FPR_FULL = 1,
  FPR_PARTIAL = 2
};

enum NavMeshType : unsigned
{
  NMT_SIMPLE = 0,
  NMT_TILED,
  NMT_TILECACHED
};

enum PolyArea : unsigned
{
  POLYAREA_UNWALKABLE = 0,
  POLYAREA_GROUND = 1,
  POLYAREA_OBSTACLE = 2,
  POLYAREA_BRIDGE = 3,
  POLYAREA_BLOCKED = 4,
  POLYAREA_JUMP = 5
  // NB: please do not change/add new fields inside this enum, add game specific enum inside das and use it instead
};

enum PolyFlag : unsigned
{
  POLYFLAG_GROUND = 0x01,
  POLYFLAG_OBSTACLE = 0x02,
  POLYFLAG_LADDER = 0x04,
  POLYFLAG_JUMP = 0x08,
  POLYFLAG_BLOCKED = 0x10
  // NB: please do not change/add new fields inside this enum, add game specific enum inside das and use it instead
};

#define POLYFLAGS_WALK (pathfinder::POLYFLAG_GROUND | pathfinder::POLYFLAG_OBSTACLE)

enum ShouldAct : unsigned
{
  SHOULD_NONE = 0,
  SHOULD_JUMP = 1,
  SHOULD_CLIMB_LADDER = 2,
};

typedef uint32_t obstacle_handle_t;

void clear(bool clearNavData = true);
bool loadNavMesh(IGenLoad &crd, NavMeshType type = NMT_SIMPLE, tile_check_cb_t tile_check_cb = nullptr,
  const char *patchNavMeshFileName = nullptr);
void initWeights(const DataBlock *navQueryFilterWeightsBlk);
bool isLoaded();
FindPathResult findPath(Tab<Point3> &path, FindRequest &req, float step_size, float slop, const CustomNav *custom_nav);
FindPathResult findPath(const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, float dist_to_path = 10.f,
  float step_size = 10.f, float slop = 2.5f, const CustomNav *custom_nav = nullptr, const dag::Vector<Point2> &areasCost = {},
  int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, int excl_flags = POLYFLAG_BLOCKED);
FindPathResult findPath(const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, const Point3 &extents,
  float step_size = 10.f, float slop = 2.5f, const CustomNav *custom_nav = nullptr,
  int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, int excl_flags = POLYFLAG_BLOCKED);
// Checks if path exists without optimizations.
bool check_path(FindRequest &req, float horz_threshold = -1.f, float max_vert_dist = 10.f);
bool check_path(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, float horz_threshold = -1.f,
  float max_vert_dist = 10.f, const CustomNav *custom_nav = nullptr, int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER,
  int excl_flags = POLYFLAG_BLOCKED);
bool check_path(dtNavMeshQuery *nav_query, FindRequest &req, const NavParams &nav_params, float horz_threshold = -1.f,
  float max_vert_dist = 10.f, const CustomNav *custom_nav = nullptr);
bool check_path(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  const NavParams &nav_params, float horz_threshold = -1.f, float max_vert_dist = 10.f, const CustomNav *custom_nav = nullptr,
  int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, int excl_flags = POLYFLAG_BLOCKED);
float calc_approx_path_length(FindRequest &req, float horz_threshold, float max_vert_dist);
float calc_approx_path_length(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, float horz_threshold = -1.f,
  float max_vert_dist = 10.f, int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, int excl_flags = POLYFLAG_BLOCKED);
float calc_approx_path_length(dtNavMeshQuery *nav_query, FindRequest &req, const NavParams &nav_params, float horz_threshold,
  float max_vert_dist);

bool traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, Point3 &out_pos,
  const CustomNav *custom_nav = nullptr);
bool traceray_navmesh(const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents, Point3 &out_pos, dtPolyRef &out_poly,
  const CustomNav *custom_nav = nullptr);
bool traceray_navmesh(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  const NavParams &nav_params, Point3 &out_pos, const CustomNav *custom_nav = nullptr);
bool traceray_navmesh(dtNavMeshQuery *nav_query, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  const NavParams &nav_params, Point3 &out_pos, dtPolyRef &out_poly, const CustomNav *custom_nav = nullptr);
bool project_to_nearest_navmesh_point_ref(Point3 &pos, const Point3 &extents, dtPolyRef &nearestPoly);
bool project_to_nearest_navmesh_point(Point3 &pos, const Point3 &extents, const CustomNav *custom_nav = nullptr);
bool project_to_nearest_navmesh_point(Point3 &pos, float horz_extents, const CustomNav *custom_nav = nullptr);
bool project_to_nearest_navmesh_point_no_obstacles(Point3 &pos, const Point3 &extents, const CustomNav *custom_nav = nullptr);
bool project_to_nearest_navmesh_point_ex(int nav_mesh_idx, Point3 &pos, float horz_extents, const CustomNav *custom_nav = nullptr);
bool project_to_nearest_navmesh_point_ex(int nav_mesh_idx, Point3 &pos, const Point3 &extents, const CustomNav *custom_nav = nullptr);
bool project_to_nearest_navmesh_point_no_obstacles_ex(int nav_mesh_idx, Point3 &pos, const Point3 &extents,
  const CustomNav *custom_nav = nullptr);
bool navmesh_point_has_obstacle(const Point3 &pos, const CustomNav *custom_nav = nullptr);
bool query_navmesh_projections(const Point3 &pos, const Point3 &extents, Tab<Point3> &projections, int points_num = 8,
  const CustomNav *custom_nav = nullptr);
bool query_navmesh_projections(const Point3 &pos, const Point3 &extents, Tab<Point3> &projections, Tab<dtPolyRef> &out_polys,
  int points_num = 8, const CustomNav *custom_nav = nullptr);
float get_distance_to_wall(const Point3 &pos, float horz_extents, float search_rad, const CustomNav *custom_nav = nullptr);
float get_distance_to_wall(const Point3 &pos, float horz_extents, float search_rad, Point3 &hit_norm,
  const CustomNav *custom_nav = nullptr);

bool find_random_point_around_circle(const Point3 &pos, float radius, Point3 &res, const CustomNav *custom_nav = nullptr);
bool find_random_points_around_circle(const Point3 &pos, float radius, int num_points, Tab<Point3> &points);
bool find_random_point_inside_circle(const Point3 &start_pos, float radius, float extends, Point3 &res);

bool is_on_same_polygon(const Point3 &p1, const Point3 &p2, const CustomNav *custom_nav = nullptr);

void renderDebug(const Frustum *p_frustum = NULL, int nav_mesh_idx = NM_MAIN, bool flag_coloring = false);
void setPathForDebug(dag::ConstSpan<Point3> path);
// api for dtPathCorridor, so we'll not need to cast and search for navmesh in client code.
struct CorridorInput
{
  Point3 start;
  Point3 target;
  Point3 extents;
  int includeFlags;
  int excludeFlags;
  float maxJumpUpHeight;
  dtPolyRef startPoly;
  dtPolyRef targetPoly;

  dag::Vector<Point2> areasCost;
  ska::flat_hash_map<dtPolyRef, float> costAddition;
};

void init_path_corridor(dtPathCorridor &corridor);
FindPathResult set_path_corridor(dtPathCorridor &corridor, const CorridorInput &inp, const CustomNav *custom_nav = nullptr);
FindPathResult set_curved_path_corridor(dtPathCorridor &corridor, const CorridorInput &inp, float max_deflection_angle,
  float min_curved_pathlen_sq_threshold, const CustomNav *custom_nav = nullptr);
FindCornersResult find_corridor_corners(dtPathCorridor &corridor, Tab<Point3> &corners, int num,
  const CustomNav *custom_nav = nullptr);

bool set_corridor_agent_position(dtPathCorridor &corridor, const Point3 &pos, dag::ConstSpan<Point2> *areas_cost,
  const CustomNav *custom_nav = nullptr);
bool set_corridor_target(dtPathCorridor &corridor, const Point3 &target, dag::ConstSpan<Point2> *areas_cost,
  const CustomNav *custom_nav = nullptr);

bool optimize_corridor_path(dtPathCorridor &corridor, const FindRequest &req, bool inc_jump, const CustomNav *custom_nav = nullptr);
bool optimize_corridor_path(dtPathCorridor &corridor, const Point3 &extents, bool inc_jump, const CustomNav *custom_nav = nullptr);
bool get_poly_flags(dtPolyRef ref, unsigned short &result_flags);
bool set_poly_flags(dtPolyRef ref, unsigned short flags);
bool get_poly_area(dtPolyRef ref, unsigned char &result_area);
bool set_poly_area(dtPolyRef ref, unsigned char area);
bool corridor_moveOverOffmeshConnection(dtPathCorridor &corridor, dtPolyRef offmesh_con_ref, dtPolyRef &start_ref, dtPolyRef &end_ref,
  Point3 &start_pos, Point3 &end_pos);
int move_over_offmesh_link_in_corridor(dtPathCorridor &corridor, const Point3 &pos, const Point3 &extents,
  const FindCornersResult &ctx, Tab<Point3> &corners, int &over_link, Point3 &out_from, Point3 &out_to,
  const CustomNav *custom_nav = nullptr);
bool is_corridor_valid(dtPathCorridor &corridor, const CustomNav *custom_nav = nullptr);

void draw_corridor_path(const dtPathCorridor &corridor);

bool move_along_surface(FindRequest &req);

bool navmesh_is_valid_poly_ref(dtPolyRef ref);
bool is_valid_poly_ref(const FindRequest &req);
bool reload_nav_mesh(int extra_tiles);
struct NavMeshTriangle
{
  Point3 p0, p1, p2;
  dtPolyRef polyRef;
};
bool get_triangle_by_pos(const Point3 &pos, NavMeshTriangle &result, float horz_dist, int incl_flags, int excl_flags,
  const CustomNav * = nullptr, float max_vert_dist = FLT_MAX);
bool get_triangle_by_poly(const Point3 &pos, const dtPolyRef poly_id, NavMeshTriangle &result);
bool get_triangle_by_poly(const dtPolyRef poly_id, NavMeshTriangle &result);
bool find_nearest_triangle_by_pos(const Point3 &pos, const dtPolyRef poly_id, float dist_threshold, NavMeshTriangle &result);
int squash_jumplinks(const TMatrix &tm, const BBox3 &bbox);
bool find_polys_in_circle(dag::Vector<dtPolyRef, framemem_allocator> &polys, const Point3 &pos, float radius,
  float height_half_offset);

void clear_nav_mesh(int nav_mesh_idx, bool clear_nav_data = true);
bool load_nav_mesh_ex(int nav_mesh_idx, const char *kind, IGenLoad &crd, NavMeshType type = NMT_SIMPLE,
  tile_check_cb_t tile_check_cb = nullptr, const char *patch_nav_mesh_file_name = nullptr);
void init_weights_ex(int nav_mesh_idx, const DataBlock *navQueryFilterWeightsBlk);
bool is_loaded_ex(int nav_mesh_idx);
FindPathResult find_path_ex(int nav_mesh_idx, Tab<Point3> &path, FindRequest &req, float step_size, float slop,
  const CustomNav *custom_nav = nullptr);
FindPathResult find_path_ex(int nav_mesh_idx, const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path,
  float dist_to_path = 10.f, float step_size = 10.f, float slop = 2.5f, const CustomNav *custom_nav = nullptr,
  const dag::Vector<Point2> &areasCost = {}, int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER,
  int excl_flags = POLYFLAG_BLOCKED);
FindPathResult find_path_ex(int nav_mesh_idx, const Point3 &start_pos, const Point3 &end_pos, Tab<Point3> &path, const Point3 &extents,
  float step_size = 10.f, float slop = 2.5f, const CustomNav *custom_nav = nullptr,
  int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, int excl_flags = POLYFLAG_BLOCKED);
// Checks if path exists without optimizations.
bool check_path_ex(int nav_mesh_idx, FindRequest &req, float horz_threshold = -1.f, float max_vert_dist = 10.f);
bool check_path_ex(int nav_mesh_idx, const Point3 &start_pos, const Point3 &end_pos, const Point3 &extents,
  float horz_threshold = -1.f, float max_vert_dist = 10.f, const CustomNav *custom_nav = nullptr,
  int incl_flags = POLYFLAGS_WALK | POLYFLAG_JUMP | POLYFLAG_LADDER, int excl_flags = POLYFLAG_BLOCKED);
bool get_triangle_by_pos_ex(int nav_mesh_idx, const Point3 &pos, NavMeshTriangle &result, float horz_dist, int incl_flags,
  int excl_flags, const CustomNav * = nullptr, float max_vert_dist = FLT_MAX);
bool set_poly_flags_ex(int nav_mesh_idx, dtPolyRef ref, unsigned short flags);
bool get_poly_flags_ex(int nav_mesh_idx, dtPolyRef ref, unsigned short &result_flags);
bool find_polys_in_circle_ex(int nav_mesh_idx, dag::Vector<dtPolyRef, framemem_allocator> &polys, const Point3 &pos, float radius,
  float height_half_offset);
FindPathResult set_path_corridor_ex(int nav_mesh_idx, dtPathCorridor &corridor, const CorridorInput &inp,
  const CustomNav *custom_nav = nullptr);
bool set_corridor_agent_position_ex(int nav_mesh_idx, dtPathCorridor &corridor, const Point3 &pos, dag::ConstSpan<Point2> *areas_cost,
  const CustomNav *custom_nav = nullptr);
FindCornersResult find_corridor_corners_ex(int nav_mesh_idx, dtPathCorridor &corridor, Tab<Point3> &corners, int num,
  const CustomNav *custom_nav = nullptr);

const char *get_nav_mesh_kind(int nav_mesh_idx);
dtNavMesh *get_nav_mesh_ptr(int nav_mesh_idx);
dtNavMesh *getNavMeshPtr();

void tilecache_restart();
void tilecache_start(tile_check_cb_t tile_check_cb = nullptr, const char *obstacle_settings_path = nullptr);
void tilecache_start_add_ri(tile_check_cb_t tile_check_cb, rendinst::riex_handle_t riex_handle);
void tilecache_start_ladders(const scene::TiledScene *ladders);
bool tilecache_is_blocking(rendinst::riex_handle_t riex_handle);
bool tilecache_is_working();
bool tilecache_is_loaded();
bool tilecache_is_inside(const BBox3 &box);
void tilecache_sync();
void tilecache_update(float dt);
void tilecache_stop();

void tilecache_disable_tile_remove_cb();
void tilecache_set_tile_remove_cb(tile_remove_cb_t tile_remove_cb = nullptr);

void tilecache_disable_dynamic_jump_links();
void tilecache_disable_dynamic_ladder_links();
bool tilecache_is_dynamic_ladder_links_enabled();

obstacle_handle_t tilecache_obstacle_add(const TMatrix &tm, const BBox3 &oobb, const Point2 &padding = ZERO<Point2>(),
  bool block = false, bool skip_rebuild = false, bool sync = false);
obstacle_handle_t tilecache_obstacle_add(const Point3 &c, const Point3 &ext, float angY, bool block = false, bool skip_rebuild = false,
  bool sync = false);

void rebuildNavMesh_init();
void rebuildNavMesh_setup(const char *name, float value);
void rebuildNavMesh_setup(const char *name, const Point2 &value);
void rebuildNavMesh_addBBox(const BBox3 &);
bool rebuildNavMesh_update(bool interactive);
int rebuildNavMesh_getProgress();
int rebuildNavMesh_getTotalTiles();
bool rebuildNavMesh_saveToFile(const char *);
void rebuildNavMesh_close();

uint32_t patchedNavMesh_getFileSizeAndNumTiles(const char *, int &num_tiles);
bool patchedNavMesh_loadFromFile(const char *, dtTileCache *, uint8_t *, ska::flat_hash_set<uint32_t> &);

const Tab<uint32_t> &get_removed_tile_cache_tiles();
void clear_removed_tile_cache_tiles();

const Tab<uint32_t> &get_removed_rebuild_tile_cache_tiles();
void clear_removed_rebuild_tile_cache_tiles();

void tilecache_obstacle_move(obstacle_handle_t obstacle_handle, const TMatrix &tm, const BBox3 &oobb,
  const Point2 &padding = ZERO<Point2>(), bool block = false, bool sync = false);
bool tilecache_obstacle_remove(obstacle_handle_t obstacle_handle, bool sync = false);

void tilecache_ri_walk_obstacles(ri_obstacle_cb_t ri_obstacle_cb);
void tilecache_ri_enable_obstacle(rendinst::riex_handle_t ri_handle, bool enable, bool sync = false);

// Add / remove obstacle that's bound to RI, the obstacle will auto-remove if RI is removed.
bool tilecache_ri_obstacle_add(rendinst::riex_handle_t ri_handle, const BBox3 &oobb_inflate, const Point2 &padding = ZERO<Point2>(),
  bool sync = false);
bool tilecache_ri_obstacle_remove(rendinst::riex_handle_t ri_handle, bool sync = false);
void mark_polygons_lower(float, unsigned char);
void mark_polygons_upper(float, unsigned char);

inline uint8_t default_area_cb(uint8_t was_area, uint16_t /*flags*/) { return was_area; };

void change_navpolys_flags_in_poly(int nav_mesh_idx, const Point2 *points, int points_count, unsigned short set_flags,
  unsigned short reset_flags, unsigned short incl_flags, unsigned short excl_flags, int ignore_area = -1,
  float max_length_to_check_only_center = FLT_MAX,
  eastl::fixed_function<sizeof(void *) * 4, uint8_t(uint8_t, uint16_t)> area_cb = default_area_cb);
void change_navpolys_flags_in_box(int nav_mesh_idx, const BBox2 &area, unsigned short set_flags, unsigned short reset_flags,
  unsigned short incl_flags, unsigned short excl_flags, int ignore_area = -1, bool check_only_centers = true,
  eastl::fixed_function<sizeof(void *) * 4, uint8_t(uint8_t, uint16_t)> area_cb = default_area_cb);
void change_navpolys_flags_all(int nav_mesh_idx, unsigned short set_flags, unsigned short reset_flags, unsigned short incl_flags,
  unsigned short excl_flags, int ignore_area = -1);

bool find_nearest_ladder(const Point3 &pos, float radius, Point3 &out_pos, Point3 &out_forw, Point3 &out_along);

int get_poly_count(int nav_mesh_idx);
bool store_mesh_state(int nav_mesh_idx, Tab<PolyState> *buffer = nullptr);
bool restore_mesh_state(int nav_mesh_idx, const Tab<PolyState> *buffer = nullptr);
bool is_stored_state_valid(int nav_mesh_idx);
Tab<PolyState> &get_internal_stored_state_buffer(int nav_mesh_idx);
bool update_walkability(int nav_mesh_idx, const eastl::function<bool(const Point2 &)> &is_walkable_cb);

}; // namespace pathfinder
