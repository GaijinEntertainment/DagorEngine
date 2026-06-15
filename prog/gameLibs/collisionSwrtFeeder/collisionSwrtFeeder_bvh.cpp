// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_bvhSerialization.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_bvhReencode.h>
#include <daBVH/dag_swBLAS_ray.h>
#include <daSWRT/swBVH.h>
#include <daSWRT/swBLASBoxResemblance.h>
#include <generic/dag_tab.h>
#include <debug/dag_assert.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_mathBase.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/swBLASLeafDefs.hlsli>

// Fast path: re-encode the whole combined-per-behavior collision BLAS into a daSWRT BuiltBLAS,
// skipping the triangle-soup flatten + daBVH SAH rebuild. The grid already holds a daBVH quad-BLAS
// over exactly the IDENT mesh nodes of its behavior, in (almost) the SWRT BuiltBLAS layout:
// internal-node headers (16 B packed bbox + skip/encoding word) are byte-identical, leaves carry a
// trailing 4 B relOfs. Two things differ: vertex stride (collision packs vert21 at 8 B/vert, SWRT
// stores float3 at 12 B/vert) and encoding (collision keeps quantized uint16 [0,65535], SWRT wants
// FP16 [-1,1]). So we copy the tree re-deriving each leaf's relOfs for the wider stride, expand
// vert21 -> float3 in the same [0,65535] frame, and let reencodeQuadBlasToFP16 map node boxes and
// verts into SWRT's representation in one pass. The caller guarantees the grid's resident-node set
// equals the feeder's eligible set, so the whole tree is exactly SWRT's model (all nodes IDENT ->
// resource-local == node-local, no TM to apply, no geometry dropped).
static int buildSwrtBLAS_gridFast(RenderSWRT &swrt, const CollisionResource::Grid &grid, float dim_as_box_min, float dim_as_box_max)
{
  const int treeBytes = (int)grid.blasTreeBytes;
  const int srcVertsOfs = (int)grid.blasVertsOfs(); // 8-aligned; tree..vertsOfs gap is padding
  const uint8_t *srcBuf = grid.blasData.data();     // [tree][pad][vert21]: tree starts at offset 0
  const uint8_t *srcVerts = srcBuf + srcVertsOfs;
  const int vertCount = (int)((grid.blasData.size() - (size_t)srcVertsOfs) / 8u);
  G_ASSERT(treeBytes > 0 && vertCount > 0);

  daSWRT::BuiltBLAS built;
  // Verts/tree were quantized against grid.blasBBox; the SWRT model box must be that same box so the
  // [0,65535] -> [-1,1] remap below stays consistent with the runtime world -> BLAS mapping.
  built.box = grid.blasBBox;
  built.data.resize((size_t)treeBytes + (size_t)vertCount * 12, 0);
  built.treeBytes = (uint32_t)treeBytes;
  uint8_t *dst = built.data.data();

  // Score box-resemblance on the SOURCE grid tree (still Quantized16). The dst tree's boxes get
  // re-encoded to FP16 inline below, so read the score off the source where Quantized16 is
  // unambiguous -- matches the soup path, which scores writeQuadBLAS output before addPreBuiltModel
  // re-encodes.
  built.dimAsBoxDist = dim_as_box_max;
  if (dim_as_box_max > dim_as_box_min)
  {
    float boxLike = daSWRT::computeBlasBoxResemblanceVoxel(srcBuf, 0, treeBytes, built.box, daSWRT::BlasBoxEncoding::Quantized16);
    boxLike = powf(boxLike, 1.5f);
    built.dimAsBoxDist = lerp(dim_as_box_max, dim_as_box_min, boxLike);
  }

  // Bulk-copy the whole tree, then fix it up in a single walk. The tree keeps its byte layout
  // verbatim (16 B internal / 20 B leaf), so one memcpy beats N per-node copies and the fixup needs
  // only one cursor (dst offset == src offset). We do NOT copy the vert21 stream: it changes stride
  // (8 B -> 12 B float3), so the vertex loop below expands it from the source into the wider slots.
  memcpy(dst, srcBuf, (size_t)treeBytes);
  int ofs = 0;
  while (ofs < treeBytes)
  {
    uint32_t encWord;
    memcpy(&encWord, dst + ofs + 12, sizeof(uint32_t)); // QUAD_LEAF_FLAG + quad/single encoding live at +12
    const bool isLeaf = (encWord & QUAD_LEAF_FLAG) != 0;
    build_bvh::reencodeBoxNodeToFP16(dst + ofs); // box uint16 [0,65535] -> FP16 [-1,1]; preserves the +12 word
    ofs += BVH_BLAS_NODE_SIZE;

    if (isLeaf)
    {
      // relOfs (signed byte offset from this field to the leaf's first vertex) sits after the 16 B
      // header. The copied value targets vert21 (8 B stride); re-derive the vertex index and re-emit
      // it for the 12 B float3 stride (field offset unchanged, dst offset == src).
      int relOfsOrig;
      memcpy(&relOfsOrig, dst + ofs, sizeof(int));
      const int vertIdx = (ofs + relOfsOrig - srcVertsOfs) / 8; // ofs + relOfs = vertIdx * 8 + srcVertsOfs
      const int relOfsNew = vertIdx * 12 + treeBytes - ofs;
      memcpy(dst + ofs, &relOfsNew, sizeof(int));
      ofs += BVH_BLAS_LEAF_SIZE - BVH_BLAS_NODE_SIZE; // = 4
    }
  }
  // The walk must land exactly on treeBytes: node sizes (16 B internal / 20 B leaf, keyed on
  // QUAD_LEAF_FLAG) sum to treeBytes by construction. Mirrors the producer's invariant assert
  // (Grid::buildBLAS / writeQuadBLAS both G_ASSERTF dataOffset == treeBytes).
  G_ASSERT(ofs == treeBytes);

  // Decode vert21 (8 B/vert, [0,65535]) straight into the final FP16-space float3 (12 B/vert,
  // [-1,1]) in ONE pass: unpack, then apply the same f/32767.5 - 1 map reencodeQuadBlasToFP16's
  // vertex pass would. No intermediate float3, no second walk. (Read from the source grid, not in
  // place -- grid.blasData is the live collision BLAS and must not be written.)
  const vec4f vertToNorm = v_splats(1.0f / 32767.5f);
  const vec4f vertBias = v_splats(-1.0f);
  uint8_t *vertDst = dst + treeBytes;
  for (int v = 0; v < vertCount; ++v)
  {
    vec3f n = v_madd(RayData::unpackVert21(srcVerts + (size_t)v * 8), vertToNorm, vertBias);
    alignas(16) float f[4];
    v_st(f, n);
    memcpy(vertDst + (size_t)v * 12, f, 12); //-V1086
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
  //     reproduced without re-splicing leaves.
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
        // Gate on blasNodeRanges membership, NOT BLAS_RESIDENT: residency only says whose vert21
        // array node->verticesOfs indexes (a TRACEABLE|PHYS_COLLIDABLE node is stamped resident in
        // the COLLIDABLE grid -- see getBlasGridForResidentNode), and a post-dup vert span > 65536
        // leaves a covered node non-resident. Either way its triangles are in THIS grid's BLAS
        // bytes, which is all gridFast re-encodes.
        bool inGrid = false;
        for (const auto &nr : grid.blasNodeRanges)
          if (nr.nodeIndex == node->nodeIndex)
          {
            inGrid = true;
            break;
          }
        allEligibleInGrid &= inGrid;
      }
      if (allEligibleInGrid && eligibleCount > 0 && eligibleCount == grid.blasNodeRanges.size())
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

  return swrt.addPreBuiltModel(box, eastl::move(blasBytes), vertCountTotal, (int)nodes.size(), (int)prims.size(), dimAsBoxDist);
}
