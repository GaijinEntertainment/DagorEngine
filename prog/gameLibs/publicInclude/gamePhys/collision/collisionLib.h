//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>

#include <phys/dag_physDecl.h>
#include <gamePhys/collision/collisionObject.h>
#include <gamePhys/collision/physLayers.h>
#include <scene/dag_physMat.h>
#include <sceneRay/dag_sceneRayDecl.h>

#include <math/dag_Point3.h>
#include <gameMath/traceUtils.h>
#include <generic/dag_enumBitMask.h>
#include <phys/dag_physShapeQueryResult.h>
#include <rendInst/constants.h>

#include <EASTL/fixed_function.h>

class PhysWorld;

namespace gamephys
{
struct CollisionContactData;
struct CollisionObjectInfo;
}; // namespace gamephys

class LandMeshManager;
struct TraceMeshFaces;
class DataBlock;
class CollisionResource;
class IGenLoad;
class FFTWater;
class GeomNodeTree;
struct PhysMap;
struct CollisionNode;
class FFTWater;
struct PhysShapeQueryOutput;

namespace dacoll
{
typedef ::PhysShapeQueryResult ShapeQueryOutput;
typedef eastl::fixed_function<64, bool(const rendinst::RendInstDesc &, float)> RIFilterCB;

enum CollType : uint32_t
{
  ETF_LMESH = 1 << 0,
  ETF_FRT = 1 << 1,
  ETF_RI = 1 << 2,
  ETF_RESTORABLES = 1 << 3,
  ETF_OBJECTS_GROUP = 1 << 4,
  ETF_STRUCTURES = 1 << 5,
  ETF_HEIGHTMAP = 1 << 6,
  ETF_STATIC = 1 << 7,
  ETF_RI_TREES = 1 << 8,
  ETF_RI_PHYS = 1 << 9, // Trace against PHYS_COLLIDABLE instead of TRACEABLE
  ETF_DEFAULT = ETF_LMESH | ETF_HEIGHTMAP | ETF_FRT | ETF_RI | ETF_RESTORABLES | ETF_OBJECTS_GROUP | ETF_STRUCTURES,
  ETF_ALL = -1 & ~ETF_RI_PHYS // Always specify use of phys collision explicitly
};

enum class InitFlags
{
  None = 0,
  ForceUpdateAABB = 1 << 0,
  EnableDebugDraw = 1 << 1,
  SingleThreaded = 1 << 2,
  Default = ForceUpdateAABB | EnableDebugDraw
};
DAGOR_ENABLE_ENUM_BITMASK(InitFlags);

enum class TestPairFlags
{
  None = 0,
  CheckInWorld = 1 << 0,
  Default = CheckInWorld
};
DAGOR_ENABLE_ENUM_BITMASK(TestPairFlags);

void init_collision_world(InitFlags flags, float collapse_contact_thr);
void term_collision_world();
void destroy_static_collision();
void clear_collision_world();

void set_add_instances_to_world(bool flag);
void set_ttl_for_collision_instances(float value);

void add_static_collision_frt(DeserializedStaticSceneRayTracer *frt, const char *name, dag::ConstSpan<unsigned char> *pmid = NULL);
void set_water_tracer(BuildableStaticSceneRayTracer *tracer);
bool load_static_collision_frt(IGenLoad *crd);
void add_collision_hmap(LandMeshManager *land, float restitution, float margin);
void add_collision_hmap_custom(const Point3 &collision_pos, const BBox3 &collision_box, const Point2 &hmap_offset, float hmap_scale,
  int hmap_step);
void add_collision_landmesh(LandMeshManager *land, const char *name, float restitution = 0.f);
void set_landmesh_mirroring(int cells_x_pos, int cells_x_neg, int cells_z_pos, int cells_z_neg);
void set_water_source(FFTWater *water, bool in_has_only_water2d = false);
void set_lmesh_phys_map(PhysMap *phys_map);
const PhysMap *get_lmesh_phys_map();

bool traceray_normalized_frt(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm);
bool traceray_normalized_lmesh(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm);
bool traceray_normalized_ri(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm,
  rendinst::TraceFlags additional_trace_flags = {}, rendinst::RendInstDesc *out_desc = nullptr, int ray_mat_id = -1,
  const TraceMeshFaces *handle = nullptr, rendinst::riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL);
bool traceray_normalized_coll_type(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm,
  int flags = ETF_DEFAULT, rendinst::RendInstDesc *out_desc = nullptr, int *out_coll_type = nullptr, int ray_mat_id = -1,
  const TraceMeshFaces *handle = nullptr);
bool traceray_normalized(const Point3 &p, const Point3 &dir, real &t, int *out_pmid, Point3 *out_norm, int flags = ETF_DEFAULT,
  rendinst::RendInstDesc *out_desc = nullptr, int ray_mat_id = -1, const TraceMeshFaces *handle = nullptr);

bool rayhit_normalized_frt(const Point3 &p, const Point3 &dir, real t);
bool rayhit_normalized_lmesh(const Point3 &p, const Point3 &dir, real t);
bool rayhit_normalized_ri(const Point3 &p, const Point3 &dir, real t, rendinst::TraceFlags additional_trace_flags = {},
  int ray_mat_id = -1, const TraceMeshFaces *handle = nullptr, rendinst::riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL);
bool rayhit_normalized(const Point3 &p, const Point3 &dir, real t, int flags = ETF_DEFAULT, int ray_mat_id = -1,
  const TraceMeshFaces *handle = nullptr, rendinst::riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL);
bool rayhit_normalized_transparency(const Point3 &p, const Point3 &dir, float t, float threshold = 1.f, int ray_mat_id = -1);
bool rayhit_normalized_sound_occlusion(const Point3 &p, const Point3 &dir, float t, int ray_mat_id, float &accumulated_occlusion,
  float &max_occlusion);

bool traceray_normalized_multiray(dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc, int flags = ETF_DEFAULT,
  int ray_mat_id = -1, const TraceMeshFaces *handle = nullptr);
bool tracedown_normalized(const Point3 &p, real &t, int *out_pmid, Point3 *out_norm, int flags = ETF_DEFAULT,
  rendinst::RendInstDesc *out_desc = nullptr, int ray_mat_id = -1, const TraceMeshFaces *handle = nullptr);
bool tracedown_normalized_multiray(dag::Span<Trace> traces, dag::Span<rendinst::RendInstDesc> ri_desc, int flags = ETF_DEFAULT,
  int ray_mat_id = -1, const TraceMeshFaces *handle = nullptr);
bool tracedown_hmap_cache_multiray(dag::Span<Trace> traces, const TraceMeshFaces *handle, Point3_vec4 *ray_start_pos_vec,
  Point3_vec4 *v_out_norm);

bool traceray_normalized_contact(const Point3 &from, const Point3 &to, Tab<gamephys::CollisionContactData> &out_contacts,
  int ray_mat_id, int flags = ETF_DEFAULT);

// trace version of sphere cast. it's faster than sphere cast, but does just traces.
bool trace_sphere_cast_ex(const Point3 &from, const Point3 &to, float rad, int num_casts, dacoll::ShapeQueryOutput &out,
  int cast_mat_id, int ignore_game_obj_id, const TraceMeshFaces *handle, int flags = ETF_DEFAULT);

typedef bool (*trace_game_objects_cb)(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel, int ignore_game_obj_id,
  int ray_mat_id);
void set_trace_game_objects_cb(trace_game_objects_cb cb);
bool trace_game_objects(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel, int ignore_game_obj_id, int ray_mat_id);

using GameObjectsCollisionsCb = eastl::fixed_function<6 * sizeof(void *), void(CollisionObject obj)>;
typedef void (*gather_game_objects_collision_on_ray_cb)(const Point3 &from, const Point3 &to, float radius,
  GameObjectsCollisionsCb coll_cb);
void set_gather_game_objects_on_ray_cb(gather_game_objects_collision_on_ray_cb cb);


void tracemultiray_lmesh(dag::Span<Trace> &traces);
float traceht_lmesh(const Point2 &pos);
float traceht_hmap(const Point2 &pos);
bool traceht_water(const Point3 &pos, float &t);
float traceht_water_at_time(const Point3 &pos, float time, Point3 *out_displacement = nullptr);
float traceht_water_at_time(const Point3 &pos, float t, float time, bool &underWater, float minWaterCoastDist = 1.5f);
float traceht_water_at_time_no_ground(const Point3 &pos, float t, float time, bool &underwater);
bool is_valid_heightmap_pos(const Point2 &pos);
bool is_valid_water_height(float height);
bool traceray_water_at_time(const Point3 &start, const Point3 &end, float time, float &t);
float getSignificantWaterWaveHeight();

bool get_min_max_hmap_in_circle(const Point2 &center, float rad, float &min_ht, float &max_ht);
void get_min_max_hmap_list_in_circle(const Point2 &center, float rad, Tab<Point2> &list);

void set_enable_apex(bool flag);
bool is_apex_enabled();

CollisionObject create_coll_obj_from_shape(const PhysCollision &shape, void *userPtr, bool kinematic, bool add_to_world,
  bool auto_mask, int group_filter = EPL_KINEMATIC, int mask = DEFAULT_DYN_COLLISION_MASK, const TMatrix *wtm = nullptr);

CollisionObject add_dynamic_cylinder_collision(const TMatrix &tm, float rad, float ht, void *user_ptr = nullptr,
  bool add_to_world = true);
CollisionObject add_dynamic_sphere_collision(const TMatrix &tm, float rad, void *user_ptr = nullptr, bool add_to_world = true);
CollisionObject add_dynamic_capsule_collision(const TMatrix &tm, float rad, float height, void *user_ptr = nullptr,
  bool add_to_world = true);
CollisionObject add_dynamic_box_collision(const TMatrix &tm, const Point3 &width, void *user_ptr = nullptr, bool add_to_world = true);
CollisionObject add_dynamic_collision_with_mask(const DataBlock &props, void *userPtr, bool is_player, bool add_to_world,
  bool auto_mask, int mask = DEFAULT_DYN_COLLISION_MASK, const TMatrix *wtm = nullptr);
inline CollisionObject add_dynamic_collision(const DataBlock &props, void *userPtr = nullptr, bool is_player = false,
  bool add_to_world = true, int mask = DEFAULT_DYN_COLLISION_MASK, const TMatrix *wtm = nullptr)
{
  return add_dynamic_collision_with_mask(props, userPtr, is_player, add_to_world, !add_to_world, mask, wtm);
}
enum AddDynCOFromCollResFlags
{
  ACO_NONE = 0,
  ACO_BOXIFY = 1 << 0,
  ACO_KINEMATIC = 1 << 1,
  ACO_ADD_TO_WORLD = 1 << 2,
  ACO_APEX_IS_PLAYER = 1 << 3,
  ACO_APEX_IS_VEHICLE = 1 << 4,
  ACO_APEX_ADD = 1 << 5,
  ACO_FORCE_CONVEX_HULL = 1 << 6,
};
CollisionObject add_dynamic_collision_from_coll_resource(const DataBlock *props, const CollisionResource *coll_resource,
  void *user_ptr = nullptr, /*AddDynCOFromCollResFlags */ int flags = ACO_NONE, int phys_layer = EPL_KINEMATIC,
  int mask = DEFAULT_DYN_COLLISION_MASK, const TMatrix *wtm = nullptr);
CollisionObject add_simple_dynamic_collision_from_coll_resource(const DataBlock &props, const CollisionResource *resource,
  GeomNodeTree *tree, float margin, float scale, Point3 &out_center, TMatrix &out_tm, TMatrix &out_tm_in_model, bool add_to_world);
CollisionObject add_dynamic_collision_convex_from_coll_resource(dag::ConstSpan<const CollisionNode *> coll_nodes, const Point3 &offset,
  void *user_ptr, int node_type_flags, bool kinematic, bool add_to_world, int phys_layer = EPL_KINEMATIC,
  int mask = DEFAULT_DYN_COLLISION_MASK, const TMatrix *wtm = nullptr);

struct PhysBodyProperties
{
  Point3 centerOfMass = Point3(0, 0, 0);
  Point3 momentOfInertia = Point3(0, 0, 0);
  float volume = 0.f;
};

void build_dynamic_collision_from_coll_resource(const CollisionResource *coll_resource, PhysCompoundCollision *compound,
  PhysBodyProperties &out_properties);

CollisionObject build_dynamic_collision_from_coll_resource(const CollisionResource *coll_resource, bool add_to_world, int phys_layer,
  int mask, PhysBodyProperties &out_properties);

void destroy_dynamic_collision(CollisionObject co);
void add_object_to_world(CollisionObject co);
void remove_object_from_world(CollisionObject co);

CollisionObject &get_reusable_sphere_collision();
CollisionObject &get_reusable_capsule_collision();
CollisionObject &get_reusable_box_collision();

void set_collision_object_tm(const CollisionObject &co, const TMatrix &tm);
void set_vert_capsule_shape_size(const CollisionObject &co, float cap_rad, float cap_cyl_ht);
void set_collision_sphere_rad(const CollisionObject &co, float rad);

bool test_collision_frt(const CollisionObject &co, Tab<gamephys::CollisionContactData> &out_contacts, int mat_id = -1);
bool test_collision_lmesh(const CollisionObject &co, const TMatrix &tm, float max_rad, int def_mat_id,
  Tab<gamephys::CollisionContactData> &out_contacts, int mat_id = -1);
bool test_collision_ri(const CollisionObject &co, const BBox3 &box, Tab<gamephys::CollisionContactData> &out_contacts,
  const TraceMeshFaces *trace_cache = NULL, int mat_id = -1);
bool test_collision_ri(const CollisionObject &co, const BBox3 &box, Tab<gamephys::CollisionContactData> &out_contacts,
  bool provide_coll_info, float at_time, const TraceMeshFaces *trace_cache = NULL, int mat_id = -1,
  bool process_tree_behaviour = true);
void update_ri_cache_in_volume_to_phys_world(const BBox3 &box);
bool test_collision_world(const CollisionObject &co, Tab<gamephys::CollisionContactData> &out_contacts, int mat_id = -1,
  dacoll::PhysLayer group = EPL_DEFAULT, int mask = EPL_ALL);
bool test_collision_world(dag::ConstSpan<CollisionObject> collision, const TMatrix &tm, float bounding_rad,
  Tab<gamephys::CollisionContactData> &out_contacts, const TraceMeshFaces *trace_cache);
bool test_collision_world(dag::ConstSpan<CollisionObject> collision, float bounding_rad,
  Tab<gamephys::CollisionContactData> &out_contacts, const TraceMeshFaces *trace_cache);
bool test_sphere_collision_world(const Point3 &pos, float radius, int mat_id, Tab<gamephys::CollisionContactData> &out_contacts,
  dacoll::PhysLayer group = EPL_DEFAULT, int mask = EPL_ALL);
bool test_box_collision_world(const TMatrix &tm, int mat_id, Tab<gamephys::CollisionContactData> &out_contacts,
  dacoll::PhysLayer group = EPL_DEFAULT, int mask = EPL_ALL);

bool test_pair_collision(dag::ConstSpan<CollisionObject> co_a, uint64_t cof_a, dag::ConstSpan<CollisionObject> co_b, uint64_t cof_b,
  Tab<gamephys::CollisionContactData> &out_contacts, TestPairFlags flags = TestPairFlags::Default);

bool test_pair_collision(dag::ConstSpan<CollisionObject> co_a, uint64_t cof_a, const TMatrix &tm_a,
  dag::ConstSpan<CollisionObject> co_b, uint64_t cof_b, const TMatrix &tm_b, Tab<gamephys::CollisionContactData> &out_contacts,
  TestPairFlags flags = TestPairFlags::Default, bool set_co_tms = true);

bool test_pair_collision_continuous(dag::ConstSpan<CollisionObject> co_a, uint64_t cof_a, const TMatrix &tm_a,
  const TMatrix &tm_a_prev, dag::ConstSpan<CollisionObject> co_b, uint64_t cof_b, const TMatrix &tm_b, const TMatrix &tm_b_prev,
  Tab<gamephys::CollisionContactData> &out_contacts);

PhysBody *get_convex_shape(CollisionObject co);

void shape_query_frt(const PhysSphereCollision &shape, const TMatrix &from, const TMatrix &to, ShapeQueryOutput &out);
void shape_query_lmesh(const PhysSphereCollision &shape, const TMatrix &from, const TMatrix &to, ShapeQueryOutput &out);
void shape_query_frt(const PhysBody *shape, const TMatrix &from, const TMatrix &to, ShapeQueryOutput &out);
void shape_query_lmesh(const PhysBody *shape, const TMatrix &from, const TMatrix &to, ShapeQueryOutput &out);
void shape_query_ri(const PhysBody *shape, const TMatrix &from, const TMatrix &to, float rad, ShapeQueryOutput &out,
  int cast_mat_id = -1, Tab<rendinst::RendInstDesc> *out_desc = nullptr, const TraceMeshFaces *handle = nullptr,
  RIFilterCB *filterCB = nullptr);
bool sphere_query_ri(const Point3 &from, const Point3 &to, float rad, ShapeQueryOutput &out, int cast_mat_id = -1,
  Tab<rendinst::RendInstDesc> *out_desc = nullptr, const TraceMeshFaces *handle = nullptr, RIFilterCB *filterCB = nullptr);
bool sphere_cast(const Point3 &from, const Point3 &to, float rad, ShapeQueryOutput &out, int cast_mat_id = -1,
  const TraceMeshFaces *handle = nullptr);
bool sphere_cast_land(const Point3 &from, const Point3 &to, float rad, ShapeQueryOutput &out, int hmap_step = -1);
static constexpr int DEFAULT_SPHERE_CAST_MASK = EPL_ALL & ~(EPL_CHARACTER | EPL_DEBRIS);
bool sphere_cast_ex(const Point3 &from, const Point3 &to, float rad, ShapeQueryOutput &out, int cast_mat_id,
  dag::ConstSpan<CollisionObject> ignore_objs, const TraceMeshFaces *handle = nullptr, int mask = DEFAULT_SPHERE_CAST_MASK,
  int hmap_step = -1);
bool box_cast_ex(const TMatrix &from, const TMatrix &to, Point3 rad, ShapeQueryOutput &out, int cast_mat_id,
  dag::ConstSpan<CollisionObject> ignore_objs, const TraceMeshFaces *handle, int mask, int hmap_step);

void draw_phys_body(const PhysBody *body);
void draw_collision_object(const CollisionObject &co);
void draw_collision_object(const CollisionObject &co, const TMatrix &tm);
void force_debug_draw(bool flag);
bool is_debug_draw_forced();

void fetch_sim_res(bool wait);

void set_obj_motion(CollisionObject obj, const TMatrix &tm, const Point3 &vel, const Point3 &omega);
void set_obj_active(CollisionObject &coll_obj, bool active, bool is_kinematic = false);
bool is_obj_active(const CollisionObject &coll_obj);
void disable_obj_deactivation(CollisionObject &coll_obj);

void obj_apply_impulse(CollisionObject obj, const Point3 &impulse, const Point3 &rel_pos);
void get_coll_obj_tm(CollisionObject obj, TMatrix &tm);

int set_hmap_step(int step);
int get_hmap_step();

PhysWorld *get_phys_world();
void phys_world_start_sim(float dt, bool wake_up_thread = true);
void phys_world_set_invalid_fetch_sim_res_thread(int64_t tid); // Note: 0 - cur thread, -1 - invalid
void phys_world_set_control_thread_id(int64_t tid);

PhysMat::MatID get_lmesh_mat_id_at_point(const Point2 &pos);

FFTWater *get_water();
}; // namespace dacoll
