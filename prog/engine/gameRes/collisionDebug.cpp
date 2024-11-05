// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <math/dag_geomTree.h>
#include <util/dag_bitArray.h>

void CollisionResource::drawDebug(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  const DebugDrawData &debug_data) const
{
#if DAGOR_DBGLEVEL > 0
  if (debug_data.drawBits & CRDD_NODES)
    for (const CollisionNode *meshNode = meshNodesHead; meshNode; meshNode = meshNode->nextNode)
    {
      if (!meshNode->geomNodeId && !(debug_data.drawBits & CRDD_NON_GEOM_TREE_NODES))
        continue;

      if (debug_data.drawMask && meshNode->nodeIndex < debug_data.drawMask->size() && !(*debug_data.drawMask)[meshNode->nodeIndex])
        continue;

      TMatrix tm;
      getCollisionNodeTm(meshNode, TMatrix::IDENT, geom_node_tree, tm);
      if (debug_data.localNodeTree)
        tm = instance_tm * tm;

      if (debug_data.shouldDrawText)
        add_debug_text_mark(tm.getcol(3), meshNode->name, -1, 0.f);

      set_cached_debug_lines_wtm(tm);
      for (unsigned int indexNo = 0; indexNo < meshNode->indices.size(); indexNo += 3)
      {
        const Point3 *points[3] = {
#define _P3(n) points[n] = &meshNode->vertices[meshNode->indices[indexNo + (n)]]
          _P3(0), _P3(1), _P3(2)
#undef _P3
        };
        draw_cached_debug_line(*points[0], *points[1], debug_data.color);
        draw_cached_debug_line(*points[1], *points[2], debug_data.color);
        draw_cached_debug_line(*points[2], *points[0], debug_data.color);
      }

      // draw_cached_debug_sphere(
      //   tm * meshNode->boundingSphere.c,
      //   meshNode->boundingSphere.r * scale,
      //   0xFF00FF00);

      // draw_cached_debug_box(instance_tm * meshNode->modelBBox,
      //     E3DCOLOR_MAKE(0,0,255,255));
    }

  set_cached_debug_lines_wtm(instance_tm);

  if (debug_data.drawBits & CRDD_BSPHERE)
  {
    float scale = instance_tm.getcol(1).length();
    Point3_vec4 bsphere;
    v_st(&bsphere.x, vBoundingSphere);
    float bsphere_rad = boundingSphereRad;
    if (debug_data.bsphereCNode && geom_node_tree)
    {
      set_cached_debug_lines_wtm(TMatrix::IDENT);
      vec4f v_sph = geom_node_tree->nodeWtmMulVec3p(debug_data.bsphereCNode, debug_data.bsphereOffset);
      v_st(&bsphere.x, v_sph);
      bsphere_rad *= v_extract_w(debug_data.bsphereOffset);
    }
    draw_cached_debug_sphere(bsphere, bsphere_rad * scale, E3DCOLOR_MAKE(255, 255, 255, 255));
    if (debug_data.bsphereCNode && geom_node_tree)
      set_cached_debug_lines_wtm(instance_tm);
  }

  if (debug_data.drawBits & CRDD_NODES)
  {
    for (const CollisionNode *boxNode = boxNodesHead; boxNode; boxNode = boxNode->nextNode)
      draw_cached_debug_box(boxNode->modelBBox, debug_data.color);

    for (const CollisionNode *sphereNode = sphereNodesHead; sphereNode; sphereNode = sphereNode->nextNode)
      draw_cached_debug_sphere(sphereNode->boundingSphere.c, sphereNode->boundingSphere.r, debug_data.color);

    for (const CollisionNode *capsuleNode = capsuleNodesHead; capsuleNode; capsuleNode = capsuleNode->nextNode)
      draw_cached_debug_capsule(capsuleNode->capsule, debug_data.color, TMatrix::IDENT);
  }
#else
  G_UNUSED(instance_tm);
  G_UNUSED(geom_node_tree);
  G_UNUSED(debug_data);
#endif
}
