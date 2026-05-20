// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_bvhSerialization.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daSWRT/swBVH.h>
#include <generic/dag_tab.h>
#include <debug/dag_assert.h>
#include <gameRes/dag_collisionResource.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/swBLASLeafDefs.hlsli>

int CollisionGeometryFeeder::buildSwrtBLASFromCollisionResource(RenderSWRT &swrt, const CollisionResource &coll_res,
  const PhysMatFilter &node_filter, float dim_as_box_dist, BuildSwrtBLASScratch &scratch)
{
  // Clear every caller-owned vector up front so both success and failure paths leave scratch in a
  // known-empty state; the header promises this contract.
  scratch.verts.clear();
  scratch.indices.clear();
  scratch.primBoxes.clear();

  const auto allNodes = coll_res.getAllNodes();

  auto nodeIsEligible = [&](const CollisionNode *node) {
    if (!node || !node->checkBehaviorFlags(CollisionNode::TRACEABLE))
      return false;
    // Require both verts AND indices: a node with indices but zero verts is malformed and would
    // make the indices loop below reference the previous node's vertex base (firstVertex does not
    // advance for the zero-vert node, but we still push its indices).
    if (coll_res.getNodeVertCount((int)node->nodeIndex) == 0 || coll_res.getNodeIndexCount((int)node->nodeIndex) == 0)
      return false;
    if (node->type != COLLISION_NODE_TYPE_MESH && node->type != COLLISION_NODE_TYPE_CONVEX)
      return false;
    if (node_filter && !node_filter(node->physMatId))
      return false;
    return true;
  };

  int totalVxCnt = 0, totalIdxCnt = 0;
  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!nodeIsEligible(node))
      continue;
    totalVxCnt += coll_res.getNodeVertCount(ni);
    totalIdxCnt += coll_res.getNodeIndexCount(ni);
  }
  if (totalIdxCnt == 0 || totalVxCnt == 0)
    return -1;

  scratch.verts.reserve(totalVxCnt);
  scratch.indices.reserve(totalIdxCnt);

  uint32_t firstVertex = 0;
  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!nodeIsEligible(node))
      continue;

    const int vertCount = coll_res.getNodeVertCount(ni);
    const bool needsTransform = (node->flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) != CollisionNode::IDENT;
    if (needsTransform)
    {
      mat44f nodeTm;
      v_mat44_make_from_43cu_unsafe(nodeTm, coll_res.getNodeTm(ni)[0]);
      coll_res.iterateNodeVerts(ni, [&](int, vec4f v) { v_st(&scratch.verts.push_back().x, v_mat44_mul_vec3p(nodeTm, v)); });
    }
    else
    {
      coll_res.iterateNodeVerts(ni, [&](int, vec4f v) { v_st(&scratch.verts.push_back().x, v); });
    }

    const uint32_t vertOffset = firstVertex;
    coll_res.iterateNodeFaces(ni, [&](int, uint16_t i0, uint16_t i1, uint16_t i2) {
      scratch.indices.push_back((uint32_t)i0 + vertOffset);
      scratch.indices.push_back((uint32_t)i1 + vertOffset);
      scratch.indices.push_back((uint32_t)i2 + vertOffset);
    });
    firstVertex += vertCount;
  }

  // BLAS leaf format packs vertex offsets in 10 bits each (max spread QUAD_O*_MAX = 1023,
  // see swBLASLeafDefs.hlsli). Triangles whose vertex indices span more than that can't
  // be represented in one leaf and would silently truncate to a wrapped offset, producing
  // random triangle connectivity at runtime. For each such triangle, duplicate its three
  // verts into a fresh compact range so its indices become (base, base+1, base+2) --
  // spread = 2, always fits.
  // Per-triangle spread is bounded by (vertCount - 1), so meshes at or below the limit
  // can't overflow -- skip the scan entirely. Covers the bulk of small RI pools.
  if (scratch.verts.size() > QUAD_O1_MAX + 1)
  {
    const int idxCount = (int)scratch.indices.size();
    for (int t = 0; t < idxCount; t += 3)
    {
      uint32_t &i0 = scratch.indices[t + 0];
      uint32_t &i1 = scratch.indices[t + 1];
      uint32_t &i2 = scratch.indices[t + 2];
      const uint32_t mn = min(min(i0, i1), i2);
      const uint32_t mx = max(max(i0, i1), i2);
      if (mx - mn > QUAD_O1_MAX)
      {
        // Copy to locals first: push_back may reallocate scratch.verts, after which a
        // reference into the old storage (scratch.verts[iN]) would dangle.
        const Point3_vec4 v0 = scratch.verts[i0];
        const Point3_vec4 v1 = scratch.verts[i1];
        const Point3_vec4 v2 = scratch.verts[i2];
        const uint32_t base = (uint32_t)scratch.verts.size();
        scratch.verts.push_back(v0);
        scratch.verts.push_back(v1);
        scratch.verts.push_back(v2);
        i0 = base;
        i1 = base + 1;
        i2 = base + 2;
      }
    }
  }

  const vec4f *vertsPtr = (const vec4f *)scratch.verts.data();
  const int vertCountTotal = (int)scratch.verts.size();
  const int idxCountTotal = (int)scratch.indices.size();
  bbox3f box = build_bvh::calcBox(vertsPtr, vertCountTotal);

  if (build_bvh::checkIfIsBox(scratch.indices.data(), idxCountTotal, vertsPtr, vertCountTotal, box))
    return swrt.addBoxModel(box.bmin, box.bmax);

  const int faceCount = idxCountTotal / 3;
  Tab<build_bvh::QuadPrim> prims;
  int quadCount = 0, singleCount = 0;
  build_bvh::buildQuadPrims(prims, quadCount, singleCount, scratch.indices.data(), faceCount, vertsPtr);

  scratch.primBoxes.clear();
  scratch.primBoxes.resize(prims.size());
  build_bvh::addQuadPrimitivesAABBList(scratch.primBoxes.data(), prims.data(), (int)prims.size(), vertsPtr);

  Tab<bbox3f> nodes;
  int maxDepth = 0;
  const int root = build_bvh::create_bvh_node_sah(nodes, scratch.primBoxes.data(), (uint32_t)prims.size(), 4, maxDepth);

  dag::Vector<uint8_t> blasBytes;
  build_bvh::writeQuadBLAS(blasBytes, box, nodes.data(), root, prims.data(), (int)prims.size(),
    reinterpret_cast<const uint8_t *>(scratch.verts.data()), (int)sizeof(Point3_vec4), vertCountTotal);

  return swrt.addPreBuiltModel(box, eastl::move(blasBytes), vertCountTotal, (int)nodes.size(), (int)prims.size(), dim_as_box_dist);
}
