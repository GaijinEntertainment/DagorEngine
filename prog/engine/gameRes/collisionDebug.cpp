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
      // Consume faces through the node iterator instead of indexing raw vertex/index arrays directly,
      // so this keeps working for nodes whose slices were dropped (BLAS-resident) or never existed as
      // raw arrays (owning resources hold no raw verts). The iterator decodes the same per-node chunk
      // / grid vert21 data the traces use, so behavior is consistent.
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
    // draw stored geometry through the geometry transform: eps-IDENT prims keep the exporter
    // bake, so composing the raw placement (getNodeTm) would double-apply the sub-eps T
    auto primDrawTm = [&](int node_index) {
      TMatrix tm;
      v_mat_43cu_from_mat44(tm.array, defaultInstance.getNodeGeometryTm(node_index));
      return instance_tm * tm;
    };
    for (uint16_t bi = boxNodesHead; bi != CollisionNode::INVALID_IDX; bi = allNodesList[bi].nextNode)
    {
      set_cached_debug_lines_wtm(primDrawTm(allNodesList[bi].nodeIndex));
      draw_cached_debug_box(allNodesList[bi].modelBBox, debug_data.color);
    }

    for (uint16_t si = sphereNodesHead; si != CollisionNode::INVALID_IDX; si = allNodesList[si].nextNode)
    {
      const CollisionNode &sphereNode = allNodesList[si];
      set_cached_debug_lines_wtm(primDrawTm(sphereNode.nodeIndex));
      draw_cached_debug_sphere(sphereNode.boundingSphere.c, sphereNode.boundingSphere.r, debug_data.color);
    }

    for (uint16_t ci = capsuleNodesHead; ci != CollisionNode::INVALID_IDX; ci = allNodesList[ci].nextNode)
    {
      // draw the node-local capsule under the full composed wtm: Capsule::transform collapses a
      // non-uniform pose into one radius, while the wtm keeps the true anisotropic shape
      set_cached_debug_lines_wtm(instance_tm * getNodeTm(allNodesList[ci].nodeIndex));
      draw_cached_debug_capsule(capsules[allNodesList[ci].capsuleIndex], debug_data.color, TMatrix::IDENT);
    }
  }
#else
  G_UNUSED(instance_tm);
  G_UNUSED(geom_node_tree);
  G_UNUSED(debug_data);
#endif
}

void CollisionResource::drawDebug(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const DebugDrawData &debug_data) const
{
#if DAGOR_DBGLEVEL > 0
  // Stored-pose debug draw: tree-backed instances fall back to the default pose here.
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "drawDebug"); // never null
  // instance poses are resource-local by construction, so instance_tm always applies (the
  // localNodeTree distinction only exists for world-space GeomNodeTrees)
  auto nodeDrawTm = [&](const CollisionNode &node) {
    TMatrix tm;
    getCollisionNodeTm(&node, TMatrix::IDENT, *inst, tm);
    return instance_tm * tm;
  };
  auto nodeGeometryDrawTm = [&](const CollisionNode &node) {
    TMatrix tm;
    v_mat_43cu_from_mat44(tm.array, inst->getNodeGeometryTm(node.nodeIndex));
    return instance_tm * tm;
  };

  if (debug_data.drawBits & CRDD_NODES)
    for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
    {
      const CollisionNode *meshNode = &allNodesList[mi];
      if (!inst->isNodeEnabled(meshNode->nodeIndex))
        continue;
      if (debug_data.drawMask && meshNode->nodeIndex < debug_data.drawMask->size() && !(*debug_data.drawMask)[meshNode->nodeIndex])
        continue;

      TMatrix tm = nodeDrawTm(*meshNode);
      if (debug_data.shouldDrawText)
        add_debug_text_mark(tm.getcol(3), getNodeName(meshNode->nodeIndex), -1, 0.f);

      set_cached_debug_lines_wtm(tm);
      iterateNodeFacesVerts(meshNode->nodeIndex, [&](int, vec4f v0, vec4f v1, vec4f v2) {
        Point3_vec4 p0, p1, p2;
        v_st(&p0.x, v0);
        v_st(&p1.x, v1);
        v_st(&p2.x, v2);
        draw_cached_debug_line(p0, p1, debug_data.color);
        draw_cached_debug_line(p1, p2, debug_data.color);
        draw_cached_debug_line(p2, p0, debug_data.color);
      });
    }

  set_cached_debug_lines_wtm(instance_tm);

  if (debug_data.drawBits & CRDD_BSPHERE)
  {
    // debug_data.bsphereCNode/bsphereOffset apply to the GeomNodeTree form only; here the center
    // follows the resource's bsphereCenterNode
    mat44f identTm;
    v_mat44_ident(identTm);
    Point3_vec4 bsphere;
    // Selected center node when set, else the accessor's bind-center fallback -- resource-local.
    // *inst, not instance: an identity tm must not lazily refresh a tree-backed pose.
    v_st(&bsphere.x, getWorldBoundingSphere(identTm, *inst));
    // resource-local radius: the cached WTM (instance_tm) already scales the generated points.
    // A posed instance's nodes can leave the bind sphere -- this is a DISPLAY, not the accessor
    // contract, so recenter on and cover the posed bounds when no center node is selected.
    float debugRad = boundingSphereRad;
    if (DAGOR_UNLIKELY(inst->isPosedSinceBind()))
    {
      const bbox3f rootBox = inst->getRootBBox();
      if (!inst->hasBsphereCenterLocal)
        v_st(&bsphere.x, v_bbox3_center(rootBox));
      const vec4f vC = v_ldu(&bsphere.x);
      // Farthest of the eight corners from an off-center vC: componentwise max of the corner offsets.
      const vec3f far3 = v_max(v_abs(v_sub(rootBox.bmax, vC)), v_abs(v_sub(rootBox.bmin, vC)));
      debugRad = max(debugRad, v_extract_x(v_length3_x(far3)));
    }
    draw_cached_debug_sphere(bsphere, debugRad, E3DCOLOR_MAKE(255, 255, 255, 255));
  }

  if (debug_data.drawBits & CRDD_NODES)
  {
    for (uint16_t bi = boxNodesHead; bi != CollisionNode::INVALID_IDX; bi = allNodesList[bi].nextNode)
    {
      const CollisionNode &boxNode = allNodesList[bi];
      if (!inst->isNodeEnabled(boxNode.nodeIndex))
        continue;
      set_cached_debug_lines_wtm(nodeGeometryDrawTm(boxNode));
      draw_cached_debug_box(boxNode.modelBBox, debug_data.color);
    }

    for (uint16_t si = sphereNodesHead; si != CollisionNode::INVALID_IDX; si = allNodesList[si].nextNode)
    {
      const CollisionNode &sphereNode = allNodesList[si];
      if (!inst->isNodeEnabled(sphereNode.nodeIndex))
        continue;
      set_cached_debug_lines_wtm(nodeGeometryDrawTm(sphereNode));
      draw_cached_debug_sphere(sphereNode.boundingSphere.c, sphereNode.boundingSphere.r, debug_data.color);
    }

    for (uint16_t ci = capsuleNodesHead; ci != CollisionNode::INVALID_IDX; ci = allNodesList[ci].nextNode)
    {
      const CollisionNode &capsuleNode = allNodesList[ci];
      if (!inst->isNodeEnabled(capsuleNode.nodeIndex))
        continue;
      TMatrix capTm = TMatrix::IDENT;
      getCollisionNodeTm(&capsuleNode, TMatrix::IDENT, *inst, capTm); // capsules[] is node-local: T places it
      // draw the node-local capsule under the full composed wtm: Capsule::transform collapses a
      // non-uniform pose into one radius, while the wtm keeps the true anisotropic shape
      set_cached_debug_lines_wtm(instance_tm * capTm);
      draw_cached_debug_capsule(capsules[capsuleNode.capsuleIndex], debug_data.color, TMatrix::IDENT);
    }
  }
#else
  G_UNUSED(instance_tm);
  G_UNUSED(instance);
  G_UNUSED(debug_data);
#endif
}
