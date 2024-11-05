// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collisionUtils.h"
#include <util/dag_string.h>

bool add_verts_from_node(const dag::ConstSpan<CollisionNode> &nodes, const String &node_name, dag::Vector<Point3_vec4> &verts)
{
  for (const auto &node : nodes)
  {
    if (node_name == node.name.c_str())
    {
      for (const auto &vertex : node.vertices)
      {
        verts.push_back(vertex * node.tm);
      }
      return true;
    }
  }
  return false;
}

bool add_verts_and_indices_from_node(const dag::ConstSpan<CollisionNode> &nodes, const String &node_name, dag::Vector<Point3> &verts,
  dag::Vector<uint16_t> &indices)
{
  for (const auto &node : nodes)
  {
    if (node_name == node.name.c_str())
    {
      if (node.vertices.empty())
        logerr("Node <%s> don't have vertices for merge. This node can be used only for box or sphere collision type creation.",
          node_name);
      const auto vertsSize = verts.size();
      for (const auto &vertex : node.vertices)
      {
        verts.push_back(vertex * node.tm);
      }
      for (const auto index : node.indices)
      {
        indices.push_back(index + vertsSize);
      }
      return true;
    }
  }
  return false;
}

bool add_verts_and_indices_from_node(const dag::ConstSpan<CollisionNode> &nodes, const String &node_name, dag::Vector<float> &verts,
  dag::Vector<unsigned int> &indices)
{
  for (const auto &node : nodes)
  {
    if (node_name == node.name.c_str())
    {
      const auto vertsSize = verts.size() / 3;
      for (const auto &vertex : node.vertices)
      {
        Point3 vert = vertex * node.tm;
        verts.push_back(vert.x);
        verts.push_back(vert.y);
        verts.push_back(vert.z);
      }
      for (const auto index : node.indices)
      {
        indices.push_back(index + vertsSize);
      }
      return true;
    }
  }
  return false;
}
