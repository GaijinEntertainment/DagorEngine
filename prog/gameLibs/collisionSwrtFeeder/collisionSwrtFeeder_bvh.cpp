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
