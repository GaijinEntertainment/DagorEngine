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
    for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
    {
      const CollisionNode *meshNode = &allNodesList[mi];
      if (!meshNode->geomNodeId && !(debug_data.drawBits & CRDD_NON_GEOM_TREE_NODES))
        continue;

      if (debug_data.drawMask && meshNode->nodeIndex < debug_data.drawMask->size() && !(*debug_data.drawMask)[meshNode->nodeIndex])
        continue;

      TMatrix tm;
      getCollisionNodeTm(meshNode, TMatrix::IDENT, geom_node_tree, tm);
      if (debug_data.localNodeTree)
        tm = instance_tm * tm;

      if (debug_data.shouldDrawText)
        add_debug_text_mark(tm.getcol(3), getNodeName(meshNode->nodeIndex), -1, 0.f);

      set_cached_debug_lines_wtm(tm);
      // Consume faces through the node iterator instead of indexing meshVertsBase/meshIndicesBase
      // directly, so this keeps working once a node's vertex/index slices may be dropped (its
      // verticesOfs/indicesOfs no longer addressing ownVertices/ownIndices). For directly-stored
      // nodes the iterator reads the same ownVertices/ownIndices, so behavior is unchanged.
      iterateNodeFacesVerts(meshNode->nodeIndex, [&](int, vec4f v0, vec4f v1, vec4f v2) {
        Point3_vec4 p0, p1, p2;
        v_st(&p0.x, v0);
        v_st(&p1.x, v1);
        v_st(&p2.x, v2);
        draw_cached_debug_line(p0, p1, debug_data.color);
        draw_cached_debug_line(p1, p2, debug_data.color);
        draw_cached_debug_line(p2, p0, debug_data.color);
      });

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
    for (uint16_t bi = boxNodesHead; bi != CollisionNode::INVALID_IDX; bi = allNodesList[bi].nextNode)
      draw_cached_debug_box(allNodesList[bi].modelBBox, debug_data.color);

    for (uint16_t si = sphereNodesHead; si != CollisionNode::INVALID_IDX; si = allNodesList[si].nextNode)
    {
      const CollisionNode &sphereNode = allNodesList[si];
      draw_cached_debug_sphere(sphereNode.boundingSphere.c, sphereNode.boundingSphere.r, debug_data.color);
    }

    for (uint16_t ci = capsuleNodesHead; ci != CollisionNode::INVALID_IDX; ci = allNodesList[ci].nextNode)
      draw_cached_debug_capsule(capsules[allNodesList[ci].capsuleIndex], debug_data.color, TMatrix::IDENT);
  }
#else
  G_UNUSED(instance_tm);
  G_UNUSED(geom_node_tree);
  G_UNUSED(debug_data);
#endif
}
