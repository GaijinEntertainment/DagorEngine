// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collisionUtils.h"
#include <util/dag_string.h>

bool add_verts_from_node(const CollisionResource &collres, const dag::ConstSpan<CollisionNode> &nodes, const String &node_name,
  dag::Vector<Point3_vec4> &verts)
{
  for (const auto &node : nodes)
  {
    if (node_name == collres.getNodeName(node.nodeIndex))
    {
      const TMatrix &nodeTm = collres.getNodeTm(node.nodeIndex);
      collres.iterateNodeVerts(node.nodeIndex, [&](int, vec4f v) {
        Point3_vec4 p;
        v_st(&p.x, v);
        verts.push_back(p * nodeTm);
      });
      return true;
    }
  }
  return false;
}

bool add_verts_and_indices_from_node(const CollisionResource &collres, const dag::ConstSpan<CollisionNode> &nodes,
  const String &node_name, dag::Vector<Point3> &verts, dag::Vector<uint16_t> &indices)
{
  for (const auto &node : nodes)
  {
    if (node_name == collres.getNodeName(node.nodeIndex))
    {
      if (collres.getNodeVertCount(node.nodeIndex) == 0)
        logerr("Node <%s> don't have vertices for merge. This node can be used only for box or sphere collision type creation.",
          node_name);
      const TMatrix &nodeTm = collres.getNodeTm(node.nodeIndex);
      const auto vertsSize = verts.size();
      collres.iterateNodeVerts(node.nodeIndex, [&](int, vec4f v) {
        Point3_vec4 p;
        v_st(&p.x, v);
        verts.push_back(p * nodeTm);
      });
      collres.iterateNodeFaces(node.nodeIndex, [&](int, uint16_t i0, uint16_t i1, uint16_t i2) {
        indices.push_back(i0 + vertsSize);
        indices.push_back(i1 + vertsSize);
        indices.push_back(i2 + vertsSize);
      });
      return true;
    }
  }
  return false;
}

bool add_verts_and_indices_from_node(const CollisionResource &collres, const dag::ConstSpan<CollisionNode> &nodes,
  const String &node_name, dag::Vector<float> &verts, dag::Vector<unsigned int> &indices)
{
  for (const auto &node : nodes)
  {
    if (node_name == collres.getNodeName(node.nodeIndex))
    {
      const TMatrix &nodeTm = collres.getNodeTm(node.nodeIndex);
      const auto vertsSize = verts.size() / 3;
      collres.iterateNodeVerts(node.nodeIndex, [&](int, vec4f v) {
        Point3_vec4 p;
        v_st(&p.x, v);
        Point3 vert = p * nodeTm;
        verts.push_back(vert.x);
        verts.push_back(vert.y);
        verts.push_back(vert.z);
      });
      collres.iterateNodeFaces(node.nodeIndex, [&](int, uint16_t i0, uint16_t i1, uint16_t i2) {
        indices.push_back(i0 + vertsSize);
        indices.push_back(i1 + vertsSize);
        indices.push_back(i2 + vertsSize);
      });
      return true;
    }
  }
  return false;
}
