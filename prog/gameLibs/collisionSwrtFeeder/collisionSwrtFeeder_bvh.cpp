// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_bvhSerialization.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_bvhReencode.h>
#include <daBVH/dag_swBLAS_leaf.h>        // RayData::unpackVert21; no stackless traversal here
#include <daBVH/dag_swBLAS_soa4Convert.h> // soa4::buildStackless (grid stores the SoA4 CPU layout)
#include <daSWRT/swBVH.h>
#include <daSWRT/swBLASBoxResemblance.h>
#include <generic/dag_tab.h>
#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_mathBase.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/swBLASLeafDefs.hlsli>

// Fast path: emit the pre-built collision grid BLAS as a daSWRT BuiltBLAS, skipping the
// triangle-soup flatten + SAH rebuild: convert the grid's SoA4 tree to the stackless GPU layout
// (byte-identical vert region), then reencode boxes and verts to the same-size GPU formats in
// place (details at the loops below). The caller gates on fp16 (the 12 B float3 GPU format widens
// verts and takes the soup path) and guarantees the grid's resident-node set equals the eligible
// set (all nodes IDENT, so no TM to apply and no geometry dropped).
static int buildSwrtBLAS_gridFast(RenderSWRT &swrt, const CollisionResource::Grid &grid, float dim_as_box_min, float dim_as_box_max)
{
  daSWRT::BuiltBLAS built;
  // Verts/tree were quantized against grid.blasBBox; the SWRT model box must be that same box so the
  // [0,65535] -> [-1,1] remap below stays consistent with the runtime world -> BLAS mapping.
  built.box = grid.blasBBox;
  // built.vertsFp16 stays at the BuiltBLAS default (true): this fast path's in-place reencode emits 8 B
  // fp16 verts, and the caller gates it on blasVertsFp16.

  const soa4::StacklessResult rt = soa4::buildStackless(grid.blasData.data(), grid.blasRootRef, (int)grid.blasVertsOfs(),
    (int)(grid.blasData.size() - grid.blasVertsOfs()), built.data);
  if (!rt.valid())
  {
    // Structurally impossible for a validly built grid (the conversion round-trips 1:1); degrade
    // loudly to "no SWRT model for this resource", never a corrupt upload.
    logerr("swrtFeeder: SoA4->stackless conversion failed for grid BLAS; no SWRT model");
    return -1;
  }
  const int treeBytes = rt.treeBytes;
  const int srcVertsOfs = rt.vertsOfs;
  const int vertCount = (int)((built.data.size() - (size_t)srcVertsOfs) / 8u);
  G_ASSERT(treeBytes > 0 && vertCount > 0);
  built.treeBytes = (uint32_t)treeBytes;

  // Score box-resemblance on the emitted stackless tree (still Quantized16), before the boxes are
  // reencoded.
  built.dimAsBoxDist = dim_as_box_max;
  if (dim_as_box_max > dim_as_box_min)
  {
    float boxLike =
      daSWRT::computeBlasBoxResemblanceVoxel(built.data.data(), 0, treeBytes, built.box, daSWRT::BlasBoxEncoding::Quantized16);
    boxLike = powf(boxLike, 1.5f);
    built.dimAsBoxDist = lerp(dim_as_box_max, dim_as_box_min, boxLike);
  }

  uint8_t *dst = built.data.data();
  for (int ofs = 0; ofs < treeBytes;)
  {
    uint32_t encWord;
    memcpy(&encWord, dst + ofs + 12, sizeof(uint32_t)); // QUAD_LEAF_FLAG + quad/single encoding live at +12
    build_bvh::reencodeBoxNodeToFP16(dst + ofs);        // box uint16 [0,65535] -> FP16 [-1,1]; preserves +12
    ofs += (encWord & QUAD_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE;
  }

  // vert21 -> fp16 in place: unpack to box space, map f/32767.5 - 1 to [-1,1], repack. Same 8 B slot, so
  // the read completes (into a register) before the write -- no aliasing.
  const vec4f vertToNorm = v_splats(1.0f / 32767.5f);
  const vec4f vertBias = v_splats(-1.0f);
  uint8_t *verts = dst + srcVertsOfs;
  for (int v = 0; v < vertCount; ++v)
  {
    vec3f n = v_madd(RayData::unpackVert21(verts + (size_t)v * 8), vertToNorm, vertBias);
    build_bvh::writeGpuBlasVert(verts + (size_t)v * 8, n, /*fp16*/ true);
  }

  return swrt.addBuiltModel(eastl::move(built));
}

int CollisionGeometryFeeder::buildSwrtBLASFromCollisionResource(RenderSWRT &swrt, const CollisionResource &coll_res,
  const PhysMatFilter &node_filter, float dim_as_box_min, float dim_as_box_max, BuildSwrtBLASScratch &scratch)
{
  // Clear every caller-owned vector up front so both success and failure paths leave scratch in a
  // known-empty state; the header promises this contract.
  scratch.verts.clear();
  scratch.indices.clear();
  scratch.orderedVerts.clear();
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

  // Whole-grid fast path. The TRACEABLE combined-per-behavior BLAS already covers exactly the IDENT
  // mesh nodes of that behavior. When every node this feeder would emit is precisely that grid's
  // resident set, re-encode the grid BLAS in one pass (buildSwrtBLAS_gridFast) and skip the soup +
  // SAH rebuild. Bail to the soup path when:
  //   - the grid wasn't built (small/SOLID/non-IDENT-only resource -- hasBlas is false);
  //   - any eligible node is CONVEX or non-IDENT mesh (never in the BLAS, so grid bytes would miss
  //     geometry); or
  //   - node_filter carves out a strict subset of the grid's nodes (e.g. skyquake filters out
  //     transparent phys-mats) -- eligibleCount trails blasNodeRanges and a partial tree can't be
  //     reproduced without re-splicing leaves; or
  //   - the GPU vertex format is 12 B float3 (swrt.blasVertsFp16 == false) -- the in-place reencode
  //     only matches the 8 B fp16 layout, so the wider-stride case takes the soup path.
  // SOLID needs no special test: a single SOLID node in TRACEABLE makes Grid::buildBLAS abandon the
  // whole grid, so hasBlas already excludes that.
  {
    const CollisionResource::Grid &grid = coll_res.getBlasGrid(CollisionNode::TRACEABLE);
    if (!grid.blasData.empty())
    {
      uint32_t eligibleCount = 0;
      bool allEligibleInGrid = true;
      for (int ni = 0, ne = (int)allNodes.size(); ni < ne && allEligibleInGrid; ++ni)
      {
        const CollisionNode *node = coll_res.getNode(ni);
        if (!nodeIsEligible(node))
          continue;
        ++eligibleCount;
        // Gate on blasNodeRanges membership, NOT residency: residency only says whose vert21
        // array node->verticesOfs indexes (a TRACEABLE|PHYS_COLLIDABLE node is stamped resident in
        // the COLLIDABLE grid -- see getBlasGridForResidentNode). A node in this grid's ranges has
        // its triangles in THIS grid's BLAS bytes regardless of vert span, which is all gridFast
        // re-encodes.
        bool inGrid = false;
        for (const auto &nr : grid.blasNodeRanges)
          if (nr.nodeIndex == node->nodeIndex)
          {
            inGrid = true;
            break;
          }
        allEligibleInGrid &= inGrid;
      }
      if (allEligibleInGrid && eligibleCount > 0 && eligibleCount == grid.blasNodeRanges.size() && swrt.blasVertsFp16)
        return buildSwrtBLAS_gridFast(swrt, grid, dim_as_box_min, dim_as_box_max);
    }
  }

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
    coll_res.iterateNodeFaces(ni, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
      scratch.indices.push_back(i0 + vertOffset);
      scratch.indices.push_back(i1 + vertOffset);
      scratch.indices.push_back(i2 + vertOffset);
    });
    firstVertex += vertCount;
  }

  // Box fast path on the raw gather, before the SAH reorder: a box resource is emitted as an analytic
  // box, so leafOrderVertexFetch's triangle-box + SAH-order work would be built only to be discarded.
  // checkIfIsBox is vertex-order independent and a genuine box gathers exactly its 8 verts, so the
  // verdict is identical pre- and post-reorder.
  {
    const bbox3f rawBox = build_bvh::calcBox((const vec4f *)scratch.verts.data(), (int)scratch.verts.size());
    if (build_bvh::checkIfIsBox(scratch.indices.data(), (int)scratch.indices.size(), (const vec4f *)scratch.verts.data(),
          (int)scratch.verts.size(), rawBox))
      return swrt.addBoxModel(rawBox.bmin, rawBox.bmax);
  }

  // SAH-leaf-order renumber + shared window-block over-spread dup (build_bvh, see dag_bvhBuild.h).
  // scratch.orderedVerts becomes the BLAS vertex array; scratch.indices is rewritten to index it.
  build_bvh::leafOrderVertexFetch(scratch.indices.data(), (unsigned)scratch.indices.size(), (const vec4f *)scratch.verts.data(),
    (unsigned)scratch.verts.size(), scratch.orderedVerts);

  const vec4f *vertsPtr = scratch.orderedVerts.data();
  const int vertCountTotal = (int)scratch.orderedVerts.size();
  const int idxCountTotal = (int)scratch.indices.size();
  bbox3f box = build_bvh::calcBox(vertsPtr, vertCountTotal);

  const int faceCount = idxCountTotal / 3;
  Tab<build_bvh::QuadPrim> prims;
  int quadCount = 0, singleCount = 0;
  build_bvh::buildQuadPrims(prims, quadCount, singleCount, scratch.indices.data(), faceCount, vertsPtr);
  // buildQuadPrims drops duplicate-index (zero-area) faces; if every gathered face is degenerate it
  // yields no prims. writeQuadBLAS would emit nothing and the treeBytes math below would underflow, so
  // report no model (same as the empty-input guard above).
  if (prims.empty())
    return -1;

  // Pair into double-quad leaves (4 tris/leaf). This soup BLAS feeds GPU RT only (no per-node
  // tri_ref / filtering), so pairing is unconstrained (vert_group = nullptr).
  dag::Vector<build_bvh::DoubleQuadPrim> dqPrims;
  build_bvh::buildDoubleQuadPrims(dqPrims, prims.data(), (int)prims.size(), vertsPtr);

  scratch.primBoxes.clear();
  scratch.primBoxes.resize(dqPrims.size());
  build_bvh::addDoubleQuadPrimitivesAABBList(scratch.primBoxes.data(), dqPrims.data(), (int)dqPrims.size(), vertsPtr);

  Tab<bbox3f> nodes;
  int maxDepth = 0;
  const int root = build_bvh::create_bvh_node_sah(nodes, scratch.primBoxes.data(), (uint32_t)dqPrims.size(), 4, maxDepth);

  dag::Vector<uint8_t> blasBytes;
  const bool builtBlas = build_bvh::writeDoubleQuadBLAS(blasBytes, box, nodes.data(), root, dqPrims.data(), (int)dqPrims.size(),
    reinterpret_cast<const uint8_t *>(vertsPtr), (int)sizeof(vec4f), vertCountTotal);
  if (!builtBlas) // vert span overflowed the unsigned 24-bit leaf base; drop this model's SWRT BLAS
    return -1;

  // Score box-resemblance on the SWRT BuiltBLAS itself (FP16-encoded tree)
  // if its already a box, we have early exit above (build_bvh::checkIfIsBox)
  float dimAsBoxDist = dim_as_box_max;
  if (dim_as_box_max > dim_as_box_min)
  {
    const int treeBytes = (int)blasBytes.size() - vertCountTotal * 12;
    float boxLike = daSWRT::computeBlasBoxResemblanceVoxel(blasBytes.data(), 0, treeBytes, box, daSWRT::BlasBoxEncoding::Quantized16);
    boxLike = powf(boxLike, 1.5f);
    dimAsBoxDist = lerp(dim_as_box_max, dim_as_box_min, boxLike);
  }

  return swrt.addPreBuiltModel(box, eastl::move(blasBytes), vertCountTotal, (int)nodes.size(), (int)dqPrims.size(), dimAsBoxDist);
}
