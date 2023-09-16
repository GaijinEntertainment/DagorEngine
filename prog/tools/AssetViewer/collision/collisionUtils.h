#pragma once
#include <gameRes/dag_collisionResource.h>

static unsigned int colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFFFF00FF, 0xFF00FFFF, 0xFF800000, 0xFF008000,
  0xFF000080, 0xFF808000, 0xFF800080, 0xFF008080};

bool add_verts_from_node(const dag::ConstSpan<CollisionNode> &nodes, const String &node_name, dag::Vector<Point3_vec4> &verts);

bool add_verts_and_indices_from_node(const dag::ConstSpan<CollisionNode> &nodes, const String &node_name, dag::Vector<Point3> &verts,
  dag::Vector<uint16_t> &indices);

bool add_verts_and_indices_from_node(const dag::ConstSpan<CollisionNode> &nodes, const String &node_name, dag::Vector<float> &verts,
  dag::Vector<unsigned int> &indices);
