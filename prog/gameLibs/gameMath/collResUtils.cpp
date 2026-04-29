// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/collResUtils.h>

#include <gameMath/mathUtils.h>
#include <gameRes/dag_collisionResource.h>


float calc_distance_to_coll_node(const CollisionResource &coll_res, int node_id, const Point3 &p, Point3 &out_contact, Point3 *out_dir,
  Point3 *out_normal)
{
  float dist = VERY_BIG_NUMBER;

  Point3 normal(0.f, 1.f, 0.f);
  coll_res.iterateNodeFacesVerts(node_id, [&](int, vec4f vv0, vec4f vv1, vec4f vv2) {
    Point3_vec4 corner0, corner1, corner2;
    v_st(&corner0.x, vv0);
    v_st(&corner1.x, vv1);
    v_st(&corner2.x, vv2);
    Point3 curContactPos;
    Point3 curNormal;
    float curDist = distance_to_triangle(p, corner0, corner1, corner2, curContactPos, curNormal);
    if (curDist < dist)
    {
      dist = curDist;
      out_contact = curContactPos;
      normal = curNormal;
    }
  });

  if (out_dir)
  {
    (*out_dir) = out_contact - p;
    if (out_dir->lengthSq() < VERY_SMALL_NUMBER)
      (*out_dir) = -normal;
  }

  if (out_normal)
    (*out_normal) = normal;

  return dist;
}
