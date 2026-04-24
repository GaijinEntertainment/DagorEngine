//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>

#include <dasModules/aotAnimchar.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasManagedTab.h>

#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>

DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(CollisionNode::BehaviorFlag, BehaviorFlag);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(CollisionNode::NodeFlag, CollisionNodeFlag);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(CollisionResourceNodeType, CollisionResourceNodeType);

MAKE_TYPE_FACTORY(CollisionNode, CollisionNode);
MAKE_TYPE_FACTORY(CollisionResource, CollisionResource);
MAKE_TYPE_FACTORY(IntersectedNode, IntersectedNode);

typedef Tab<int> CollisionTriIndices;
typedef Tab<Point3> CollisionTriVertices;

DAS_BIND_VECTOR(CollResIntersectionsType, CollResIntersectionsType, IntersectedNode, " ::CollResIntersectionsType");
DAS_BIND_VECTOR(CollResHitNodesType, CollResHitNodesType, int, " ::CollResHitNodesType");

namespace bind_dascript
{
inline bool collres_traceray(const CollisionResource &collres, const das::float3x4 &tm, const GeomNodeTree *geom_node_tree,
  const Point3 &tracePos, const Point3 &traceDir, float &t, Point3 &norm)
{
  int outMatId, outNodeId;
  return collres.traceRay(reinterpret_cast<const TMatrix &>(tm), geom_node_tree, tracePos, traceDir, t, &norm, outMatId, outNodeId);
}

inline bool collres_traceray2(const CollisionResource &collres, const das::float3x4 &tm, const Point3 &from, const Point3 &dir,
  float &current_t, Point3 *normal)
{
  int outMatId, outNodeId;
  return collres.traceRay(reinterpret_cast<const TMatrix &>(tm), nullptr, from, dir, current_t, normal, outMatId, outNodeId);
}

inline bool collres_traceray3(const CollisionResource &collres, const das::float3x4 &tm, const GeomNodeTree *geom_node_tree,
  const Point3 &tracePos, const Point3 &traceDir, float &t, Point3 &norm, int &outMatId, int &outNodeId)
{
  return collres.traceRay(reinterpret_cast<const TMatrix &>(tm), geom_node_tree, tracePos, traceDir, t, &norm, outMatId, outNodeId);
}

inline bool collres_traceray_out_intersections(const CollisionResource &collres, const das::float3x4 &instance_tm,
  const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float start_t, bool sort_intersections,
  uint8_t behaviour_filter, const das::TBlock<void, bool, const das::TTemporary<const CollResIntersectionsType>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  CollResIntersectionsType intersectedNodesList;
  bool ret = collres.traceRay(reinterpret_cast<const TMatrix &>(instance_tm), geom_node_tree, from, dir, start_t, intersectedNodesList,
    sort_intersections, behaviour_filter);
  vec4f args[] = {das::cast<bool>::from(ret), das::cast<const CollResIntersectionsType &>::from(intersectedNodesList)};
  context->invoke(block, args, nullptr, at);
  return ret;
}

inline bool collres_traceray_out_intersections2(const CollisionResource &collres, const das::float3x4 &instance_tm,
  const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float start_t, CollResIntersectionsType &isects,
  bool sort_intersections)
{
  return collres.traceRay(reinterpret_cast<const TMatrix &>(instance_tm), geom_node_tree, from, dir, start_t, isects,
    sort_intersections);
}

inline bool collres_traceCapsule_out_intersections(const CollisionResource &collres, const das::float3x4 &instance_tm,
  const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float start_t, float radius, IntersectedNode &inode)
{
  return collres.traceCapsule(reinterpret_cast<const TMatrix &>(instance_tm), geom_node_tree, from, dir, start_t, radius, inode);
}

inline bool collres_capsuleHit_out_intersections(const CollisionResource &collres, const das::float3x4 &instance_tm,
  const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float start_t, float radius,
  CollResHitNodesType &nodes_hit)
{
  return collres.capsuleHit(reinterpret_cast<const TMatrix &>(instance_tm), geom_node_tree, from, dir, start_t, radius, nodes_hit);
}

inline bool collres_traceray_out_mat(const CollisionResource &collres, const das::float3x4 &tm, const Point3 &tracePos,
  const Point3 &traceDir, float &t, Point3 &norm, int &out_mat_id)
{
  return collres.traceRay(reinterpret_cast<const TMatrix &>(tm), tracePos, traceDir, t, &norm, out_mat_id);
}

inline bool collres_rayhit(const CollisionResource &collres, const das::float3x4 &tm, const GeomNodeTree *geom_node_tree,
  const Point3 &tracePos, const Point3 &traceDir, float t)
{
  return collres.rayHit(reinterpret_cast<const TMatrix &>(tm), geom_node_tree, tracePos, traceDir, t);
}

inline int collres_get_node_index_by_name(const CollisionResource &collres, const char *name)
{
  return collres.getNodeIndexByName(name ? name : "");
}

inline const CollisionNode *collres_get_node(const CollisionResource &collres, const int coll_node_id)
{
  return collres.getNode(coll_node_id);
}

inline int collres_get_nodesCount(const CollisionResource &collres) { return collres.getAllNodes().size(); }

inline BBox3 collres_get_node_bbox(const CollisionResource &collres, int node_id) { return collres.getNodeBBox(node_id); }
inline bool collres_get_node_capsule(const CollisionResource &collres, int node_id, Capsule &out)
{
  return collres.getNodeCapsule(node_id, out);
}
inline const char *collres_get_node_name(const CollisionResource &collres, int node_id) { return collres.getNodeName(node_id); }
inline const TMatrix &collres_get_node_tm(const CollisionResource &collres, int node_id) { return collres.getNodeTm(node_id); }
inline float collres_get_node_max_tm_scale(const CollisionResource &collres, int node_id)
{
  return collres.getNodeMaxTmScale(node_id);
}
inline Point3 collres_get_node_bsphere_center(const CollisionResource &collres, int node_id)
{
  return collres.getNodeBSphereCenter(node_id);
}
inline float collres_get_node_bsphere_radius(const CollisionResource &collres, int node_id)
{
  return collres.getNodeBSphereRadius(node_id);
}

inline void collres_get_collision_node_tm(const CollisionResource &collres, int coll_node_id, const das::float3x4 &instance_tm,
  const GeomNodeTree *geom_node_tree, das::float3x4 &out_tm, das::Context *context, das::LineInfoArg *line_info)
{
  const CollisionNode *node = collres.getNode(coll_node_id);
  if (node == nullptr)
    context->throw_error_at(line_info, "No collision node found with id: %d", coll_node_id);

  collres.getCollisionNodeTm(node, reinterpret_cast<const TMatrix &>(instance_tm), geom_node_tree,
    reinterpret_cast<TMatrix &>(out_tm));
}

inline bool test_collres_intersection(const CollisionResource &col1, const das::float3x4 &tm1, const CollisionResource &col2,
  const das::float3x4 &tm2, Point3 &collPt1, Point3 &collPt2, const bool checkOnlyPhysNodes)
{
  return CollisionResource::testIntersection(&col1, reinterpret_cast<const TMatrix &>(tm1), &col2,
    reinterpret_cast<const TMatrix &>(tm2), collPt1, collPt2, checkOnlyPhysNodes);
}

inline bool test_collres_intersection_out_intersections(const CollisionResource &col1, const das::float3x4 &tm1,
  const CollisionResource &col2, const das::float3x4 &tm2, Point3 &collPt1, Point3 &collPt2, uint16_t &nodeIndex1,
  uint16_t &nodeIndex2, const das::TBlock<void, const das::TTemporary<const das::TArray<uint16_t>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  Tab<uint16_t> intersectionNodesIndices(framemem_ptr());
  bool ret = CollisionResource::testIntersection(&col1, reinterpret_cast<const TMatrix &>(tm1), CollisionNodeFilter(), &col2,
    reinterpret_cast<const TMatrix &>(tm2), CollisionNodeFilter(), collPt1, collPt2, &nodeIndex1, &nodeIndex2,
    &intersectionNodesIndices);
  das::Array arr;
  das::array_mark_locked(arr, intersectionNodesIndices.data(), intersectionNodesIndices.size());
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return ret;
}

inline BSphere3 collres_get_node_bsphere(const CollisionResource &collres, int node_idx) { return collres.getNodeBSphere(node_idx); }

inline int collres_get_node_face_count(const CollisionResource &collres, int node_idx) { return collres.getNodeFaceCount(node_idx); }

inline int collres_get_node_vert_count(const CollisionResource &collres, int node_idx) { return collres.getNodeVertCount(node_idx); }

template <typename TT>
inline void collres_node_iterate_faces_T(const CollisionResource &collres, int node_idx, TT &&block, das::Context *,
  das::LineInfoArg *)
{
  collres.iterateNodeFaces(node_idx, [&](int fi, uint16_t i0, uint16_t i1, uint16_t i2) { block(fi, (int)i0, (int)i1, (int)i2); });
}

inline void collres_node_iterate_faces(const CollisionResource &collres, int node_idx,
  const das::TBlock<void, int, int, int, int> &block, das::Context *context, das::LineInfoArg *at)
{
  vec4f args[4];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      collres.iterateNodeFaces(node_idx, [&](int fi, uint16_t i0, uint16_t i1, uint16_t i2) {
        args[0] = das::cast<int>::from(fi);
        args[1] = das::cast<int>::from((int)i0);
        args[2] = das::cast<int>::from((int)i1);
        args[3] = das::cast<int>::from((int)i2);
        code->eval(*context);
      });
    },
    at);
}

template <typename TT>
inline void collres_node_iterate_face_verts_T(const CollisionResource &collres, int node_idx, TT &&block, das::Context *,
  das::LineInfoArg *)
{
  collres.iterateNodeFacesVerts(node_idx, [&](int fi, vec4f v0, vec4f v1, vec4f v2) {
    block(fi, *reinterpret_cast<das::float3 *>(&v0), *reinterpret_cast<das::float3 *>(&v1), *reinterpret_cast<das::float3 *>(&v2));
  });
}

inline void collres_node_iterate_face_verts(const CollisionResource &collres, int node_idx,
  const das::TBlock<void, int, das::float3, das::float3, das::float3> &block, das::Context *context, das::LineInfoArg *at)
{
  vec4f args[4];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      collres.iterateNodeFacesVerts(node_idx, [&](int fi, vec4f v0, vec4f v1, vec4f v2) {
        args[0] = das::cast<int>::from(fi);
        args[1] = v0;
        args[2] = v1;
        args[3] = v2;
        code->eval(*context);
      });
    },
    at);
}

template <typename TT>
inline void collres_node_iterate_verts_T(const CollisionResource &collres, int node_idx, TT &&block, das::Context *,
  das::LineInfoArg *)
{
  collres.iterateNodeVerts(node_idx, [&](int vi, vec4f v) { block(vi, *reinterpret_cast<das::float3 *>(&v)); });
}

inline void collres_node_iterate_verts(const CollisionResource &collres, int node_idx,
  const das::TBlock<void, int, das::float3> &block, das::Context *context, das::LineInfoArg *at)
{
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      collres.iterateNodeVerts(node_idx, [&](int vi, vec4f v) {
        args[0] = das::cast<int>::from(vi);
        args[1] = v;
        code->eval(*context);
      });
    },
    at);
}

inline bool collres_check_grid_available(const CollisionResource &collres, uint8_t behavior_filter)
{
  return collres.checkGridAvailable(behavior_filter);
}
} // namespace bind_dascript
