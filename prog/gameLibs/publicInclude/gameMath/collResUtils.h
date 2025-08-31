//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point3;
struct CollisionNode;

float calc_distance_to_coll_node(const Point3 &p, const CollisionNode *cnode, Point3 &out_contact, Point3 *out_dir = nullptr,
  Point3 *out_normal = nullptr);
