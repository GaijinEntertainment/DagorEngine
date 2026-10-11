// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameMath/collResUtils.h>

#include <gameMath/mathUtils.h>
#include <gameRes/dag_collisionResource.h>


float calc_distance_to_coll_node(const CollisionResource &coll_res, const GeomNodeTree *node_tree, const TMatrix &instance_tm,
  int node_id, const Point3 &p, Point3 *out_contact, Point3 *out_dir, Point3 *out_normal)
{
  const CollisionNode *node = coll_res.getNode(node_id);
  if (!node)
    return VERY_BIG_NUMBER;

  TMatrix nodeTm;
  coll_res.getCollisionNodeTm(node, instance_tm, node_tree, nodeTm);
  if (fabsf(nodeTm.det()) <= 1e-12f)
    return VERY_BIG_NUMBER;
  const Point3 pNode = inverse(nodeTm) * p;

  float dist = VERY_BIG_NUMBER;
  Point3 contact = Point3::ZERO;
  Point3 normal(0.f, 1.f, 0.f);
  coll_res.iterateNodeFacesVerts(node_id, [&](int, vec4f vv0, vec4f vv1, vec4f vv2) {
    Point3_vec4 corner0, corner1, corner2;
    v_st(&corner0.x, vv0);
    v_st(&corner1.x, vv1);
    v_st(&corner2.x, vv2);
    Point3 curContactPos;
    Point3 curNormal;
    float curDist = distance_to_triangle(pNode, corner0, corner1, corner2, curContactPos, curNormal);
    if (curDist < dist)
    {
      dist = curDist;
      contact = curContactPos;
      normal = curNormal;
    }
  });

  if (dist == VERY_BIG_NUMBER)
    return dist;

  contact = nodeTm * contact;
  normal = normalize(nodeTm % normal);

  if (out_contact)
    (*out_contact) = contact;

  if (out_dir)
  {
    (*out_dir) = contact - p;
    if (out_dir->lengthSq() < VERY_SMALL_NUMBER)
      (*out_dir) = -normal;
  }

  if (out_normal)
    (*out_normal) = normal;

  return length(contact - p);
}
