//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point3;
class CollisionResource;

float calc_distance_to_coll_node(const CollisionResource &coll_res, int node_id, const Point3 &p, Point3 &out_contact,
  Point3 *out_dir = nullptr, Point3 *out_normal = nullptr);
