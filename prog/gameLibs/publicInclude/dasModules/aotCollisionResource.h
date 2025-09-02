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
inline const char *collnode_get_name(const CollisionNode *collnode) { return collnode->name.c_str(); }

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
  arr.data = (char *)intersectionNodesIndices.data();
  arr.capacity = arr.size = uint32_t(intersectionNodesIndices.size());
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
  return ret;
}

inline void get_collnode_geom(const CollisionNode *node,
  const das::TBlock<void, const das::TTemporary<const das::TArray<uint16_t>>, const das::TTemporary<const das::TArray<Point4>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  das::Array indices;
  indices.data = (char *)node->indices.data();
  indices.size = uint32_t(node->indices.size());
  indices.capacity = indices.size;
  indices.lock = 1;
  indices.flags = 0;
  das::Array vertices;
  vertices.data = (char *)node->vertices.data();
  vertices.size = uint32_t(node->vertices.size());
  vertices.capacity = vertices.size;
  vertices.lock = 1;
  vertices.flags = 0;
  vec4f args[] = {das::cast<das::Array *>::from(&indices), das::cast<das::Array *>::from(&vertices)};
  context->invoke(block, args, nullptr, at);
}

inline bool collres_check_grid_available(const CollisionResource &collres, uint8_t behavior_filter)
{
  return collres.checkGridAvailable(behavior_filter);
}
} // namespace bind_dascript
