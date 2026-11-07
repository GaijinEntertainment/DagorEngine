// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameRes/dag_collisionResource.h>
#include <libTools/collision/collisionNodeColors.h>

bool add_verts_from_node(const CollisionResource &collres, const dag::ConstSpan<CollisionNode> &nodes, const String &node_name,
  dag::Vector<Point3_vec4> &verts);

bool add_verts_and_indices_from_node(const CollisionResource &collres, const dag::ConstSpan<CollisionNode> &nodes,
  const String &node_name, dag::Vector<Point3> &verts, dag::Vector<uint16_t> &indices);

bool add_verts_and_indices_from_node(const CollisionResource &collres, const dag::ConstSpan<CollisionNode> &nodes,
  const String &node_name, dag::Vector<float> &verts, dag::Vector<unsigned int> &indices);
