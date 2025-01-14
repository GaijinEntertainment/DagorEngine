//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <scene/dag_physMatIdDecl.h>
#include <gameMath/traceUtils.h>

#include <rendInst/rendInstDesc.h>
#include <rendInst/constants.h>
#include <dag/dag_relocatable.h>


// This is actually private data, potential leak of abstraction here
struct RendInstGenData;

namespace rendinst
{

struct CollisionInfo
{
  void *handle = nullptr;
  CollisionResource *collRes = nullptr;
  SimpleString destrFxTemplate;
  RendInstDesc desc;
  int riPoolRef = -1;
  TMatrix tm = TMatrix::IDENT;
  BBox3 localBBox = BBox3::IDENT;
  float destrImpulse = 0.0f;
  float hp = 0.0f;
  float initialHp = 0.0f;
  int destrFxId = -1;
  int32_t userData = -1;
  bool isImmortal = false;
  bool stopsBullets = true;
  bool isDestr = false;
  bool bushBehaviour = false;
  bool treeBehaviour = false;
  bool isParent = false;
  bool destructibleByParent = false;
  int destroyNeighbourDepth = 1;

  explicit CollisionInfo(const RendInstDesc &ri_desc = RendInstDesc()) : desc(ri_desc) {}
};

struct RendInstCollisionCB
{
  virtual void addCollisionCheck(const CollisionInfo &coll_info) = 0;
  virtual void addTreeCheck(const CollisionInfo &coll_info) = 0;
};

typedef eastl::fixed_function<64, bool(int)> CollisionNodeFilter;

// ======= object to RI intersections ========

struct RiGenCollidableData
{
  TMatrix tm;
  const CollisionResource *collres;
  RendInstDesc desc;
  bool immortal;
  float dist;
};
using rigen_collidable_data_t = dag::RelocatableFixedVector<RiGenCollidableData, 64, true, framemem_allocator>;

bool testObjToRIGenIntersection(CollisionResource *obj_res, const CollisionNodeFilter &filter, const TMatrix &obj_tm,
  const Point3 &obj_vel, Point3 *intersected_obj_pos, bool *tree_sphere_intersected, Point3 *collisionPoint);

void testObjToRIGenIntersection(const BSphere3 &obj_sph, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache = nullptr, PhysMat::MatID ray_mat = PHYSMAT_INVALID, bool unlock_in_cb = false);
void testObjToRIGenIntersection(const BBox3 &obj_box, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache = nullptr, PhysMat::MatID ray_mat = PHYSMAT_INVALID, bool unlock_in_cb = false);
struct OrientedObjectBox
{
  mat44f tm;
  bbox3f bbox;
};
void testObjToRIGenIntersection(const BBox3 &obj_box, const TMatrix &obj_tm, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache = nullptr, PhysMat::MatID ray_mat = PHYSMAT_INVALID, bool unlock_in_cb = false);
void testObjToRIGenIntersection(const Capsule &obj_capsule, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache = nullptr, PhysMat::MatID ray_mat = PHYSMAT_INVALID, bool unlock_in_cb = false);

inline bool testObjectToRendinstIntersection(CollisionResource *object_res, const CollisionNodeFilter &filter,
  const TMatrix &object_tm, const Point3 &velocity, Point3 *intersected_object_pos, String *intersected_object_name,
  bool *tree_sphere_intersected, Point3 *collisionPoint, bool *intersectedWithWood,
  bool, // intersectWithWood
  bool) // intersectWithOther
{
  (void)intersected_object_name;
  if (tree_sphere_intersected)
    *tree_sphere_intersected = false;

  if (testObjToRIGenIntersection(object_res, filter, object_tm, velocity, intersected_object_pos, tree_sphere_intersected,
        collisionPoint))
  {
    if (intersectedWithWood)
      *intersectedWithWood = true;
    return true;
  }
  return false;
}

const char *get_rendinst_res_name_from_col_info(const CollisionInfo &col_info);
CollisionInfo getRiGenDestrInfo(const RendInstDesc &desc);

CheckBoxRIResultFlags checkBoxToRIGenIntersection(const BBox3 &box);
void gatherRIGenCollidableInRadius(rigen_collidable_data_t &out_data, const Point3 &pos, float radius, GatherRiTypeFlags flags);

struct TraceRayRendInstData
{
  rendinst::RendInstDesc riDesc;
  Point3 normIn;
  int matId;
  unsigned collisionNodeId;

  union
  {
    int sortKey;
    float tIn;
  };

  bool operator<(const TraceRayRendInstData &rhs) const { return sortKey < rhs.sortKey; }
};

struct TraceRayRendInstSolidData : TraceRayRendInstData
{
  float tOut;
  Point3 normOut;
};

using RendInstsIntersectionsList = dag::Vector<TraceRayRendInstData, framemem_allocator>;
using RendInstsSolidIntersectionsList = dag::Vector<TraceRayRendInstSolidData, framemem_allocator>;

void computeRiIntersectedSolids(RendInstsSolidIntersectionsList &intersected, const Point3 &from, const Point3 &dir,
  SolidSectionsMerge merge_mode);

uint32_t setMaxNumRiCollisionCb(uint32_t new_max_num);

// ======= trace ray stuff ========

bool traceRayRendInstsNormalized(const Point3 &from, const Point3 &dir, float &tout, Point3 &norm, bool extend_bbox = false,
  bool trace_meshes = false, rendinst::RendInstDesc *ri_desc = nullptr, bool trace_trees = false, int ray_mat_id = -1,
  int *out_mat_id = nullptr);
bool traceRayRendInstsListNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsIntersectionsList &ri_data,
  bool trace_meshes = false);
bool traceRayRendInstsListAllNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsIntersectionsList &ri_data,
  bool trace_meshes = false, float extend_ray = 0.0f);
bool traceRayRendInstsListAllNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsSolidIntersectionsList &ri_data,
  bool trace_meshes = false, float extend_ray = 0.0f);


bool traceRayRIGenNormalized(dag::Span<Trace> traces, TraceFlags trace_flags, int ray_mat_id = -1,
  rendinst::RendInstDesc *ri_desc = nullptr, const TraceMeshFaces *ri_cache = nullptr,
  riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL);

void initTraceTransparencyParameters(float tree_trunk_opacity, float tree_canopy_opacity);

bool traceTransparencyRayRIGenNormalizedWithDist(const Point3 &pos, const Point3 &dir, float &t, float transparency_threshold,
  PhysMat::MatID ray_mat = PHYSMAT_INVALID, rendinst::RendInstDesc *ri_desc = nullptr, int *out_mat_id = nullptr,
  float *out_transparency = nullptr, bool check_canopy = true);
bool traceTransparencyRayRIGenNormalized(const Point3 &pos, const Point3 &dir, float mint, float transparency_threshold,
  PhysMat::MatID ray_mat = PHYSMAT_INVALID, rendinst::RendInstDesc *ri_desc = nullptr, int *out_mat_id = nullptr,
  float *out_transparency = nullptr, bool check_canopy = true);
bool traceTransparencyRayRIGenNormalizedAllLayers(const Point3 &pos, const Point3 &dir, float mint, float transparency_threshold,
  PhysMat::MatID ray_mat = PHYSMAT_INVALID, rendinst::RendInstDesc *ri_desc = nullptr, int *out_mat_id = nullptr,
  float *out_transparency = nullptr, bool check_canopy = true, float min_height_second_layer = 1.f);

bool traceSoundOcclusionRayRIGenNormalized(const Point3 &p, const Point3 &dir, float t, int ray_mat_id, float &accumulated_occlusion,
  float &max_occlusion);

inline bool traceRayRendInstsNormalized(dag::Span<Trace> traces, bool = false, bool trace_meshes = false,
  rendinst::RendInstDesc *ri_desc = nullptr, bool trace_trees = false, int ray_mat_id = -1, const TraceMeshFaces *ri_cache = nullptr)
{
  TraceFlags traceFlags = TraceFlag::Destructible;
  if (trace_meshes)
    traceFlags |= TraceFlag::Meshes;
  if (trace_trees)
    traceFlags |= TraceFlag::Trees;
  return traceRayRIGenNormalized(traces, traceFlags, ray_mat_id, ri_desc, ri_cache);
}

bool traceDownMultiRay(dag::Span<Trace> traces, bbox3f_cref rayBox, dag::Span<RendInstDesc> ri_desc,
  const TraceMeshFaces *ri_cache = nullptr, int ray_mat_id = -1, TraceFlags trace_flags = TraceFlag::Destructible,
  Bitarray *filter_pools = nullptr, IgnoreFunc ignore_func = nullptr); // all rays should be down


bool rayhitRendInstNormalized(const Point3 &from, const Point3 &dir, float t, int ray_mat_id, const RendInstDesc &ri_desc);
bool rayhitRendInstsNormalized(const Point3 &from, const Point3 &dir, float t, float min_size, int ray_mat_id,
  rendinst::RendInstDesc *ri_desc);
inline bool rayhitRendInstsNormalized(const Point3 &from, const Point3 &dir, float t, int ray_mat_id = -1,
  rendinst::RendInstDesc *ri_desc = nullptr)
{
  return rayhitRendInstsNormalized(from, dir, t, 0.0f, ray_mat_id, ri_desc);
}

struct ForeachCB
{
  virtual void executeForTm(RendInstGenData * /* rgl */, const RendInstDesc & /* ri_desc */, const TMatrix & /* tm */) {}
  virtual void executeForPos(RendInstGenData * /* rgl */, const RendInstDesc & /* ri_desc */, const TMatrix & /* tm */) {}
};
void foreachRIGenInBox(const BBox3 &box, GatherRiTypeFlags ri_types, ForeachCB &cb);

void clipCapsuleRI(const ::Capsule &c, Point3 &lpt, Point3 &wpt, real &md, const Point3 &movedirNormalized,
  const TraceMeshFaces *ri_cache);

bool checkCachedRiData(const TraceMeshFaces *ri_cache);
bool initializeCachedRiData(TraceMeshFaces *ri_cache);

} // namespace rendinst

DAG_DECLARE_RELOCATABLE(rendinst::CollisionInfo);
