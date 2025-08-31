// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/collResUtils.h>

#include <gameMath/mathUtils.h>
#include <gameRes/dag_collisionResource.h>


float calc_distance_to_coll_node(const Point3 &p, const CollisionNode *cnode, Point3 &out_contact, Point3 *out_dir, Point3 *out_normal)
{
  float dist = VERY_BIG_NUMBER;

  Point3 normal(0.f, 1.f, 0.f);
  const uint16_t *__restrict cur = cnode->indices.data();
  const uint16_t *__restrict end = cur + cnode->indices.size();
  for (; cur < end;)
  {
    Point3 corner0 = cnode->vertices[*(cur++)];
    Point3 corner1 = cnode->vertices[*(cur++)];
    Point3 corner2 = cnode->vertices[*(cur++)];
    Point3 curContactPos;
    Point3 curNormal;
    float curDist = distance_to_triangle(p, corner0, corner1, corner2, curContactPos, curNormal);
    if (curDist < dist)
    {
      dist = curDist;
      out_contact = curContactPos;
      normal = curNormal;
    }
  }

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
