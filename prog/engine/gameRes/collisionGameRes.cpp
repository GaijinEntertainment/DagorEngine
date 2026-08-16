// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <math/dag_triangleBoxIntersection.h>
#include <math/dag_geomTree.h>
#include <math/dag_traceRayTriangle.h>
#include <math/dag_plane3.h>
#include <generic/dag_sort.h>
#include <math/dag_triangleTriangleIntersection.h>
#include <math/dag_mathUtils.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_capsuleTriangle.h>
#include <supp/dag_prefetch.h>
#include <math/dag_math3d.h>
#include <sceneRay/dag_sceneRay.h>
#include <scene/dag_physMat.h>
#include <osApiWrappers/dag_cpuFeatures.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_finally.h>
#include <debug/dag_debug.h>
#include <daBVH/dag_swBLAS_ray.h>
#include <daBVH/dag_swBLAS_soa4Validate.h>
#include "collisionTraceOOL.h"
#include <EASTL/bitvector.h>
#include "collisionGameResInternal.h"

// Largest singular value, used where column lengths under-bound shear. Rounded a few ULPs
// upward so every consumer may treat it as an upper bound; the shear-class persisted stamp
// adds its own 1.0002f margin on top.
float mat33_spectral_norm(mat44f_cref m_in)
{
  // The eigenvalue terms are fourth powers of the scale; normalize by the max element so a large
  // FINITE transform cannot overflow them into the non-finite arm and under-report as 1. The
  // reciprocal must stay a normal float: past 2^126 FTZ flushes 1/s to zero (the norm would
  // silently read 0), and a subnormal s (outside the engine FTZ contract) overflows 1/s to Inf.
  const vec4f absMax3 = v_max(v_abs(m_in.col0), v_max(v_abs(m_in.col1), v_abs(m_in.col2)));
  const float s = v_extract_x(v_hmax3(absMax3));
  if (DAGOR_UNLIKELY(!(s >= FLT_MIN && s <= 1.f / FLT_MIN)))
  {
#if DAGOR_DBGLEVEL > 0
    if (s != 0.f)
      LOGERR_ONCE("collision: non-finite or degenerate-scale node tm in spectral norm; trace-side determinant gates hide such nodes");
#endif
    // FLT_MAX, not 1: the pair-query outer transforms have NO determinant gate, and a huge
    // finite stretch reported as 1 would let the whole-pair sphere cull reject real geometry.
    return s == 0.f ? 0.f : FLT_MAX;
  }
  // Divide, not multiply-by-reciprocal: 1/1e38 is subnormal and FTZ flushes it to zero, which
  // would zero every column and report norm 0 for a huge FINITE stretch.
  const vec4f vS = v_splats(s);
  mat44f m;
  m.col0 = v_div(m_in.col0, vS);
  m.col1 = v_div(m_in.col1, vS);
  m.col2 = v_div(m_in.col2, vS);
  const float b00 = v_extract_x(v_dot3_x(m.col0, m.col0)), b11 = v_extract_x(v_dot3_x(m.col1, m.col1));
  const float b22 = v_extract_x(v_dot3_x(m.col2, m.col2)), b01 = v_extract_x(v_dot3_x(m.col0, m.col1));
  const float b02 = v_extract_x(v_dot3_x(m.col0, m.col2)), b12 = v_extract_x(v_dot3_x(m.col1, m.col2));
  const float q = (b00 + b11 + b22) / 3.f;
  const float p2 = sqr(b00 - q) + sqr(b11 - q) + sqr(b22 - q) + 2.f * (sqr(b01) + sqr(b02) + sqr(b12));
  // Upward so consumers may treat the result as an upper bound; 2e-6 budgets Gram rounding
  // plus the near-isotropic trig path's cancellation.
  const float upward = 1.000002f;
  // The cutoff is scale-relative so tiny transforms remain conservative.
  if (p2 <= 1e-14f * q * q)
    return s * sqrtf(max(q, 0.f)) * upward;
  const float p = sqrtf(p2 / 6.f);
  const float c00 = (b00 - q) / p, c11 = (b11 - q) / p, c22 = (b22 - q) / p;
  const float c01 = b01 / p, c02 = b02 / p, c12 = b12 / p;
  const float halfDet = 0.5f * (c00 * (c11 * c22 - c12 * c12) - c01 * (c01 * c22 - c12 * c02) + c02 * (c01 * c12 - c11 * c02));
  const float phi = acosf(clamp(halfDet, -1.f, 1.f)) / 3.f;
  return s * sqrtf(max(q + 2.f * p * cosf(phi), 0.f)) * upward;
}

// Shared ownership/freshness gate for updates and dispatch.
bool check_instance_owned_and_fresh(const CollisionResource *res, const CollisionResourceInstance &instance, const char *site)
{
  // The layout generation catches what the count cannot: sortNodesList permutes indices in place,
  // so a pre-sort instance would serve the previous occupant's pose for every node.
  const bool ok = instance.getResource() == res && instance.nodeCount() == (int)res->getAllNodes().size() &&
                  instance.nodeLayoutGeneration() == res->nodeLayoutGeneration();
  G_ASSERTF(ok,
    "%s: CollisionResourceInstance is foreign or stale (res %p vs %p, %d nodes vs %d, layout gen %u vs %u): instances must be "
    "(re)created after all post-load node mutation",
    site, instance.getResource(), res, instance.nodeCount(), (int)res->getAllNodes().size(), instance.nodeLayoutGeneration(),
    res->nodeLayoutGeneration());
  if (DAGOR_UNLIKELY(!ok))
    LOGERR_ONCE("%s: CollisionResource %p used with a foreign/stale instance (res %p, %d nodes vs %d, layout gen %u vs %u)", site, res,
      instance.getResource(), instance.nodeCount(), (int)res->getAllNodes().size(), instance.nodeLayoutGeneration(),
      res->nodeLayoutGeneration());
  return ok;
}

// #define VERIFY_TRACE_RESULTS 1 // May affect performance

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define IF_CONSTEXPR if constexpr
#else
#define IF_CONSTEXPR if
#endif

// Squared radius for CollisionNode bounding sphere preserving the empty-sphere convention (r < 0 => r2 == -1).
static inline float get_bsphere_r2(float r) { return r < 0 ? -1.f : r * r; }

// BSphere3 from CollisionNode's stored c+r preserving the empty-sphere convention. Plain BSphere3(c, r) recomputes r2 = r*r,
// so r=-1 would become r2=1 (non-empty); use this helper instead so r<0 produces a fully empty sphere.
static inline BSphere3 make_node_bsphere(const Point3 &c, float r) { return r < 0 ? BSphere3() : BSphere3(c, r); }

struct CollResProfileStats
{
  unsigned meshNodesNum = 0;
  unsigned meshNodesSphCheckPassed = 0;
  unsigned meshNodesBoxCheckPassed = 0;
  unsigned meshTrianglesTraced = 0;
  unsigned meshTrianglesHits = 0;
};

template <typename T>
static inline void sort_collres_intersections(T &intersected_nodes_list)
{
  fast_sort_branchless(intersected_nodes_list.begin(), intersected_nodes_list.end());
}

// All BLAS walks below run over the SoA4 CPU layout (daBVH/dag_swBLAS_soa4.h): filtered iteration
// via soa4::iterateFiltered*, ray casts via the collision_blas SoA4 OOL entries, leaf identity via
// the persistent soa4::LeafRef (what tri_ref carries as its opaque token).

// Dequantize a vert21 leaf vertex back to resource-local coordinates. Packed value is box-space
// [0,65535]; inverse is `local = q * (1/blasScale) + blasBBox.bmin`. This matches the trace
// dispatch frame (vLocalFrom/vLocalDir), so the per-triangle ray test needs no further conversion.
struct BlasLocalUnquant
{
  vec3f bmin, invScale;
  // vertIdx is a SIGNED apex-relative offset (13-bit, may be negative); keep both the index and the
  // stride multiply signed. An unsigned product would wrap a negative offset to a multi-GB byte delta
  // and fault (BVH_BLAS_VERT21_STRIDE is uint32_t, so it would force the whole expression unsigned).
  __forceinline vec3f operator()(const uint8_t *d, int baseOfs, int vertIdx) const
  {
    vec3f q = RayData::unpackVert21(d + baseOfs + vertIdx * (int)BVH_BLAS_VERT21_STRIDE);
    return v_madd(q, invScale, bmin);
  }
  static BlasLocalUnquant make(vec3f blas_bmin, vec3f blas_scale)
  {
    BlasLocalUnquant u;
    u.bmin = blas_bmin;
    u.invScale = v_rcp(blas_scale);
    return u;
  }
};

// Convert a resource-local ray to box-space (the BLAS inner-node bbox encoding frame). t is invariant
// under the per-axis scaling (box_dir = local_dir * scale), so the same max-t prunes both frames.
struct BlasBoxRay
{
  vec3f dirInv, originScaled;
  static BlasBoxRay make(vec3f vLocalFrom, vec3f vLocalDir, vec3f blas_scale, vec3f blas_ofs)
  {
    BlasBoxRay r;
    vec3f bRayOrigin = v_madd(vLocalFrom, blas_scale, blas_ofs);
    vec3f bRayDir = v_mul(vLocalDir, blas_scale);
    r.dirInv = v_rcp(v_sel(v_splats(1e-32f), bRayDir, v_cmp_gt(v_abs(bRayDir), v_splats(1e-32f))));
    r.originScaled = v_neg(v_mul(bRayOrigin, r.dirInv));
    return r;
  }
};

// LeafRef -> quad-A apex byte offset for refs held outside a filtered walk (the walkers pass the
// apex to their callbacks); the OOL ray accept path re-derives it here from its stored ref.
static inline uint32_t leafApexByteOfs(const uint8_t *data, soa4::LeafRef ref)
{
  const soa4::LeafLoc l = soa4::decodeLeafRef(data, ref);
  return (uint32_t)(l.bodyOfs + (int)soa4::leafFields(data, l).relBaseBytes);
}

// Decoded per-node BLAS chunk frame (chunk = [NodeBlasChunkHeader][quad tree][ownVerts21 block]):
// the tree span plus the two frames every chunk descent needs -- the q-space transform (scale/qOfs
// from the chunk header) for tree traversal, and the decode frame (bmin/invScale from the block
// header, the single bmin copy) for vert/normal recovery. Reads the chunk layout in one place.
struct NodeChunkFrame
{
  const uint8_t *tree;
  soa4::RootRef rootRef;
  int treeBytes;        // dev-only token validation (getNodeFaceVertsByRef); the walks are root-driven
  vec3f scale;          // exact pack scale: node-local -> chunk q-space (ray/box transform)
  vec3f qOfs;           // -bmin * scale
  BlasLocalUnquant unq; // bmin + invScale: q-space -> node-local (vert/normal decode)
};
static NodeChunkFrame decode_node_chunk_frame(const uint8_t *chunk)
{
  const CollisionResource::NodeBlasChunkHeader *hdr = (const CollisionResource::NodeBlasChunkHeader *)chunk;
  NodeChunkFrame f;
  f.tree = chunk + sizeof(CollisionResource::NodeBlasChunkHeader);
  f.rootRef = hdr->rootRef;
  f.treeBytes = (int)hdr->treeBytes;
  f.scale = v_ldu_p3(hdr->scale);
  const uint8_t *block = f.tree + CollisionResource::alignVert21StreamOfs(hdr->treeBytes);
  f.unq.bmin = v_ldu_p3((const float *)block);
  f.unq.invScale = v_ldu_p3((const float *)block + 3);
  f.qOfs = v_neg(v_mul(f.unq.bmin, f.scale));
  return f;
}

// tri_ref leaf tokens are minted by our own traversals over loader-built trees -- correct by
// construction, so decoding one back needs no checks. Dev PC builds still run the paranoid
// validation (daBVH/dag_swBLAS_soa4Validate.h) to catch stale/forged refs early.
#define VALIDATE_TRI_REF_TOKENS (_TARGET_PC_WIN && DAGOR_DBGLEVEL > 0)

// Recover a leaf's source CollisionNode from its first-vert vert21 index. buildBLAS keeps each
// node's vert21 contiguous (drops the global fetch-remap, keeps QUAD_O_MAX dups inside the owning
// node's block), so a leaf's verts all live in one source node's sub-range: upper-bound the sorted
// blasNodeRanges to the containing range. Filtered-walk consumers derive v0_idx from the apex byte
// offset the walker passes alongside the ref, so no leaf re-decode is needed.
static inline uint16_t blas_src_node_for_vert_idx(const CollisionResource::Grid &g, uint32_t v0_idx)
{
  const auto *first = g.blasNodeRanges.data();
  const auto *last = first + g.blasNodeRanges.size();
  auto it =
    eastl::upper_bound(first, last, v0_idx, [](uint32_t v, const CollisionResource::Grid::NodeRange &r) { return v < r.verticesOfs; });
  G_ASSERTF(it != first, "BLAS leaf vert21 index %u precedes all blasNodeRanges entries", v0_idx);
  --it;
  G_ASSERTF(v0_idx < it->verticesEnd, "BLAS leaf vert21 index %u falls into gap between blasNodeRanges entries", v0_idx);
  return it->nodeIndex;
}

// LeafRef-only variant for the OOL ray accept thunk, whose dispatch ABI carries just the ref:
// re-derives the apex byte offset from the leaf before the range lookup.
static inline uint16_t blas_src_node_for_leaf(const CollisionResource::Grid &g, soa4::LeafRef ref)
{
  const uint8_t *d = g.blasData.data();
  const uint32_t v0ByteOfs = leafApexByteOfs(d, ref);
  G_ASSERTF(v0ByteOfs >= g.blasVertsOfs(), "BLAS leaf vert byte offset %u is inside the tree region (vertsOfs=%u, leafRef=%u)",
    v0ByteOfs, g.blasVertsOfs(), (unsigned)ref);
  return blas_src_node_for_vert_idx(g, (v0ByteOfs - g.blasVertsOfs()) / BVH_BLAS_VERT21_STRIDE);
}

// Visit each triangle of one mesh/convex node whose node-local AABB overlaps [local_box_min,
// local_box_max], with verts dequantised to node-local space, until leaf_test(v0,v1,v2) returns true
// (any-hit). Spatial-culled through the node's quad-BLAS: a per-node chunk, or the node's range inside
// its behavior grid (sibling-node leaves that fall inside the query box are filtered out). Returns
// true iff a leaf_test hit. node_blas_data is the resource's nodeBlasData base (only read for chunked
// nodes). Shared by the sphere / capsule overlap tests so each pays BLAS-log cost, not O(verts+faces).
template <class LeafTest>
static bool walkNodeTrisInLocalBox(const CollisionResource &res, const uint8_t *node_blas_data, const CollisionNode &node,
  uint32_t node_blas_ofs, vec4f local_box_min, vec4f local_box_max, const LeafTest &leaf_test)
{
  // Caller guarantees node has geometry (indicesCount != 0).
  if (res.isGridResident(node))
  {
    const CollisionResource::Grid &g = res.getBlasGridForResidentNode(node);
    if (g.blasData.empty())
      return false;
    const uint8_t *bData = g.blasData.data();
    const uint32_t vertsOfs = g.blasVertsOfs();
    // stampBlasResidentNodes() copied this node's NodeRange (from this same grid) into
    // verticesOfs/Count, so use them directly instead of rescanning blasNodeRanges on every test.
    const uint32_t nodeVOfs = node.getResidentVertsOfs(), nodeVEnd = node.getResidentVertsEnd();
    const vec3f qMin = v_madd(local_box_min, g.blasScale, g.blasOfs);
    const vec3f qMax = v_madd(local_box_max, g.blasScale, g.blasOfs);
    const BlasLocalUnquant unq = BlasLocalUnquant::make(g.blasBBox.bmin, g.blasScale);
    return soa4::iterateFilteredVerts(
      bData, g.blasRootRef,
      [qMin, qMax](vec3f bmn, vec3f bmx) { return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(qMax, bmn), v_cmp_ge(bmx, qMin))); },
      [&, vertsOfs, nodeVOfs, nodeVEnd](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int apexByteOfs) -> bool {
        const uint32_t v0Idx = ((uint32_t)apexByteOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
        if (v0Idx < nodeVOfs || v0Idx >= nodeVEnd)
          return false; // sibling-node leaf inside the query box
        return leaf_test(v0, v1, v2);
      },
      unq);
  }
  if (node_blas_ofs != ~0u)
  {
    const NodeChunkFrame fr = decode_node_chunk_frame(node_blas_data + node_blas_ofs);
    const vec3f qMin = v_madd(local_box_min, fr.scale, fr.qOfs);
    const vec3f qMax = v_madd(local_box_max, fr.scale, fr.qOfs);
    return soa4::iterateFilteredVerts(
      fr.tree, fr.rootRef,
      [qMin, qMax](vec3f bmn, vec3f bmx) { return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(qMax, bmn), v_cmp_ge(bmx, qMin))); },
      [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int) -> bool { return leaf_test(v0, v1, v2); }, fr.unq);
  }
  return false;
}


void CollisionResource::bakeNodeTransform(int node_id)
{
  // Single-node variant of the bake block in collapseAndOptimize (collisionGameResLoad.cpp): transforms vertices, transforms convex
  // planes via inverse-transpose, rebuilds bbox/bsphere from the transformed vertices, flips triangle winding when the TM is mirrored
  // (det<0), and normalizes transform metadata (default T / flags). Capsule/box/sphere primitives store auxiliary shape data
  // outside vertices/indices, so they are out of scope for this helper.
  CollisionNode *n = getNode(node_id);
  if (!n)
    return;
  if (n->type != COLLISION_NODE_TYPE_MESH && n->type != COLLISION_NODE_TYPE_CONVEX)
    return;
  if ((defaultInstance.poseMeta[n->nodeIndex].flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT ||
      !n->hasGeometry())
    return;
  // a singular live pose stays hidden (untraceable): inverting it would feed invalid convex
  // planes and the identity restamp below would revive the node as traceable
  if (!defaultInstance.poseMeta[n->nodeIndex].isTraceable())
    return;

  // A grid-resident node is always IDENT, so the early-out above already excluded it. Decode the
  // node's per-node chunk verts, transform, then re-chunk from the baked geometry.
  const uint32_t vCount = (uint32_t)n->verticesCount;
  dag::Vector<Point3_vec4, framemem_allocator> verts;
  verts.resize(vCount);
  {
    const PackedVerts21 p = getPackedNodeVerts21(*n);
    for (uint32_t i = 0; i < vCount; ++i)
      v_st(&verts[i].x, v_madd(RayData::unpackVert21(p.verts21 + (size_t)i * 8u), p.invScale, p.bmin));
  }
  Point3_vec4 *nodeVerts = verts.data();
  // The node keeps no index list; its face list lives in its current chunk. Materialise it into a
  // transient buffer to re-chunk from the baked geometry (chunk-local indices match the decoded verts).
  dag::Vector<uint32_t, framemem_allocator> faceIdx;
  faceIdx.reserve(n->indicesCount);
  walkNodeChunkLeavesForFaces(*n, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
    faceIdx.push_back(i0);
    faceIdx.push_back(i1);
    faceIdx.push_back(i2);
  });
  uint32_t *nodeIdx = faceIdx.data();
  // indicesCount is a cache of the chunk's face-index count; the chunk walk is the source of truth.
  // A mismatch means the cache went stale (e.g. a face-dropping rebuild that did not re-stamp it),
  // which is a bug: shout and reconcile so every later reader sees the real count.
  if (faceIdx.size() != n->indicesCount)
  {
    logerr("collision: node <%s> indicesCount %u != materialised face indices %u; reconciling", getNodeName(n->nodeIndex),
      n->indicesCount, (uint32_t)faceIdx.size());
    n->indicesCount = (uint32_t)faceIdx.size();
  }

  // sanitize the w lanes: the full 4x4 inverse for the plane transform below needs them defined
  mat44f nodeTm;
  v_mat44_make_from_43cu(nodeTm, defaultInstance.nodeTm[n->nodeIndex].array);
  bbox3f box;
  v_bbox3_init_empty(box);
  for (Point3_vec4 *v = nodeVerts, *ve = v + vCount; v != ve; ++v)
  {
    vec4f p = v_mat44_mul_vec3p(nodeTm, v_ld(&v->x));
    v_st(&v->x, p);
    v_bbox3_add_pt(box, p);
  }
  vec4f vSphereC = v_bbox3_center(box), sphereRad2 = v_zero();
  for (const Point3_vec4 *v = nodeVerts, *ve = v + vCount; v != ve; ++v)
    sphereRad2 = v_max(sphereRad2, v_length3_sq_x(v_sub(vSphereC, v_ld(&v->x))));

  // Flip winding BEFORE re-chunking when the TM is mirrored: the chunk bakes the triangle structure
  // into its BVH, so the indices must carry the final winding when the chunk is built.
  const float tmDet = v_extract_x(v_dot3_x(nodeTm.col0, v_cross3(nodeTm.col1, nodeTm.col2)));
  if (tmDet < 0.f)
    for (uint32_t i = 0, e = (uint32_t)faceIdx.size(); i + 2 < e; i += 3)
      eastl::swap(nodeIdx[i + 0], nodeIdx[i + 2]);

  // Re-chunk from the baked verts -- a NEW chunk appended to nodeBlasData; the old chunk is orphaned.
  // Batch callers reclaim the orphans with one compactNodeBlasData() after baking (cheaper than a
  // per-node pass). A node the chunk build rejects is dropped to indicesCount == 0 below.
  n->nodeBlasOfs = ~0u;
  dag::Vector<vec4f> sScr, oScr;
  dag::Vector<Point3_vec4> pScr;
  dag::Vector<vec4f> qScr;
  dag::Vector<uint8_t> stkScr, soaScr;
  if (!buildOneNodeBlasChunk(*n, nodeVerts, vCount, nodeIdx, (unsigned)n->indicesCount, sScr, oScr, pScr, qScr, stkScr, soaScr))
  {
    // Rejected chunk (reason logerr'd by the builder): drop its geometry -- no collision surface.
    n->indicesCount = 0;
  }

  if (n->planesCount)
  {
    mat44f N, TN;
    v_mat44_inverse(N, nodeTm);
    v_mat44_transpose(TN, N);
    plane3f *base = convexPlanes.data() + n->planesOfs;
    for (plane3f *p = base, *pe = base + n->planesCount; p != pe; ++p)
      *p = v_mat44_mul_vec4(TN, *p);
  }

  v_stu_bbox3(n->modelBBox, box);
  v_stu_p3(&n->boundingSphere.c.x, vSphereC);
  n->boundingSphere.r = v_extract_x(v_sqrt_x(sphereRad2));

  n->flags =
    (n->flags & ~(CollisionNode::TRANSLATE | CollisionNode::ORTHOUNIFORM)) | CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED;
  {
    mat44f identTm;
    v_mat44_ident(identTm);
    setAuthoredNodeTm(n->nodeIndex, identTm, n->flags, 1.f);
  }
  ++nodeBlasBuildId; // re-chunk relocated this node's tree: invalidate stale per-node BLAS tri_refs
}

void CollisionResource::bakeMirroredNodes()
{
  // Bake mirrored (det<0) mesh/convex nodes so the runtime never sees a negative-determinant node TM,
  // then reclaim the orphaned chunks once. Owning both ends here keeps the orphan lifecycle off callers.
  bool baked = false;
  for (int ni = 0, ne = (int)allNodesList.size(); ni < ne; ++ni)
  {
    const CollisionNode &n = allNodesList[ni];
    const TMatrix &t = defaultInstance.nodeTm[ni];
    if ((n.type == COLLISION_NODE_TYPE_MESH || n.type == COLLISION_NODE_TYPE_CONVEX) && t.det() < 0.f)
    {
      bakeNodeTransform(ni);
      baked = true;
    }
  }
  if (baked)
    compactNodeBlasData();
}

void CollisionResource::compactNodeBlasData()
{
  if (nodeBlasData.empty())
    return;
  // Any tri_ref into a chunk is invalidated by the reshuffle (refs are only valid until the next
  // rebuild). Resident nodes (nodeBlasOfs == ~0u) decode from the grid and are skipped.
  dag::Vector<uint8_t> packed;
  packed.reserve(nodeBlasData.size());
  for (CollisionNode &n : allNodesList)
  {
    if (n.nodeBlasOfs == ~0u)
      continue;
    const uint8_t *chunk = nodeBlasData.data() + n.nodeBlasOfs;
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)chunk;
    // Chunk layout (buildOneNodeBlasChunk): [header][tree padded to 8][24 B vert block header][vert21s].
    const size_t chunkSize = sizeof(NodeBlasChunkHeader) + alignVert21StreamOfs(hdr->treeBytes) + OWN_VERTS21_HEADER_BYTES +
                             (size_t)n.verticesCount * BVH_BLAS_VERT21_STRIDE;
    n.nodeBlasOfs = (uint32_t)packed.size();
    packed.insert(packed.end(), chunk, chunk + chunkSize);
  }
  nodeBlasData.swap(packed);
  nodeBlasData.shrink_to_fit();
  ++nodeBlasBuildId; // invalidate any per-node BLAS tri_ref minted against the pre-pack layout
}

const CollisionNode *CollisionResource::getNode(uint32_t index) const
{
  return index < allNodesList.size() ? allNodesList.data() + index : nullptr;
}

CollisionNode *CollisionResource::getNode(uint32_t index)
{
  return index < allNodesList.size() ? allNodesList.data() + index : nullptr;
}

int CollisionResource::getNodeIndexByName(const char *name) const
{
  if (name && *name && !names.empty())
    for (int i = 0; i < allNodesList.size(); i++)
      if (strcmp(names.data() + allNodesList[i].nameOfs, name) == 0)
        return i;
  return -1;
}

uint32_t CollisionResource::addName(const char *name)
{
  if (names.empty())
    names.push_back('\0');
  if (!name || !*name)
    return 0;
  uint32_t ofs = (uint32_t)names.size();
  size_t len = strlen(name);
  names.insert(names.end(), name, name + len + 1);
  return ofs;
}

CollisionNode *CollisionResource::getNodeByName(const char *name) { return getNode(getNodeIndexByName(name)); }

const CollisionNode *CollisionResource::getNodeByName(const char *name) const { return getNode(getNodeIndexByName(name)); }

template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, bool pose_may_refresh,
  typename pose_t, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f tm, const pose_t &instance, vec3f from, vec3f dir, float len,
  bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
  TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  CollisionTrace in{
    .vFrom = from,
    .vDir = dir,
    .vTo = v_madd(dir, v_splats(len), from),
    .t = len,
  };

  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<trace_mode, trace_type, true /*is_single_ray*/, pose_may_refresh>(tm, instance, traces, calc_normal,
    bsphere_scale, behavior_filter, filter, callback, out_stats, force_no_cull);
}


// Heavy all-in-one node iterator, calculations unused by caller and deadcode will be automatically stripped by compiler
template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, bool is_single_ray,
  bool pose_may_refresh, typename pose_t, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f original_tm, const pose_t &instance,
  dag::Span<CollisionTrace> traces, bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter,
  const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  TIME_PROFILE_DEV(collres_trace);
  // A tree-backed temp view never refreshes (bind meta, no rootBBox); tree-less legacy calls
  // read the default instance, and owned-matrix forms are mutator-maintained.
  if constexpr (pose_may_refresh)
  {
    if (DAGOR_UNLIKELY(instance.getTree() != nullptr))
      instance.refreshIfStale(original_tm);
  }
  else
    G_ASSERT(instance.getTree() == nullptr || instance.isMetaAliased());

  // Move instance_tm to zero for better precision
  // Line below needed to bypass 32-bit compiler bug when original instance tm passed by constant pointer
  // was modified by "tm.col3 = v_zero()" instead of it's copy in arguments of that forceinline function
  const mat44f tm = {original_tm.col0, original_tm.col1, original_tm.col2, v_zero()};
  vec3f woffset = original_tm.col3;

  for (CollisionTrace &trace : traces)
  {
    // Move all traces following instance_tm
    trace.vFrom = v_sub(trace.vFrom, woffset);
    trace.vTo = v_sub(trace.vTo, woffset);
  }

  vec4f maxScaleSq = v_mat44_max_scale43_sq(tm);
  vec3f xyzScaleSq = v_mat44_scale43_sq(tm);
  // 0.025 * sqrt(3) of absolute error max
  const float eps = eastl::min(0.008f, 0.025f / getBoundingSphereRad());
  // Scaled tm requires more complicated norm calculation code and additional T conversion from world to local basis and back
  vec3f otmMask = v_and(v_cmp_gt(xyzScaleSq, v_splats(1.f - eps)), v_cmp_lt(xyzScaleSq, v_splats(1.f + eps)));
  // Pairwise orthogonality too: a unit-column SHEAR passes any length-only band, would keep the
  // under-scaled sphere reject AND take the transpose-inverse branch below.
  const float d01 = fabsf(v_extract_x(v_dot3_x(tm.col0, tm.col1)));
  const float d02 = fabsf(v_extract_x(v_dot3_x(tm.col0, tm.col2)));
  const float d12 = fabsf(v_extract_x(v_dot3_x(tm.col1, tm.col2)));
  bool bIsOrthonormalizedTm = v_check_xyz_all_true(otmMask) && d01 < eps && d02 < eps && d12 < eps; // .w of scale43_sq is 0, not a
                                                                                                    // scale
  // Column lengths under-bound a sheared instance tm: widen the sphere-reject scale by the
  // spectral norm for the non-orthonormal class (exact for uniform scale, conservative for shear).
  if (DAGOR_UNLIKELY(!bIsOrthonormalizedTm))
    maxScaleSq = v_max(maxScaleSq, v_splats(sqr(mat33_spectral_norm(tm))));
  else
  {
    // The accepted class still admits dots/length slack up to eps: Gershgorin over the tolerance
    // volume (sigma^2 <= max_i(len_i^2 + sum_j |g_ij|)) keeps the sphere rejects covering.
    alignas(16) float ls[4];
    v_st(ls, xyzScaleSq);
    const float g = max(ls[0] + d01 + d02, max(ls[1] + d01 + d12, ls[2] + d02 + d12));
    maxScaleSq = v_max(maxScaleSq, v_splats(g));
  }

  // Check bounding
  bool anyTraceIntersectsBounding = false;
  {
    vec3f vBsphCenter;
    vec4f vBsphR2;
    if (DAGOR_LIKELY(!instance.isMetaAliased()))
    {
      if (DAGOR_LIKELY(!instance.isPosedSinceBind() && bindTraceSphereStamped))
      {
        // The cached bind sphere is tighter than rootBBox's circumsphere.
        vBsphCenter = v_mat44_mul_vec3p(tm, vBindTraceSphere);
        vBsphR2 = v_mul_x(v_mul_x(maxScaleSq, v_splat_w(vBindTraceSphere)), v_set_x(bsphere_scale * bsphere_scale));
      }
      else
      {
        // rootBBox conservatively bounds every enabled posed node.
        const bbox3f rootBox = instance.getRootBBox();
        // Overflow-safe midpoint: (bmin + bmax) can overflow for finite far-huge bounds.
        const vec3f rootCenter = v_madd(rootBox.bmin, V_C_HALF, v_mul(rootBox.bmax, V_C_HALF));
        vBsphCenter = v_mat44_mul_vec3p(tm, rootCenter);
        vBsphR2 =
          v_mul_x(v_mul_x(maxScaleSq, v_length3_sq_x(v_sub(rootBox.bmax, rootCenter))), v_set_x(bsphere_scale * bsphere_scale));
      }
    }
    else
    {
      // Temp views carry no rootBBox: keep the legacy whole-resource bounds from the tree.
      const GeomNodeTree *geomNodeTree = instance.getTree();
      if (bsphereCenterNode)
        vBsphCenter = v_add(geomNodeTree->getNodeWposRel(bsphereCenterNode), v_sub(geomNodeTree->getWtmOfs(), woffset));
      else
        vBsphCenter = v_mat44_mul_vec3p(tm, vBoundingSphere);
      vBsphR2 = v_mul_x(v_mul_x(maxScaleSq, v_splat_w(vBoundingSphere)), v_set_x(bsphere_scale * bsphere_scale));
    }
    for (CollisionTrace &trace : traces)
    {
      vec4f vExtBsphR2 = vBsphR2;
      if (trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT)
      {
        vec4f vExtBsphR = v_add_x(v_sqrt_x(vBsphR2), v_set_x(trace.capsuleRadius));
        vExtBsphR2 = v_mul_x(vExtBsphR, vExtBsphR);
      }
      trace.isectBounding = DAGOR_LIKELY(trace.t > VERY_SMALL_NUMBER) &&
                            v_test_ray_sphere_intersection(trace.vFrom, trace.vDir, v_splats(trace.t), vBsphCenter, vExtBsphR2);

#if defined(_WIN32) && DAGOR_DBGLEVEL > 0 && defined(_M_IX86_FP) && _M_IX86_FP == 0
      G_ASSERT(!check_nan(v_extract_x(trace.vDir)) && !check_nan(v_extract_y(trace.vDir)) && !check_nan(v_extract_z(trace.vDir)) &&
               !check_nan(trace.t));
#endif

      float dirLenSq = v_extract_x(v_length3_sq_x(trace.vDir));
      if (DAGOR_UNLIKELY(fabsf(dirLenSq - 1.f) > 0.01f))
      {
        if (dirLenSq > VERY_SMALL_NUMBER)
          logerr("Not normalized dir " FMT_P3 " lenSq=%f used in collres.traceRay (tm scale=%f)", V3D(trace.vDir), dirLenSq,
            sqrtf(v_extract_x(maxScaleSq)));
        trace.isectBounding &= dirLenSq > VERY_SMALL_NUMBER;
      }

      anyTraceIntersectsBounding |= trace.isectBounding;
    }
    if (is_single_ray) // hint for optimizer, helps to remove !trace.isectBounding checks for each node
      anyTraceIntersectsBounding = traces.front().isectBounding;
  }

  bool res = false;
  if (anyTraceIntersectsBounding)
  {
#if VERIFY_TRACE_RESULTS
    dag::Vector<CollisionTrace> initialTraces(traces.begin(), traces.end());
#endif

    auto cb_wrapper = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      pos = v_add(pos, woffset); // Fix isect position for callback. Warning: trace.vFrom and trace.vTo isn't fixed!

#if VERIFY_TRACE_RESULTS
      bool verified = true;
      float normLen = 0.f;
      Point3_vec4 n, p, from, to;
      v_st(&n.x, normal);
      v_st(&p.x, pos);
      v_st(&from.x, initialTraces[trace_id].vFrom);
      v_st(&to.x, initialTraces[trace_id].vTo);
      verified &= !check_nan(t);
      verified &= t >= 0 && t <= initialTraces[trace_id].t;
      if (calc_normal)
      {
        normLen = v_extract_x(v_length3_x(normal));
        verified &= are_approximately_equal(normLen, 1.f, 0.005f);
        verified &= !check_nan(n);
      }
      verified &= !check_nan(p);
      if (!verified)
      {
        logerr("Trace %i intersection verification failed: from " FMT_P3 " to " FMT_P3 " max_t %f calc_norm %i", trace_id, P3D(from),
          P3D(to), initialTraces[trace_id].t, calc_normal);
        logerr("Results: t %f, norm " FMT_P3 " norm_len %.3f isect_pos " FMT_P3 " node_type %i", t, P3D(n), normLen, P3D(p),
          node->type);
        if (calc_normal && (trace_type == CollisionTraceType::RAY_HIT || trace_type == CollisionTraceType::CAPSULE_HIT))
          logerr("Normal calculation is not supported in ray hit traces");
      }
#endif
      return callback(trace_id, node, t, normal, pos, tri_ref);
    };

    if (DAGOR_LIKELY(bIsOrthonormalizedTm))
      // The real outer scale, not 1: the orthonormal class admits in-band scale, and the
      // per-mesh sphere reject would drop hits in the shell between radius 1 and 1 + band.
      res = forEachIntersectedNode<true, trace_mode, trace_type, is_single_ray>(tm, v_extract_x(maxScaleSq), woffset, instance, traces,
        calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull);
    else
      res = forEachIntersectedNode<false, trace_mode, trace_type, is_single_ray>(tm, v_extract_x(maxScaleSq), woffset, instance,
        traces, calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull);
  }

#if DAGOR_DBGLEVEL > 0
  addTracesProfileTag(traces);
#endif

  // Fix traces to true world coords back
  for (CollisionTrace &trace : traces)
  {
    trace.vFrom = v_add(trace.vFrom, woffset);
    trace.vTo = v_add(trace.vTo, woffset);
  }

  return res;
}

float CollisionResource::mat33SpectralNorm(mat44f_cref tm) { return mat33_spectral_norm(tm); }

bool CollisionResource::relativeDetAboveFloor(mat44f_cref tm, float &out_ndet)
{
  out_ndet = 0.f;
  const vec4f absMax3 = v_max(v_abs(tm.col0), v_max(v_abs(tm.col1), v_abs(tm.col2)));
  const float s = v_extract_x(v_hmax3(absMax3));
  // The element range must keep 1/s a normal float (mirrors mat33_spectral_norm's band).
  if (!(s >= FLT_MIN && s <= 1.f / FLT_MIN))
    return false;
  const vec4f vS = v_splats(s);
  mat44f m;
  m.col0 = v_div(tm.col0, vS);
  m.col1 = v_div(tm.col1, vS);
  m.col2 = v_div(tm.col2, vS);
  out_ndet = v_extract_x(v_dot3_x(m.col0, v_cross3(m.col1, m.col2)));
  const float g2 = v_extract_x(v_max(v_length3_sq_x(m.col0), v_max(v_length3_sq_x(m.col1), v_length3_sq_x(m.col2))));
  const float g = sqrtf(g2);
  return fabsf(out_ndet) > 1e-6f * g * g * g;
}

// Signed-permutation bases with one exact scale per axis: the only bases whose column max IS
// the spectral norm bit-exactly, so no conservative restamp is due.
static inline bool is_exact_axis_aligned_basis(const TMatrix &tm)
{
  for (int a = 0; a < 3; a++)
  {
    const Point3 c = tm.getcol(a);
    if (((c.x != 0.f) + (c.y != 0.f) + (c.z != 0.f)) != 1)
      return false;
  }
  return true;
}

// Conservative caller-tm stretch for the pair-test radius culls: column lengths are exact for
// the near-orthonormal class (the common case), so only scale or shear outside the band pays
// the spectral bound.
static float conservative_outer_scale(mat44f_cref tm)
{
  const vec3f xyzScaleSq = v_mat44_scale43_sq(tm);
  const float g01 = fabsf(v_extract_x(v_dot3_x(tm.col0, tm.col1)));
  const float g02 = fabsf(v_extract_x(v_dot3_x(tm.col0, tm.col2)));
  const float g12 = fabsf(v_extract_x(v_dot3_x(tm.col1, tm.col2)));
  if (DAGOR_LIKELY(
        v_check_xyz_all_true(v_and(v_cmp_gt(xyzScaleSq, v_splats(1.f - 0.008f)), v_cmp_lt(xyzScaleSq, v_splats(1.f + 0.008f)))) &&
        g01 < 0.008f && g02 < 0.008f && g12 < 0.008f))
  {
    // Gershgorin row-sum bound on the Gram matrix: sigma^2 <= max_i(len_i^2 + sum_j |g_ij|).
    // The accepted off-diagonals are NOT free -- the bare column max would under-bound an
    // in-band shear by up to sqrt(1 + 2 * 0.008), and these values feed ungated pair culls.
    alignas(16) float lens[4];
    v_stu(lens, xyzScaleSq);
    const float r0 = lens[0] + g01 + g02, r1 = lens[1] + g01 + g12, r2 = lens[2] + g02 + g12;
    return sqrtf(max(r0, max(r1, r2)));
  }
  return mat33_spectral_norm(tm);
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4701) // potentially uninitialized local variable 'XXX' used
#endif

template <bool orthonormalized_instance_tm, CollisionResource::IterationMode trace_mode,
  CollisionResource::CollisionTraceType trace_type, bool is_single_ray, typename pose_t, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(const mat44f tm, float max_tm_scale_sq, vec3f woffset,
  const pose_t &instance, dag::Span<CollisionTrace> traces, bool calc_normal, uint8_t behavior_filter, const filter_t &filter,
  const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  bool hasCollision = false;
  bool isTraceByCapsule = trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT;
  calc_normal &= trace_type == CollisionTraceType::TRACE_RAY || trace_type == CollisionTraceType::TRACE_CAPSULE;
  // The BLAS bakes BLAS-eligible mesh-node triangles in resource-local (bind) space and is walked using
  // only the instance tm -- it never consults a bound GeomNodeTree. A tree-driven node hit via the BLAS
  // would therefore be tested at its bind pose, not its animated pose. The invariant (asserted in
  // initializeWithGeomNodeTree) is that tree-bound resources are never grid-optimized, so hasBlas() is
  // already false for them; the explicit !getTree() guard makes the trace path honour that contract
  // in release builds too -- a tree-backed pose falls through to the per-node loop, which applies
  // the animated per-node tm via getPosedNodeWtmInline. An instance licenses the grid walk only while
  // every grid-resident node still holds its seed pose and is enabled (one-way latch); once posed,
  // grid-resident nodes fall through to the per-node loop like any other.
  const bool useBlas = hasBlas(behavior_filter) && !isTraceByCapsule && !instance.getTree() && instance.isGridResidentPoseAtBind();

  // The instance inverse serves the whole-resource reject, the per-trace BLAS/box/sphere/capsule
  // pass at the end, and identity-pose mesh nodes, which reuse it as their node inverse. A non-BLAS
  // resource with no box/sphere/capsule nodes returns right after the mesh per-node pass (early-out
  // below) and reaches neither tail consumer, so the eager compute gates on the same test; the mesh
  // loop derives the inverse lazily on first identity-pose use when the gate skipped it.
  const bool hasNonMeshNodes = boxNodesHead != CollisionNode::INVALID_IDX || sphereNodesHead != CollisionNode::INVALID_IDX ||
                               capsuleNodesHead != CollisionNode::INVALID_IDX;

  alignas(EA_CACHE_LINE_SIZE) mat44f itm;
  // Always the full affine inverse: the orthonormal epsilon class's transpose maps segments
  // by s instead of 1/s; tm * inv(tm) = I keeps the parameter scale for the t semantics below.
  bool itmComputed =
    useBlas || hasNonMeshNodes || (!instance.isMetaAliased() && is_single_ray) || meshNodesHead == CollisionNode::INVALID_IDX;
  if (itmComputed)
    v_mat44_inverse43(itm, tm);

  // A single non-capsule ray may reject against the posed node union. Only the tree-backed
  // temp view skips it (no rootBBox); the tree-less legacy form reads the default instance's.
  if (!instance.isMetaAliased() && is_single_ray && !isTraceByCapsule)
  {
    const CollisionTrace &tr = traces.front();
    if (!v_test_segment_box_intersection(v_mat44_mul_vec3p(itm, tr.vFrom), v_mat44_mul_vec3p(itm, tr.vTo), instance.getRootBBox()))
      return false;
  }

  // Skip the whole per-node loop only when the combined grid BLAS covers every mesh node: all nodes
  // BLAS-eligible (no convex / non-IDENT) AND none carry a per-node chunk (empty nodeBlasData => every
  // node is grid-resident). A chunked (non-grid) node is absent from the combined walk, so its presence
  // forces the loop to run -- that is the per-node-BLAS half of "keep both".
  const bool skipMeshPerNodePass = useBlas && allMeshNodesBlasEligible && nodeBlasData.empty();

  // Per-node trace of the mesh list (mesh + convex nodes). A BLAS covers only BLAS-eligible (IDENT)
  // mesh nodes; those are traced by the BLAS walk below and skipped here. Convex and non-IDENT mesh
  // nodes are never in the BLAS, so they must still be traced per-node or they'd be silently dropped.
  if (meshNodesHead != CollisionNode::INVALID_IDX && !skipMeshPerNodePass)
  {
    TIME_PROFILE_DEV(collres_trace_mesh_per_node);
    CollResProfileStats profileStats;
    profileStats.meshNodesNum = numMeshNodes;
#if DAGOR_DBGLEVEL > 0
    Finally addTag([&profileStats] { addMeshNodesProfileTag(profileStats); });
#endif

    bbox3f traceBox;
    if (!is_single_ray)
    {
      v_bbox3_init_empty(traceBox);
      for (const CollisionTrace &trace : traces)
      {
        if (!trace.isectBounding)
          continue;
        vec3f rMin = v_min(trace.vFrom, trace.vTo);
        vec3f rMax = v_max(trace.vFrom, trace.vTo);
        if (isTraceByCapsule)
        {
          rMin = v_sub(rMin, v_splats(trace.capsuleRadius));
          rMax = v_add(rMax, v_splats(trace.capsuleRadius));
        }
        v_bbox3_add_pt(traceBox, rMin);
        v_bbox3_add_pt(traceBox, rMax);
      }
    }
    // Nodes whose triangles are in the WALKED grid are covered by the combined BLAS walk below; skip
    // them in the loop to avoid double-tracing. Membership (GRID_TRACEABLE/GRID_PHYS, stamped from the
    // grid's own ranges) is the exact test -- NOT eligibility or residency: chunking/residency is keyed
    // off the AUTHORITATIVE grid (getBlasGridForResidentNode), so a chunked node can still sit in the
    // other, walked grid (collidable grid gate-vetoed), and an appended node (collres__desc_add) is
    // eligible-looking yet in NO grid and must fall through to its per-node chunk walk. Convex and
    // non-IDENT nodes never carry the flags and fall through too.
    const uint8_t walkedGridFlag =
      useBlas ? (isCollidableGridForTrace(behavior_filter) ? CollisionNode::GRID_PHYS : CollisionNode::GRID_TRACEABLE) : 0;
    const GeomNodeTree *instTree = instance.getTree();
    // An owned pose composes stored matrices; a tree-backed one reads live wtms and cannot take
    // the identity shortcut below.
    const bool ownedPose = instTree == nullptr;
    for (uint16_t mi = meshNodesHead; DAGOR_LIKELY(mi != CollisionNode::INVALID_IDX); mi = allNodesList[mi].nextNode)
    {
      const CollisionNode *meshNode = &allNodesList[mi];
      if (meshNode->flags & walkedGridFlag)
        continue; // covered by the grid walk below
      // Tree-backed poses read the live wtm; bind meta stays conservative under the no-scale
      // contract (temp views alias it verbatim). One fetch feeds the hide gate, the traceable gate
      // and the pose class: each per-node accessor otherwise re-bounds-checks its slot (temp views
      // through the PoseView delegation), and the absent-slot fallback already encodes exactly
      // the answers the individual accessors give out of range (enabled, not traceable).
      const CollisionResourceInstance::PoseMeta &pm = instance.getPoseMeta(meshNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue; // structurally hidden or untraceable pose: skipped before any filter
      if (!filter(meshNode))
        continue;

      const float nodeMaxTmScale = pm.maxTmScale;
      const uint8_t nodeTmFlags = pm.flags;
      alignas(EA_CACHE_LINE_SIZE) mat44f nodeTm, nodeItm;
      // A stored identity composes to the entity tm bit-for-bit, so taking it directly skips the
      // 3x4 load, the mat44 conversion and the compose for every node placed at the origin --
      // which is most of them in a real resource.
      const bool poseIsIdent = ownedPose && pm.isPoseIdentity();
      if (DAGOR_LIKELY(poseIsIdent))
        nodeTm = tm;
      else
        nodeTm = getPosedNodeWtmInline(meshNode, tm, woffset, instance);
      vec3f bsphCenter = v_mat44_mul_vec3p(nodeTm, v_ldu(&meshNode->boundingSphere.c.x));
      // A driven node's wtm lives in tree world: the trace tm never composes into it, so its
      // scale must not size the reject (an entity scale < 1 would under-cover the sphere).
      const bool treeDriven = instTree && meshNode->geomNodeId.index() < instTree->nodeCount();
      float bsphR2 = get_bsphere_r2(meshNode->boundingSphere.r) * (treeDriven ? 1.f : max_tm_scale_sq) * sqr(nodeMaxTmScale);
      if (is_single_ray)
      {
        vec3f vFrom = traces.front().vFrom;
        vec3f vDir = traces.front().vDir;
        float r2 = isTraceByCapsule ? sqr(sqrtf(bsphR2) + traces.front().capsuleRadius) : bsphR2;
        if (!v_test_ray_sphere_intersection(vFrom, vDir, v_splats(traces.front().t), bsphCenter, v_set_x(r2)))
          continue;
      }
      else
      {
        if (!v_bbox3_test_sph_intersect(traceBox, bsphCenter, v_set_x(bsphR2)))
          continue;
      }
      profileStats.meshNodesSphCheckPassed++;

      // both instance and node tms
      bool isOrthonormalizedTm = orthonormalized_instance_tm && (nodeTmFlags & CollisionNode::ORTHONORMALIZED);

      // ORTHONORMALIZED is an epsilon class: within it the transpose maps segments by s
      // instead of 1/s and truncates them against the geometry. The full inverse keeps
      // hit/miss exact, and tm * inv(tm) = I preserves the parameter scale, so the
      // orthonormal-arm t semantics below stay valid without a rescale. An identity pose
      // reuses the per-call inverse: same input matrix, so the same bits without per-node cost.
      if (DAGOR_LIKELY(poseIsIdent))
      {
        if (DAGOR_UNLIKELY(!itmComputed))
        {
          v_mat44_inverse43(itm, tm);
          itmComputed = true;
        }
        nodeItm = itm;
      }
      else
        v_mat44_inverse43(nodeItm, nodeTm);

      // BLAS-resident materialisation: this per-node fallback is the only forEachIntersectedNode path
      // for capsule traces (useBlas is false when isTraceByCapsule), and ray traces land here when the
      // target grid is empty. The pfn / traceCapsule* / capsuleHit* helpers read `verts_base +
      // node.verticesOfs` / `idx_base + node.indicesOfs`, but BLAS-resident MESH nodes dropped both
      // raw slices. resolveNodeVertsForCall materialises verts (and resident indices) into framemem scratches
      // and rebases the node copy's verticesOfs/indicesOfs to 0 so the helpers index unchanged.
      // Chunk routing: ray closest/any-hit, ray all-hits, and capsule traces descend the node's
      // per-node quad-BLAS instead of the scalar per-tri loops. Skipping resolveNodeVertsForCall
      // below is half the win: materialising a big node's verts into a scratch per call is exactly
      // the cost the chunk removes. Only all-hits CAPSULE stays out (unimplemented -- asserts in the
      // ALL_INTERSECTIONS arm).
      const bool useNodeChunk =
        meshNode->nodeBlasOfs != ~0u && !(trace_mode == ALL_INTERSECTIONS && trace_type == CollisionTraceType::TRACE_CAPSULE);
      vec3f chunkScale = v_zero(), chunkQOfs = v_zero();
      const uint8_t *chunkTree = nullptr;
      soa4::RootRef chunkRootRef;
      BlasLocalUnquant chunkUnquant = {};
      // These locals feed only the traceChunkRay lambda (ray closest / any-hit). All-hits TRACE_RAY
      // descends via traceAllHitsNodeChunk, which decodes the chunk frame itself, so skip the
      // pre-decode there instead of decoding the frame twice per node.
      const bool needsChunkRayDecode = trace_mode != ALL_INTERSECTIONS || trace_type == CollisionTraceType::RAY_HIT;
      if (DAGOR_UNLIKELY(useNodeChunk) && !isTraceByCapsule && needsChunkRayDecode)
      {
        // q-space transform (chunk header scale) for the tree walk; decode frame (block bmin/invScale)
        // for hit normals.
        const NodeChunkFrame fr = decode_node_chunk_frame(nodeBlasData.data() + meshNode->nodeBlasOfs);
        chunkScale = fr.scale;
        chunkQOfs = fr.qOfs;
        chunkTree = fr.tree;
        chunkRootRef = fr.rootRef;
        chunkUnquant = fr.unq;
      }

      dag::Vector<Point3_vec4, framemem_allocator> blasResidentVScratch;
      dag::Vector<uint32_t, framemem_allocator> blasResidentIScratch;
      CollisionNode rebasedNodeCopy;
      const Point3_vec4 *vertsForCall = nullptr;
      const uint32_t *idxForCall = nullptr;
      const CollisionNode *nodeForCall = nullptr;
      if (!useNodeChunk)
        resolveNodeVertsForCall(*meshNode, blasResidentVScratch, blasResidentIScratch, rebasedNodeCopy, vertsForCall, idxForCall,
          nodeForCall);
      if (DAGOR_UNLIKELY(isGridResident(*meshNode) && getBlasGridForResidentNode(*meshNode).blasData.empty()))
        continue; // defensive: residency stamped but grid wasn't built. Skip silently.

      // |nodeItm d| under-bounds the radial widening of a mixed-scale node frame; capsules need
      // the full inverse stretch: exact for uniform scale, conservative for shear, and it covers
      // the class tolerance bands without a fixed pad (a down-scale inside the band needs MORE
      // than any constant). A bit-exact rigid basis has inverse stretch exactly 1: skip the
      // spectral path for that common population instead of paying it per node per capsule call.
      const float nodeItmCapsuleScale = !isTraceByCapsule ? 1.f : is_exact_rigid_basis_v(nodeItm) ? 1.f : mat33_spectral_norm(nodeItm);

      for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
      {
        CollisionTrace &trace = traces[traceId];
        if (!trace.isectBounding)
          continue;

        vec3f vNodeLocalFrom = v_mat44_mul_vec3p(nodeItm, trace.vFrom);
        // Linear-part direction map (endpoint differences cancel far from the origin);
        // renormalize only when the mapping changed the length (re-division adds ULP noise).
        vec3f vNodeLocalDir = v_mat44_mul_vec3v(nodeItm, trace.vDir);
        float localT = trace.t;
        const float nodeDirLen = v_extract_x(v_length3_x(vNodeLocalDir));
        if (fabsf(nodeDirLen - 1.f) > 1e-6f)
        {
          vNodeLocalDir = v_div(vNodeLocalDir, v_splats(nodeDirLen));
          localT = trace.t * nodeDirLen;
        }
        if (DAGOR_UNLIKELY(!(localT >= VERY_SMALL_NUMBER)))
          continue;
        float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * nodeItmCapsuleScale : 0.f;

        // check mesh node bounding
        bbox3f bbox = v_ldu_bbox3(meshNode->modelBBox);
        // Degenerate-axis inflation: meshNode->modelBBox can have an exact-zero extent on
        // one axis (a Y=0 ground plane, an axis-aligned curtain mesh, etc.). The slab test
        // then rejects rays that start exactly on the surface even when they hit a triangle
        // -- the FRT brute-force per-tri loop below would have accepted the hit. Inflate
        // degenerate axes by 1e-4 (matching the BVH backend's blas_size_eps) so the slab
        // admits tangent rays. Mirrors the BVH-side fix in buildTLAS.
        {
          vec3f bsize = v_sub(bbox.bmax, bbox.bmin);
          vec3f safeSize = v_max(bsize, v_splats(0.0001f));
          vec3f sizeDelta = v_mul(v_sub(safeSize, bsize), V_C_HALF);
          bbox.bmin = v_sub(bbox.bmin, sizeDelta);
          bbox.bmax = v_add(bbox.bmax, sizeDelta);
        }
        if (isTraceByCapsule)
        {
          // For capsule trace we do simple bbox extension by capsule radius and test ray intersection.
          // It's very fast and gives very low amount of extra false intersections on small radiuses (<<box size).
          v_bbox3_extend(bbox, v_splats(localCapsuleRadius));
        }
        if (!v_test_ray_box_intersection_unsafe(vNodeLocalFrom, vNodeLocalDir, v_set_x(localT), bbox)) // slower than 'ray vs sphere',
                                                                                                       // but more precise for big
                                                                                                       // objects like vehicles
          continue;
        profileStats.meshNodesBoxCheckPassed++;
        profileStats.meshTrianglesTraced += meshNode->indicesCount / 3;

        IF_CONSTEXPR (trace_mode != ALL_INTERSECTIONS || trace_type == CollisionTraceType::RAY_HIT ||
                      trace_type == CollisionTraceType::CAPSULE_HIT)
        {
          if (out_stats && meshNode->nodeIndex < out_stats->size())
            (*out_stats)[meshNode->nodeIndex]++;

          float inOutLocalT = localT;
          vec3f vNodeLocalNorm, vNodeLocalCapsuleHitPos;
          vec3f *normPtr = calc_normal ? &vNodeLocalNorm : nullptr;

          bool isHit = false;
          bool isLightNode = meshNode->indicesCount < (uint32_t)traceMeshNodeLocalApi.threshold;
          // scalar mesh hits keep their historical non-tri ref; chunk hits carry the leaf identity
          tri_ref_t hitRef = tri_ref::makeForNonTri(meshNode->nodeIndex);
          // Per-node chunk descent for ray closest/any-hit: the node-local ray transforms into the
          // chunk's q-space with an unnormalized direction, so the parametric t carries over
          // unchanged. Cull is unconditionally CCW -- the scalar arm it replaces never no-culls
          // (SOLID affects only the all-hits arm and grid two-sidedness), and parity with the
          // scalar path is the invariant here.
          const auto traceChunkRay = [&](bool any_hit) -> bool {
            RayData rd;
            rd.data = chunkTree;
            rd.rayOrigin = v_madd(vNodeLocalFrom, chunkScale, chunkQOfs);
            rd.rayDir = v_mul(vNodeLocalDir, chunkScale);
            rd.t = inOutLocalT;
            rd.calc();
            const collision_blas::LeafAcceptRef acceptAll = [](void *, soa4::LeafRef) -> bool { return true; };
            soa4::LeafRef bestRef = 0;
            int bestSub = 0;
            const bool hit =
              any_hit ? collision_blas::raySoa4AnyHitFilteredOOLCullCCW(rd, chunkRootRef, acceptAll, nullptr, bestRef, bestSub)
                      : collision_blas::raySoa4ClosestFilteredOOLCullCCW(rd, chunkRootRef, acceptAll, nullptr, bestRef, bestSub);
            if (!hit)
              return false;
            inOutLocalT = rd.t; // q-space t == node-local t (unnormalized scaled direction)
            if (normPtr)
            {
              vec3f a, b, c;
              soa4::fetchLeafTri(chunkTree, bestRef, bestSub, chunkUnquant, a, b, c);
              // unnormalized, matching the scalar arm's contract; the common hit tail normalizes once
              *normPtr = v_cross3(v_sub(b, a), v_sub(c, a));
            }
            hitRef = tri_ref::make_node_blas(meshNode->nodeIndex, bestRef, (uint32_t)bestSub, nodeBlasBuildId);
            return true;
          };
          switch (trace_type)
          {
            case CollisionTraceType::TRACE_RAY:
              if (DAGOR_UNLIKELY(useNodeChunk))
              {
                isHit = traceChunkRay(false /*any_hit*/);
                break;
              }
              isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalCullCCW
                                   : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalCullCCW)(vertsForCall, idxForCall,
                *nodeForCall, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, normPtr); //-V522 non-null when !useNodeChunk
              break;
            case CollisionTraceType::TRACE_CAPSULE:
              if (DAGOR_UNLIKELY(useNodeChunk))
              {
                isHit = traceCapsuleNodeChunkCullCCW(*meshNode, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, localCapsuleRadius,
                  vNodeLocalNorm, vNodeLocalCapsuleHitPos);
                break;
              }
              isHit = traceCapsuleMeshNodeLocalCullCCW(vertsForCall, idxForCall, *nodeForCall, vNodeLocalFrom, vNodeLocalDir,
                inOutLocalT, localCapsuleRadius, vNodeLocalNorm, vNodeLocalCapsuleHitPos);
              break;
            case CollisionTraceType::RAY_HIT:
              if (DAGOR_UNLIKELY(useNodeChunk))
              {
                isHit = traceChunkRay(true /*any_hit*/);
                break;
              }
              isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnRayHitMeshNodeLocalCullCCW
                                   : traceMeshNodeLocalApi.heavy.pfnRayHitMeshNodeLocalCullCCW)(vertsForCall, idxForCall, *nodeForCall,
                vNodeLocalFrom, vNodeLocalDir, inOutLocalT);
              break;
            case CollisionTraceType::CAPSULE_HIT:
              if (DAGOR_UNLIKELY(useNodeChunk))
              {
                isHit = capsuleHitNodeChunkCullCCW(*meshNode, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, localCapsuleRadius);
                break;
              }
              isHit = capsuleHitMeshNodeLocalCullCCW(vertsForCall, idxForCall, *nodeForCall, vNodeLocalFrom, vNodeLocalDir,
                inOutLocalT, localCapsuleRadius);
              break;
            default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
          }
          if (isHit)
          {
            hasCollision = true;
            float intersectionT = trace.t * (inOutLocalT / localT);
            vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
            vec3f vIntersectionNorm = v_zero();
            if (trace_mode == FIND_BEST_INTERSECTION)
            {
              trace.t = intersectionT;
              trace.vTo = vIntersectionPos;
            }
            if (calc_normal)
            {
              // Forward rotation is the inverse transpose only for bit-exact rigid bases; the
              // epsilon classes admit shear that tilts a forward-mapped normal off the plane.
              if (DAGOR_LIKELY(isOrthonormalizedTm) && is_exact_rigid_basis_v(nodeTm))
                vIntersectionNorm = v_mat44_mul_vec3v(nodeTm, vNodeLocalNorm);
              else
              {
                mat33f itm33, titm33;
                v_mat33_from_mat44(itm33, nodeItm);
                v_mat33_transpose(titm33, itm33);
                vIntersectionNorm = v_mat33_mul_vec3(titm33, vNodeLocalNorm);
              }
              vIntersectionNorm = v_norm3(vIntersectionNorm);
            }
            if (trace_type == CollisionTraceType::TRACE_CAPSULE)
            {
              // For capsule trace we have custom intersection pos output, which often lie not on a ray
              vIntersectionPos = v_mat44_mul_vec3p(nodeTm, vNodeLocalCapsuleHitPos);
            }
            profileStats.meshTrianglesHits++;
            callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos, hitRef);
            if (trace_mode == ANY_ONE_INTERSECTION)
              return hasCollision;
            if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
            {
              trace.isectBounding = false;
              if (is_single_ray)
                return hasCollision;
            }
          }
        }
        else IF_CONSTEXPR (trace_mode == ALL_INTERSECTIONS)
        {
          if (trace_type == CollisionTraceType::TRACE_CAPSULE)
          {
            G_ASSERT(false); //-V1037 not implemented yet. It's simple, but will generate too much intersections
                             // and need to be redesigned. Maybe return only the best node IN and OUT positions?
            continue;
          }

          if (out_stats && meshNode->nodeIndex < out_stats->size())
            (*out_stats)[meshNode->nodeIndex]++;

          all_collres_nodes_t ret;
          all_collres_tri_indices_t retTriIndices; // scalar arm: per-hit source face index
          all_collres_tri_refs_t retRefs;          // chunk arm: per-hit BLAS tri_ref (parallel to ret)
          bool isHit;
          if (DAGOR_UNLIKELY(useNodeChunk))
            isHit = traceAllHitsNodeChunk(*meshNode, vNodeLocalFrom, vNodeLocalDir, localT, calc_normal, force_no_cull, ret, retRefs);
          else
          {
            bool isLightNode = meshNode->indicesCount < (uint32_t)traceMeshNodeLocalApi.threshold;
            isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalAllHits
                                 : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalAllHits)(vertsForCall, idxForCall, *nodeForCall,
              vNodeLocalFrom, vNodeLocalDir, localT, calc_normal, force_no_cull, ret, retTriIndices);
          }
          if (isHit)
          {
            hasCollision = true;
            mat33f titm33;
            // Forward rotation is the inverse transpose only for bit-exact rigid bases; the
            // epsilon classes admit shear that tilts a forward-mapped normal off the plane.
            const bool exactRigidNormals = isOrthonormalizedTm && is_exact_rigid_basis_v(nodeTm);
            if (!exactRigidNormals && calc_normal)
            {
              mat33f itm33;
              v_mat33_from_mat44(itm33, nodeItm);
              v_mat33_transpose(titm33, itm33);
            }
            for (int j = 0, jn = ret.size(); j < jn; j++)
            {
              const vec4f n_t = ret[j];
              float intersectionT = trace.t * (v_extract_w(n_t) / localT);
              vec3f vIntersectionNorm = v_zero();
              vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
              if (calc_normal)
              {
                mat33f normTm;
                v_mat33_from_mat44(normTm, nodeTm);
                if (DAGOR_UNLIKELY(!exactRigidNormals))
                  normTm = titm33;
                vIntersectionNorm = v_norm3(v_mat33_mul_vec3(normTm, n_t));
              }
              const tri_ref_t hitRef = useNodeChunk ? retRefs[j] : tri_ref::make(meshNode->nodeIndex, (uint32_t)retTriIndices[j]);
              callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos, hitRef);
            }
            profileStats.meshTrianglesHits += ret.size();
          }
        } // allow multiple intersection of one node or not
      } // traces loop
    } // mesh nodes loop

    // Grid-less early-out only: with useBlas set we still need the BLAS walk and the box/sphere/
    // capsule loops below, so don't return here.
    if (!useBlas && DAGOR_LIKELY(!hasNonMeshNodes))
      return hasCollision;
  } // mesh-list per-node pass

  // for normals
  mat33f titm33;
  bool titmCalculated = false;

  // Primitive nodes use the pose type's own primitive source: an instance is it, a legacy view
  // answers with the default instance it wraps -- the knowledge lives in the type, so a future
  // pose type must answer explicitly instead of falling into a hardcoded else.
  const CollisionResourceInstance &primPose = instance.primPoseSource();
  // Resource-local posed geometry tm for a primitive node, form-aware: owned poses read the
  // stored matrix, tree-backed poses compose the live wtm back into resource space (itm and the
  // live wtm share the woffset-relative frame, so their product is exactly resource-local).
  auto primGeometryTm = [&](int node_index) {
    if (DAGOR_LIKELY(!primPose.getTree()))
      return primPose.getNodeGeometryTm(node_index);
    const mat44f world = getPosedNodeWtmInline(&allNodesList[node_index], tm, woffset, primPose);
    mat44f posed;
    v_mat44_mul43(posed, itm, world);
    return geometryTmFromPosed(node_index, posed, primPose.getPoseMeta(node_index));
  };

  // Sparse per-batch cache of non-identity prim inverses, keyed by nodeIndex: sized by the
  // posed prims actually visited, not the node list.
  struct PrimInvCache //-V730 invT/maxInvScale are set under the computed gate; eager init is per-trace waste
  {
    mat44f invT;
    float maxInvScale;
    int nodeIndex = -1;
    bool computed = false;
  };
  dag::Vector<PrimInvCache, framemem_allocator> primInvCache;
  // Node-indexed slot map, BATCH scope like the cache it indexes (a per-ray map would re-append
  // the same node every ray and overflow narrow slots): a scan-based find-or-add is quadratic
  // across the first ray's appends. Sized lazily so plain-IDENT batches never touch it.
  dag::Vector<int, framemem_allocator> primSlotOfNode;

  // The inverse spectral stretch converts world sweep radii to resource units: exact for uniform
  // scale (1 bit-exactly for rigid bases), conservative for shear, and it covers the outer
  // orthonormal tolerance band (down-scales inside the band need MORE than any fixed pad).
  const float itmCapsuleScale = isTraceByCapsule ? mat33_spectral_norm(itm) : 1.f;

  for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
  {
    CollisionTrace &trace = traces[traceId];
    if (!trace.isectBounding)
      continue;

    // Linear-part direction map (endpoint differences cancel far from the origin);
    // renormalize only when the mapping changed the length (re-division adds ULP noise).
    vec3f vLocalFrom = v_mat44_mul_vec3p(itm, trace.vFrom);
    vec3f vLocalDir = v_mat44_mul_vec3v(itm, trace.vDir);
    float localT = trace.t;
    const float outerDirLen = v_extract_x(v_length3_x(vLocalDir));
    if (fabsf(outerDirLen - 1.f) > 1e-6f)
    {
      vLocalDir = v_div(vLocalDir, v_splats(outerDirLen));
      localT = trace.t * outerDirLen;
    }
    if (DAGOR_UNLIKELY(!(localT >= VERY_SMALL_NUMBER)))
      continue;
    vec3f vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
    float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * itmCapsuleScale : 0.f;

    // Pull the resource-local ray into each primitive's stored frame.
    struct PrimRay
    {
      vec3f from, dir;
      float t;
      float capsuleRadius;
    };
    // pm comes from the caller's hide/traceable gate: re-fetching it here would re-bounds-check
    // it for every primitive node of every trace.
    auto makePrimRay = [&](const CollisionNode *node) {
      PrimRay r;
      r.from = vLocalFrom;
      r.dir = vLocalDir;
      r.t = localT;
      r.capsuleRadius = localCapsuleRadius;
      // Callers gate on posed: unplaced or IDENT prims never reach this lambda.
      // A single ray reads each prim once: caching would be one-shot waste, so compute inline
      // instead (compile-time branch). Multiray batches use the O(1) slot map.
      PrimInvCache singleRayPc;
      PrimInvCache *pcp = &singleRayPc;
      if (!is_single_ray)
      {
        if (DAGOR_UNLIKELY(primSlotOfNode.empty()))
          primSlotOfNode.assign(allNodesList.size(), -1);
        int &slot = primSlotOfNode[node->nodeIndex];
        if (slot < 0)
        {
          slot = (int)primInvCache.size();
          primInvCache.push_back();
          primInvCache.back().nodeIndex = node->nodeIndex;
        }
        pcp = &primInvCache[slot];
      }
      PrimInvCache &pc = *pcp;
      if (!pc.computed)
      {
        // ALWAYS the full inverse with parameter rescaling below: ORTHONORMALIZED is an epsilon
        // class, and its transpose shortcut maps a ray by s instead of 1/s while keeping t and
        // radius -- enough to miss a boundary hit on a 1.0004-scale pose.
        const mat44f nodeT = primGeometryTm(node->nodeIndex);
        v_mat44_inverse43(pc.invT, nodeT);
        // The inverse spectral norm keeps anisotropic capsules conservative; plain rays
        // never read it, so do not pay for the norm outside capsule modes.
        if (isTraceByCapsule)
          pc.maxInvScale = mat33_spectral_norm(pc.invT) * 1.0002f;
        pc.computed = true;
      }
      {
        r.from = v_mat44_mul_vec3p(pc.invT, vLocalFrom);
        vec3f nodeTo = v_mat44_mul_vec3p(pc.invT, v_madd(vLocalDir, v_splats(localT), vLocalFrom));
        vec3f fullDir = v_sub(nodeTo, r.from);
        vec4f len = v_length3(fullDir);
        r.t = v_extract_x(len);
        r.dir = v_div(fullDir, len);
        if (isTraceByCapsule)
          r.capsuleRadius = localCapsuleRadius * max(pc.maxInvScale, r.t / localT);
      }
      return r;
    };
    // Posed primitive normals use the full inverse transpose.
    auto posedPrimNormal = [](mat44f_cref full_tm, vec3f local_norm) {
      mat44f ifull;
      v_mat44_inverse43(ifull, full_tm);
      mat33f i33, t33;
      v_mat33_from_mat44(i33, ifull);
      v_mat33_transpose(t33, i33);
      return v_norm3(v_mat33_mul_vec3(t33, local_norm));
    };

    if (useBlas)
    {
      TIME_PROFILE_DEV(collres_trace_mesh_grid);
#if DA_PROFILER_ENABLED
      int tris = getTrianglesCount(behavior_filter);
      DA_PROFILE_TAG(tris, ": %u", tris);
#endif

      // moved it in (BLAS) block for rendinsts only, because in most cases we have not more than one not-mesh node
      // This reject also covers posed primitives, so bind bounds are insufficient.
      if (!v_test_segment_box_intersection(vLocalFrom, vLocalTo, primPose.getRootBBox()))
        continue;

      const Grid &blasGrid = getBlasGrid(behavior_filter);
      const tri_ref::GridSelector gridSelector =
        isCollidableGridForTrace(behavior_filter) ? tri_ref::GridSelector::Collidable : tri_ref::GridSelector::Traceable;
      {
        // BLAS path: walk the combined-per-behavior BLAS once per trace. The bbox check is box-space
        // (the BLAS quantization frame); the per-triangle ray test is resource-local on verts
        // dequantized via BlasLocalUnquant. blas_src_node_for_vert_idx / blas_src_node_for_leaf
        // (file-static above) map a leaf's first-vert vert21 index to its CollisionNode so we can
        // apply filter() and stamp the tri_ref with that node + the leaf's blasToken.
        const BlasLocalUnquant unquantVL = BlasLocalUnquant::make(blasGrid.blasBBox.bmin, blasGrid.blasScale);
        const uint8_t *bData = blasGrid.blasData.data();

        IF_CONSTEXPR (trace_mode != ALL_INTERSECTIONS || trace_type == CollisionTraceType::RAY_HIT)
        {
          // Closest/any/ray-hit via the canonical SoA BLAS rayCast (collisionTraceOOL): the 4-wide SoA
          // leaf test the occluder/TLAS paths use, cull bound at compile time, per-node filter via a
          // type-erased accept callback. The ray fed in is UNNORMALIZED box-space (origin =
          // local*scale+ofs, dir = local_dir*scale) so the returned t equals the resource-local t (see
          // BlasBoxRay) and the prune bound r.t is the local ray length. Cull: backface-cull CCW unless
          // this grid replaces a two-sided FRT (force_no_cull only affected the all-hits path). A
          // filter-rejected hit restores r.t inside the OOL so it can't shadow a farther accepted hit;
          // any-hit stops at the first accepted hit.
          RayData rd;
          rd.data = bData;
          rd.rayOrigin = v_madd(vLocalFrom, blasGrid.blasScale, blasGrid.blasOfs);
          rd.rayDir = v_mul(vLocalDir, blasGrid.blasScale);
          rd.t = localT;
          rd.calc();

          // Type-erased per-leaf filter: recover the source node, run the caller's filter. Invoked only
          // on leaves the SoA test hit (rare), so the indirect call is off the hot path.
          struct AcceptCtx
          {
            const Grid *grid = nullptr;
            const CollisionNode *nodes = nullptr;
            const filter_t *filter = nullptr;
            // Source node of the last ACCEPTED leaf. The OOL updates its out_ref on exactly the
            // accepted leaves, so at exit this is out_ref's node -- no second leaf decode needed.
            uint16_t srcNodeAccepted = 0;
          } actx{&blasGrid, allNodesList.data(), &filter};
          const collision_blas::LeafAcceptRef acceptLeaf = [](void *ctx, soa4::LeafRef ref) -> bool {
            AcceptCtx *c = (AcceptCtx *)ctx;
            const uint16_t srcNode = blas_src_node_for_leaf(*c->grid, ref);
            if (!(*c->filter)(&c->nodes[srcNode]))
              return false;
            c->srcNodeAccepted = srcNode;
            return true;
          };

          constexpr bool anyHit = (trace_mode == ANY_ONE_INTERSECTION) || (trace_type == CollisionTraceType::RAY_HIT);
          soa4::LeafRef bestRef = 0;
          int bestSubTri = 0;
          bool foundHit;
          IF_CONSTEXPR (anyHit)
            foundHit =
              blasGrid.blasTwoSided
                ? collision_blas::raySoa4AnyHitFilteredOOL(rd, blasGrid.blasRootRef, acceptLeaf, &actx, bestRef, bestSubTri)
                : collision_blas::raySoa4AnyHitFilteredOOLCullCCW(rd, blasGrid.blasRootRef, acceptLeaf, &actx, bestRef, bestSubTri);
          else
            foundHit =
              blasGrid.blasTwoSided
                ? collision_blas::raySoa4ClosestFilteredOOL(rd, blasGrid.blasRootRef, acceptLeaf, &actx, bestRef, bestSubTri)
                : collision_blas::raySoa4ClosestFilteredOOLCullCCW(rd, blasGrid.blasRootRef, acceptLeaf, &actx, bestRef, bestSubTri);

          if (foundHit)
          {
            hasCollision = true;
            const uint16_t bestSrcNode = actx.srcNodeAccepted; // == bestRef's node, see AcceptCtx
            const CollisionNode *meshNode = &allNodesList[bestSrcNode];
            float localIntersectionT = rd.t; // box-space t == resource-local t (unnormalized scaled dir)
            float intersectionT = trace.t * (localIntersectionT / localT);
            vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
            vec3f vIntersectionNorm = v_zero();
            if (trace_mode == FIND_BEST_INTERSECTION)
            {
              trace.t = intersectionT;
              localT = localIntersectionT;
              vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
            }
            if (calc_normal)
            {
              // The SoA test ran in box-space; recover the winning sub-triangle's resource-local verts
              // (unquantVL) for the geometric normal, same winding as the scalar path.
              vec3f qa, qb, qc;
              soa4::fetchLeafTri(bData, bestRef, bestSubTri, unquantVL, qa, qb, qc);
              vec3f vLocalNorm = v_norm3(v_cross3(v_sub(qb, qa), v_sub(qc, qa)));
              if (orthonormalized_instance_tm)
                vIntersectionNorm = v_mat44_mul_vec3v(tm, vLocalNorm);
              else
              {
                if (!titmCalculated)
                {
                  mat33f itm33;
                  v_mat33_from_mat44(itm33, itm);
                  v_mat33_transpose(titm33, itm33);
                  titmCalculated = true;
                }
                vIntersectionNorm = v_norm3(v_mat33_mul_vec3(titm33, vLocalNorm));
              }
            }
            callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos,
              tri_ref::make_blas(bestSrcNode, bestRef, (uint32_t)bestSubTri, gridSelector));
            if (trace_mode == ANY_ONE_INTERSECTION)
              return hasCollision;
            if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
              goto next_trace;
          }
        }
        else IF_CONSTEXPR (trace_mode == ALL_INTERSECTIONS)
        {
          G_ASSERT(trace_type == CollisionTraceType::TRACE_RAY); // capsule not supported
          // All-hits: bbox prune uses fixed localT so no far hit is missed; each per-tri test starts
          // from a fresh `t = localT` so independent hits don't bleed into each other.
          const float fullLocalT = localT;
          if (!orthonormalized_instance_tm && !titmCalculated)
          {
            mat33f itm33;
            v_mat33_from_mat44(itm33, itm);
            v_mat33_transpose(titm33, itm33);
            titmCalculated = true;
          }
          // Box-space ray for the per-node bbox prune (the triangle test below uses the local ray
          // directly). Only the all-hits walk needs it.
          const BlasBoxRay bRay = BlasBoxRay::make(vLocalFrom, vLocalDir, blasGrid.blasScale, blasGrid.blasOfs);
          const uint32_t vertsOfs = blasGrid.blasVertsOfs();
          // All-hits: cull CCW unless this grid replaces a two-sided FRT or the caller forces no-cull.
          // Mirrors traceRayMeshNodeLocalAllHits's `noCull = force_no_cull || SOLID`; SOLID nodes never
          // enter a BLAS, so force_no_cull is the only no-cull source on top of the grid's two-sided
          // mode. Constant for the whole walk, bound once to a compile-time RayTriangleIntersect below.
          auto walkAll = [&]<bool CullCCW>() {
            soa4::iterateFiltered(
              bData, blasGrid.blasRootRef,
              [fullLocalT, &bRay](vec3f bmn, vec3f bmx) {
                return RayIntersectsBoxT0T1(v_madd(bmn, bRay.dirInv, bRay.originScaled), v_madd(bmx, bRay.dirInv, bRay.originScaled),
                  fullLocalT);
              },
              [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef ref, int subTri, int apexByteOfs) -> bool {
                // All-hits walker never early-exits: the bool return is the iterateFiltered protocol;
                // always return false (continue) here. subTri 0..3 identifies the sub-triangle.
                float t = fullLocalT;
                Point2 bc;
                if (RayTriangleIntersect<CullCCW>(vLocalFrom, vLocalDir, v0, v1, v2, t, bc))
                {
                  const uint16_t srcNode =
                    blas_src_node_for_vert_idx(blasGrid, ((uint32_t)apexByteOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE);
                  const CollisionNode *meshNode = &allNodesList[srcNode];
                  if (filter(meshNode))
                  {
                    hasCollision = true;
                    float intersectionT = trace.t * (t / fullLocalT);
                    vec3f vIntersectionNorm = v_zero();
                    vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
                    if (calc_normal)
                    {
                      vec3f vLocalNorm = v_norm3(v_cross3(v_sub(v1, v0), v_sub(v2, v0)));
                      mat33f normTm;
                      if (!orthonormalized_instance_tm)
                        normTm = titm33;
                      else
                        v_mat33_from_mat44(normTm, tm);
                      vIntersectionNorm = v_norm3(v_mat33_mul_vec3(normTm, vLocalNorm));
                    }
                    callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos,
                      tri_ref::make_blas(srcNode, ref, (uint32_t)subTri, gridSelector));
                  }
                }
                return false;
              },
              unquantVL);
          };
          if (blasGrid.blasTwoSided || force_no_cull)
            walkAll.template operator()<false>(); // two-sided
          else
            walkAll.template operator()<true>(); // CullCCW
        }
      } // BLAS path
    } // if (useBlas)

    for (uint16_t bi = boxNodesHead; bi != CollisionNode::INVALID_IDX; bi = allNodesList[bi].nextNode)
    {
      const CollisionNode *boxNode = &allNodesList[bi];
      const CollisionResourceInstance::PoseMeta &pm = primPose.getPoseMeta(boxNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      if (!filter(boxNode))
        continue;
      // A box is an OOBB, so a placed one costs a ray transform into its frame -- the same work
      // the pre-instance dispatch did. An UNPLACED one cost nothing back then, because its stored
      // geometry was already resource-local, and it must cost nothing now: no frame pull, no ray
      // struct, no per-node branch on the result -- just the per-trace ray as-is.
      const bool posed = !(pm.flags & CollisionNode::IDENT);
      vec3f boxFrom = vLocalFrom, boxTo = vLocalTo;
      if (DAGOR_UNLIKELY(posed))
      {
        const PrimRay pr = makePrimRay(boxNode);
        if (pr.t < VERY_SMALL_NUMBER)
          continue;
        boxFrom = pr.from;
        boxTo = v_madd(pr.dir, v_splats(pr.t), pr.from);
      }
      float atMin = 1.f, atMax = 1.f; // [0; 1]
      int side = 0;
      bool isHit = false;
      bbox3f modelBBox = v_ldu_bbox3(boxNode->modelBBox);
      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          side = v_segment_box_intersection_side(boxFrom, boxTo, modelBBox, atMin, atMax);
          isHit = side != -1;
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          G_ASSERTF(false, "CollisionResource trace failed: capsule trace for box nodes is not implemented yet"); //-V1037
          // Very complex when done correct for all theoretically possible cases
          // Try to use traceRay for collres with box nodes
          break;
        case CollisionTraceType::RAY_HIT: isHit = v_test_segment_box_intersection(boxFrom, boxTo, modelBBox); break;
        case CollisionTraceType::CAPSULE_HIT:
          G_ASSERTF(false, "CollisionResource trace failed: capsule rayhit for box nodes is not implemented yet"); //-V1037
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        // atMin is frame-invariant because it is a segment fraction.
        float intersectionT = trace.t * atMin;
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          localT *= atMin; // keep the local length consistent with the tightened trace.t below
          trace.t = intersectionT;
          vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
        }
        if (calc_normal)
        {
          if (DAGOR_UNLIKELY(posed))
          {
            // Non-orthogonal poses require the inverse transpose.
            mat44f fullTm;
            v_mat44_mul43(fullTm, tm, primGeometryTm(boxNode->nodeIndex));
            const vec3f faceAxis = side % 3 == 0 ? V_C_UNIT_1000 : (side % 3 == 1 ? V_C_UNIT_0100 : V_C_UNIT_0010);
            vIntersectionNorm = posedPrimNormal(fullTm, faceAxis);
          }
          else if (DAGOR_LIKELY(orthonormalized_instance_tm))
          {
            switch (side % 3) // better than index
            {
              case 0: vIntersectionNorm = tm.col0; break;
              case 1: vIntersectionNorm = tm.col1; break;
              case 2: vIntersectionNorm = tm.col2; break;
            }
          }
          else
          {
            // A sheared outer tm maps face normals by the INVERSE TRANSPOSE: a normalized tangent
            // column still tilts off the true world face plane.
            const vec3f faceAxis = side % 3 == 0 ? V_C_UNIT_1000 : (side % 3 == 1 ? V_C_UNIT_0100 : V_C_UNIT_0010);
            vIntersectionNorm = posedPrimNormal(tm, faceAxis);
          }
          if (side < 3)
            vIntersectionNorm = v_neg(vIntersectionNorm);
        }
        callback(traceId, boxNode, intersectionT, vIntersectionNorm, vIntersectionPos, tri_ref::makeForNonTri(boxNode->nodeIndex));
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // box nodes loop

    for (uint16_t si = sphereNodesHead; si != CollisionNode::INVALID_IDX; si = allNodesList[si].nextNode)
    {
      const CollisionNode *sphereNode = &allNodesList[si];
      const CollisionResourceInstance::PoseMeta &pm = primPose.getPoseMeta(sphereNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      // Zero-vert marker: adding a sweep radius to the negative marker would resurrect it as
      // positive geometry in the capsule arms.
      if (DAGOR_UNLIKELY(sphereNode->boundingSphere.r < 0.f))
        continue;
      if (!filter(sphereNode))
        continue;
      // Unplaced primitives keep the pre-instance shape: their stored geometry is already
      // resource-local, so the per-trace ray is used as-is with no per-node frame pull.
      const bool posed = !(pm.flags & CollisionNode::IDENT);
      vec3f primFrom = vLocalFrom, primDir = vLocalDir;
      float primT = localT, primR = localCapsuleRadius;
      if (DAGOR_UNLIKELY(posed))
      {
        const PrimRay pr = makePrimRay(sphereNode);
        if (pr.t < VERY_SMALL_NUMBER)
          continue;
        primFrom = pr.from;
        primDir = pr.dir;
        primT = pr.t;
        primR = pr.capsuleRadius;
      }
      bool isHit = false;
      vec3f bsphPos = v_ldu(&sphereNode->boundingSphere.c.x);
      vec4f bsphR2 = v_set_x(get_bsphere_r2(sphereNode->boundingSphere.r));
      vec4f bsphCapsuleR = v_set_x(sphereNode->boundingSphere.r + primR);
      vec4f bsphCapsuleR2 = v_mul(bsphCapsuleR, bsphCapsuleR);
      vec4f inOutLocalT = v_set_x(primT);

      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY: isHit = v_ray_sphere_intersection(primFrom, primDir, inOutLocalT, bsphPos, bsphR2); break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit = v_ray_sphere_intersection(primFrom, primDir, inOutLocalT, bsphPos, bsphCapsuleR2);
          break;
        case CollisionTraceType::RAY_HIT:
          isHit = v_test_ray_sphere_intersection(primFrom, primDir, v_splat_x(inOutLocalT), bsphPos, bsphR2);
          break;
        case CollisionTraceType::CAPSULE_HIT:
          isHit = v_test_ray_sphere_intersection(primFrom, primDir, v_splat_x(inOutLocalT), bsphPos, bsphCapsuleR2);
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * (v_extract_x(inOutLocalT) / primT);
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          float newLocalT = posed ? localT * (v_extract_x(inOutLocalT) / primT) : v_extract_x(inOutLocalT);
          trace.t = intersectionT;
          localT = newLocalT;
        }
        if (calc_normal || trace_type == CollisionTraceType::TRACE_CAPSULE)
        {
          vec3f vLocalHitPos = v_madd(primDir, v_splat_x(inOutLocalT), primFrom);
          vec3f vLocalNorm = v_sub(vLocalHitPos, v_ldu(&sphereNode->boundingSphere.c.x));
          mat44f fullTm;
          if (DAGOR_UNLIKELY(posed))
            v_mat44_mul43(fullTm, tm, primGeometryTm(sphereNode->nodeIndex));

          if (calc_normal)
          {
            if (DAGOR_UNLIKELY(posed))
              vIntersectionNorm = posedPrimNormal(fullTm, vLocalNorm);
            else if (orthonormalized_instance_tm)
              vIntersectionNorm = v_norm3(v_mat44_mul_vec3v(tm, vLocalNorm));
            else
            {
              if (!titmCalculated)
              {
                mat33f itm33;
                v_mat33_from_mat44(itm33, itm);
                v_mat33_transpose(titm33, itm33);
                titmCalculated = true;
              }
              vIntersectionNorm = v_norm3(v_mat33_mul_vec3(titm33, vLocalNorm));
            }
          }
          if (trace_type == CollisionTraceType::TRACE_CAPSULE)
          {
            // vLocalNorm points to the capsule-expanded sphere hit (length r + capsule radius):
            // normalize before placing the contact on the target sphere surface
            vec3f vLocalPos =
              v_madd(v_norm3(vLocalNorm), v_splats(sphereNode->boundingSphere.r), v_ldu(&sphereNode->boundingSphere.c.x));
            vIntersectionPos = v_mat44_mul_vec3p(posed ? fullTm : tm, vLocalPos);
          }
        }
        callback(traceId, sphereNode, intersectionT, vIntersectionNorm, vIntersectionPos,
          tri_ref::makeForNonTri(sphereNode->nodeIndex));
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // sphere nodes loop

    for (uint16_t ci = capsuleNodesHead; ci != CollisionNode::INVALID_IDX; ci = allNodesList[ci].nextNode)
    {
      const CollisionNode *capsuleNode = &allNodesList[ci];
      const CollisionResourceInstance::PoseMeta &pm = primPose.getPoseMeta(capsuleNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      // Zero-vert marker: a pose write re-classifies it traceable, and the degenerate stored
      // capsule plus a sweep radius would fake a hit at the pose origin.
      if (DAGOR_UNLIKELY(capsuleNode->boundingSphere.r < 0.f))
        continue;
      if (!filter(capsuleNode))
        continue;
      // Unplaced primitives keep the pre-instance shape: their stored geometry is already
      // resource-local, so the per-trace ray is used as-is with no per-node frame pull.
      const bool posed = !(pm.flags & CollisionNode::IDENT);
      vec3f primFrom = vLocalFrom, primDir = vLocalDir;
      float primT = localT, primR = localCapsuleRadius;
      if (DAGOR_UNLIKELY(posed))
      {
        const PrimRay pr = makePrimRay(capsuleNode);
        if (pr.t < VERY_SMALL_NUMBER)
          continue;
        primFrom = pr.from;
        primDir = pr.dir;
        primT = pr.t;
        primR = pr.capsuleRadius;
      }
      float inOutLocalT = primT;
      bool isHit = false;
      vec3f vOutLocalNorm = v_zero();
      vec3f vOutLocalPos = v_zero();

      const Capsule &nodeCapsule = capsules[capsuleNode->capsuleIndex];
      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY: isHit = nodeCapsule.traceRay(primFrom, primDir, inOutLocalT, vOutLocalNorm); break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit = nodeCapsule.traceCapsule(primFrom, primDir, inOutLocalT, primR, vOutLocalNorm, vOutLocalPos);
          break;
        case CollisionTraceType::RAY_HIT: isHit = nodeCapsule.rayHit(primFrom, primDir, primT); break;
        case CollisionTraceType::CAPSULE_HIT: isHit = nodeCapsule.capsuleHit(primFrom, primDir, primT, primR); break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * (inOutLocalT / primT);
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        mat44f fullTm;
        if (DAGOR_UNLIKELY(posed))
          v_mat44_mul43(fullTm, tm, primGeometryTm(capsuleNode->nodeIndex));
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          float newLocalT = posed ? localT * (inOutLocalT / primT) : inOutLocalT;
          trace.t = intersectionT;
          localT = newLocalT;
        }
        if (calc_normal)
        {
          if (DAGOR_UNLIKELY(posed))
            vIntersectionNorm = posedPrimNormal(fullTm, vOutLocalNorm);
          else if (orthonormalized_instance_tm)
            vIntersectionNorm = v_mat44_mul_vec3v(tm, vOutLocalNorm);
          else
          {
            if (!titmCalculated)
            {
              mat33f itm33;
              v_mat33_from_mat44(itm33, itm);
              v_mat33_transpose(titm33, itm33);
              titmCalculated = true;
            }
            vIntersectionNorm = v_norm3(v_mat33_mul_vec3(titm33, vOutLocalNorm));
          }
        }
        if (trace_type == CollisionTraceType::TRACE_CAPSULE)
        {
          vIntersectionPos = v_mat44_mul_vec3p(posed ? fullTm : tm, vOutLocalPos);
        }
        callback(traceId, capsuleNode, intersectionT, vIntersectionNorm, vIntersectionPos,
          tri_ref::makeForNonTri(capsuleNode->nodeIndex));
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // capsule nodes loop
  next_trace:;
  } // traces loop

  return hasCollision;
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

static const float bsphereScale = sqrtf(3.f); // moving nodes can extend outside of bound box

class CollisionResourceTraceAdapter
{
public:
  // Wrapper-frame pose holder for the legacy overloads: a GNT argument materializes a stack temp
  // view over the tree (aliasing the default-pose metadata, zero allocation, never refreshed), a
  // null one resolves to the embedded default instance, which is never tree-backed. Neither can be
  // a pose generation behind, so mayRefresh is false. Never copied, bound by reference for
  // the duration of one trace call.
  // Read-only pose the legacy entry points hand to the trace core. Both legacy forms read node
  // metadata from the DEFAULT instance -- a tree-backed call aliases it, a tree-less call is it --
  // so one aggregate covers both and the tree pointer alone selects the behaviour. Trivially
  // destructible by design: the core is force-inlined, and a destructor that can free anywhere
  // inside it costs every trace call whether or not it frees.
  struct PoseView
  {
    const CollisionResourceInstance *owned; // default instance: stored matrices and metadata
    const GeomNodeTree *treePtr;            // null for a tree-less call

    bool posed;        // a live tree pose never licenses bind shortcuts
    bool gridResident; // ... nor the grid walk

    const GeomNodeTree *getTree() const { return treePtr; }
    // A temp view over a tree carries no rootBBox and never refreshes; the tree-less form is the
    // default instance itself, so it is not aliased.
    bool isMetaAliased() const { return treePtr != nullptr; }
    bool isPosedSinceBind() const { return posed; }
    bool isGridResidentPoseAtBind() const { return gridResident; }
    // Primitive source for the trace core: both legacy forms read primitives from the
    // default instance the view wraps.
    const CollisionResourceInstance &primPoseSource() const { return *owned; }
    // Borrowed owned-pose accessors: the core reads them only on the tree-less path (the
    // isMetaAliased gates), never for a tree-backed view -- asserted so a future core edit
    // cannot silently read default-instance data for a driven pose.
    bbox3f getRootBBox() const
    {
      G_ASSERT(treePtr == nullptr);
      return owned->getRootBBox();
    }
    mat44f getNodeTm(int node_index) const
    {
      G_ASSERT(treePtr == nullptr);
      return owned->getNodeTm(node_index);
    }
    // One out-of-range contract: the default instance is never aliased, so its own accessor
    // reads the same slice with the same absent-slot fallback.
    const CollisionResourceInstance::PoseMeta &getPoseMeta(int node_index) const { return owned->getPoseMeta(node_index); }
  };

  struct LegacyPose
  {
    static constexpr bool mayRefresh = false;

    PoseView view;

    __forceinline LegacyPose(const CollisionResource &res, const GeomNodeTree *tree)
    {
      const CollisionResourceInstance &def = res.getDefaultInstance();
      view.owned = &def;
      view.treePtr = tree;
      view.posed = tree != nullptr || def.isPosedSinceBind();
      view.gridResident = tree == nullptr && def.isGridResidentPoseAtBind();
    }
    __forceinline const PoseView &pose() const { return view; }
  };

  // Pose holder for the entry points that statically cannot have a tree. They hand the core the
  // embedded default instance itself, so it sees the owning type with none of the view's
  // indirection -- the shape the pre-instance dispatch had. Costs one extra instantiation of the
  // force-inlined core, which is why it is reserved for the compile-time-treeless entry points
  // rather than used for every legacy call.
  struct DefaultPose
  {
    static constexpr bool mayRefresh = false;

    const CollisionResourceInstance *inst;

    __forceinline DefaultPose(const CollisionResource &res) : inst(&res.getDefaultInstance()) {}
    __forceinline const CollisionResourceInstance &pose() const { return *inst; }
  };

  // Pose holder for the instance overloads: a caller-owned instance may be tree-backed, so its
  // derived state may be a pose generation behind and the trace core has to check.
  struct InstancePose
  {
    static constexpr bool mayRefresh = true;

    const CollisionResourceInstance *inst;

    __forceinline InstancePose(const CollisionResource &res, const CollisionResourceInstance &instance) :
      inst(res.resolveInstanceForTrace(instance))
    {}
    __forceinline const CollisionResourceInstance &pose() const { return *inst; }
  };

  static __forceinline LegacyPose pose(const CollisionResource &res, const GeomNodeTree *tree) { return LegacyPose(res, tree); }

  static __forceinline DefaultPose pose(const CollisionResource &res) { return DefaultPose(res); }

  static __forceinline InstancePose pose(const CollisionResource &res, const CollisionResourceInstance &instance)
  {
    return InstancePose(res, instance);
  }

  template <typename pose_t>
  static __forceinline bool traceRayClosest(const CollisionResource &res, const mat44f &tm, const pose_t &pose, const Point3 &from,
    const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, int *out_node_id, const CollisionNodeFilter *filter,
    int ray_mat_id, uint8_t behavior_filter)
  {
    auto nodeFilter = [&](const CollisionNode *node) -> bool {
      return node->checkBehaviorFlags(behavior_filter) && (!filter || !*filter || (*filter)(node->nodeIndex)) &&
             (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
    };

    auto callback = [&](int, const CollisionNode *node, float t, vec3f normal, vec3f, tri_ref_t) {
      in_out_t = t;
      if (out_normal)
        v_stu_p3(&out_normal->x, normal);
      out_mat_id = node->physMatId;
      if (out_node_id)
        *out_node_id = node->nodeIndex;
    };

    return res.forEachIntersectedNode<CollisionResource::FIND_BEST_INTERSECTION, CollisionResource::CollisionTraceType::TRACE_RAY,
      pose_t::mayRefresh>(tm, pose.pose(), v_ldu(&from.x), v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f, behavior_filter,
      nodeFilter, callback, nullptr, false);
  }

  template <typename pose_t>
  static __forceinline bool traceRayAll(const CollisionResource &res, const mat44f &tm, const pose_t &pose, vec3f from, vec3f dir,
    float in_t, CollResIntersectionsType &intersections, bool sort_intersections, uint8_t behavior_filter,
    const CollisionNodeFilter *filter, const CollisionNodeMask *mask, float bsphere_scale, TraceCollisionResourceStats *out_stats,
    bool force_no_cull)
  {
    intersections.clear();
    auto nodeFilter = [&](const CollisionNode *node) -> bool {
      return node->checkBehaviorFlags(behavior_filter) && (!filter || !*filter || (*filter)(node->nodeIndex)) &&
             (!mask || mask->test(node->nodeIndex, true));
    };

    auto callback = [&](int, const CollisionNode *, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      IntersectedNode *inode = (IntersectedNode *)intersections.push_back_uninitialized();
      v_stu_p3(&inode->normal.x, normal);
      inode->intersectionT = t;
      v_stu_p3(&inode->intersectionPos.x, pos);
      inode->triRef = tri_ref;
    };

    res.forEachIntersectedNode<CollisionResource::ALL_INTERSECTIONS, CollisionResource::CollisionTraceType::TRACE_RAY,
      pose_t::mayRefresh>(tm, pose.pose(), from, dir, in_t, true, bsphere_scale, behavior_filter, nodeFilter, callback, out_stats,
      force_no_cull);
    if (sort_intersections)
      sort_collres_intersections(intersections);
    return !intersections.empty();
  }

  template <typename pose_t>
  static __forceinline bool traceCapsuleClosest(const CollisionResource &res, const mat44f &tm, const pose_t &pose, const Point3 &from,
    const Point3 &dir, float in_t, float radius, IntersectedNode &intersection, float bsphere_scale, const CollisionNodeFilter *filter,
    uint8_t behavior_filter)
  {
    CollisionTrace trace;
    initCapsuleTrace(trace, from, dir, in_t, radius);
    float closest = VERY_BIG_NUMBER;

    auto nodeFilter = [&](const CollisionNode *node) -> bool {
      return node->checkBehaviorFlags(behavior_filter) && (!filter || !*filter || (*filter)(node->nodeIndex));
    };
    auto callback = [&](int, const CollisionNode *, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      const vec3f line_pos = v_closest_point_on_line(pos, trace.vFrom, trace.vDir);
      const float distance = v_extract_x(v_length3_sq_x(v_sub(line_pos, pos)));
      if (distance < closest)
      {
        closest = distance;
        v_stu_p3(&intersection.normal.x, normal);
        intersection.intersectionT = t;
        v_stu_p3(&intersection.intersectionPos.x, pos);
        intersection.triRef = tri_ref;
      }
    };

    dag::Span<CollisionTrace> traces(&trace, 1);
    return res.forEachIntersectedNode<CollisionResource::ALL_NODES_INTERSECTIONS, CollisionResource::CollisionTraceType::TRACE_CAPSULE,
      true /*is_single_ray*/, pose_t::mayRefresh>(tm, pose.pose(), traces, true, bsphere_scale, behavior_filter, nodeFilter, callback,
      nullptr, false);
  }

  template <typename pose_t>
  static __forceinline bool traceCapsuleBest(const CollisionResource &res, const mat44f &tm, const pose_t &pose, const Point3 &from,
    const Point3 &dir, float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id)
  {
    auto nodeFilter = [](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(CollisionNode::TRACEABLE); };
    auto callback = [&](int, const CollisionNode *node, float t, vec3f normal, vec3f pos, tri_ref_t) {
      in_out_t = t;
      v_stu_p3(&out_normal.x, normal);
      v_stu_p3(&out_pos.x, pos);
      out_mat_id = node->physMatId;
    };

    CollisionTrace trace;
    initCapsuleTrace(trace, from, dir, in_out_t, radius);
    dag::Span<CollisionTrace> traces(&trace, 1);
    return res.forEachIntersectedNode<CollisionResource::FIND_BEST_INTERSECTION, CollisionResource::CollisionTraceType::TRACE_CAPSULE,
      true /*is_single_ray*/, pose_t::mayRefresh>(tm, pose.pose(), traces, true, 1.f, CollisionNode::TRACEABLE, nodeFilter, callback,
      nullptr, false);
  }

  template <typename pose_t>
  static __forceinline bool traceMultiRay(const CollisionResource &res, const mat44f &tm, const pose_t &pose,
    dag::Span<CollisionTrace> traces, MultirayCollResIntersectionsType &intersections, bool sort_intersections, float bsphere_scale,
    uint8_t behavior_filter, const CollisionNodeMask *mask, TraceCollisionResourceStats *out_stats)
  {
    intersections.clear();
    auto nodeFilter = [&](const CollisionNode *node) -> bool {
      return node->checkBehaviorFlags(behavior_filter) && (!mask || mask->test(node->nodeIndex, true));
    };
    auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      traces[trace_id].outMatId = node->physMatId;
      traces[trace_id].outNodeId = node->nodeIndex;
      traces[trace_id].isHit = true;

      MultirayIntersectedNode *inode = (MultirayIntersectedNode *)intersections.push_back_uninitialized();
      v_stu_p3(&inode->normal.x, normal);
      inode->intersectionT = t;
      v_stu_p3(&inode->intersectionPos.x, pos);
      inode->rayId = trace_id;
      inode->triRef = tri_ref;
    };

    prepareTraces(traces);
    res.forEachIntersectedNode<CollisionResource::ALL_INTERSECTIONS, CollisionResource::CollisionTraceType::TRACE_RAY,
      false /*is_single_ray*/, pose_t::mayRefresh>(tm, pose.pose(), traces, true, bsphere_scale, behavior_filter, nodeFilter, callback,
      out_stats, false);
    if (sort_intersections)
      sort_collres_intersections(intersections);
    return !intersections.empty();
  }

  template <typename pose_t>
  static __forceinline bool rayHit(const CollisionResource &res, const mat44f &tm, const pose_t &pose, const Point3 &from,
    const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *mask, int *out_mat_id)
  {
    auto nodeFilter = [&](const CollisionNode *node) -> bool {
      return node->checkBehaviorFlags(CollisionNode::TRACEABLE) && (!mask || mask->test(node->nodeIndex, true));
    };
    auto callback = [out_mat_id](int, const CollisionNode *node, float, vec3f, vec3f, tri_ref_t) {
      if (out_mat_id)
        *out_mat_id = node->physMatId;
    };

    return res.forEachIntersectedNode<CollisionResource::ANY_ONE_INTERSECTION, CollisionResource::CollisionTraceType::RAY_HIT,
      pose_t::mayRefresh>(tm, pose.pose(), v_ldu(&from.x), v_ldu(&dir.x), in_t, false, bsphere_scale, CollisionNode::TRACEABLE,
      nodeFilter, callback, nullptr, false);
  }

  template <typename pose_t>
  static __forceinline bool capsuleHit(const CollisionResource &res, const mat44f &tm, const pose_t &pose, const Point3 &from,
    const Point3 &dir, float in_t, float radius, CollResHitNodesType &nodes_hit)
  {
    CollisionTrace trace;
    initCapsuleTrace(trace, from, dir, in_t, radius);
    auto nodeFilter = [](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(CollisionNode::TRACEABLE); };
    auto callback = [&](int, const CollisionNode *node, float, vec3f, vec3f, tri_ref_t) { nodes_hit.push_back(node->nodeIndex); };

    dag::Span<CollisionTrace> traces(&trace, 1);
    return res.forEachIntersectedNode<CollisionResource::ALL_NODES_INTERSECTIONS, CollisionResource::CollisionTraceType::CAPSULE_HIT,
      true /*is_single_ray*/, pose_t::mayRefresh>(tm, pose.pose(), traces, false, 1.f, CollisionNode::TRACEABLE, nodeFilter, callback,
      nullptr, false);
  }

  template <typename pose_t>
  static __forceinline bool multiRayHit(const CollisionResource &res, const mat44f &tm, const pose_t &pose,
    dag::Span<CollisionTrace> traces)
  {
    auto nodeFilter = [](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(CollisionNode::TRACEABLE); };
    auto callback = [&](int trace_id, const CollisionNode *node, float, vec3f, vec3f, tri_ref_t) {
      traces[trace_id].isHit = true;
      traces[trace_id].outMatId = node->physMatId;
      traces[trace_id].outNodeId = node->nodeIndex;
    };

    prepareTraces(traces);
    return res.forEachIntersectedNode<CollisionResource::ANY_ONE_INTERSECTION, CollisionResource::CollisionTraceType::RAY_HIT,
      false /*is_single_ray*/, pose_t::mayRefresh>(tm, pose.pose(), traces, false, 1.f, CollisionNode::TRACEABLE, nodeFilter, callback,
      nullptr, false);
  }

private:
  static __forceinline void initCapsuleTrace(CollisionTrace &trace, const Point3 &from, const Point3 &dir, float in_t, float radius)
  {
    trace.vFrom = v_ldu(&from.x);
    trace.vDir = v_ldu(&dir.x);
    trace.vTo = v_madd(trace.vDir, v_splats(in_t), trace.vFrom);
    trace.t = in_t;
    trace.capsuleRadius = radius;
    trace.isectBounding = false;
  }

  static __forceinline void prepareTraces(dag::Span<CollisionTrace> traces)
  {
    for (CollisionTrace &trace : traces)
    {
      trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
      trace.isectBounding = false;
      trace.outMatId = PHYSMAT_INVALID;
      trace.outNodeId = -1;
      trace.isHit = false;
    }
  }
};

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, int &out_node_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  return CollisionResourceTraceAdapter::traceRayClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from,
    dir, in_out_t, out_normal, out_mat_id, &out_node_id, nullptr, PHYSMAT_INVALID, CollisionNode::TRACEABLE);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, const CollisionNodeFilter &filter, int ray_mat_id,
  uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  return CollisionResourceTraceAdapter::traceRayClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from,
    dir, in_out_t, out_normal, out_mat_id, nullptr, &filter, ray_mat_id, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const Point3 &from, const Point3 &dir, float &in_out_t,
  Point3 *out_normal, int &out_mat_id, int ray_mat_id, uint8_t behavior_filter) const
{
  return CollisionResourceTraceAdapter::traceRayClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this), from, dir, in_out_t,
    out_normal, out_mat_id, nullptr, nullptr, ray_mat_id, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter,
  const CollisionNodeMask *collision_node_mask, bool force_no_cull) const
{
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    v_ldu(&from.x), v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, behavior_filter, nullptr, collision_node_mask,
    1.f, nullptr, force_no_cull);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  const CollisionNodeFilter &filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    v_ldu(&from.x), v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, &filter, nullptr, 1.f,
    nullptr, false);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  const CollisionNodeMask &collision_node_mask, TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    v_ldu(&from.x), v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, nullptr,
    &collision_node_mask, bsphereScale, out_stats, false);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
  TraceCollisionResourceStats *out_stats) const
{
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from, dir,
    in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, nullptr, &collision_node_mask, bsphereScale, out_stats,
    false);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const CollisionNodeFilter &filter, const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceCapsuleClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    from, dir, in_t, radius, intersected_node, bsphere_scale, &filter, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  return CollisionResourceTraceAdapter::traceCapsuleBest(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from,
    dir, in_out_t, radius, out_normal, out_pos, out_mat_id);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceCapsuleClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    from, dir, in_t, radius, intersected_node, bsphere_scale, nullptr, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceMultiRay(const mat44f &tm, dag::Span<CollisionTrace> traces, int ray_mat_id,
  uint8_t behavior_filter) const
{
  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };

  auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/, tri_ref_t /*tri_ref*/) {
    CollisionTrace &trace = traces[trace_id];
    v_stu_p3(&trace.norm.x, normal);
    trace.t = t;
    trace.outMatId = node->physMatId;
    trace.outNodeId = node->nodeIndex;
    trace.isHit = true;
  };

  for (CollisionTrace &trace : traces)
  {
    trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
    trace.isectBounding = false;
    trace.outMatId = PHYSMAT_INVALID;
    trace.outNodeId = -1;
    trace.isHit = false;
  }

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY, false /*is_single_ray*/,
    false /*pose_may_refresh: the default instance is never tree-backed*/>(tm, defaultInstance, traces, true /*calc_normal*/,
    1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceMultiRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  dag::Span<CollisionTrace> traces, MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  float bsphere_scale, uint8_t behavior_filter, const CollisionNodeMask *collision_node_mask,
  TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceMultiRay(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), traces,
    intersected_nodes_list, sort_intersections, bsphere_scale, behavior_filter, collision_node_mask, out_stats);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, int &out_node_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  return CollisionResourceTraceAdapter::traceRayClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir,
    in_out_t, out_normal, out_mat_id, &out_node_id, nullptr, PHYSMAT_INVALID, CollisionNode::TRACEABLE);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, const CollisionNodeFilter &filter,
  int ray_mat_id, uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  return CollisionResourceTraceAdapter::traceRayClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir,
    in_out_t, out_normal, out_mat_id, nullptr, &filter, ray_mat_id, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const CollisionResourceInstance &instance, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter,
  const CollisionNodeMask *collision_node_mask, bool force_no_cull) const
{
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), v_ldu(&from.x),
    v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, behavior_filter, nullptr, collision_node_mask, 1.f, nullptr,
    force_no_cull);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  const CollisionNodeFilter &filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), v_ldu(&from.x),
    v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, &filter, nullptr, 1.f, nullptr, false);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  const CollisionNodeMask &collision_node_mask, TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), v_ldu(&from.x),
    v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, nullptr, &collision_node_mask,
    bsphereScale, out_stats, false);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const CollisionResourceInstance &instance, vec3f from, vec3f dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
  TraceCollisionResourceStats *out_stats) const
{
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir, in_t,
    intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, nullptr, &collision_node_mask, bsphereScale, out_stats,
    false);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const CollisionNodeFilter &filter, const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceCapsuleClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir,
    in_t, radius, intersected_node, bsphere_scale, &filter, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  return CollisionResourceTraceAdapter::traceCapsuleBest(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir,
    in_out_t, radius, out_normal, out_pos, out_mat_id);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceCapsuleClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir,
    in_t, radius, intersected_node, bsphere_scale, nullptr, behavior_filter);
}

DAGOR_NOINLINE bool CollisionResource::traceMultiRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  dag::Span<CollisionTrace> traces, MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  float bsphere_scale, uint8_t behavior_filter, const CollisionNodeMask *collision_node_mask,
  TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceMultiRay(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), traces,
    intersected_nodes_list, sort_intersections, bsphere_scale, behavior_filter, collision_node_mask, out_stats);
}

// Materialise a node's verts AND indices into the supplied framemem scratches and produce a
// CollisionNode copy with verticesOfs/indicesOfs rebased to zero: verts decode from the grid (residents)
// or the per-node chunk block (non-residents); indices come from the BLAS leaf walk in both cases
// (the resource keeps no index list at runtime). Lets the per-node trace helpers (traceRayMeshNodeLocal*,
// test_*_node_intersection, traceQuad, etc.) keep their (verts_base + node.verticesOfs) /
// (idx_base + node.indicesOfs) indexing unchanged. BLAS residents are always MESH (Convex never
// enters the BLAS).
void CollisionResource::resolveNodeVertsForCall(const CollisionNode &node, dag::Vector<Point3_vec4, framemem_allocator> &vertsScratch,
  dag::Vector<uint32_t, framemem_allocator> &idxScratch, CollisionNode &node_copy, const Point3_vec4 *&out_verts_base,
  const uint32_t *&out_idx_base, const CollisionNode *&out_node) const
{
  if (!node.hasGeometry())
  {
    // Degenerate-dropped node (no buildable geometry): no chunk/grid slice and nodeBlasOfs may be ~0u,
    // so getPackedNodeVerts21 below would read out of bounds. Hand back an empty rebased copy; zero
    // counts make every (base + ofs) consumer iterate nothing.
    node_copy = node;
    node_copy.verticesOfs = 0;
    node_copy.verticesCount = 0;
    node_copy.indicesOfs = 0;
    vertsScratch.clear();
    idxScratch.clear();
    out_verts_base = vertsScratch.data();
    out_idx_base = idxScratch.data();
    out_node = &node_copy;
    return;
  }
  if (!isGridResident(node))
  {
    // owning mode: decode the node's per-node chunk block into the scratch and materialise the face
    // list from the chunk leaf walk (no persistent index list); rebase verticesOfs/indicesOfs to 0 so
    // callers keep their (base + ofs) idiom.
    const PackedVerts21 p = getPackedNodeVerts21(node);
    const uint32_t vc = (uint32_t)node.verticesCount;
    vertsScratch.resize(vc);
    for (uint32_t i = 0; i < vc; ++i)
      v_st(&vertsScratch[i].x, v_madd(RayData::unpackVert21(p.verts21 + (size_t)i * 8u), p.invScale, p.bmin));
    idxScratch.reserve(node.indicesCount);
    idxScratch.clear();
    walkNodeChunkLeavesForFaces(node, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
      idxScratch.push_back(i0);
      idxScratch.push_back(i1);
      idxScratch.push_back(i2);
    });
    node_copy = node;
    node_copy.verticesOfs = 0;
    node_copy.indicesOfs = 0;
    // The leaf walk is the only index store; buildQuadPrims may have dropped degenerate/duplicate faces,
    // so the source indicesCount can exceed what we materialised. The trace kernels iterate indicesCount
    // over idxScratch, so keep the rebased copy in sync or they read past the scratch buffer.
    node_copy.indicesCount = (uint32_t)idxScratch.size();
    out_verts_base = vertsScratch.data();
    out_idx_base = idxScratch.data();
    out_node = &node_copy;
    return;
  }
  const Grid &g = getBlasGridForResidentNode(node);
  if (DAGOR_UNLIKELY(g.blasData.empty()))
  {
    // Defensive: residency stamped but the resolved grid has no BLAS (shouldn't happen). The
    // node's raw slice is gone (compaction dropped it when stamping), so hand back an empty rebased
    // copy: zero counts make every (base + ofs) consumer loop over nothing instead of decoding
    // garbage (release-safe; the prior fallback re-read meshVertsBase, which no longer exists in
    // owning mode).
    node_copy = node;
    node_copy.verticesOfs = 0;
    node_copy.verticesCount = 0;
    node_copy.indicesOfs = 0;
    node_copy.indicesCount = 0;
    vertsScratch.clear();
    idxScratch.clear();
    out_verts_base = vertsScratch.data();
    out_idx_base = idxScratch.data();
    out_node = &node_copy;
    return;
  }
  const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)node.verticesOfs * BVH_BLAS_VERT21_STRIDE;
  const vec3f invScale = g.blasInvScale;
  const vec3f bmin = g.blasBBox.bmin;
  const uint32_t vc = (uint32_t)node.verticesCount;
  vertsScratch.resize(vc);
  for (uint32_t i = 0; i < vc; ++i)
  {
    vec3f q = RayData::unpackVert21(vbase + i * BVH_BLAS_VERT21_STRIDE);
    v_st(&vertsScratch[i].x, v_madd(q, invScale, bmin));
  }
  // Resident nodes keep no index list; materialise from the BLAS leaf walk and rebase indicesOfs to 0
  // so callers keep their (idx_base + indicesOfs) idiom.
  idxScratch.reserve(node.indicesCount);
  idxScratch.clear();
  iterateNodeFaces((int)node.nodeIndex, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
    idxScratch.push_back(i0);
    idxScratch.push_back(i1);
    idxScratch.push_back(i2);
  });
  node_copy = node;
  node_copy.verticesOfs = 0;
  node_copy.indicesOfs = 0;
  node_copy.indicesCount = (uint32_t)idxScratch.size();
  out_verts_base = vertsScratch.data();
  out_idx_base = idxScratch.data();
  out_node = &node_copy;
}

bool CollisionResource::traceRayMeshNodeLocal(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
  float &in_out_t, vec4f *v_out_norm) const
{
  if (!node.checkBehaviorFlags(CollisionNode::TRACEABLE))
    return false;

  bbox3f bbox = v_ldu_bbox3(node.modelBBox);
  if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), bbox))
    return false;

  // Per-node quad-BLAS descent: skips materialising every chunk vert + face into framemem that
  // resolveNodeVertsForCall would do per ray. Callers that loop nodes (InteractiveObject::traceRay)
  // pay only the tree walk. Falls back to the scalar path for nodes without a chunk BLAS.
  if (DAGOR_LIKELY(node.nodeBlasOfs != ~0u))
    return traceRayNodeChunkCullCCW(node, v_local_from, v_local_dir, in_out_t, v_out_norm);

  dag::Vector<Point3_vec4, framemem_allocator> vScratch;
  dag::Vector<uint32_t, framemem_allocator> iScratch;
  CollisionNode rebased;
  const Point3_vec4 *vbase = nullptr;
  const uint32_t *ibase = nullptr;
  const CollisionNode *nodeForCall = nullptr;
  resolveNodeVertsForCall(node, vScratch, iScratch, rebased, vbase, ibase, nodeForCall);

  bool isLightNode = node.indicesCount < (uint32_t)traceMeshNodeLocalApi.threshold;
  return (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalCullCCW
                      : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalCullCCW)(vbase, ibase, *nodeForCall, v_local_from,
    v_local_dir, in_out_t, v_out_norm);
}


bool CollisionResource::traceRayMeshNodeLocalAllHits(const CollisionNode &node, const Point3 &from, const Point3 &dir, float in_t,
  CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, bool force_no_cull) const
{
  if (!node.checkBehaviorFlags(CollisionNode::TRACEABLE))
    return false;
  const vec4f vLocalFrom = v_ldu_p3(&from.x);
  const vec4f vLocalDir = v_ldu_p3(&dir.x);
  const bbox3f bbox = v_ldu_bbox3(node.modelBBox);
  if (!v_test_ray_box_intersection_unsafe(vLocalFrom, vLocalDir, v_set_x(in_t), bbox))
    return false;

  all_collres_nodes_t allNodes;
  all_collres_tri_indices_t allTriIndices; // scalar arm: per-hit source face index
  all_collres_tri_refs_t allTriRefs;       // chunk arm: per-hit BLAS tri_ref (parallel to allNodes)
  bool res;
  const bool useNodeChunk = node.nodeBlasOfs != ~0u;
  if (DAGOR_LIKELY(useNodeChunk))
    res = traceAllHitsNodeChunk(node, vLocalFrom, vLocalDir, in_t, true, force_no_cull, allNodes, allTriRefs);
  else
  {
    dag::Vector<Point3_vec4, framemem_allocator> vScratch;
    dag::Vector<uint32_t, framemem_allocator> iScratch;
    CollisionNode rebased;
    const Point3_vec4 *vbase = nullptr;
    const uint32_t *ibase = nullptr;
    const CollisionNode *nodeForCall = nullptr;
    resolveNodeVertsForCall(node, vScratch, iScratch, rebased, vbase, ibase, nodeForCall);

    const bool isLightNode = node.indicesCount < (uint32_t)traceMeshNodeLocalApi.threshold;
    // -V::601 for some reason PVS glitches and fails with "The 'true' value becomes a class object" on calc_normal parameter
    res = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalAllHits
                       : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalAllHits)(vbase, ibase, *nodeForCall, vLocalFrom,
      vLocalDir, in_t, true, force_no_cull, allNodes, allTriIndices);
  }

  intersected_nodes_list.reserve(allNodes.size());
  for (int k = 0, cnt = allNodes.size(); k < cnt; k++)
  {
    const vec4f &normAndT = allNodes[k];
    auto &n = intersected_nodes_list.push_back();
    n.intersectionT = v_extract_w(normAndT);
    v_stu_p3(&n.intersectionPos.x, v_madd(vLocalDir, v_splat_w(normAndT), vLocalFrom));
    v_stu_p3(&n.normal.x, normAndT);
    n.triRef = useNodeChunk ? allTriRefs[k] : tri_ref::make(node.nodeIndex, (uint32_t)allTriIndices[k]);
  }
  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);

  return res;
}

bool CollisionResource::testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth, int &out_node_id) const
{
  return testSphereIntersection(filter, sphere, dir_norm, out_norm, out_depth, out_node_id, getDefaultInstance());
}

bool CollisionResource::testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth, int &out_node_id, const CollisionResourceInstance &instance) const
{
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "testSphereIntersection");
  vec4f bsph = v_ldu_bsphere3(sphere);
  // Bind bounds may miss posed nodes.
  if (!v_bbox3_test_sph_intersect(inst->getRootBBox(), bsph, v_splat_w(v_mul(bsph, bsph))))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if ((filter && !filter(meshNode->nodeIndex)) || (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE)))
      continue;
    if (!inst->isNodeEnabled(meshNode->nodeIndex))
      continue;
    Point3 nodeNorm;
    float nodeDepth;
    if (test_sphere_node_intersection(sphere, meshNode, *inst, dir_norm, nodeNorm, nodeDepth))
    {
      const TMatrix &nodeTm = inst->getNodeTmRef(meshNode->nodeIndex);
      if (is_exact_rigid_basis(nodeTm))
        out_norm = nodeTm % nodeNorm; // exact rotation: length preserved
      else
      {
        // Epsilon classes admit shear/slight scale, so even a class-orthonormal pose maps
        // normals by the inverse transpose, then renormalizes.
        mat44f full, ifull;
        v_mat44_make_from_43cu_unsafe(full, nodeTm.array);
        v_mat44_inverse43(ifull, full);
        mat33f i33, t33;
        v_mat33_from_mat44(i33, ifull);
        v_mat33_transpose(t33, i33);
        Point3_vec4 n = nodeNorm;
        v_stu_p3(&out_norm.x, v_norm3(v_mat33_mul_vec3(t33, v_ldu(&n.x))));
      }
      out_depth = nodeDepth;
      out_node_id = meshNode->nodeIndex;
      return true;
    }
  }
  return false;
}

bool CollisionResource::testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius) const
{
  return testCapsuleNodeIntersection(p0, p1, radius, getDefaultInstance());
}

bool CollisionResource::testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius,
  const CollisionResourceInstance &instance) const
{
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "testCapsuleNodeIntersection");
  bbox3f bbox;
  vec3f radV = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, v_ldu_p3_safe(&p0.x), radV);
  v_bbox3_add_pt(bbox, v_sub(v_ldu_p3_safe(&p1.x), radV));
  v_bbox3_add_pt(bbox, v_add(v_ldu_p3_safe(&p1.x), radV));

  if (!v_bbox3_test_box_intersect(bbox, inst->getRootBBox()))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    if (!inst->isNodeEnabled(meshNode->nodeIndex))
      continue;
    if (test_capsule_node_intersection(p0, p1, radius, meshNode, *inst))
      return true;
  }
  return false;
}

VECTORCALL bool CollisionResource::traceQuad(vec3f a00, vec3f a01, vec3f a10, vec3f a11, Point3 &out_point, int &out_node_index) const
{
  bbox3f bbox;
  v_bbox3_init(bbox, a00);
  v_bbox3_add_pt(bbox, a01);
  v_bbox3_add_pt(bbox, a10);
  v_bbox3_add_pt(bbox, a11);

  // Bind bounds may miss posed nodes.
  if (!v_bbox3_test_box_intersect(bbox, defaultInstance.getRootBBox()))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    // same structural contract as every other trace: disabled or singular-posed nodes never test
    if (!defaultInstance.isNodeEnabled(meshNode->nodeIndex) || !defaultInstance.isNodeTraceable(meshNode->nodeIndex))
      continue;
    mat44f nodeTm, nodeItm;
    v_mat44_make_from_43cu_unsafe(nodeTm, defaultInstance.getNodeTmRef(meshNode->nodeIndex).array);
    bbox3f nodeBox;
    v_bbox3_init(nodeBox, nodeTm, v_ldu_bbox3(meshNode->modelBBox));
    if (!v_bbox3_test_box_intersect(nodeBox, bbox))
      continue;
    v_mat44_inverse43(nodeItm, nodeTm);
    vec3f localA00 = v_mat44_mul_vec3p(nodeItm, a00);
    vec3f localA01 = v_mat44_mul_vec3p(nodeItm, a01);
    vec3f localA10 = v_mat44_mul_vec3p(nodeItm, a10);
    vec3f localA11 = v_mat44_mul_vec3p(nodeItm, a11);

    vec3f localQuadUnnormal = v_cross3(v_sub(localA10, localA00), v_sub(localA11, localA10));
    if (v_extract_x(v_length3_sq_x(localQuadUnnormal)) < 1.0e-3f)
      return false;
    vec3f localQuadNormal = v_norm3(localQuadUnnormal);
    vec3f moveDirUnnorm = v_cross3(v_sub(localA11, localA10), localQuadNormal);

    // Walk the node's quad-BLAS filtered by the quad's node-local AABB, so only candidate leaves are
    // tested, instead of materialising the node and brute-forcing every face.
    bbox3f localQuadBox;
    v_bbox3_init(localQuadBox, localA00);
    v_bbox3_add_pt(localQuadBox, localA01);
    v_bbox3_add_pt(localQuadBox, localA10);
    v_bbox3_add_pt(localQuadBox, localA11);

    if (walkNodeTrisInLocalBox(*this, nodeBlasData.data(), *meshNode, meshNode->nodeBlasOfs, localQuadBox.bmin, localQuadBox.bmax,
          [&](vec3f corner0, vec3f corner1, vec3f corner2) -> bool {
            vec3f triangleUnnormal = v_cross3(v_sub(corner2, corner0), v_sub(corner1, corner0));
            if (v_extract_x(v_dot3_x(moveDirUnnorm, triangleUnnormal)) > 0.0f)
              return false;
            if (!v_test_triangle_triangle_intersection(corner0, corner1, corner2, localA00, localA01, localA11) &&
                !v_test_triangle_triangle_intersection(corner0, corner1, corner2, localA00, localA11, localA10))
              return false;
            vec3f trgCenter = v_mul(v_add(v_add(corner0, corner1), corner2), v_splats(0.333333f));
            v_stu_p3(&out_point.x, v_mat44_mul_vec3p(nodeTm, trgCenter));
            out_node_index = meshNode->nodeIndex;
            return true; // any-hit: stop on the first intersecting face
          }))
      return true;
  }
  return false;
}

template <bool check_bounding>
DAGOR_NOINLINE bool CollisionResource::traceRayMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
  const CollisionNode &node,
  const vec4f &v_local_from, // better to hold it in memory
  const vec4f &v_local_dir,  // it prevents inefficient loop optimizations
  float &in_out_t, vec4f *v_out_norm)
{
  int resultIdx = -1;
  const uint32_t *__restrict indices = indicesBase + node.indicesOfs;
  const Point3_vec4 *__restrict vertices = verticesBase + node.verticesOfs;
  const uint32_t indicesSize = node.indicesCount;

  const uint32_t batchSize = 4;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (batchSize * 3 - 1))); i += batchSize * 3)
  {
    bbox3f box;
    v_bbox3_init(box, v_ld(&vertices[indices[i]].x));
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t j = 0; j < batchSize; j++)
    {
      v_bbox3_add_pt(box, vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x));
      v_bbox3_add_pt(box, vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x));
      v_bbox3_add_pt(box, vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x));
    }

    if (check_bounding && DAGOR_LIKELY(!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), box)))
      continue;

    int ret = traceray4TrianglesCullCCW(v_local_from, v_local_dir, in_out_t, vert, batchSize);
    if (ret >= 0)
      resultIdx = i + ret * 3;
  }

  if (DAGOR_UNLIKELY(i < indicesSize))
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    uint32_t count = 0;
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < batchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    int ret = traceray4TrianglesCullCCW(v_local_from, v_local_dir, in_out_t, vert, count);
    if (ret >= 0)
      resultIdx = i + ret * 3;
  }

  if (resultIdx >= 0 && v_out_norm)
  {
    vec4f v0 = v_ld(&vertices[indices[resultIdx + 0]].x);
    vec4f v1 = v_ld(&vertices[indices[resultIdx + 1]].x);
    vec4f v2 = v_ld(&vertices[indices[resultIdx + 2]].x);
    *v_out_norm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
  }
  return resultIdx >= 0;
}

DAGOR_NOINLINE bool CollisionResource::traceCapsuleMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
  const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm,
  vec4f &v_out_pos) const
{
  bool ret = false;
  const uint32_t *__restrict indices = idx_base + node.indicesOfs;
  const Point3_vec4 *__restrict vertices = verts_base + node.verticesOfs;
  const uint32_t indicesSize = node.indicesCount;

  // float bestScore = FLT_MIN;

  for (uint32_t i = 0; DAGOR_LIKELY(i < indicesSize); i += 3)
  {
    vec3f v0 = v_ld(&vertices[indices[i + 0]].x);
    vec3f v1 = v_ld(&vertices[indices[i + 1]].x);
    vec3f v2 = v_ld(&vertices[indices[i + 2]].x);

    // fast bounding check
    bbox3f bbox;
    v_bbox3_init(bbox, v0);
    v_bbox3_add_pt(bbox, v1);
    v_bbox3_add_pt(bbox, v2);
    v_bbox3_extend(bbox, v_splats(radius));
    if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), bbox))
      continue;

    vec4f norm, pos;
    float t = in_out_t;
    if (!test_capsule_triangle_intersection(v_local_from, v_local_dir, v0, v1, v2, radius, t, norm, pos, false))
      continue;
    float hitRadius = v_extract_x(v_distance_to_line_x(pos, v_local_from, v_local_dir));
    if (hitRadius < radius)
    {
      ret = true;
      radius = hitRadius;
      in_out_t = t;
      v_out_norm = norm;
      v_out_pos = pos;
    }
  }

  return ret;
}

template <bool check_bounding>
DAGOR_NOINLINE bool CollisionResource::traceRayMeshNodeLocalAllHits(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
  const CollisionNode &node,
  const vec4f &v_local_from, // better to hold it in memory
  const vec4f &v_local_dir,  // it prevents inefficient loop optimizations
  float in_t, bool calc_normal, bool force_no_cull, all_collres_nodes_t &ret_array, all_collres_tri_indices_t &tri_indices)
{
  const uint32_t *__restrict indices = indicesBase + node.indicesOfs;
  const Point3_vec4 *__restrict vertices = verticesBase + node.verticesOfs;
  const uint32_t indicesSize = node.indicesCount;
  bool noCull = force_no_cull || node.checkBehaviorFlags(CollisionNode::SOLID);

  const uint32_t batchSize = 4;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (batchSize * 3 - 1))); i += batchSize * 3)
  {
    bbox3f box;
    v_bbox3_init(box, v_ld(&vertices[indices[i]].x));
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t j = 0; j < batchSize; j++)
    {
      v_bbox3_add_pt(box, vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x));
      v_bbox3_add_pt(box, vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x));
      v_bbox3_add_pt(box, vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x));
    }

    if (check_bounding && DAGOR_LIKELY(!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_t), box)))
      continue;

    vec4f vInOutT = v_splats(in_t);
    int ret = traceray4TrianglesMask(v_local_from, v_local_dir, vInOutT, vert, noCull);
    if (DAGOR_UNLIKELY(ret != 0))
    {
      alignas(16) float outT[batchSize];
      v_st(outT, vInOutT);
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(1)
#endif
      for (uint32_t j = 0; j < batchSize; j++, ret >>= 1)
      {
        if (DAGOR_UNLIKELY(ret & 1u))
        {
          vec3f vNorm = v_zero();
          if (calc_normal)
          {
            vec4f v0 = v_ld(&vertices[indices[i + j * 3 + 0]].x);
            vec4f v1 = v_ld(&vertices[indices[i + j * 3 + 1]].x);
            vec4f v2 = v_ld(&vertices[indices[i + j * 3 + 2]].x);
            vNorm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
          }
          ret_array.push_back(v_perm_xyzd(vNorm, v_splats(outT[j])));
          tri_indices.push_back(int(i / 3 + j));
        }
      }
    }
  }

  // tail
  if (DAGOR_UNLIKELY(i < indicesSize))
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    uint32_t count = 0;
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < batchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }

    vec4f vInOutT = v_splats(in_t);
    int ret = traceray4TrianglesMask(v_local_from, v_local_dir, vInOutT, vert, noCull);
    if (DAGOR_UNLIKELY(ret != 0))
    {
      alignas(16) float outT[batchSize];
      v_st(outT, vInOutT);
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(1)
#endif
      for (uint32_t j = 0; j < count; j++, ret >>= 1)
      {
        if (DAGOR_UNLIKELY(ret & 1u))
        {
          vec3f vNorm = v_zero();
          if (calc_normal)
          {
            vec4f v0 = vert[j][0];
            vec4f v1 = vert[j][1];
            vec4f v2 = vert[j][2];
            vNorm = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
          }
          ret_array.push_back(v_perm_xyzd(vNorm, v_splats(outT[j])));
          tri_indices.push_back(int(i / 3 + j));
        }
      }
    }
  }
  return !ret_array.empty();
}

template <bool check_bounding>
DAGOR_NOINLINE bool CollisionResource::rayHitMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
  const CollisionNode &node, const vec3f &v_local_from, const vec3f &v_local_dir, float in_t)
{
  const uint32_t *__restrict indices = indicesBase + node.indicesOfs;
  const Point3_vec4 *__restrict vertices = verticesBase + node.verticesOfs;
  const uint32_t indicesSize = node.indicesCount;

  const uint32_t batchSize = 4;

  uint32_t i;
  for (i = 0; DAGOR_LIKELY(int(i) < int(indicesSize - (batchSize * 3 - 1))); i += batchSize * 3)
  {
    bbox3f box;
    v_bbox3_init(box, v_ld(&vertices[indices[i]].x));
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    for (uint32_t j = 0; j < batchSize; j++)
    {
      v_bbox3_add_pt(box, vert[j][0] = v_ld(&vertices[indices[i + j * 3 + 0]].x));
      v_bbox3_add_pt(box, vert[j][1] = v_ld(&vertices[indices[i + j * 3 + 1]].x));
      v_bbox3_add_pt(box, vert[j][2] = v_ld(&vertices[indices[i + j * 3 + 2]].x));
    }

    if (check_bounding && DAGOR_LIKELY(!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_t), box)))
      continue;

    if (rayhit4TrianglesCullCCW(v_local_from, v_local_dir, in_t, vert, batchSize))
      return true;
  }

  if (DAGOR_UNLIKELY(i < indicesSize))
  {
    alignas(EA_CACHE_LINE_SIZE) vec4f vert[batchSize][3];
    uint32_t count = 0;
#if defined(__clang__) || defined(__GNUC__)
#pragma unroll(4)
#endif
    for (uint32_t j = i; j < indicesSize && count < batchSize; j += 3)
    {
      vert[count][0] = v_ld(&vertices[indices[j + 0]].x);
      vert[count][1] = v_ld(&vertices[indices[j + 1]].x);
      vert[count][2] = v_ld(&vertices[indices[j + 2]].x);
      count++;
    }
    if (rayhit4TrianglesCullCCW(v_local_from, v_local_dir, in_t, vert, count))
      return true;
  }

  return false;
}

DAGOR_NOINLINE bool CollisionResource::capsuleHitMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
  const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, float radius) const
{
  const uint32_t *__restrict indices = idx_base + node.indicesOfs;
  const Point3_vec4 *__restrict vertices = verts_base + node.verticesOfs;
  const uint32_t indicesSize = node.indicesCount;

  for (uint32_t i = 0; DAGOR_LIKELY(i < indicesSize); i += 3)
  {
    vec3f v0 = v_ld(&vertices[indices[i + 0]].x);
    vec3f v1 = v_ld(&vertices[indices[i + 1]].x);
    vec3f v2 = v_ld(&vertices[indices[i + 2]].x);

    // fast bounding check
    bbox3f bbox;
    v_bbox3_init(bbox, v0);
    v_bbox3_add_pt(bbox, v1);
    v_bbox3_add_pt(bbox, v2);
    v_bbox3_extend(bbox, v_splats(radius));
    if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_t), bbox))
      continue;

    if (test_capsule_triangle_hit(v_local_from, v_local_dir, v0, v1, v2, radius, in_t, false))
      return true;
  }

  return false;
}

// Shared swept-capsule chunk walk for the two capsule helpers below. Decodes the node chunk frame,
// builds the conservative swept q-space box (per-axis anisotropic radius, padded one cell over the
// exact integer node bounds) from the ENTRY *seg_t / *radius, and for each quad-BLAS triangle the
// swept box overlaps and the per-tri segment-vs-padded-bbox precheck passes, calls per_tri(v0,v1,v2).
// The precheck reads *seg_t / *radius live per leaf, so the closest-hit caller (which shrinks them
// through these pointers) tightens the prune exactly like the scalar loop; the any-hit caller points
// at fixed values. per_tri returns true to stop the walk (any-hit) or false to keep collecting.
template <class TriCb>
static inline void walk_capsule_node_chunk(const uint8_t *node_blas_data, uint32_t node_blas_ofs, vec4f v_local_from,
  vec4f v_local_dir, const float *seg_t, const float *radius, TriCb per_tri)
{
  const NodeChunkFrame fr = decode_node_chunk_frame(node_blas_data + node_blas_ofs);
  const vec3f lTo = v_madd(v_local_dir, v_splats(*seg_t), v_local_from);
  bbox3f swept;
  swept.bmin = v_madd(v_sub(v_min(v_local_from, lTo), v_splats(*radius)), fr.scale, fr.qOfs);
  swept.bmax = v_madd(v_add(v_max(v_local_from, lTo), v_splats(*radius)), fr.scale, fr.qOfs);
  v_bbox3_extend(swept, V_C_ONE);
  soa4::iterateFilteredVerts(
    fr.tree, fr.rootRef,
    [&](vec3f bmn, vec3f bmx) { return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(swept.bmax, bmn), v_cmp_ge(bmx, swept.bmin))); },
    [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int) -> bool {
      bbox3f bbox;
      v_bbox3_init(bbox, v0);
      v_bbox3_add_pt(bbox, v1);
      v_bbox3_add_pt(bbox, v2);
      v_bbox3_extend(bbox, v_splats(*radius));
      if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(*seg_t), bbox))
        return false;
      return per_tri(v0, v1, v2);
    },
    fr.unq);
}

// Chunk twins of the two scalar capsule helpers above: identical per-triangle math (test_capsule_*),
// but triangles come from the node's quad-BLAS filtered by the swept capsule's q-space box instead of
// a full index scan (walk_capsule_node_chunk).
DAGOR_NOINLINE bool CollisionResource::traceCapsuleNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos) const
{
  bool ret = false;
  walk_capsule_node_chunk(nodeBlasData.data(), node.nodeBlasOfs, v_local_from, v_local_dir, &in_out_t, &radius,
    [&](vec3f v0, vec3f v1, vec3f v2) -> bool { //-V657 always-false: collect best hit, never early-exit
      vec4f norm, pos;
      float t = in_out_t;
      if (!test_capsule_triangle_intersection(v_local_from, v_local_dir, v0, v1, v2, radius, t, norm, pos, false))
        return false;
      float hitRadius = v_extract_x(v_distance_to_line_x(pos, v_local_from, v_local_dir));
      if (hitRadius < radius)
      {
        ret = true;
        radius = hitRadius;
        in_out_t = t;
        v_out_norm = norm;
        v_out_pos = pos;
      }
      return false; // collect the best across all leaves
    });
  return ret;
}

DAGOR_NOINLINE bool CollisionResource::capsuleHitNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float in_t, float radius) const
{
  bool hit = false;
  walk_capsule_node_chunk(nodeBlasData.data(), node.nodeBlasOfs, v_local_from, v_local_dir, &in_t, &radius,
    [&](vec3f v0, vec3f v1, vec3f v2) -> bool {
      if (test_capsule_triangle_hit(v_local_from, v_local_dir, v0, v1, v2, radius, in_t, false))
      {
        hit = true;
        return true; // any-hit: stop the walk
      }
      return false;
    });
  return hit;
}

DAGOR_NOINLINE bool CollisionResource::traceRayNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm) const
{
  // Mirrors the traceChunkRay(any_hit=false) lambda in forEachIntersectedNode: node-local ray into the
  // chunk's q-space (unnormalized scaled direction, so parametric t carries over), CCW cull.
  const NodeChunkFrame fr = decode_node_chunk_frame(nodeBlasData.data() + node.nodeBlasOfs);
  RayData rd;
  rd.data = fr.tree;
  rd.rayOrigin = v_madd(v_local_from, fr.scale, fr.qOfs);
  rd.rayDir = v_mul(v_local_dir, fr.scale);
  rd.t = in_out_t;
  rd.calc();
  const collision_blas::LeafAcceptRef acceptAll = [](void *, soa4::LeafRef) -> bool { return true; };
  soa4::LeafRef bestRef = 0;
  int bestSub = 0;
  if (!collision_blas::raySoa4ClosestFilteredOOLCullCCW(rd, fr.rootRef, acceptAll, nullptr, bestRef, bestSub))
    return false;
  in_out_t = rd.t; // q-space t == node-local t (unnormalized scaled direction)
  if (v_out_norm)
  {
    vec3f a, b, c;
    soa4::fetchLeafTri(fr.tree, bestRef, bestSub, fr.unq, a, b, c);
    // unnormalized, matching the scalar traceRayMeshNodeLocalCullCCW normal contract
    *v_out_norm = v_cross3(v_sub(b, a), v_sub(c, a));
  }
  return true;
}

DAGOR_NOINLINE bool CollisionResource::traceAllHitsNodeChunk(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, all_collres_nodes_t &ret_array,
  all_collres_tri_refs_t &ret_refs) const
{
  const NodeChunkFrame fr = decode_node_chunk_frame(nodeBlasData.data() + node.nodeBlasOfs);
  const uint8_t *tree = fr.tree;
  const BlasLocalUnquant unq = fr.unq;
  const vec3f scale = fr.scale;
  const vec3f qOfs = fr.qOfs;
  // ray-segment AABB in the chunk's q-space (the tree boxes are q-space integer boxes), padded a cell
  const vec3f lTo = v_madd(v_local_dir, v_splats(in_t), v_local_from);
  bbox3f rayBox;
  rayBox.bmin = v_madd(v_min(v_local_from, lTo), scale, qOfs);
  rayBox.bmax = v_madd(v_max(v_local_from, lTo), scale, qOfs);
  v_bbox3_extend(rayBox, V_C_ONE);
  const bool noCull = force_no_cull || node.checkBehaviorFlags(CollisionNode::SOLID);
  bool any = false;
  soa4::iterateFiltered(
    tree, fr.rootRef,
    [&](vec3f bmn, vec3f bmx) { return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(rayBox.bmax, bmn), v_cmp_ge(bmx, rayBox.bmin))); },
    [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef ref, int subTri, int) -> bool {
      // Same per-tri predicate as traceRayMeshNodeLocalAllHits (shared 4-wide kernel);
      // no t-pruning -- every crossing within [0, in_t].
      float t = in_t;
      if (traceray1Triangle(v_local_from, v_local_dir, t, v0, v1, v2, noCull))
      {
        const vec3f vNorm = calc_normal ? v_cross3(v_sub(v1, v0), v_sub(v2, v0)) : v_zero();
        ret_array.push_back(v_perm_xyzd(vNorm, v_splats(t)));
        ret_refs.push_back(tri_ref::make_node_blas(node.nodeIndex, ref, (uint32_t)subTri, nodeBlasBuildId));
        any = true;
      }
      return false; // collect across all leaves
    },
    unq);
  return any;
}

// Decode a node's per-face triangle into resource-local vert positions. Two paths:
//   - BLAS-resident (always MESH): walks the node's BLAS leaves (walkBlasResidentNodeLeavesForFaces)
//     to the face_idx-th sub-triangle and decodes its verts from the grid's vert21 array. For
//     low-frequency callers (aiTargetES random face, damage-debug, AssetViewer); hot per-face paths
//     should use iterateNodeFacesVerts.
//   - Non-resident: the original direct-index path; decode each vert from the node's per-node chunk
//     vert21 block (getPackedNodeVerts21). CONVEX nodes always land here.
bool CollisionResource::getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n || face_idx < 0 || (uint32_t)(face_idx * 3 + 2) >= n->indicesCount)
    return false;
  if (isGridResident(*n))
  {
    const Grid &g = getBlasGridForResidentNode(*n);
    if (g.blasData.empty())
      return false;
    const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)n->verticesOfs * BVH_BLAS_VERT21_STRIDE;
    const vec3f invScale = g.blasInvScale;
    const vec3f bmin = g.blasBBox.bmin;
    bool found = false;
    walkBlasResidentNodeLeavesForFaces(*n, [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) {
      if (found || fi != face_idx)
        return;
      vec3f q0 = RayData::unpackVert21(vbase + i0 * BVH_BLAS_VERT21_STRIDE);
      vec3f q1 = RayData::unpackVert21(vbase + i1 * BVH_BLAS_VERT21_STRIDE);
      vec3f q2 = RayData::unpackVert21(vbase + i2 * BVH_BLAS_VERT21_STRIDE);
      v_stu_p3(&v0.x, v_madd(q0, invScale, bmin));
      v_stu_p3(&v1.x, v_madd(q1, invScale, bmin));
      v_stu_p3(&v2.x, v_madd(q2, invScale, bmin));
      found = true;
    });
    return found;
  }
  // owning mode: walk the per-node chunk tree to the face_idx-th sub-triangle, decode from the
  // ownVerts21 block (mirrors the resident arm above; low-frequency caller).
  const PackedVerts21 p = getPackedNodeVerts21(*n);
  bool found = false;
  walkNodeChunkLeavesForFaces(*n, [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) {
    if (found || fi != face_idx)
      return;
    v_stu_p3(&v0.x, v_madd(RayData::unpackVert21(p.verts21 + (size_t)i0 * 8u), p.invScale, p.bmin));
    v_stu_p3(&v1.x, v_madd(RayData::unpackVert21(p.verts21 + (size_t)i1 * 8u), p.invScale, p.bmin));
    v_stu_p3(&v2.x, v_madd(RayData::unpackVert21(p.verts21 + (size_t)i2 * 8u), p.invScale, p.bmin));
    found = true;
  });
  return found;
}

// Decode a tri_ref_t back to the three source-triangle vertices.
// - BLAS refs (type=1): grid bit selects gridForTraceable/gridForCollidable; walk the quad leaf at
//   blasToken directly, pick the sub-triangle per the sub-tri index (0..3), unquantize via the grid's
//   blasScale/blasBBox. No side table -- the leaf carries enough on its own.
// - Non-BLAS refs (type=0): dispatch to getNodeFaceVerts via the encoded per-node srcFace.
// Returns false for non-tri refs, missing geometry and stale node-chunk generations. Leaf tokens
// are trusted (correct by construction); only VALIDATE_TRI_REF_TOKENS builds reject a forged one.
bool CollisionResource::getNodeFaceVertsByRef(tri_ref_t ref, Point3 &v0, Point3 &v1, Point3 &v2) const
{
  if (!tri_ref::hasTri(ref))
    return false;
  if (tri_ref::isNodeBlas(ref))
  {
    // Per-node chunk hit: same quad-leaf decode as the grid arm below, addressed from the node's
    // chunk tree and dequantized through the embedded block's exact frame.
    const CollisionNode *n = getNode((int)tri_ref::nodeIndex(ref));
    if (!n || n->nodeBlasOfs == ~0u)
      return false;
    // Reject a ref minted before the chunks were re-packed/re-chunked: its leaf offset would otherwise
    // decode stale-but-in-bounds geometry from the relocated tree (refs are valid only until the next
    // build). Generation is the low NODE_BLAS_GEN_BITS, so compare nodeBlasBuildId in that width.
    if (tri_ref::nodeBlasGeneration(ref) != (nodeBlasBuildId & ((1u << tri_ref::NODE_BLAS_GEN_BITS) - 1u)))
      return false;
    const NodeChunkFrame fr = decode_node_chunk_frame(nodeBlasData.data() + n->nodeBlasOfs);
    soa4::LeafLoc l;
#if VALIDATE_TRI_REF_TOKENS
    if (!soa4::validateLeafToken(fr.tree, (uint32_t)fr.treeBytes, tri_ref::nodeBlasToken(ref), l))
      return false; // stale/forged token
    const QuadLeafFields f = soa4::leafFields(fr.tree, l);
    if (!soa4::leafEmitsSubTri(f, (uint32_t)tri_ref::subTriIndex(ref)))
      return false; // lane this leaf does not emit: stale/forged token
    const uint32_t chunkVertsOfs = alignVert21StreamOfs((uint32_t)fr.treeBytes) + OWN_VERTS21_HEADER_BYTES;
    if (!soa4::leafVertsInRange(l, f, chunkVertsOfs, chunkVertsOfs + n->verticesCount * 8u))
      return false; // leaf body addresses verts outside the chunk's own block: stale/forged token
#else
    l = soa4::decodeLeafRef(fr.tree, (soa4::LeafRef)tri_ref::nodeBlasToken(ref));
#endif
    vec3f a, b, c;
    soa4::fetchLeafTri(fr.tree, l, (int)tri_ref::subTriIndex(ref), fr.unq, a, b, c);
    v_stu_p3(&v0.x, a);
    v_stu_p3(&v1.x, b);
    v_stu_p3(&v2.x, c);
    return true;
  }
  if (!tri_ref::isBlas(ref))
    return getNodeFaceVerts((int)tri_ref::nodeIndex(ref), (int)tri_ref::faceIndex(ref), v0, v1, v2);

  const Grid &g = tri_ref::isCollidableGrid(ref) ? gridForCollidable : gridForTraceable;
  if (g.blasData.empty())
    return false;
  const uint8_t *d = g.blasData.data();
  soa4::LeafLoc l;
#if VALIDATE_TRI_REF_TOKENS
  // An out-of-range token means a corrupt/forged ref -- assert and fail.
  if (!soa4::validateLeafToken(d, g.blasTreeBytes, tri_ref::blasToken(ref), l))
  {
    G_ASSERTF(0, "getNodeFaceVertsByRef: stale/forged BLAS leaf token 0x%llx", (unsigned long long)tri_ref::blasToken(ref));
    return false;
  }
  const QuadLeafFields f = soa4::leafFields(d, l);
  if (!soa4::leafEmitsSubTri(f, (uint32_t)tri_ref::subTriIndex(ref)))
  {
    G_ASSERTF(0, "getNodeFaceVertsByRef: leaf does not emit sub-tri %u, token 0x%llx", (unsigned)tri_ref::subTriIndex(ref),
      (unsigned long long)tri_ref::blasToken(ref));
    return false;
  }
  if (!soa4::leafVertsInRange(l, f, g.blasVertsOfs(), (uint32_t)g.blasData.size()))
  {
    G_ASSERTF(0, "getNodeFaceVertsByRef: leaf verts escape the vert region, token 0x%llx",
      (unsigned long long)tri_ref::blasToken(ref));
    return false;
  }
#else
  l = soa4::decodeLeafRef(d, (soa4::LeafRef)tri_ref::blasToken(ref));
#endif
  // fetchLeafTri unpacks just the 3 verts of sub-triangle 0..3 (0/1 = quad A, 2/3 = quad B)
  // directly into resource-local space.
  vec3f a, b, c;
  soa4::fetchLeafTri(d, l, (int)tri_ref::subTriIndex(ref), BlasLocalUnquant::make(g.blasBBox.bmin, g.blasScale), a, b, c);
  v_stu_p3(&v0.x, a);
  v_stu_p3(&v1.x, b);
  v_stu_p3(&v2.x, c);
  return true;
}

bool CollisionResource::checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list) const
{
  return checkInclusion(pos, intersected_nodes_list, getDefaultInstance());
}

bool CollisionResource::checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list,
  const CollisionResourceInstance &instance) const
{
  intersected_nodes_list.clear();
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "checkInclusion");
  vec4f vPos = v_ldu(&pos.x);
  // Bind bounds may miss posed nodes.
  if (!v_bbox3_test_pt_inside(inst->getRootBBox(), vPos))
    return false;

  auto nodeTestable = [inst](int node_index) { return inst->isNodeEnabled(node_index) && inst->isNodeTraceable(node_index); };

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if (!meshNode->hasGeometry())
      continue; // degenerate-dropped node: no collision surface, its modelBBox is stale
    if (!nodeTestable(meshNode->nodeIndex))
      continue;
    // Test only for bbox
    TMatrix itm = invInstNodeTm(*inst, meshNode->nodeIndex);
    Point3 localPos = itm * pos;
    if (meshNode->modelBBox & localPos)
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(meshNode->nodeIndex);
  }

  // Pull primitive probes into their stored geometry frames.
  for (uint16_t bi = boxNodesHead; bi != CollisionNode::INVALID_IDX; bi = allNodesList[bi].nextNode)
  {
    const CollisionNode *boxNode = &allNodesList[bi];
    if (nodeTestable(boxNode->nodeIndex) && (boxNode->modelBBox & (invInstNodeTm(*inst, boxNode->nodeIndex) * pos)))
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(boxNode->nodeIndex);
  }

  for (uint16_t si = sphereNodesHead; si != CollisionNode::INVALID_IDX; si = allNodesList[si].nextNode)
  {
    const CollisionNode *sphNode = &allNodesList[si];
    if (nodeTestable(sphNode->nodeIndex) && lengthSq(invInstNodeTm(*inst, sphNode->nodeIndex) * pos - sphNode->boundingSphere.c) <=
                                              get_bsphere_r2(sphNode->boundingSphere.r))
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(sphNode->nodeIndex);
  }

  for (uint16_t ci = capsuleNodesHead; ci != CollisionNode::INVALID_IDX; ci = allNodesList[ci].nextNode)
  {
    const CollisionNode *capNode = &allNodesList[ci];
    if (nodeTestable(capNode->nodeIndex) && capsules[capNode->capsuleIndex].isInside(invInstNodeTm(*inst, capNode->nodeIndex) * pos))
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(capNode->nodeIndex);
  }

  return !intersected_nodes_list.empty();
}

bool CollisionResource::calcOffsetForIntersection(const TMatrix &tm1, const CollisionNode &node_to_move,
  const CollisionNode &node_to_check, Point3 &offset) const
{
  offset.zero();

  G_ASSERTF(node_to_check.type == COLLISION_NODE_TYPE_CONVEX, "final node #%u is not of convex type, only convex is supported",
    (unsigned)node_to_check.nodeIndex);
  TMatrix finalTm = invNodeTm(node_to_check.nodeIndex) * tm1;
  const plane3f *checkPlanes = convexPlanes.data() + node_to_check.planesOfs;
  const int checkPlanesCount = node_to_check.planesCount;
  const int numIterations = 5;
  vec4f vOffset = v_zero();
  if (node_to_move.type == COLLISION_NODE_TYPE_BOX)
  {
    carray<vec4f, 8> boundPoints;
    for (int i = 0; i < boundPoints.size(); ++i)
    {
      Point3_vec4 point = finalTm * node_to_move.modelBBox.point(i);
      boundPoints[i] = v_ld(&point.x);
    }
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < boundPoints.size(); ++i)
      {
        for (int j = 0; j < checkPlanesCount; ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(checkPlanes[j], v_add(boundPoints[i], vOffset)), v_zero());
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), checkPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_SPHERE)
  {
    BSphere3 localSph = finalTm * make_node_bsphere(node_to_move.boundingSphere.c, node_to_move.boundingSphere.r);
    vec4f sph = v_ldu(&localSph.c.x);
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int j = 0; j < checkPlanesCount; ++j)
      {
        vec4f dist = v_max(v_add(v_plane_dist_x(checkPlanes[j], v_add(sph, vOffset)), v_splat_w(sph)), v_zero());
        hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
        vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), checkPlanes[j]));
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_MESH)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    Tab<vec4f> vertices(tmpmem);
    reserve_and_resize(vertices, (uint32_t)node_to_move.verticesCount);
    // Decode vert21 via iterateNodeVerts: the resource holds no raw Point3_vec4 array (verts live in
    // the grid for BLAS-resident nodes, the per-node chunk block otherwise).
    int vi = 0;
    iterateNodeVerts((int)node_to_move.nodeIndex, [&](int, vec4f v) {
      if (vi < vertices.size())
        vertices[vi++] = v_mat44_mul_vec3p(vFinalTm, v);
    });
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < vertices.size(); ++i)
      {
        vec4f vert = vertices[i];
        for (int j = 0; j < checkPlanesCount; ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(checkPlanes[j], v_add(vert, vOffset)), v_zero());
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), checkPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else
    G_ASSERTF(0, "unsupported type to check against box");
  return false;
}

bool CollisionResource::calcOffsetForSeparation(const TMatrix &tm1, const CollisionNode &node_to_move,
  const CollisionNode &node_to_check, const Point3 &axis, Point3 &offset) const
{
  offset.zero();

  G_ASSERTF(node_to_check.type == COLLISION_NODE_TYPE_CONVEX, "final node #%u is not of convex type, only convex is supported",
    (unsigned)node_to_check.nodeIndex);
  TMatrix finalTm = invNodeTm(node_to_check.nodeIndex) * tm1;
  const plane3f *checkPlanes = convexPlanes.data() + node_to_check.planesOfs;
  const int checkPlanesCount = node_to_check.planesCount;
  const int numIterations = 5;
  vec4f zero = v_zero();
  vec4f vOffset = v_zero();
  vec4f axis_v = v_ldu(&axis.x);
  if (node_to_move.type == COLLISION_NODE_TYPE_BOX)
  {
    carray<vec4f, 8> boundPoints;
    for (int i = 0; i < boundPoints.size(); ++i)
    {
      Point3_vec4 point = finalTm * node_to_move.modelBBox.point(i);
      boundPoints[i] = v_ld(&point.x);
    }
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < boundPoints.size(); ++i)
      {
        for (int j = 0; j < checkPlanesCount; ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(checkPlanes[j], v_add(boundPoints[i], vOffset)), zero);
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), checkPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        return true;
      }
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_SPHERE)
  {
    BSphere3 localSph = finalTm * make_node_bsphere(node_to_move.boundingSphere.c, node_to_move.boundingSphere.r);
    vec4f sph = v_ldu(&localSph.c.x);
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool allInside = true;
      vec4f separationAxis = zero;
      vec4f minDist = V_C_MAX_VAL;
      for (int j = 0; j < checkPlanesCount; ++j)
      {
        vec4f dist = v_sub(v_plane_dist(checkPlanes[j], v_add(sph, vOffset)), v_splat_w(sph));
        dist = v_mul(dist, v_abs(v_dot3(checkPlanes[j], axis_v)));
        allInside &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
        separationAxis = v_sel(separationAxis, checkPlanes[j], v_cmp_gt(minDist, v_splat_x(dist)));
        minDist = v_min(minDist, dist);
      }
      // All points lies inside
      if (!allInside)
      {
        v_stu_p3(&offset.x, vOffset);
        offset = offset - (offset * axis) * axis;
        return true;
      }
      else
        vOffset = v_add(vOffset, v_mul(minDist, separationAxis));
    }
    return false;
  }
  else if (node_to_move.type == COLLISION_NODE_TYPE_MESH)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    Tab<vec4f> vertices(tmpmem);
    reserve_and_resize(vertices, (uint32_t)node_to_move.verticesCount);
    int vi = 0;
    iterateNodeVerts((int)node_to_move.nodeIndex, [&](int, vec4f v) {
      if (vi < vertices.size())
        vertices[vi++] = v_mat44_mul_vec3p(vFinalTm, v);
    });
    for (int iteration = 0; iteration < numIterations; ++iteration)
    {
      bool hasCollision = false;
      for (int i = 0; i < vertices.size(); ++i)
      {
        vec4f vert = vertices[i];
        for (int j = 0; j < checkPlanesCount; ++j)
        {
          vec4f dist = v_max(v_plane_dist_x(checkPlanes[j], v_add(vert, vOffset)), zero);
          hasCollision |= (v_test_vec_x_gt(dist, V_C_EPS_VAL) != 0);
          vOffset = v_add(vOffset, v_mul(v_neg(v_splat_x(dist)), checkPlanes[j]));
        }
      }
      if (!hasCollision)
      {
        v_stu_p3(&offset.x, vOffset);
        offset = offset - (offset * axis) * axis;
        return true;
      }
    }
    return false;
  }
  else
    G_ASSERTF(0, "unsupported type to check against box");
  return false;
}

bool CollisionResource::testInclusion(int test_node_index, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex,
  const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree, Point3 *res_pos) const
{
  const CollisionNode *node_to_test = getNode((uint32_t)test_node_index);
  if (!node_to_test)
    return false;

  if (!defaultInstance.isNodeEnabled(test_node_index) || !defaultInstance.isNodeTraceable(test_node_index))
    return false;

  // Zero-vert marker: enabled/traceable does not exclude it, and its negative radius would
  // still satisfy every plane of a large convex below.
  if (node_to_test->boundingSphere.r < 0.f)
    return false;

  TMatrix testTm;
  // Primitive probes use the stored geometry frame.
  if (node_to_test->type == COLLISION_NODE_TYPE_BOX || node_to_test->type == COLLISION_NODE_TYPE_SPHERE)
  {
    TMatrix geomTm;
    v_mat_43cu_from_mat44(geomTm.array, defaultInstance.getNodeGeometryTm(test_node_index));
    testTm = tm_test * geomTm;
  }
  else if (test_node_tree)
    getCollisionNodeTm(node_to_test, tm_test, test_node_tree, testTm);
  else
    testTm = tm_test * getNodeTm(node_to_test->nodeIndex);

  return testNodeInclusionInConvex(*node_to_test, testTm, convex, tm_restrain, res_pos);
}

bool CollisionResource::testInclusion(int test_node_index, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex,
  const TMatrix &tm_restrain, const CollisionResourceInstance &test_instance, Point3 *res_pos) const
{
  const CollisionNode *node_to_test = getNode((uint32_t)test_node_index);
  if (!node_to_test)
    return false;
  // Stored-pose read for EVERY arm: a tree-backed instance falls back to the default pose,
  // so one query answers at one pose regardless of the probed node's type.
  const CollisionResourceInstance *ts = resolveOwnedPoseForQuery(test_instance, "testInclusion");
  if (!ts->isNodeEnabled(test_node_index) || !ts->isNodeTraceable(test_node_index))
    return false;

  // Zero-vert marker: enabled/traceable does not exclude it, and its negative radius would
  // still satisfy every plane of a large convex below.
  if (node_to_test->boundingSphere.r < 0.f)
    return false;

  TMatrix testTm;
  // Primitive probes use the stored geometry frame.
  if (node_to_test->type == COLLISION_NODE_TYPE_BOX || node_to_test->type == COLLISION_NODE_TYPE_SPHERE)
  {
    TMatrix geomTm;
    v_mat_43cu_from_mat44(geomTm.array, ts->getNodeGeometryTm(test_node_index));
    testTm = tm_test * geomTm;
  }
  else
    getCollisionNodeTm(node_to_test, tm_test, *ts, testTm);

  return testNodeInclusionInConvex(*node_to_test, testTm, convex, tm_restrain, res_pos);
}

bool CollisionResource::testNodeInclusionInConvex(const CollisionNode &node, const TMatrix &test_tm, dag::ConstSpan<plane3f> convex,
  const TMatrix &tm_restrain, Point3 *res_pos) const
{
  const CollisionNode *node_to_test = &node;
  const TMatrix &testTm = test_tm;
  TMatrix finalTm = inverse(tm_restrain) * testTm;
  if (node_to_test->type == COLLISION_NODE_TYPE_BOX)
  {
    for (int i = 0; i < 8; ++i)
    {
      Point3_vec4 point = finalTm * node_to_test->modelBBox.point(i);
      vec4f vert = v_ld(&point.x);
      bool insideAll = true;
      for (int j = 0; j < convex.size() && insideAll; ++j)
      {
        vec4f dist = v_plane_dist_x(convex[j], vert);
        insideAll &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
      }
      if (insideAll)
      {
        if (res_pos)
          *res_pos = tm_restrain * point;
        return true;
      }
    }
    return false;
  }
  else if (node_to_test->type == COLLISION_NODE_TYPE_SPHERE)
  {
    // Column lengths under-bound sheared sphere placements.
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    BSphere3 localSph =
      make_node_bsphere(finalTm * node_to_test->boundingSphere.c, node_to_test->boundingSphere.r * mat33_spectral_norm(vFinalTm));
    vec4f sph = v_ldu(&localSph.c.x);
    bool insideAll = true;
    for (int j = 0; j < convex.size() && insideAll; ++j)
    {
      vec4f dist = v_sub(v_plane_dist_x(convex[j], sph), v_splat_w(sph));
      insideAll &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
    }
    if (insideAll && res_pos)
      *res_pos = tm_restrain * localSph.c;
    return insideAll;
  }
  else if (node_to_test->type == COLLISION_NODE_TYPE_MESH)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    // Decode vert21 via iterateNodeVerts: the resource holds no raw Point3_vec4 array.
    bool found = false;
    iterateNodeVerts((int)node_to_test->nodeIndex, [&](int, vec4f v) {
      if (found)
        return;
      vec4f vert = v_mat44_mul_vec3p(vFinalTm, v);
      bool insideAll = true;
      for (int j = 0; j < convex.size() && insideAll; ++j)
      {
        vec4f dist = v_plane_dist_x(convex[j], vert);
        insideAll &= (v_test_vec_x_lt(dist, V_C_EPS_VAL) != 0);
      }
      if (insideAll)
      {
        if (res_pos)
        {
          Point3_vec4 pt;
          v_st(&pt.x, vert);
          *res_pos = tm_restrain * pt;
        }
        found = true;
      }
    });
    return found;
  }
  else
    G_ASSERTF(0, "unsupported type to check against convex");
  return false;
}

bool CollisionResource::testInclusion(int test_node_index, const TMatrix &tm_test, const CollisionResource *restraining_resource,
  int restraining_node_index, const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree,
  const GeomNodeTree *restrain_node_tree) const
{
  if (!restraining_resource)
    return false;
  const CollisionNode *restraining_node = restraining_resource->getNode((uint32_t)restraining_node_index);
  if (!restraining_node)
    return false;
  G_ASSERTF(restraining_node->type == COLLISION_NODE_TYPE_CONVEX, "restrain node #%u is not of convex type, only convex is supported",
    (unsigned)restraining_node->nodeIndex);

  const CollisionResourceInstance &restrainDef = restraining_resource->getDefaultInstance();
  if (!restrainDef.isNodeEnabled(restraining_node_index) || !restrainDef.isNodeTraceable(restraining_node_index))
    return false;

  TMatrix restrainTm;
  if (restrain_node_tree)
    restraining_resource->getCollisionNodeTm(restraining_node, tm_restrain, restrain_node_tree, restrainTm);
  else
    restrainTm = tm_restrain * restraining_resource->getNodeTm(restraining_node->nodeIndex);

  return testInclusion(test_node_index, tm_test, restraining_resource->getNodeConvexPlanes(restraining_node_index), restrainTm,
    test_node_tree);
}

bool CollisionResource::testInclusion(int test_node_index, const TMatrix &tm_test, const CollisionResource *restraining_resource,
  int restraining_node_index, const TMatrix &tm_restrain, const CollisionResourceInstance &test_instance,
  const CollisionResourceInstance &restrain_instance) const
{
  if (!restraining_resource)
    return false;
  const CollisionNode *restraining_node = restraining_resource->getNode((uint32_t)restraining_node_index);
  if (!restraining_node)
    return false;
  G_ASSERTF(restraining_node->type == COLLISION_NODE_TYPE_CONVEX, "restrain node #%u is not of convex type, only convex is supported",
    (unsigned)restraining_node->nodeIndex);

  // Stored-pose read, like the test arm: a tree-backed restrain instance answers at the
  // default pose until the adoption pass.
  const CollisionResourceInstance *ri = restraining_resource->resolveOwnedPoseForQuery(restrain_instance, "testInclusion");
  if (!ri->isNodeEnabled(restraining_node_index) || !ri->isNodeTraceable(restraining_node_index))
    return false;

  TMatrix restrainTm;
  restraining_resource->getCollisionNodeTm(restraining_node, tm_restrain, *ri, restrainTm);

  return testInclusion(test_node_index, tm_test, restraining_resource->getNodeConvexPlanes(restraining_node_index), restrainTm,
    test_instance);
}

DAGOR_NOINLINE bool CollisionResource::rayHit(const mat44f &tm, const Point3 &from, const Point3 &dir, float in_t, int ray_mat_id,
  int &out_mat_id, uint8_t behavior_filter) const
{
  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };
  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float /*t*/, vec3f /*normal*/, vec3f /*pos*/,
                    tri_ref_t /*tri_ref*/) { out_mat_id = node->physMatId; };

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT,
    false /*pose_may_refresh: the default instance is never tree-backed*/>(tm, defaultInstance, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    false /*out_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *collision_node_mask, int *out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::rayHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from, dir, in_t,
    bsphere_scale, collision_node_mask, out_mat_id);
}

DAGOR_NOINLINE bool CollisionResource::capsuleHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, CollResHitNodesType &nodes_hit) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::capsuleHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from, dir,
    in_t, radius, nodes_hit);
}

DAGOR_NOINLINE bool CollisionResource::multiRayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  dag::Span<CollisionTrace> traces) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::multiRayHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), traces);
}

DAGOR_NOINLINE bool CollisionResource::rayHit(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *collision_node_mask,
  int *out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::rayHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir, in_t,
    bsphere_scale, collision_node_mask, out_mat_id);
}

DAGOR_NOINLINE bool CollisionResource::capsuleHit(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  const Point3 &from, const Point3 &dir, float in_t, float radius, CollResHitNodesType &nodes_hit) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::capsuleHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), from, dir, in_t,
    radius, nodes_hit);
}

DAGOR_NOINLINE bool CollisionResource::multiRayHit(const TMatrix &instance_tm, const CollisionResourceInstance &instance,
  dag::Span<CollisionTrace> traces) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::multiRayHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, instance), traces);
}

void CollisionResource::initializeWithGeomNodeTree(const GeomNodeTree &geom_node_tree)
{
  // Contract: a geom-node-driven resource is never FRT-optimized. The FRT bakes mesh-node triangles
  // in resource-local (bind) space and is traced without consulting the tree (forEachIntersectedNode),
  // so an FRT-resident tree-driven node would be hit at its baked pose instead of the animated one.
  // Tree-bound resources (DM / attachable / hangar) are always loaded non-optimized, so no grid is
  // built; this asserts the invariant the trace path silently relies on.
  // G_ASSERTF(!checkGridAvailable(CollisionNode::TRACEABLE) && !checkGridAvailable(CollisionNode::PHYS_COLLIDABLE),
  //  "collision resource bound to GeomNodeTree '%s' must not be FRT-optimized (grid-resident nodes are "
  //  "traced at their bind pose, ignoring the tree)",
  //  geom_node_tree.empty() ? "<empty>" : geom_node_tree.getNodeName(GeomNodeTree::Index16(0)));
  // currently, this contract is indeed violated by any(!) mimic
  // Shared resources may be rebound only to an identical tree layout.
  geomNodeTreeBound = true;
  for (auto &node : allNodesList)
    node.geomNodeId = geom_node_tree.findNodeIndex(getNodeNameStr(node));
}

CollisionResource *CollisionResource::deepCopy(void *inplace_ptr) const
{
  auto *collRes = new (inplace_ptr ? inplace_ptr : midmem->alloc(sizeof(CollisionResource)), _NEW_INPLACE) CollisionResource();
  collRes->vFullBBox = vFullBBox;
  collRes->vBoundingSphere = vBoundingSphere;
  collRes->vBindTraceSphere = vBindTraceSphere;
  collRes->bindTraceSphereStamped = bindTraceSphereStamped;
  collRes->boundingBox = boundingBox;
  collRes->boundingSphereRad = boundingSphereRad;
  collRes->collisionFlags = collisionFlags;
  collRes->bsphereCenterNode = bsphereCenterNode;
  collRes->allNodesList = allNodesList; // geomNodeId stamps travel with the nodes
  collRes->geomNodeTreeBound = geomNodeTreeBound;
  collRes->relGeomNodeTms = relGeomNodeTms;
  collRes->names = names;
  collRes->capsules = capsules;
  collRes->convexPlanes = convexPlanes;
  collRes->nodeBlasData = nodeBlasData; // node.nodeBlasOfs refs travel inside the allNodesList copy
  // No index copy: the resource keeps no source-face list; faces are read from nodeBlasData.

  // stampBlasResidentNodes stamps residency on nodes whose verts/indices live in these grids'
  // blasData. allNodesList was deep-copied above (carrying the flag + reinterpreted verticesOfs), so
  // the BLAS storage must be cloned too -- otherwise iterateNodeFaces / iterateNodeVerts / Jolt
  // MeshShape builds on resident nodes would read empty grids and return zero faces or assert.
  // Grid::tracer is null at HEAD (FRT path gone), so it needs no cloning; reintroducing a tracer here
  // would need an explicit clone path since unique_ptr isn't copy-constructible.
  auto copyGrid = [](Grid &dst, const Grid &src) {
    dst.blasData = src.blasData;
    dst.blasBBox = src.blasBBox;
    dst.blasScale = src.blasScale;
    dst.blasInvScale = src.blasInvScale; // cached 1/blasScale; read directly by every vert21 decode path
    dst.blasOfs = src.blasOfs;
    dst.blasTreeBytes = src.blasTreeBytes;
    dst.blasRootRef = src.blasRootRef;
    dst.blasNodeRanges = src.blasNodeRanges;
    dst.blasTwoSided = src.blasTwoSided; // cull mode must survive the clone or a two-sided asset reverts to CullCCW
  };
  copyGrid(collRes->gridForTraceable, gridForTraceable);
  copyGrid(collRes->gridForCollidable, gridForCollidable);

  // Preserve the source's current pose. The ctor bound defaultInstance.res to the copy;
  // re-bind explicitly so the invariant survives any future ctor or field re-targeting.
  collRes->defaultInstance.res = collRes;
  collRes->defaultInstance.nodeTm = defaultInstance.nodeTm;
  collRes->authoredNodeTm = authoredNodeTm;
  collRes->defaultInstance.poseMeta = defaultInstance.poseMeta; // status bits (hides) travel with the pose
  collRes->defaultInstance.rootBBox = defaultInstance.rootBBox;
  collRes->defaultInstance.gridResidentPoseAtBind = defaultInstance.gridResidentPoseAtBind;
  collRes->defaultInstance.posedSinceBind = defaultInstance.posedSinceBind;
  collRes->defaultInstance.bsphereCenterLocal = defaultInstance.bsphereCenterLocal;
  collRes->defaultInstance.hasBsphereCenterLocal = defaultInstance.hasBsphereCenterLocal;

  collRes->rebuildNodesLL();
  return collRes;
}

uint8_t CollisionResource::classifyNodeTmFlags(mat44f_cref tm, float &out_max_scale)
{
  uint8_t flags = 0;
  float dot01 = v_extract_x(v_dot3_x(tm.col0, tm.col1));
  float dot02 = v_extract_x(v_dot3_x(tm.col0, tm.col2));
  float dot12 = v_extract_x(v_dot3_x(tm.col1, tm.col2));
  float len0sq = v_extract_x(v_length3_sq_x(tm.col0));
  float len1sq = v_extract_x(v_length3_sq_x(tm.col1));
  float len2sq = v_extract_x(v_length3_sq_x(tm.col2));
  float len3sq = v_extract_x(v_length3_sq_x(tm.col3));
  const float eps = 1e-3;
  if (fabs(dot01) < eps && fabs(dot02) < eps && fabs(dot12) < eps && fabsf(len0sq - len1sq) < eps && fabsf(len0sq - len2sq) < eps)
  {
    if (fabs(len0sq - 1.f) < eps && fabs(len1sq - 1.f) < eps && fabs(len2sq - 1.f) < eps)
    {
      flags = CollisionNode::ORTHONORMALIZED;
      if (v_extract_x(tm.col0) == 1.f && v_extract_y(tm.col1) == 1.f && v_extract_z(tm.col2) == 1.f)
      {
        if (len3sq < eps)
          flags |= CollisionNode::IDENT;
        else
          flags |= CollisionNode::TRANSLATE;
      }
    }
    else
      flags = CollisionNode::ORTHOUNIFORM;
  }
  // Preserve the serialized column-max scale for authored poses. Squared lengths overflow for
  // large finite scales and FTZ-underflow to zero below ~1e-19; the spectral norm normalizes
  // internally and stays finite and nonzero for any valid basis.
  const float maxLenSq = max(len0sq, max(len1sq, len2sq));
  out_max_scale = (maxLenSq >= FLT_MIN && maxLenSq <= FLT_MAX) ? sqrtf(maxLenSq) : mat33_spectral_norm(tm);
  return flags;
}

bbox3f CollisionResource::getNodeGeometryBBox(const CollisionNode &node) const
{
  bbox3f box;
  if (node.type == COLLISION_NODE_TYPE_SPHERE)
  {
    vec3f c = v_ldu(&node.boundingSphere.c.x);
    vec3f r = v_splats(node.boundingSphere.r);
    box.bmin = v_sub(c, r);
    box.bmax = v_add(c, r);
  }
  else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
  {
    const Capsule &c = capsules[node.capsuleIndex];
    vec3f a = v_ldu(&c.a.x), b = v_ldu(&c.b.x), r = v_splats(c.r);
    box.bmin = v_sub(v_min(a, b), r);
    box.bmax = v_add(v_max(a, b), r);
  }
  else
    box = v_ldu_bbox3(node.modelBBox);
  // Match the trace slab's minimum axis thickness. The dispatch clamp works in size space
  // (max(bsize, eps)), so the endpoint expansion must survive rounding at any magnitude:
  // far from the origin the absolute half-delta can fall below one ULP and absorb away,
  // leaving a thin axis the slab still traces outside this box.
  vec3f bsize = v_sub(box.bmax, box.bmin);
  vec3f sizeDelta = v_mul(v_sub(v_max(bsize, v_splats(0.0001f)), bsize), V_C_HALF);
  vec3f mag = v_max(v_abs(box.bmin), v_abs(box.bmax));
  vec3f ulpFloor = v_mul(mag, v_splats(2.4e-7f)); // ~2 ULP: x + x*2eps always grows for normal x
  vec3f expand = v_and(v_max(sizeDelta, ulpFloor), v_cmp_gt(sizeDelta, v_zero()));
  box.bmin = v_sub(box.bmin, expand);
  box.bmax = v_add(box.bmax, expand);
  return box;
}

Point3 CollisionResource::getNodeResourceCenter(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n)
    return Point3(0, 0, 0);
  // Exact center of getNodeResourceBBox (affine maps commute with box midpoints) at one point
  // transform; the same degenerate set anchors on the node placement column.
  if (n->boundingSphere.r < 0 || n->type == COLLISION_NODE_TYPE_POINTS ||
      ((n->type == COLLISION_NODE_TYPE_MESH || n->type == COLLISION_NODE_TYPE_CONVEX) && !n->hasGeometry()) ||
      DAGOR_UNLIKELY(!defaultInstance.isNodeTraceable(node_id)))
    return getNodeTm(node_id).getcol(3);
  Point3 c;
  v_stu_p3(&c.x, v_bbox3_center(getNodeGeometryBBox(*n)));
  TMatrix gt;
  v_mat_43cu_from_mat44(gt.array, defaultInstance.getNodeGeometryTm(node_id));
  return gt * c;
}

// Shared with the tree-backed prim path, which derives `posed` from the live tree.
mat44f CollisionResource::geometryTmFromPosed(int node_index, mat44f_cref posed, const CollisionResourceInstance::PoseMeta &pm) const
{
  const CollisionNode &node = getAllNodes()[node_index];
  // A singular authored primitive remains in its baked frame; a RETAINED bake (valid general
  // sphere, Ritter radius unrecoverable) keeps stored geometry in the authored frame and
  // composes the same compatibility transform as the eps-IDENT prims below.
  if (DAGOR_UNLIKELY(pm.isGeometryBaked() && !pm.isRetainedBake()))
  {
    mat44f identity;
    v_mat44_ident(identity);
    return identity;
  }
  if (!usesAuthoredFrame(node, pm))
    return posed;

  G_ASSERT((uint32_t)node_index < authoredNodeTm.size());
  mat44f authored;
  v_mat44_make_from_43cu_unsafe(authored, authoredNodeTm[node_index].array);
  if (v_check_xyzw_all_true(v_cmp_eq(posed.col0, authored.col0)) && v_check_xyzw_all_true(v_cmp_eq(posed.col1, authored.col1)) &&
      v_check_xyzw_all_true(v_cmp_eq(posed.col2, authored.col2)) && v_check_xyzw_all_true(v_cmp_eq(posed.col3, authored.col3)))
  {
    mat44f identity;
    v_mat44_ident(identity);
    return identity;
  }
  mat44f invAuthored, geometryTm;
  if (DAGOR_LIKELY((uint32_t)node_index < authoredNodeItm.size()))
    v_mat44_make_from_43cu_unsafe(invAuthored, authoredNodeItm[node_index].array);
  else
    v_mat44_inverse43(invAuthored, authored);
  v_mat44_mul43(geometryTm, posed, invAuthored);
  return geometryTm;
}


// Live IDENT means exact identity; loader epsilon classes are not dispatch no-ops.
// Widens the shared pose meta scale to the conservative spectral bound; for shear-class
// authored nodes getNodeMaxTmScale, the exporter weld eps and the serialized scale follow it.
void CollisionResource::stampConservativePoseScale(int node_index)
{
  CollisionResourceInstance::PoseMeta &pm = defaultInstance.poseMeta[node_index];
  // A RETAINED bake's public scale describes its identity effective frame; reclassifying the
  // unchanged authored matrix would leak the authored scale into it.
  if (pm.isGeometryBaked())
    return;
  const bool shearLike = pm.flags == 0;
  // The column max under-reads every non-bit-exact uniform basis (slack ~eps/(2*s^2)):
  // restamp with the spectral norm; exact axis-aligned bases keep their bit-exact column max.
  const bool slackUniform = (pm.flags & CollisionNode::ORTHOUNIFORM) != 0 && (pm.flags & CollisionNode::ORTHONORMALIZED) == 0 &&
                            !is_exact_axis_aligned_basis(authoredNodeTm[node_index]);
  // ORTHONORMALIZED is an epsilon class (IDENT included): the column max can under-read the
  // stretch by the classifier tolerance; Gershgorin over the tolerance volume gives
  // sigma^2 <= 1 + 3e-3, so 1.0015 covers it. Bit-exact rigid bases keep their exact stamp.
  if ((pm.flags & CollisionNode::ORTHONORMALIZED) && !is_exact_rigid_basis(authoredNodeTm[node_index]))
    pm.maxTmScale = max(pm.maxTmScale, 1.0015f);
  if (!shearLike && !slackUniform)
    return;
  mat44f vTm;
  v_mat44_make_from_43cu_unsafe(vTm, authoredNodeTm[node_index].array);
  // No margin on the uniform-class restamp (a true uniform's spectral norm IS its scale,
  // and consumers pin the exact stamp); the shear class keeps the persisted margin.
  pm.maxTmScale = max(pm.maxTmScale, mat33_spectral_norm(vTm) * (shearLike ? 1.0002f : 1.f));
  // The widened scale raises the relative determinant floor: re-gate traceability so a
  // near-singular shear cannot stay traceable on the strength of its pre-widen column max.
  // Same normalized gate as setAuthoredNodeTm: raw det * s^3 overflows for large finite bases.
  if (pm.isTraceable())
  {
    float ndet;
    pm.setTraceable(relativeDetAboveFloor(vTm, ndet));
    if (!pm.isTraceable())
      pm.setComposable(false); // effectively singular: the stored value is the right report
  }
}

CollisionResourceInstancePtr CollisionResource::createInstance(const GeomNodeTree *tree) const
{
  CollisionResourceInstancePtr inst(new CollisionResourceInstance);
  initInstance(*inst, tree);
  return inst;
}

void CollisionResource::initInstance(CollisionResourceInstance &inst, const GeomNodeTree *tree) const
{
  // Reinitialization matches creation: rebind and seed from the resource's current pose
  // (a posed default seeds a posed copy; only structural hides reset).
  inst.hasBsphereCenterLocal = false;
  inst.res = this;
  inst.boundNodeLayoutGen = nodeLayoutGen;
  inst.tree = tree;
  inst.poseGeneration = 0;
  inst.treeNodeCountAtBind = tree ? (uint32_t)tree->nodeCount() : 0;
  inst.layoutDriftAsserted = false; // a rebind re-arms the once-per-binding drift assert
  inst.seedPose();
}

TMatrix CollisionResource::getNodeTm(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n)
    return TMatrix::IDENT;
  return defaultInstance.nodeTm[node_id];
}

void CollisionResource::setNodeTm(int node_id, const TMatrix &new_tm)
{
  G_ASSERT_RETURN(getNode(node_id), );
  mat44f t;
  v_mat44_make_from_43cu_unsafe(t, new_tm.array);
  defaultInstance.updateNodeTmImpl(node_id, t);
}

float CollisionResource::getNodeMaxTmScale(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  return n ? defaultInstance.poseMeta[node_id].maxTmScale : 1.f;
}

BBox3 CollisionResource::getNodeBBox(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n)
    return BBox3();
  if (n->type != COLLISION_NODE_TYPE_BOX && n->type != COLLISION_NODE_TYPE_SPHERE)
    return n->modelBBox;
  // IDENT and singular authored primitives retain exporter-baked bounds.
  if ((defaultInstance.poseMeta[node_id].flags & CollisionNode::IDENT) ||
      (defaultInstance.poseMeta[node_id].isGeometryBaked() && !defaultInstance.poseMeta[node_id].isRetainedBake()))
    return n->modelBBox;
  // r < 0 is the exporter's zero-vert marker for EVERY primitive: composing it would corner-map
  // the inverted box into a phantom placement. Return the stored (empty) box raw, as the
  // pre-instance accessor did.
  if (n->boundingSphere.r < 0)
    return n->modelBBox;
  // A non-composable pose (non-finite or inverse-overflowing) must not compose: NaN/Inf bounds
  // would leak. A finite mirrored pose is hidden from traces but composes fine.
  if (DAGOR_UNLIKELY(!defaultInstance.isNodeComposable(node_id)))
    return n->modelBBox;
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, defaultInstance.getNodeGeometryTm(node_id));
  if (n->type == COLLISION_NODE_TYPE_SPHERE) // exact: corner-mapping would inflate under rotation
    return composed_sphere_box(tm, n->boundingSphere.c, n->boundingSphere.r);
  BBox3 out;
  for (int k = 0; k < 8; k++)
    out += tm * n->modelBBox.point(k);
  return out;
}

BSphere3 CollisionResource::getNodeBSphere(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n || n->boundingSphere.r < 0)
    return BSphere3(); // empty: r = r2 = -1
  // A RETAINED bake is a valid poseable sphere: it composes through the compatibility
  // transform like the bbox accessor, only a SINGULAR bake stays pinned at its baked frame.
  if (n->type != COLLISION_NODE_TYPE_SPHERE || (defaultInstance.poseMeta[node_id].flags & CollisionNode::IDENT) ||
      (defaultInstance.poseMeta[node_id].isGeometryBaked() && !defaultInstance.poseMeta[node_id].isRetainedBake()))
    return BSphere3(n->boundingSphere.c, n->boundingSphere.r);
  // A non-composable pose (non-finite or inverse-overflowing) must not compose (NaN/Inf
  // center); a finite mirrored pose is hidden from traces but composes fine.
  if (DAGOR_UNLIKELY(!defaultInstance.isNodeComposable(node_id)))
    return BSphere3(n->boundingSphere.c, n->boundingSphere.r);
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, defaultInstance.getNodeGeometryTm(node_id));
  return BSphere3(tm * n->boundingSphere.c, n->boundingSphere.r * defaultInstance.poseMeta[node_id].maxTmScale);
}

TMatrix CollisionResource::getStoredToNodeLocalTm(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n)
    return TMatrix::IDENT;
  const CollisionResourceInstance::PoseMeta &pm = defaultInstance.poseMeta[node_id];
  // eps-IDENT prims (immutable serialized flags, matching getNodeGeometryTm's dispatch) and
  // retained bakes store the authored frame; a SINGULAR bake has no invertible authored tm.
  const bool authoredFrameStored = (n->type == COLLISION_NODE_TYPE_BOX || n->type == COLLISION_NODE_TYPE_SPHERE) &&
                                   ((n->flags & CollisionNode::IDENT) || pm.isRetainedBake());
  if (!authoredFrameStored)
    return TMatrix::IDENT;
  return inverse(authoredNodeTm[node_id]);
}

TMatrix CollisionResource::invNodeTm(int node_index) const { return invInstNodeTm(defaultInstance, node_index); }

TMatrix CollisionResource::invInstNodeTm(const CollisionResourceInstance &instance, int node_index)
{
  const CollisionResourceInstance::PoseMeta &pm = instance.poseMeta[node_index];
  const int nodeType = instance.res->getAllNodes()[node_index].type;
  if ((pm.flags & CollisionNode::IDENT) && (nodeType == COLLISION_NODE_TYPE_BOX || nodeType == COLLISION_NODE_TYPE_SPHERE))
    return TMatrix::IDENT;
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, instance.getNodeGeometryTm(node_index));
  TMatrix ret;
  // Only bit-exact rigid bases may use the transpose shortcut: the orthonormal class admits
  // in-band scale, and the transpose maps points by s instead of 1/s.
  if (is_exact_rigid_basis(tm))
    ret = orthonormalized_inverse(tm);
  else
    ret = inverse(tm);
  return ret;
}

const CollisionResourceInstance &CollisionResource::instanceOrDefaultFallback(const CollisionResourceInstance *instance) const
{
  if (DAGOR_UNLIKELY(instance && instance->getResource()))
  {
    G_ASSERTF(false, "collision instance of res %p used with res %p", instance->getResource(), this);
    LOGERR_ONCE("collision instance of res %p used with res %p; using the current pose", instance->getResource(), this);
  }
  return defaultInstance;
}

const CollisionResourceInstance *CollisionResource::resolveInstanceForTrace(const CollisionResourceInstance &instance) const
{
  // A resource-less instance must not pass as this resource's default.
  if (DAGOR_UNLIKELY(instance.getResource() != this))
  {
    G_ASSERTF(false, "trace: foreign or unbound CollisionResourceInstance (res %p vs %p)", instance.getResource(), this);
    LOGERR_ONCE("trace: CollisionResource %p used with a foreign/unbound instance (res %p): tracing the current pose", this,
      instance.getResource());
    return &defaultInstance;
  }
  if (instance.isDefault())
    return &defaultInstance;
  return check_instance_owned_and_fresh(this, instance, "trace") ? &instance : &defaultInstance;
}

const CollisionResourceInstance *CollisionResource::resolveOwnedPoseForQuery(const CollisionResourceInstance &instance,
  const char *site) const
{
  // Secondary queries read stored resource-local matrices; a tree-backed pose has none, so
  // they keep the default pose until their adoption pass teaches them a tree path.
  const CollisionResourceInstance *inst = resolveInstanceForTrace(instance);
  if (DAGOR_UNLIKELY(inst->getTree() != nullptr))
  {
    LOGWARN_ONCE("%s: tree-backed collres instance used with a stored-pose query on res %p; default pose is used", site, this);
    return &defaultInstance;
  }
  return inst;
}

void CollisionResource::sortNodesList()
{
  // Preserve legacy sort and containment frames for primitives.
  auto nodeSortBox = [this](const CollisionNode &n, int meta_index) -> BBox3 {
    if (n.type != COLLISION_NODE_TYPE_BOX && n.type != COLLISION_NODE_TYPE_SPHERE)
      return n.modelBBox;
    if ((defaultInstance.poseMeta[meta_index].flags & CollisionNode::IDENT) ||
        defaultInstance.poseMeta[meta_index].isGeometryBaked()) // singular authored tm: geometry stayed baked
      return n.modelBBox;
    TMatrix tm;
    v_mat_43cu_from_mat44(tm.array, defaultInstance.getNodeGeometryTm(meta_index));
    if (n.type == COLLISION_NODE_TYPE_SPHERE)
      return composed_sphere_box(tm, n.boundingSphere.c, n.boundingSphere.r); // legacy tight key
    BBox3 out;
    for (int k = 0; k < 8; k++)
      out += tm * n.modelBBox.point(k);
    return out;
  };
  // Precompute keys because element moves invalidate nodeIndex-based pose lookup during sorting.
  dag::Vector<BBox3, framemem_allocator> sortKeys(allNodesList.size());
  for (const CollisionNode &n : allNodesList)
    sortKeys[n.nodeIndex] = nodeSortBox(n, n.nodeIndex);
  auto nodeSortKey = [&sortKeys](const CollisionNode &n) -> const BBox3 & { return sortKeys[n.nodeIndex]; };
  const char *namesData = names.empty() ? "" : names.data();
  stlsort::sort(allNodesList.begin(), allNodesList.end(),
    [namesData, &nodeSortKey](const CollisionNode &left, const CollisionNode &right) {
      const BBox3 &boxL = nodeSortKey(left), &boxR = nodeSortKey(right);
      float szL = v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(boxL))));
      float szR = v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(boxR))));
      if (szL == szR)
        return strcmp(namesData + left.nameOfs, namesData + right.nameOfs) > 0;
      return szL > szR; // larger first
    });

  // Keep every nodeIndex-parallel array in the new order.
  {
    dag::Vector<TMatrix> sortedTm;
    dag::Vector<TMatrix> sortedAuthoredTm;
    dag::Vector<CollisionResourceInstance::PoseMeta> sortedMeta;
    reserve_and_resize(sortedTm, allNodesList.size());
    reserve_and_resize(sortedAuthoredTm, allNodesList.size());
    reserve_and_resize(sortedMeta, allNodesList.size());
    Tab<TMatrix> relTms(framemem_ptr());
    const bool hasRelTms = (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) != 0;
    if (hasRelTms)
    {
      reserve_and_resize(relTms, allNodesList.size());
      G_ASSERTF_RETURN(relGeomNodeTms.size() == allNodesList.size(), , "relGeomNodeTms.size()=%d allNodesList.size()=%d",
        relGeomNodeTms.size(), allNodesList.size());
    }
    G_ASSERTF_RETURN(defaultInstance.nodeTm.size() == allNodesList.size(), , "default pose size=%d allNodesList.size()=%d",
      (int)defaultInstance.nodeTm.size(), allNodesList.size());
    G_ASSERTF_RETURN(authoredNodeTm.size() == allNodesList.size(), , "authored pose size=%d allNodesList.size()=%d",
      (int)authoredNodeTm.size(), allNodesList.size());
    for (size_t nodeNo = 0; nodeNo < allNodesList.size(); nodeNo++)
    {
      const uint16_t nodeIdx = allNodesList[nodeNo].nodeIndex;
      sortedTm[nodeNo] = defaultInstance.nodeTm[nodeIdx];
      sortedAuthoredTm[nodeNo] = authoredNodeTm[nodeIdx];
      sortedMeta[nodeNo] = defaultInstance.poseMeta[nodeIdx];
      if (hasRelTms)
        relTms[nodeNo] = relGeomNodeTms[nodeIdx];
    }
    defaultInstance.nodeTm = eastl::move(sortedTm);
    authoredNodeTm = eastl::move(sortedAuthoredTm);
    authoredNodeItm.clear(); // stale order; the rebuildNodesLL that follows any sort rebuilds it
    defaultInstance.poseMeta = eastl::move(sortedMeta);
    if (hasRelTms)
      mem_copy_from(relGeomNodeTms, relTms.data());
    // Instances bound before this permutation would read the previous occupant's pose per node;
    // the freshness gate compares this generation and falls back to the current pose.
    nodeLayoutGen++;
  }

  // Containment reads the same precomputed keys (cached values; the pose-array permutation
  // above does not invalidate them, and nodeIndex is not re-stamped until rebuildNodesLL).

  for (size_t nodeNo1 = 0; nodeNo1 < allNodesList.size(); nodeNo1++)
  {
    for (size_t nodeNo2 = nodeNo1 + 1; nodeNo2 < allNodesList.size(); nodeNo2++)
    {
      bbox3f bbox1 = v_ldu_bbox3(nodeSortKey(allNodesList[nodeNo1]));
      bbox3f bbox2 = v_ldu_bbox3(nodeSortKey(allNodesList[nodeNo2]));
      if (v_bbox3_test_box_inside(bbox1, bbox2)) // bbox2 inside of bbox1.
      {
        float bboxLenSq = v_extract_x(v_length3_sq_x(v_bbox3_size(bbox1)));
        uint16_t n2ins = allNodesList[nodeNo2].insideOfNode;
        if (allNodesList[nodeNo2].insideOfNode == 0xffff ||
            bboxLenSq < v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(nodeSortKey(allNodesList[n2ins]))))))
        {
          allNodesList[nodeNo2].insideOfNode = nodeNo1;
        }
      }
    }
  }
}
void CollisionResource::rebuildNodesLL()
{
  // This function restamps indices but never reorders pose arrays.
  G_ASSERT(defaultInstance.nodeTm.size() == allNodesList.size() && authoredNodeTm.size() == allNodesList.size() &&
           defaultInstance.poseMeta.size() == allNodesList.size());
  // Recomputes the authored-inverse cache for the operations that funnel through here (load,
  // collapse, deepCopy). sortNodesList, eraseNodeAt and setAuthoredNodeTm carry their own
  // bookkeeping; an append leaves its slot past the sized cache until the next rebuild, so
  // readers take the compute fallback. Only eps-IDENT box/sphere prims and retained bakes
  // read it: a resource without them skips the N inverses outright.
  auto slotWantsItm = [&](size_t i) { return usesAuthoredFrame(allNodesList[i], defaultInstance.poseMeta[i]); };
  bool wantItm = false;
  for (size_t i = 0; i < allNodesList.size() && !wantItm; ++i)
    wantItm = slotWantsItm(i);
  authoredNodeItm.resize(wantItm ? authoredNodeTm.size() : 0);
  // Only consumer slots get the inverse (the readers gate on the same predicate); the rest
  // hold identity, so every sized slot is deterministic and a sparse-consumer resource pays
  // k inverses per rebuild, not N.
  for (size_t i = 0; i < authoredNodeItm.size(); ++i)
    if (slotWantsItm(i))
    {
      mat44f a, inv;
      v_mat44_make_from_43cu_unsafe(a, authoredNodeTm[i].array);
      v_mat44_inverse43(inv, a);
      v_mat_43cu_from_mat44(authoredNodeItm[i].array, inv);
    }
    else
      authoredNodeItm[i] = TMatrix::IDENT;
  // Re-stamping nodeIndex = position invalidates nodeIndex-keyed BLAS ranges when the list order changed.
  // Pre-stamp node indices are dense, so rebase each range through the old-to-new permutation first.
  const bool needRemap = !gridForTraceable.blasNodeRanges.empty() || !gridForCollidable.blasNodeRanges.empty();
  bool orderChanged = false;
  dag::Vector<uint16_t, framemem_allocator> oldToNew;
  if (DAGOR_UNLIKELY(needRemap))
  {
    oldToNew.resize(allNodesList.size(), 0);
    for (size_t i = 0; i < allNodesList.size(); ++i)
    {
      const uint16_t old = allNodesList[i].nodeIndex;
      if (old < oldToNew.size())
        oldToNew[old] = (uint16_t)i;
      orderChanged |= old != (uint16_t)i;
    }
  }

  constexpr uint16_t INVALID = CollisionNode::INVALID_IDX;
  meshNodesHead = boxNodesHead = sphereNodesHead = capsuleNodesHead = INVALID;
  numMeshNodes = numBoxNodes = numCapsuleNodes = 0;
  uint16_t meshNodesTail = INVALID, boxNodesTail = INVALID, sphereNodesTail = INVALID, capsuleNodesTail = INVALID;
  // See allMeshNodesBlasEligible: cleared by any convex / non-IDENT / behavior-incomplete mesh-list node.
  bool allMeshEligible = true;
  for (size_t nodeNo = 0; nodeNo < allNodesList.size(); nodeNo++)
  {
    CollisionNode &node = allNodesList[nodeNo];
    node.nextNode = INVALID;

    node.nodeIndex = nodeNo;
    const uint16_t idx = (uint16_t)nodeNo;

    if (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      if (meshNodesHead != INVALID)
      {
        // A degenerate-dropped node (indicesCount==0, no buildable geometry) is a supported inert
        // state: keep it linked/indexable -- the trace path and the node iterators early-out on
        // indicesCount==0 (see resolveNodeVertsForCall / iterateNodeVerts / iterateNodeFacesVerts).
        allNodesList[meshNodesTail].nextNode = idx;
        meshNodesTail = idx;
      }
      else
        meshNodesHead = meshNodesTail = idx;
      ++numMeshNodes;
      allMeshEligible &= node.type == COLLISION_NODE_TYPE_MESH && (defaultInstance.poseMeta[nodeNo].flags & CollisionNode::IDENT) &&
                         node.indicesCount > 0 &&
                         node.checkBehaviorFlags((uint16_t)(CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE));
    }
    else if (node.type == COLLISION_NODE_TYPE_BOX)
    {
      if (boxNodesHead != INVALID)
      {
        allNodesList[boxNodesTail].nextNode = idx;
        boxNodesTail = idx;
      }
      else
        boxNodesHead = boxNodesTail = idx;
      ++numBoxNodes;
    }
    else if (node.type == COLLISION_NODE_TYPE_SPHERE)
    {
      if (sphereNodesHead != INVALID)
      {
        allNodesList[sphereNodesTail].nextNode = idx;
        sphereNodesTail = idx;
      }
      else
        sphereNodesHead = sphereNodesTail = idx;
    }
    else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      if (capsuleNodesHead != INVALID)
      {
        allNodesList[capsuleNodesTail].nextNode = idx;
        capsuleNodesTail = idx;
      }
      else
        capsuleNodesHead = capsuleNodesTail = idx;
      ++numCapsuleNodes;
    }
  }

  allMeshNodesBlasEligible = numMeshNodes > 0 && allMeshEligible;

  if (DAGOR_UNLIKELY(needRemap && orderChanged))
  {
    for (Grid *g : {&gridForTraceable, &gridForCollidable})
      for (Grid::NodeRange &r : g->blasNodeRanges)
        if (r.nodeIndex < oldToNew.size())
          r.nodeIndex = oldToNew[r.nodeIndex];
  }

  // Every layout path converges here: restamp conservative pose scales (after any un-bake,
  // so the exporter-parity divisor is untouched), then refresh the current-pose reject.
  for (int i = 0, e = (int)allNodesList.size(); i < e; ++i)
    stampConservativePoseScale(i);
  defaultInstance.recomputeRootBBox();

  // Extend serialized bind bounds over composed epsilon-IDENT mesh frames and over un-baked
  // rotated/skewed boxes, whose conservative local box recomposes beyond the serialized sphere.
  Point3_vec4 bsphC;
  v_stu_p3(&bsphC.x, vBoundingSphere);
  float bindR2 = v_extract_w(vBoundingSphere);
  for (const CollisionNode &n : allNodesList)
  {
    const CollisionResourceInstance::PoseMeta &pm = defaultInstance.poseMeta[n.nodeIndex];
    if (n.type == COLLISION_NODE_TYPE_BOX)
    {
      // Mirror unbakePrimNode's conservative branch: exact-frame (IDENT/TRANSLATE/baked) boxes
      // recompose inside the serialized sphere.
      if ((pm.flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) || pm.isGeometryBaked())
        continue;
      const TMatrix &authored = authoredNodeTm[n.nodeIndex];
      for (int k = 0; k < 8; k++)
      {
        const Point3_vec4 p = authored * n.modelBBox.point(k);
        bindR2 = max(bindR2, lengthSq(p - *(const Point3 *)&bsphC.x));
        // The whole-resource box pre-rejects (rendinst reads vFullBBox) must cover the
        // recomposed conservative box too, not just the sphere.
        boundingBox += p;
        v_bbox3_add_pt(vFullBBox, v_ld(&p.x));
      }
      continue;
    }
    if (n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    if (!(pm.flags & CollisionNode::IDENT))
      continue;
    const TMatrix &authored = authoredNodeTm[n.nodeIndex];
    // bit-compare intended: a false mismatch only widens the bind sphere conservatively
    // via the corner extension below (one-shot, at layout finalize)
    if (memcmp(&authored, &TMatrix::IDENT, sizeof(TMatrix)) == 0) //-V1014
      continue;
    for (int k = 0; k < 8; k++)
      bindR2 = max(bindR2, lengthSq(authored * n.modelBBox.point(k) - *(const Point3 *)&bsphC.x));
  }
  vBindTraceSphere = v_perm_xyzd(vBoundingSphere, v_splats(bindR2));
  bindTraceSphereStamped = true;
}

void CollisionResource::eraseNodeAt(int node_index)
{
  G_ASSERT_RETURN((uint32_t)node_index < allNodesList.size(), );
  erase_items(allNodesList, node_index, 1);
  defaultInstance.nodeTm.erase(defaultInstance.nodeTm.begin() + node_index);
  authoredNodeTm.erase(authoredNodeTm.begin() + node_index);
  if ((uint32_t)node_index < authoredNodeItm.size())
    authoredNodeItm.erase(authoredNodeItm.begin() + node_index);
  defaultInstance.poseMeta.erase(defaultInstance.poseMeta.begin() + node_index);
  if (relGeomNodeTms.size() > node_index)
    erase_items(relGeomNodeTms, node_index, 1);
}

struct ITestIntersectionAlgo
{
  // a_head_idx / b_head_idx index into a_all / b_all and address the head of the per-type linked
  // list; CollisionNode::INVALID_IDX means the list is empty. Walk via allNodesList[i].nextNode.
  // Resources select geometry storage; instances supply placements.
  virtual bool apply(uint16_t a_head_idx, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a, uint16_t b_head_idx,
    dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes,
    const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource *res_b,
    const CollisionResourceInstance *inst_b) = 0;
  virtual ~ITestIntersectionAlgo() = 0;

  // Shared eligibility gate of every pair-test arm: hidden or unrealizable poses never collide.
  // Zero-vert markers (r < 0) and degenerate-dropped mesh/convex have no geometry either --
  // composing their stale (inverted-empty) bounds would manufacture phantom colliders.
  static bool poseCollidable(const CollisionResourceInstance *inst, const CollisionNode *n)
  {
    if (inst->getResource()->getNodeBSphereRadius(n->nodeIndex) < 0.f ||
        ((n->type == COLLISION_NODE_TYPE_MESH || n->type == COLLISION_NODE_TYPE_CONVEX) && !n->hasGeometry()))
      return false;
    return inst->isNodeEnabled(n->nodeIndex) && inst->isNodeTraceable(n->nodeIndex);
  }

  // Composed box/sphere-node geometry in its historical resource frame.
  // Pair tests preserve the legacy conservative AABB contract for boxes.
  static BBox3 composedNodeBox(const CollisionResourceInstance *inst, int node_index, const BBox3 &local_box)
  {
    if (inst->getPoseMeta(node_index).flags & CollisionNode::IDENT)
      return local_box;
    TMatrix tm;
    v_mat_43cu_from_mat44(tm.array, inst->getNodeGeometryTm(node_index));
    BBox3 out;
    for (int k = 0; k < 8; k++)
      out += tm * local_box.point(k);
    return out;
  }

  Point3 collisionPointA;
  Point3 collisionPointB;
};

ITestIntersectionAlgo::~ITestIntersectionAlgo() {}

// Per-node verts source resolver for the testIntersection algos. Returns a pointer such that
// p[idx[k]] yields the k-th vert of `node` (already offset by node.verticesOfs):
//   - non-resident: fallback_verts_base + node.verticesOfs unchanged.
//   - grid-resident: decodes vert21 into scratch and returns scratch.data() (zero-based; node
//     indices [0, verticesCount) map directly into the materialised scratch).
// Returns nullptr if grid-resident but the resolved grid has empty blasData (defensive).
class TestMeshNodeMeshNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  // b_grid carries B's combined-per-behavior BLAS (used when blasData is non-empty); pass null or an
  // empty-blasData grid to force the brute-force tri-tri path.
  TestMeshNodeMeshNodesIntersectionAlgo(vec4f a_wbsph, uint32_t a_nodes_count, vec4f b_wbsph, const CollisionResource::Grid *b_grid) :
    a_wbsph(a_wbsph), a_nodes_count(a_nodes_count), b_wbsph(b_wbsph), b_grid(b_grid)
  {}
  ~TestMeshNodeMeshNodesIntersectionAlgo() final = default;

  bool apply(uint16_t a_head_idx, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a, uint16_t b_head_idx,
    dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes,
    const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource *res_b,
    const CollisionResourceInstance *inst_b) final
  {
    aResource = res_a;
    bResource = res_b;
    aInst = inst_a;
    bInst = inst_b;
    outsideOfBounding.clear();
    reserve_and_resize(outsideOfBounding, a_nodes_count);
    aWtms.clear();
    aWtms.reserve(a_nodes_count);
    aWtms.resize(a_nodes_count);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      mat44f vWtmA;
      v_mat44_mul43(vWtmA, tm_a, inst_a->getNodeTm(nodeA->nodeIndex));
      vec4f aNodeWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x)),
        v_splats(nodeA->boundingSphere.r * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale));
      outsideOfBounding.set(nodeA->nodeIndex, !isBoundingsIntersect(aNodeWbsph, v_set_x(a_max_scale), b_wbsph, v_set_x(b_max_scale)));
      aWtms[nodeA->nodeIndex] = vWtmA;
    }

    for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
    {
      const CollisionNode *nodeB = &b_all[ib];
      if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
        continue;
      if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;

      mat44f vWtmB, vIWtmB;
      v_mat44_mul43(vWtmB, tm_b, inst_b->getNodeTm(nodeB->nodeIndex));
      // The transformed vector's w lane is not a radius.
      vec4f bNodeBoundingCenter = v_perm_xyzd(v_mat44_mul_vec3p(vWtmB, v_ldu(&nodeB->boundingSphere.c.x)),
        v_splats(nodeB->boundingSphere.r * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale));
      if (!isBoundingsIntersect(bNodeBoundingCenter, v_set_x(b_max_scale), a_wbsph, v_set_x(a_max_scale)))
        continue;
      v_mat44_inverse43(vIWtmB, vWtmB);
      vec3f sphereCenterNodeB = v_mat44_mul_vec3p(vWtmB, v_ldu(&nodeB->boundingSphere.c.x));

      for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
      {
        const CollisionNode *nodeA = &a_all[ia];
        if (outsideOfBounding[nodeA->nodeIndex])
          continue;
        // Skipped nodes have no initialized world transform.
        if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
          continue;
        if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;

        mat44f vWtmA = aWtms[nodeA->nodeIndex];
        vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale +
                       nodeB->boundingSphere.r * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        alignas(EA_CACHE_LINE_SIZE) mat44f tmAtoB;
        v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);

        // SAT overlap is frame-independent, so test in B's frame with tmAtoB (also used for the mesh
        // test below); this avoids computing the inverse of A's world tm and the reverse tmBtoA.
        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = v_ldu_bbox3(nodeB->modelBBox);
        if (v_bbox3_test_trasformed_box_likely_intersect(bboxB, bboxA, tmAtoB) == false)
          continue;

        if (isMeshNodeIntersectedWithMeshNode(nodeA, nodeB, tmAtoB, vWtmA, vWtmB))
          return true;
      }
    }

    return false;
  }

  static bool isBoundingsIntersect(vec4f a, vec4f a_max_scale, vec4f b, vec4f b_max_scale)
  {
    vec4f dist = v_length3_sq_x(v_sub(a, b));
    vec4f aRad = v_mul_x(v_splat_w(a), a_max_scale);
    vec4f bRad = v_mul_x(v_splat_w(b), b_max_scale);
    vec4f radSum = v_add_x(aRad, bRad);
    return v_test_vec_x_le(dist, v_mul_x(radSum, radSum));
  }

private:
  // Collect B's triangles overlapping the transformed-A bbox by walking B's BLAS into candidateTris
  // (resource-local verts via BlasLocalUnquant) for the outer A-loop's tri-tri tests. The combined
  // BLAS holds leaves from every behavior-matching node, so we filter to leaves owned by node_b (leaf
  // first vert21 index vs node_b's blasNodeRanges entry); otherwise a sibling's triangles would be
  // tested against node_a and the hit misattributed to node_b. Returns false when the BLAS is empty
  // or node_b has no NodeRange entry (caller falls back to brute-force).
  bool collectBlasCandidates(const bbox3f &bbox_in_b, const CollisionNode *node_b)
  {
    candidateTris.clear();
    if (!b_grid || b_grid->blasData.empty())
      return false;
    const CollisionResource::Grid::NodeRange *nodeBRange = nullptr;
    for (const auto &r : b_grid->blasNodeRanges)
      if (r.nodeIndex == node_b->nodeIndex)
      {
        nodeBRange = &r;
        break;
      }
    if (!nodeBRange)
      return false; // node_b not in this BLAS -- caller falls back to brute-force
    const uint8_t *bData = b_grid->blasData.data();
    const uint32_t vertsOfs = b_grid->blasVertsOfs();
    const uint32_t nodeVOfs = nodeBRange->verticesOfs;
    const uint32_t nodeVEnd = nodeBRange->verticesEnd;
    // Box-space bbox check: convert the A-in-B bbox into the BLAS quantization frame and AABB-overlap
    // it against each inner node's bmin/bmax (already box-space), avoiding per-node inverse transforms.
    const vec3f bScale = b_grid->blasScale;
    const vec3f bOfs = b_grid->blasOfs;
    const vec3f boxMinQ = v_madd(bbox_in_b.bmin, bScale, bOfs);
    const vec3f boxMaxQ = v_madd(bbox_in_b.bmax, bScale, bOfs);
    const BlasLocalUnquant unquantVL = BlasLocalUnquant::make(b_grid->blasBBox.bmin, b_grid->blasScale);
    soa4::iterateFilteredVerts(
      bData, b_grid->blasRootRef,
      [boxMinQ, boxMaxQ](vec3f bmn, vec3f bmx) {
        return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(boxMaxQ, bmn), v_cmp_ge(bmx, boxMinQ)));
      },
      // -V657: always returns false by design -- collects every overlapping candidate with no
      // early-out; false means "continue iterating" per the iterateFiltered contract.
      [this, vertsOfs, nodeVOfs, nodeVEnd](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int apexByteOfs) -> bool { //-V657
        // Recover the leaf's owning node (first-vert byte offset -> vert21 index, tested against
        // node_b's range); leaves owned by sibling nodes are skipped.
        const uint32_t v0Idx = ((uint32_t)apexByteOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
        if (v0Idx < nodeVOfs || v0Idx >= nodeVEnd)
          return false; // not this node's leaf
        candidateTris.push_back({v0, v1, v2});
        return false; // continue
      },
      unquantVL);
    return true;
  }

  bool isMeshNodeIntersectedWithMeshNode(const CollisionNode *node_a, const CollisionNode *node_b, const mat44f &tm_a_to_b,
    const mat44f &v_wtm_a, const mat44f &v_wtm_b)
  {
    bool useBlas = false;
    if (b_grid && !b_grid->blasData.empty())
    {
      bbox3f bbox;
      v_bbox3_init(bbox, tm_a_to_b, v_ldu_bbox3(node_a->modelBBox));
      useBlas = collectBlasCandidates(bbox, node_b);
    }

    // iterateNodeFacesVerts dispatches on residency internally (non-resident: per-node chunk tree +
    // decoded chunk block; resident: walk the grid's BLAS, decode vert21), always yielding resource-local face
    // verts. Replaces the prior pointer reads that were stale for BLAS-resident MESH nodes.
    //
    // The !useBlas brute-force branch pre-materialises node_b's faces because a BLAS-resident
    // iterateNodeFacesVerts walks the full BLAS (no per-node leaf side table); doing that inside the
    // per-node_a-face loop would be O(total_leaves) per face.
    struct CachedFace
    {
      vec4f v0, v1, v2;
    };
    dag::Vector<CachedFace, framemem_allocator> bFaceCache;
    if (!useBlas)
    {
      bFaceCache.reserve((size_t)bResource->getNodeFaceCount(node_b->nodeIndex));
      bResource->iterateNodeFacesVerts(node_b->nodeIndex,
        [&](int, vec4f b0, vec4f b1, vec4f b2) { bFaceCache.push_back({b0, b1, b2}); });
    }

    bool found = false;
    auto emitHit = [&](vec3f a0, vec3f a1, vec3f a2, vec3f b0, vec3f b1, vec3f b2) {
      vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
      vec3f bc = v_mul(v_add(b0, v_add(b1, b2)), v_splats(1 / 3.f));
      // Contacts use full world transforms.
      v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(v_wtm_a, ac));
      v_stu_p3(&collisionPointB.x, v_mat44_mul_vec3p(v_wtm_b, bc));
      found = true;
    };
    aResource->iterateNodeFacesVerts(node_a->nodeIndex, [&](int, vec4f a0, vec4f a1, vec4f a2) {
      if (found)
        return;
      vec3f a0b = v_mat44_mul_vec3p(tm_a_to_b, a0);
      vec3f a1b = v_mat44_mul_vec3p(tm_a_to_b, a1);
      vec3f a2b = v_mat44_mul_vec3p(tm_a_to_b, a2);
      if (useBlas)
      {
        for (const auto &tri : candidateTris)
          if (v_test_triangle_triangle_intersection(a0b, a1b, a2b, tri.v0, tri.v1, tri.v2))
          {
            emitHit(a0, a1, a2, tri.v0, tri.v1, tri.v2);
            return;
          }
      }
      else
      {
        for (const auto &bf : bFaceCache)
          if (v_test_triangle_triangle_intersection(a0b, a1b, a2b, bf.v0, bf.v1, bf.v2))
          {
            emitHit(a0, a1, a2, bf.v0, bf.v1, bf.v2);
            return;
          }
      }
    });
    return found;
  }

  vec4f a_wbsph; // pos|r
  vec4f b_wbsph; // pos|r
  const CollisionResource::Grid *b_grid;
  uint32_t a_nodes_count;
  struct CandidateTri
  {
    vec3f v0, v1, v2;
  };
  dag::Vector<CandidateTri, framemem_allocator> candidateTris;
  // Set on every apply() entry; read by iterateNodeFacesVerts in isMeshNodeIntersectedWithMeshNode,
  // which dispatches BLAS-resident vs non-resident internally, so no separate vert/idx base or scratch
  // members are kept here.
  const CollisionResource *aResource = nullptr;
  const CollisionResource *bResource = nullptr;
  const CollisionResourceInstance *aInst = nullptr;
  const CollisionResourceInstance *bInst = nullptr;
  eastl::bitvector<framemem_allocator> outsideOfBounding;
  dag::RelocatableFixedVector<mat44f, 40> aWtms;
};


class TestMeshNodeBoxNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestMeshNodeBoxNodesIntersectionAlgo() = default;
  ~TestMeshNodeBoxNodesIntersectionAlgo() final = default;

  bool apply(uint16_t a_head_idx, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a, uint16_t b_head_idx,
    dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes,
    const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource * /*res_b*/,
    const CollisionResourceInstance *inst_b) final
  {
    aResource = res_a;
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, tmAtoB;
      v_mat44_mul43(vWtmA, tm_a, inst_a->getNodeTm(nodeA->nodeIndex));
      v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);
      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));

      for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
          continue;
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        // Box bounding-sphere centers are stored node-local.
        vec3f sphereCenterNodeB =
          v_mat44_mul_vec3p(tm_b, v_mat44_mul_vec3p(inst_b->getNodeTm(nodeB->nodeIndex), v_ldu(&nodeB->boundingSphere.c.x)));
        float sumRad = nodeA->boundingSphere.r * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale +
                       nodeB->boundingSphere.r * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        // SAT overlap is frame-independent, so test in B's frame with tmAtoB (also used to transform A's
        // triangles below); this avoids the inverse of A's world tm and the reverse tmBtoA.
        const BBox3 composedBoxB = composedNodeBox(inst_b, nodeB->nodeIndex, nodeB->modelBBox);
        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = v_ldu_bbox3(composedBoxB);
        if (v_bbox3_test_trasformed_box_likely_intersect(bboxB, bboxA, tmAtoB) == false)
          continue;

        // iterateNodeFacesVerts dispatches on residency internally; the resource keeps no index
        // list, so the prior aIdxBase + nodeA->indicesOfs read was stale.
        bool found = false;
        aResource->iterateNodeFacesVerts(nodeA->nodeIndex, [&](int, vec4f a0, vec4f a1, vec4f a2) {
          if (found)
            return;
          Point3_vec4 a0b, a1b, a2b;
          v_st(&a0b.x, v_mat44_mul_vec3p(tmAtoB, a0));
          v_st(&a1b.x, v_mat44_mul_vec3p(tmAtoB, a1));
          v_st(&a2b.x, v_mat44_mul_vec3p(tmAtoB, a2));
          if (test_triangle_box_intersection(a0b, a1b, a2b, composedBoxB))
          {
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
            v_stu_p3(&collisionPointB.x, sphereCenterNodeB); // posed center: raw c would report bind
            found = true;
          }
        });
        if (found)
          return true;
      }
    }

    return false;
  }

  const CollisionResource *aResource = nullptr;
};

class TestBoxNodeBoxNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestBoxNodeBoxNodesIntersectionAlgo() = default;
  ~TestBoxNodeBoxNodesIntersectionAlgo() final = default;

  bool apply(uint16_t a_head_idx, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a, uint16_t b_head_idx,
    dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes,
    const CollisionResource * /*res_a*/, const CollisionResourceInstance *inst_a, const CollisionResource * /*res_b*/,
    const CollisionResourceInstance *inst_b) final
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmA, tmBToA;
    v_mat44_inverse43(vIWtmA, tm_a);
    v_mat44_mul43(tmBToA, vIWtmA, tm_b);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      // Box bounding-sphere centers are stored node-local.
      vec3f sphereCenterNodeA =
        v_mat44_mul_vec3p(tm_a, v_mat44_mul_vec3p(inst_a->getNodeTm(nodeA->nodeIndex), v_ldu(&nodeA->boundingSphere.c.x)));
      const BBox3 composedBoxA = composedNodeBox(inst_a, nodeA->nodeIndex, nodeA->modelBBox);
      for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
          continue;
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB =
          v_mat44_mul_vec3p(tm_b, v_mat44_mul_vec3p(inst_b->getNodeTm(nodeB->nodeIndex), v_ldu(&nodeB->boundingSphere.c.x)));
        float sumRad = nodeA->boundingSphere.r * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale +
                       nodeB->boundingSphere.r * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        if (v_bbox3_test_trasformed_box_likely_intersect(v_ldu_bbox3(composedBoxA),
              v_ldu_bbox3(composedNodeBox(inst_b, nodeB->nodeIndex, nodeB->modelBBox)), tmBToA))
        {
          v_stu_p3(&collisionPointA.x, sphereCenterNodeA); // posed centers: raw c would report bind
          v_stu_p3(&collisionPointB.x, sphereCenterNodeB);
          return true;
        }
      }
    }

    return false;
  }
};

class TestMeshNodeSphereNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestMeshNodeSphereNodesIntersectionAlgo() = default;
  ~TestMeshNodeSphereNodesIntersectionAlgo() final = default;

  bool apply(uint16_t a_head_idx, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a, uint16_t b_head_idx,
    dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes,
    const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource * /*res_b*/,
    const CollisionResourceInstance *inst_b) final
  {
    aResource = res_a;
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, tmAToB;
      v_mat44_mul43(vWtmA, tm_a, inst_a->getNodeTm(nodeA->nodeIndex));
      v_mat44_mul43(tmAToB, vIWtmB, vWtmA);

      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));
      for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
          continue;
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        // maxTmScale conservatively encloses a non-uniformly posed sphere.
        const bool identTB = (inst_b->getPoseMeta(nodeB->nodeIndex).flags & CollisionNode::IDENT) != 0;
        Point3_vec4 composedCB;
        v_st(&composedCB.x, identTB
                              ? v_ldu(&nodeB->boundingSphere.c.x)
                              : v_mat44_mul_vec3p(inst_b->getNodeGeometryTm(nodeB->nodeIndex), v_ldu(&nodeB->boundingSphere.c.x)));
        // A BAKED sphere's stored radius is already in the tested frame (its geometry tm is
        // identity): scaling it again would shrink or inflate the narrow phase.
        const float composedRB =
          (nodeB->boundingSphere.r < 0.f || identTB ||
            (inst_b->getPoseMeta(nodeB->nodeIndex).isGeometryBaked() && !inst_b->getPoseMeta(nodeB->nodeIndex).isRetainedBake()))
            ? nodeB->boundingSphere.r
            : nodeB->boundingSphere.r * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&composedCB.x));
        float sumRad =
          nodeA->boundingSphere.r * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale + composedRB * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        // The widened enclosing sphere is a CULL, not the geometry: the narrow phase runs in
        // the sphere's STORED frame against the stored radius, or a non-uniform pose would
        // report contacts outside its short axes.
        mat44f triToSphere = tmAToB;
        if (!identTB)
        {
          mat44f invGtmB;
          v_mat44_inverse43(invGtmB, inst_b->getNodeGeometryTm(nodeB->nodeIndex));
          v_mat44_mul43(triToSphere, invGtmB, tmAToB);
        }
        const vec3f storedCB = v_ldu(&nodeB->boundingSphere.c.x);
        const vec4f storedRB2 = v_set_x(get_bsphere_r2(nodeB->boundingSphere.r));
        bool found = false;
        aResource->iterateNodeFacesVerts(nodeA->nodeIndex, [&](int, vec4f a0, vec4f a1, vec4f a2) {
          if (found)
            return;
          vec3f a0b = v_mat44_mul_vec3p(triToSphere, a0);
          vec3f a1b = v_mat44_mul_vec3p(triToSphere, a1);
          vec3f a2b = v_mat44_mul_vec3p(triToSphere, a2);
          if (v_test_triangle_sphere_intersection(a0b, a1b, a2b, storedCB, storedRB2))
          {
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1.f / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
            v_stu_p3(&collisionPointB.x, sphereCenterNodeB);
            found = true;
          }
        });
        if (found)
          return true;
      }
    }

    return false;
  }

  const CollisionResource *aResource = nullptr;
};

bool CollisionResource::testIntersection(const CollisionResource *res_a, const TMatrix &tm_a, const CollisionResource *res_b,
  const TMatrix &tm_b, Point3 &collisionPointA, Point3 &collisionPointB, bool checkOnlyPhysNodes, /* = false,*/
  bool useTraceFaces /* = false*/)
{
  G_ASSERT(res_a);
  G_ASSERT(res_b);
  return testIntersection(res_a, tm_a, res_a->getDefaultInstance(), res_b, tm_b, res_b->getDefaultInstance(), collisionPointA,
    collisionPointB, checkOnlyPhysNodes, useTraceFaces);
}

bool CollisionResource::testIntersection(const CollisionResource *res_a, const TMatrix &tm_a,
  const CollisionResourceInstance &instance_a, const CollisionResource *res_b, const TMatrix &tm_b,
  const CollisionResourceInstance &instance_b, Point3 &collisionPointA, Point3 &collisionPointB, bool checkOnlyPhysNodes,
  bool useTraceFaces)
{
  G_ASSERT(res_a);
  G_ASSERT(res_b);
  const CollisionResourceInstance *instA = res_a->resolveOwnedPoseForQuery(instance_a, "test_collision_resources_intersection");
  const CollisionResourceInstance *instB = res_b->resolveOwnedPoseForQuery(instance_b, "test_collision_resources_intersection");

  alignas(EA_CACHE_LINE_SIZE) mat44f vTmA, vTmB;
  v_mat44_make_from_43cu_unsafe(vTmA, tm_a.array);
  v_mat44_make_from_43cu_unsafe(vTmB, tm_b.array);
  // Column lengths under-bound shear on the arbitrary outer tms, and both pair families
  // pre-reject on these scales: use the conservative bound.
  float maxScaleA = conservative_outer_scale(vTmA);
  float maxScaleB = conservative_outer_scale(vTmB);

  // Posed instances derive whole-resource bounds from rootBBox.
  auto worldBsph = [](const mat44f &tm, const CollisionResource *res, const CollisionResourceInstance *inst) {
    if (inst->isDefault() && !inst->isPosedSinceBind())
    {
      // The stamped sphere extends the serialized one (rotated boxes, eps-IDENT mesh frames).
      // A stamp smaller than the live sphere is stale (stamped before bounds): live wins.
      if (DAGOR_LIKELY(res->bindTraceSphereStamped && v_extract_w(res->vBindTraceSphere) >= sqr(res->boundingSphereRad)))
        return v_perm_xyzd(v_mat44_mul_vec3p(tm, res->vBindTraceSphere), v_sqrt(v_splat_w(res->vBindTraceSphere)));
      return v_perm_xyzd(v_mat44_mul_vec3p(tm, res->vBoundingSphere), v_splats(res->boundingSphereRad));
    }
    const bbox3f rootBox = inst->getRootBBox();
    const vec3f rootCenter = v_madd(rootBox.bmin, V_C_HALF, v_mul(rootBox.bmax, V_C_HALF)); // overflow-safe midpoint
    return v_perm_xyzd(v_mat44_mul_vec3p(tm, rootCenter), v_length3(v_sub(rootBox.bmax, rootCenter)));
  };
  vec4f aWbsph = worldBsph(vTmA, res_a, instA);
  vec4f bWbsph = worldBsph(vTmB, res_b, instB);
  if (!TestMeshNodeMeshNodesIntersectionAlgo::isBoundingsIntersect(aWbsph, v_set_x(maxScaleA), bWbsph, v_set_x(maxScaleB)))
    return false;

  // The combined grid holds bind-frame bytes: a live-posed grid member de-licenses the walk (same
  // latch as the trace core); the algo's brute-force arm composes the current pose per node.
  TestMeshNodeMeshNodesIntersectionAlgo testMeshNodeMeshNodesIntersectionAlgo(aWbsph, res_a->allNodesList.size(), bWbsph,
    useTraceFaces && instB->isGridResidentPoseAtBind() ? &res_b->gridForTraceable : nullptr);
  TestMeshNodeBoxNodesIntersectionAlgo testMeshNodeBoxNodesIntersectionAlgo;
  TestMeshNodeSphereNodesIntersectionAlgo testMeshNodeSphereNodesIntersectionAlgo;
  TestBoxNodeBoxNodesIntersectionAlgo testBoxNodeBoxNodesIntersectionAlgo;

  ITestIntersectionAlgo *arrIntersectionCall[COLLISION_NODE_TYPE_CAPSULE][COLLISION_NODE_TYPE_CAPSULE] = {};
  arrIntersectionCall[COLLISION_NODE_TYPE_MESH][COLLISION_NODE_TYPE_MESH] = &testMeshNodeMeshNodesIntersectionAlgo;
  arrIntersectionCall[COLLISION_NODE_TYPE_MESH][COLLISION_NODE_TYPE_BOX] = &testMeshNodeBoxNodesIntersectionAlgo;
  arrIntersectionCall[COLLISION_NODE_TYPE_MESH][COLLISION_NODE_TYPE_SPHERE] = &testMeshNodeSphereNodesIntersectionAlgo;
  arrIntersectionCall[COLLISION_NODE_TYPE_BOX][COLLISION_NODE_TYPE_BOX] = &testBoxNodeBoxNodesIntersectionAlgo;

  dag::ConstSpan<CollisionNode> aAll = res_a->getAllNodes();
  dag::ConstSpan<CollisionNode> bAll = res_b->getAllNodes();
  for (int nodeTypeIxA = COLLISION_NODE_TYPE_MESH; nodeTypeIxA < COLLISION_NODE_TYPE_CAPSULE; nodeTypeIxA++)
  {
    if (res_a->nodeLists[nodeTypeIxA] == CollisionNode::INVALID_IDX)
      continue;

    for (int nodeTypeIxB = COLLISION_NODE_TYPE_MESH; nodeTypeIxB < COLLISION_NODE_TYPE_CAPSULE; nodeTypeIxB++)
    {
      if (res_b->nodeLists[nodeTypeIxB] == CollisionNode::INVALID_IDX)
        continue;

      bool result = false;
      if (ITestIntersectionAlgo *testAB = arrIntersectionCall[nodeTypeIxA][nodeTypeIxB])
      {
        result = testAB->apply(res_a->nodeLists[nodeTypeIxA], aAll, vTmA, res_b->nodeLists[nodeTypeIxB], bAll, vTmB, maxScaleA,
          maxScaleB, checkOnlyPhysNodes, res_a, instA, res_b, instB);

        collisionPointA = testAB->collisionPointA;
        collisionPointB = testAB->collisionPointB;
      }
      else if (ITestIntersectionAlgo *testBA = arrIntersectionCall[nodeTypeIxB][nodeTypeIxA])
      {
        result = testBA->apply(res_b->nodeLists[nodeTypeIxB], bAll, vTmB, res_a->nodeLists[nodeTypeIxA], aAll, vTmA, maxScaleB,
          maxScaleA, checkOnlyPhysNodes, res_b, instB, res_a, instA);

        collisionPointB = testBA->collisionPointA;
        collisionPointA = testBA->collisionPointB;
      }

      if (result)
        return true;
    }
  }

  return false;
}

// Test the mesh-vs-mesh pair (node1, node2) given node1's pre-materialised face triples.
// Returns true on hit; writes cp1/cp2 (world-space centroids) and *nodeIndex1/2 if non-null.
// node2's authoritative BLAS grid (getBlasGridForResidentNode: gridForCollidable wins for a
// PHYS_COLLIDABLE node, matching Jolt and iterateNodeFacesVerts) prunes node2's faces by node1's bbox
// in res2 frame AND filters to leaves owned by node2 (the combined BLAS interleaves leaves from every
// IDENT MESH node). Without a BLAS range, falls back to brute-force tri-tri via iterateNodeFacesVerts.
// Both branches feed verts in their natural source frame (BLAS: res2 resource-local; brute: node-local
// from the decoded node block) into the inner `tm2to1 * v2` -- consistent because the BLAS holds only
// IDENT-transform nodes, so node-local == res2-local.
bool CollisionResource::testMeshNodePair(const CollisionNode *node1, dag::ConstSpan<Point3_vec4> node1Faces,
  const CollisionResource *res2, const CollisionNode *node2, const TMatrix &node2_wtm, const TMatrix &tm1ToWorld,
  const TMatrix &tm2to1, Point3 &cp1, Point3 &cp2, uint16_t *node_index1, uint16_t *node_index2, bool node2_grid_at_bind)
{
  dag::Vector<Point3_vec4, framemem_allocator> res2Faces;
  const CollisionResource::Grid &res2Blas = res2->getBlasGridForResidentNode(*node2);
  const CollisionResource::Grid::NodeRange *nr2 = nullptr;
  // The grid arm serves bind-frame bytes: a live-posed grid member must brute-force through the
  // caller's posed tms instead (same latch as the trace core).
  if (node2_grid_at_bind && !res2Blas.blasData.empty())
    for (const auto &r : res2Blas.blasNodeRanges)
      if (r.nodeIndex == node2->nodeIndex)
      {
        nr2 = &r;
        break;
      }
  if (!nr2)
  {
    // node2 isn't in res2's resident grid (the one getBlasGridForResidentNode picks: gridForCollidable
    // for a PHYS_COLLIDABLE node when grids are split, else gridForTraceable). Brute-force its faces;
    // iterateNodeFacesVerts dispatches on residency internally (gridForCollidable residents still
    // decode). This previously fell through to `return false`, silently dropping the pair.
    res2Faces.reserve((size_t)res2->getNodeFaceCount(node2->nodeIndex) * 3u);
    res2->iterateNodeFacesVerts(node2->nodeIndex, [&](int, vec4f v0, vec4f v1, vec4f v2) {
      Point3_vec4 p0, p1, p2;
      v_st(&p0.x, v0);
      v_st(&p1.x, v1);
      v_st(&p2.x, v2);
      res2Faces.push_back(p0);
      res2Faces.push_back(p1);
      res2Faces.push_back(p2);
    });
  }
  else
  {
    // Grid candidates must invert the full posed placement.
    const TMatrix tm1to2 = inverse(node2_wtm) * tm1ToWorld;
    bbox3f bboxRes2v = v_ldu_bbox3(tm1to2 * node1->modelBBox);
    const vec3f boxMinQ = v_madd(bboxRes2v.bmin, res2Blas.blasScale, res2Blas.blasOfs);
    const vec3f boxMaxQ = v_madd(bboxRes2v.bmax, res2Blas.blasScale, res2Blas.blasOfs);
    const BlasLocalUnquant unquantVL = BlasLocalUnquant::make(res2Blas.blasBBox.bmin, res2Blas.blasScale);
    const uint8_t *bData = res2Blas.blasData.data();
    const uint32_t vertsOfs = res2Blas.blasVertsOfs();
    const uint32_t nodeVOfs = nr2->verticesOfs;
    const uint32_t nodeVEnd = nr2->verticesEnd;
    soa4::iterateFilteredVerts(
      bData, res2Blas.blasRootRef,
      [boxMinQ, boxMaxQ](vec3f bmn, vec3f bmx) {
        return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(boxMaxQ, bmn), v_cmp_ge(bmx, boxMinQ)));
      },
      // -V657: always returns false by design -- collects every overlapping candidate with no
      // early-out; false means "continue iterating" per the iterateFiltered contract.
      [&res2Faces, vertsOfs, nodeVOfs, nodeVEnd](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int apexByteOfs) -> bool { //-V657
        const uint32_t v0Idx = ((uint32_t)apexByteOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
        if (v0Idx < nodeVOfs || v0Idx >= nodeVEnd)
          return false; // sibling-node leaf
        Point3_vec4 p0, p1, p2;
        v_st(&p0.x, v0);
        v_st(&p1.x, v1);
        v_st(&p2.x, v2);
        res2Faces.push_back(p0);
        res2Faces.push_back(p1);
        res2Faces.push_back(p2);
        return false;
      },
      unquantVL);
  }

  for (size_t i1 = 0; i1 + 2 < node1Faces.size(); i1 += 3)
  {
    const Point3 &v1_0 = node1Faces[i1 + 0];
    const Point3 &v1_1 = node1Faces[i1 + 1];
    const Point3 &v1_2 = node1Faces[i1 + 2];
    for (size_t i2 = 0; i2 + 2 < res2Faces.size(); i2 += 3)
    {
      const Point3 &v2_0 = res2Faces[i2 + 0];
      const Point3 &v2_1 = res2Faces[i2 + 1];
      const Point3 &v2_2 = res2Faces[i2 + 2];
      if (test_triangle_triangle_intersection_mueller(v1_0, v1_1, v1_2, tm2to1 * v2_0, tm2to1 * v2_1, tm2to1 * v2_2))
      {
        cp1 = tm1ToWorld * ((v1_0 + v1_1 + v1_2) * 0.333333f);
        cp2 = node2_wtm * ((v2_0 + v2_1 + v2_2) * 0.333333f);
        if (node_index1)
          *node_index1 = node1->nodeIndex;
        if (node_index2)
          *node_index2 = node2->nodeIndex;
        return true;
      }
    }
  }
  return false;
}

bool CollisionResource::testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
  const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2, Point3 &collisionPoint1,
  Point3 &collisionPoint2, uint16_t *nodeIndex1, uint16_t *nodeIndex2, Tab<uint16_t> *node_indices1)
{
  G_ASSERT(res1);
  G_ASSERT(res2);
  return testIntersection(res1, tm1, filter1, res1->getDefaultInstance(), res2, tm2, filter2, res2->getDefaultInstance(),
    collisionPoint1, collisionPoint2, nodeIndex1, nodeIndex2, node_indices1);
}

bool CollisionResource::testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
  const CollisionResourceInstance &instance1, const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2,
  const CollisionResourceInstance &instance2, Point3 &collisionPoint1, Point3 &collisionPoint2, uint16_t *nodeIndex1,
  uint16_t *nodeIndex2, Tab<uint16_t> *node_indices1)
{
  G_ASSERT(res1);
  G_ASSERT(res2);
  const CollisionResourceInstance *inst1 = res1->resolveOwnedPoseForQuery(instance1, "test_collres_intersection");
  const CollisionResourceInstance *inst2 = res2->resolveOwnedPoseForQuery(instance2, "test_collres_intersection");
  auto nodePlacement = [](const CollisionResourceInstance *inst, const CollisionNode *n) -> const TMatrix & {
    return inst->getNodeTmRef(n->nodeIndex);
  };

  // Conservative stretch, not one column: the outer tms are arbitrary caller matrices, and every
  // radius cull below scales by these (same bound as the dispatch pair family).
  mat44f vTm1, vTm2;
  v_mat44_make_from_43cu_unsafe(vTm1, tm1.array);
  v_mat44_make_from_43cu_unsafe(vTm2, tm2.array);
  float scale1 = conservative_outer_scale(vTm1);
  float scale2 = conservative_outer_scale(vTm2);

  Tab<bool> boxOutside(framemem_ptr());
  reserve_and_resize(boxOutside, res2->allNodesList.size());

#if DAGOR_DBGLEVEL > 0
  unsigned int numNodesDebug = 0;
  unsigned int numSpheresDebug = 0;
  unsigned int numBoxesDebug = 0;
  unsigned int numBoxesInsideDebug = 0;
#define _INC(x) ++(x)
#else
#define _INC(x)
#endif

  // Precompute node1-invariant resource-2 pose data.
  struct Mesh2Data
  {
    const CollisionNode *node;
    TMatrix wtm2;   // tm2 * node placement: the node's full world matrix
    Point3 wCenter; // world bounding-sphere center
    float rTerm;    // pose-scaled bounding-sphere radius (before the outer scale2)
  };
  struct Box2Data
  {
    const CollisionNode *node;
    BBox3 composedBox; // stored box in posed resource-2 space
    Point3 wCenter;
    float rTerm;
  };
  struct Sph2Data
  {
    const CollisionNode *node;
    Point3 composedC; // stored sphere in posed resource-2 space
    float composedR;
    TMatrix invGtm;    // resource -> stored sphere frame (identity when the pose is IDENT)
    bool pullToStored; // false: stored frame == resource frame
    Point3 wCenter;
  };
  dag::Vector<Mesh2Data, framemem_allocator> mesh2s;
  dag::Vector<Box2Data, framemem_allocator> box2s;
  dag::Vector<Sph2Data, framemem_allocator> sph2s;
  mesh2s.reserve(res2->numMeshNodes);
  box2s.reserve(res2->numBoxNodes);
  sph2s.reserve(res2->getAllNodes().size() - res2->numMeshNodes - res2->numBoxNodes - res2->numCapsuleNodes);
  for (uint16_t mi2 = res2->meshNodesHead; mi2 != CollisionNode::INVALID_IDX; mi2 = res2->allNodesList[mi2].nextNode)
  {
    const CollisionNode *node2 = &res2->allNodesList[mi2];
    if (!ITestIntersectionAlgo::poseCollidable(inst2, node2))
      continue;
    if (filter2 && !filter2(node2->nodeIndex))
      continue;
    Mesh2Data &m2 = mesh2s.push_back();
    m2.node = node2;
    m2.wtm2 = tm2 * nodePlacement(inst2, node2);
    m2.wCenter = m2.wtm2 * node2->boundingSphere.c;
    m2.rTerm = node2->boundingSphere.r * inst2->getPoseMeta(node2->nodeIndex).maxTmScale;
  }
  for (uint16_t bi2 = res2->boxNodesHead; bi2 != CollisionNode::INVALID_IDX; bi2 = res2->allNodesList[bi2].nextNode)
  {
    const CollisionNode *node2 = &res2->allNodesList[bi2];
    if (!ITestIntersectionAlgo::poseCollidable(inst2, node2))
      continue;
    if (filter2 && !filter2(node2->nodeIndex))
      continue;
    Box2Data &b2 = box2s.push_back();
    b2.node = node2;
    b2.composedBox = ITestIntersectionAlgo::composedNodeBox(inst2, node2->nodeIndex, node2->modelBBox);
    b2.wCenter = tm2 * (nodePlacement(inst2, node2) * node2->boundingSphere.c);
    b2.rTerm = node2->boundingSphere.r * inst2->getPoseMeta(node2->nodeIndex).maxTmScale;
  }
  for (uint16_t si2 = res2->sphereNodesHead; si2 != CollisionNode::INVALID_IDX; si2 = res2->allNodesList[si2].nextNode)
  {
    const CollisionNode *node2 = &res2->allNodesList[si2];
    if (!ITestIntersectionAlgo::poseCollidable(inst2, node2))
      continue;
    if (filter2 && !filter2(node2->nodeIndex))
      continue;
    // Stored sphere geometry follows its compatibility transform.
    const bool identT2 = (inst2->getPoseMeta(node2->nodeIndex).flags & CollisionNode::IDENT) != 0;
    TMatrix geometryTm2;
    v_mat_43cu_from_mat44(geometryTm2.array, inst2->getNodeGeometryTm(node2->nodeIndex));
    Sph2Data &s2 = sph2s.push_back();
    s2.node = node2;
    s2.composedC = identT2 ? node2->boundingSphere.c : geometryTm2 * node2->boundingSphere.c;
    s2.pullToStored = !identT2;
    s2.invGtm = identT2 ? TMatrix::IDENT : inverse(geometryTm2);
    // BAKED radii are already in the tested frame; see the sphere-vs-sphere arm.
    s2.composedR =
      (node2->boundingSphere.r < 0.f || identT2 ||
        (inst2->getPoseMeta(node2->nodeIndex).isGeometryBaked() && !inst2->getPoseMeta(node2->nodeIndex).isRetainedBake()))
        ? node2->boundingSphere.r
        : node2->boundingSphere.r * inst2->getPoseMeta(node2->nodeIndex).maxTmScale;
    // broadphase center in the same single-composed frame the narrow phase tests
    s2.wCenter = tm2 * s2.composedC;
  }

  // Per-node1 face materialisation feeds the per-pair mesh/box/sphere sub-loops below.
  // iterateNodeFacesVerts dispatches on residency internally; cache once per node1 because a
  // BLAS-resident walk is O(total_leaves) (no per-node side table) and would otherwise rerun per node2.
  for (uint16_t mi1 = res1->meshNodesHead; mi1 != CollisionNode::INVALID_IDX; mi1 = res1->allNodesList[mi1].nextNode)
  {
    const CollisionNode *node1 = &res1->allNodesList[mi1];
    if (!ITestIntersectionAlgo::poseCollidable(inst1, node1))
      continue;
    if (filter1 && !filter1(node1->nodeIndex))
      continue;

    const TMatrix node1Tm = nodePlacement(inst1, node1);
    Point3 sphereCenter1 = tm1 * (node1Tm * node1->boundingSphere.c);
    const float r1Term = node1->boundingSphere.r * inst1->getPoseMeta(node1->nodeIndex).maxTmScale * scale1;
    // Initialized despite the lazy-compute flags: MSVC cannot prove the flag protocol (C4701).
    TMatrix invTm1 = TMatrix::IDENT, tm1ToWorld = TMatrix::IDENT;
    bool invTm1ready = false;
    mem_set_0(boxOutside);
    dag::Vector<Point3_vec4, framemem_allocator> node1FaceVerts;
    node1FaceVerts.reserve((size_t)res1->getNodeFaceCount(node1->nodeIndex) * 3u);
    res1->iterateNodeFacesVerts(node1->nodeIndex, [&](int, vec4f v0, vec4f v1, vec4f v2) {
      Point3_vec4 p0, p1, p2;
      v_st(&p0.x, v0);
      v_st(&p1.x, v1);
      v_st(&p2.x, v2);
      node1FaceVerts.push_back(p0);
      node1FaceVerts.push_back(p1);
      node1FaceVerts.push_back(p2);
    });

    for (const Mesh2Data &m2 : mesh2s)
    {
      const CollisionNode *node2 = m2.node;
      _INC(numNodesDebug);

      // Serialized containment is valid only while parent and child remain at bind.
      if (node2->insideOfNode != 0xffff && !inst2->isPosedSinceBind() && boxOutside[node2->insideOfNode])
      {
        boxOutside[node2->nodeIndex] = true;
        continue;
      }

      _INC(numBoxesInsideDebug);

      float sumRad = r1Term + m2.rTerm * scale2;
      if (lengthSq(sphereCenter1 - m2.wCenter) >= sumRad * sumRad)
        continue;
      _INC(numSpheresDebug);

      if (!invTm1ready)
      {
        tm1ToWorld = tm1 * node1Tm;
        invTm1 = inverse(tm1ToWorld);
        invTm1ready = true;
      }
      TMatrix tm2to1 = invTm1 * m2.wtm2;

      if (!test_box_box_intersection(node1->modelBBox, node2->modelBBox, tm2to1))
      {
        boxOutside[node2->nodeIndex] = true;
        continue;
      }
      _INC(numBoxesDebug);

      if (testMeshNodePair(node1, make_span_const(node1FaceVerts), res2, node2, m2.wtm2, tm1ToWorld, tm2to1, collisionPoint1,
            collisionPoint2, nodeIndex1, nodeIndex2, inst2->isGridResidentPoseAtBind()))
      {
        if (!node_indices1)
          return true;
        node_indices1->push_back(node1->nodeIndex);
      }
    }

    TMatrix tm1to2 = TMatrix::IDENT; // initialized for the same C4701 reason as invTm1
    bool tm1to2ready = false;

    auto faceCentroidWorld = [&](size_t i1) {
      return tm1 * (node1Tm * ((node1FaceVerts[i1] + node1FaceVerts[i1 + 1] + node1FaceVerts[i1 + 2]) * 0.333333f));
    };

    for (const Box2Data &b2 : box2s)
    {
      const CollisionNode *node2 = b2.node;
      float sumRad = r1Term + b2.rTerm * scale2;
      if (lengthSq(sphereCenter1 - b2.wCenter) >= sumRad * sumRad)
        continue;
      if (!invTm1ready)
      {
        tm1ToWorld = tm1 * node1Tm;
        invTm1 = inverse(tm1ToWorld);
        invTm1ready = true;
      }
      // Map the composed box into node1's local frame.
      if (!test_box_box_intersection(node1->modelBBox, b2.composedBox, invTm1 * tm2))
        continue;
      if (!tm1to2ready)
      {
        tm1to2 = inverse(tm2) * tm1 * node1Tm;
        tm1to2ready = true;
      }

      for (size_t i1 = 0; i1 + 2 < node1FaceVerts.size(); i1 += 3)
      {
        if (!test_triangle_box_intersection(tm1to2 * node1FaceVerts[i1], tm1to2 * node1FaceVerts[i1 + 1],
              tm1to2 * node1FaceVerts[i1 + 2], b2.composedBox))
          continue;
        collisionPoint1 = faceCentroidWorld(i1);
        collisionPoint2 = b2.wCenter; // posed world center, like the sphere arm
        if (nodeIndex1)
          *nodeIndex1 = node1->nodeIndex;
        if (nodeIndex2)
          *nodeIndex2 = node2->nodeIndex;
        if (!node_indices1)
          return true;
        node_indices1->push_back(node1->nodeIndex);
        break;
      }
    }

    for (const Sph2Data &s2 : sph2s)
    {
      const CollisionNode *node2 = s2.node;
      float sumRad = r1Term + s2.composedR * scale2;
      if (lengthSq(sphereCenter1 - s2.wCenter) >= sqr(sumRad))
        continue;
      if (!tm1to2ready)
      {
        tm1to2 = inverse(tm2) * tm1 * node1Tm;
        tm1to2ready = true;
      }

      // Narrow phase in the sphere's STORED frame: the widened enclosing sphere above is only
      // the cull, and testing it directly would invent contacts outside a non-uniform pose's
      // short axes.
      const TMatrix tri2sphere = s2.pullToStored ? s2.invGtm * tm1to2 : tm1to2;
      const auto node2BSphere = make_node_bsphere(node2->boundingSphere.c, node2->boundingSphere.r);
      for (size_t i1 = 0; i1 + 2 < node1FaceVerts.size(); i1 += 3)
      {
        if (!test_triangle_sphere_intersection(tri2sphere * node1FaceVerts[i1], tri2sphere * node1FaceVerts[i1 + 1],
              tri2sphere * node1FaceVerts[i1 + 2], node2BSphere))
          continue;
        collisionPoint1 = faceCentroidWorld(i1);
        collisionPoint2 = s2.wCenter;
        if (nodeIndex1)
          *nodeIndex1 = node1->nodeIndex;
        if (nodeIndex2)
          *nodeIndex2 = node2->nodeIndex;
        if (!node_indices1)
          return true;
        node_indices1->push_back(node1->nodeIndex);
        break;
      }
    }
  }

#undef _INC

  return node_indices1 ? !node_indices1->empty() : false;
}

void CollisionResource::getCollisionNodeTm(const CollisionNode *node, const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  TMatrix &out_tm) const
{
  mat44f tm, outTm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  getCollisionNodeTm(node, tm, geom_node_tree, outTm);
  v_mat_43cu_from_mat44(out_tm.array, outTm);
}

void CollisionResource::getCollisionNodeTm(const CollisionNode *node, mat44f_cref instance_tm, const GeomNodeTree *geom_node_tree,
  mat44f &out_tm) const
{
  if (node->type == COLLISION_NODE_TYPE_MESH || node->type == COLLISION_NODE_TYPE_CONVEX || node->type == COLLISION_NODE_TYPE_CAPSULE)
  {
    const CollisionResourceTraceAdapter::LegacyPose ps(*this, geom_node_tree);
    out_tm = getPosedNodeWtmInline(node, instance_tm, v_zero(), ps.pose());
  }
  else
    out_tm = instance_tm;
}

void CollisionResource::getCollisionNodeTm(const CollisionNode *node, const TMatrix &instance_tm,
  const CollisionResourceInstance &instance, TMatrix &out_tm) const
{
  mat44f tm, outTm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  getCollisionNodeTm(node, tm, instance, outTm);
  v_mat_43cu_from_mat44(out_tm.array, outTm);
}

void CollisionResource::getCollisionNodeTm(const CollisionNode *node, mat44f_cref instance_tm,
  const CollisionResourceInstance &instance, mat44f &out_tm) const
{
  // uniform frame contract: the posed node->world matrix is instance_tm * T for every node type
  const CollisionResourceInstance *inst = resolveInstanceForTrace(instance);
  out_tm = getPosedNodeWtmInline(node, instance_tm, v_zero(), *inst);
}

template <typename pose_t>
__forceinline mat44f CollisionResource::getPosedNodeWtmInline(const CollisionNode *node, mat44f_cref instance_tm,
  vec3f instance_woffset, const pose_t &instance) const
{
  mat44f outTm;
  const GeomNodeTree *geomNodeTree = instance.getTree();
  if (auto idx = (geomNodeTree && node->geomNodeId.index() < geomNodeTree->nodeCount()) ? node->geomNodeId : dag::Index16())
  {
    outTm = geomNodeTree->getNodeWtmRel(idx); //-V1004
    outTm.col3 = v_add(outTm.col3, v_sub(geomNodeTree->getWtmOfs(), instance_woffset));
    if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    {
      mat44f relGeomNodeTm;
      v_mat44_make_from_43cu_unsafe(relGeomNodeTm, relGeomNodeTms[node->nodeIndex].array);
      v_mat44_mul43(outTm, outTm, relGeomNodeTm);
    }
  }
  else if (!geomNodeTree)
    v_mat44_mul43(outTm, instance_tm, instance.getNodeTm(node->nodeIndex));
  else
    v_mat44_mul43(outTm, instance_tm, defaultInstance.getNodeTm(node->nodeIndex)); // unbound: default pose
  return outTm;
}

void CollisionResource::clipCapsule(const TMatrix &instance_tm, const Capsule &c, Point3 &cp1, Point3 &cp2, real &md,
  const Point3 &movedirNormalized)
{
  TMatrix itm = inverse(instance_tm);

  ::Capsule nc = c;
  // probably we need to apply itm for capsule radius
  nc.transform(itm);

  real nmd = MAX_REAL;
  Point3 nlpt, nwpt;
  // probably we need to apply itm for movedirNormalized
  this->clipCapsule(nc, nlpt, nwpt, nmd, movedirNormalized);

  if (nmd < -0.0001)
  {
    nlpt = instance_tm * nlpt;
    nwpt = instance_tm * nwpt;

    nmd = -(nlpt - nwpt).length();
    if (nmd < md)
    {
      md = nmd;
      cp1 = nlpt;
      cp2 = nwpt;
    }
  }
}

void CollisionResource::clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized)
{
  clipCapsule(c, cp1, cp2, md, movedirNormalized, getDefaultInstance());
}

void CollisionResource::clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
  const CollisionResourceInstance &instance)
{
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "clipCapsule");
  const bool haveMoveDir = lengthSq(movedirNormalized) > 1e-6f;
  const vec3f vMoveDir = haveMoveDir ? v_ldu(&movedirNormalized.x) : v_zero();

  // Per-triangle capsule-clip kernel shared by the BLAS walk and the per-node fallback. Mirrors FRT
  // clipCapsule: recompute the face normal from resource-local verts, optionally skip back-faces vs
  // capsule travel, then accumulate the deepest penetration via clipCapsuleTriangle (keeps the min
  // over cp1/cp2/md). Tolerance > 0 keeps near-coplanar tris so a grazed wall still clips.
  auto clipTri = [&](vec3f v0, vec3f v1, vec3f v2, bool cull) {
    vec3f e1 = v_sub(v1, v0), e2 = v_sub(v2, v0);
    vec3f n = v_cross3(e1, e2);
    float nLen = v_extract_x(v_length3(n));
    if (nLen < 1e-9f)
      return;
    n = v_mul(n, v_rcp(v_splats(nLen)));
    if (cull && v_extract_x(v_dot3_x(n, vMoveDir)) > 1e-3f)
      return;
    alignas(16) Point3_vec4 p0, p1, p2, pn;
    v_st(&p0.x, v0);
    v_st(&p1.x, v1);
    v_st(&p2.x, v2);
    v_st(&pn.x, n);
    TriangleFace tf(p0, p1, p2, pn);
    clipCapsuleTriangle(c, cp1, cp2, md, tf);
  };

  const Grid &blasGrid = getBlasGrid(CollisionNode::PHYS_COLLIDABLE);
  // The combined grid is valid only for an enabled bind pose.
  const bool useBlas = !blasGrid.blasData.empty() && inst->isGridResidentPoseAtBind();
  if (useBlas)
  {
    // BLAS path: filter the BLAS by the capsule's resource-local bbox and clip each candidate.
    // The BLAS covers only the BLAS-eligible (IDENT) collidable mesh nodes.
    const vec3f vA = v_ldu(&c.a.x);
    const vec3f vB = v_ldu(&c.b.x);
    const vec3f vR = v_splats(c.r);
    const vec3f vCapMin = v_sub(v_min(vA, vB), vR);
    const vec3f vCapMax = v_add(v_max(vA, vB), vR);
    const vec3f vCapMinBox = v_madd(vCapMin, blasGrid.blasScale, blasGrid.blasOfs);
    const vec3f vCapMaxBox = v_madd(vCapMax, blasGrid.blasScale, blasGrid.blasOfs);

    const BlasLocalUnquant unquantVL = BlasLocalUnquant::make(blasGrid.blasBBox.bmin, blasGrid.blasScale);

    soa4::iterateFilteredVerts(
      blasGrid.blasData.data(), blasGrid.blasRootRef,
      [vCapMinBox, vCapMaxBox](vec3f bmin, vec3f bmax) {
        return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(vCapMaxBox, bmin), v_cmp_ge(bmax, vCapMinBox)));
      },
      [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int) -> bool {
        clipTri(v0, v1, v2, haveMoveDir);
        return false;
      },
      unquantVL);
  }

  // Per-node fallback for collidable nodes the BLAS branch misses: CONVEX nodes (never in the BLAS),
  // non-IDENT mesh nodes (kept out by isEligibleForBlas), appended nodes (in no grid), and -- when no
  // BLAS was built (small or SOLID resource) -- every collidable mesh node. Without this they silently
  // stop contributing to capsule clipping (e.g. convex collision in rendInstGenCollision.cpp). Skips
  // the nodes the BLAS walk above already clipped, keyed by the walked grid's membership flag (exact,
  // unlike the eligibility mirror). Triangles are read node-local (iterateNodeFacesVerts) and
  // transformed by the node's default T into resource-local (the capsule/BLAS frame).
  const uint8_t walkedGridFlag =
    useBlas ? (isCollidableGridForTrace(CollisionNode::PHYS_COLLIDABLE) ? CollisionNode::GRID_PHYS : CollisionNode::GRID_TRACEABLE)
            : 0;
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &allNodesList[mi];
    if (!node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE) || !node->hasGeometry())
      continue;
    if (node->flags & walkedGridFlag)
      continue; // already handled by the BLAS walk above
    if (!ITestIntersectionAlgo::poseCollidable(inst, node))
      continue; // structurally hidden or untraceable pose

    // SOLID nodes trace without back-face culling (never in the BLAS -- a SOLID node aborts buildBLAS),
    // so disable the movedir cull to match FRT/per-node semantics.
    const bool cull = haveMoveDir && !node->checkBehaviorFlags(CollisionNode::SOLID);
    const mat44f nodeTm = inst->getNodeTm(node->nodeIndex); // node-local -> resource-local
    iterateNodeFacesVerts(mi, [&](int, vec4f lv0, vec4f lv1, vec4f lv2) {
      clipTri(v_mat44_mul_vec3p(nodeTm, lv0), v_mat44_mul_vec3p(nodeTm, lv1), v_mat44_mul_vec3p(nodeTm, lv2), cull);
    });
  }
}

bool CollisionResource::test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node,
  const CollisionResourceInstance &instance, const Point3 &dir_norm, Point3 &out_norm, float &out_depth) const
{
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "test_sphere_node_intersection");
  if (DAGOR_UNLIKELY(!inst->isNodeTraceable(node->nodeIndex)))
    return false; // singular pose: no defined node frame to test in
  mat44f vitm;
  v_mat44_inverse43(vitm, inst->getNodeTm(node->nodeIndex));
  TMatrix itm;
  v_mat_43ca_from_mat44(itm.array, vitm);
  // Convert scaled probes conservatively through the inverse pose.
  // A bit-exact rigid frame tests exactly: padding it would report contacts across real gaps.
  if (is_exact_rigid_basis(itm))
    return testSphereNodeIntersectionLocal(itm, sphere, node, dir_norm, out_norm, out_depth);
  // The inverse spectral stretch is exact for uniform scale and conservative for shear, so the
  // class tolerance bands are covered without a fixed pad (a down-scale inside the band needs
  // MORE than any constant); the depth conversion below restores the world-radius gap.
  const float rScale = mat33_spectral_norm(vitm);
  const float rLocal = sphere.r * rScale;
  if (!testSphereNodeIntersectionLocal(itm, BSphere3(sphere.c, rLocal), node, dir_norm, out_norm, out_depth))
    return false;
  // Convert the center-to-plane gap alone (plane gaps scale by 1 / |M^-T n| under an affine
  // pose; |M n| matches that only without shear), then restore the world radius: the
  // conservatively widened local radius must not ride through the normal factor. The
  // traceability gate bounds the divisor; clamp at contact so the widened local test cannot
  // report a positive (separated) depth.
  mat33f i33, t33;
  v_mat33_from_mat44(i33, vitm);
  v_mat33_transpose(t33, i33);
  const float invN = v_extract_x(v_length3_x(v_mat33_mul_vec3(t33, v_ldu_p3_safe(&out_norm.x))));
  out_depth = min(0.f, (out_depth + rLocal) / invN - sphere.r);
  return true;
}

bool CollisionResource::test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth) const
{
  return test_sphere_node_intersection(sphere, node, getDefaultInstance(), dir_norm, out_norm, out_depth);
}

bool CollisionResource::testSphereNodeIntersectionLocal(const TMatrix &itm, const BSphere3 &sphere, const CollisionNode *node,
  const Point3 &dir_norm, Point3 &out_norm, float &out_depth) const
{
  BSphere3 localSphere(itm * sphere.c, sphere.r);
  if (!(node->modelBBox & localSphere))
    return false;
  if (!node->hasGeometry())
    return false; // degenerate-dropped: no geometry

  const Point3 localDirNorm = itm % dir_norm;
  const vec4f vR = v_splats(sphere.r);
  const vec4f vSphereC = v_ldu(&localSphere.c.x);

  // Descend the node's quad-BLAS with the sphere's AABB instead of materialising + scanning every
  // face: only overlapping leaves are decoded, and the first front-facing hit stops the walk. Leaf
  // verts arrive dequantised to node-local space (the same frame as localSphere), so the per-triangle
  // test matches the old brute-force loop exactly.
  return walkNodeTrisInLocalBox(*this, nodeBlasData.data(), *node, node->nodeBlasOfs, v_sub(vSphereC, vR), v_add(vSphereC, vR),
    [&](vec3f v0, vec3f v1, vec3f v2) -> bool {
      Point3_vec4 c0, c1, c2;
      v_st(&c0.x, v0);
      v_st(&c1.x, v1);
      v_st(&c2.x, v2);
      if (!test_triangle_sphere_intersection(c0, c1, c2, localSphere))
        return false;
      const Point3 cross = (c1 - c0) % (c2 - c0);
      if (localDirNorm * cross < -VERY_SMALL_NUMBER)
        return false; // wrong-facing triangle: keep walking
      const float crossLen = cross.length();
      out_norm = cross * safeinv(crossLen);
      out_depth = -out_norm * (c0 - localSphere.c) - sphere.r;
      return true; // any-hit: stop the walk
    });
}

bool CollisionResource::test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius,
  const CollisionNode *node) const
{
  return test_capsule_node_intersection(p0, p1, radius, node, getDefaultInstance());
}

bool CollisionResource::test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionNode *node,
  const CollisionResourceInstance &instance) const
{
  const CollisionResourceInstance *inst = resolveOwnedPoseForQuery(instance, "test_capsule_node_intersection");
  if (DAGOR_UNLIKELY(!inst->isNodeTraceable(node->nodeIndex)))
    return false; // singular pose: no defined node frame to test in
  mat44f vitm;
  v_mat44_inverse43(vitm, inst->getNodeTm(node->nodeIndex));
  TMatrix itm;
  v_mat_43ca_from_mat44(itm.array, vitm);
  Point3 localCylinderPoint0 = itm * p0;
  Point3 localCylinderPoint1 = itm * p1;

  // Convert scaled sweep radii conservatively into node-local units: the inverse spectral
  // stretch is exact for uniform scale and covers the class tolerance bands without a fixed
  // pad (a down-scale inside the band needs MORE than any constant).
  if (!is_exact_rigid_basis(itm))
    radius *= mat33_spectral_norm(vitm);

  const Point3 radiusVec(radius, radius, radius);
  BBox3 bbox;
  bbox += localCylinderPoint0 - radiusVec;
  bbox += localCylinderPoint0 + radiusVec;
  bbox += localCylinderPoint1 - radiusVec;
  bbox += localCylinderPoint1 + radiusVec;
  if (!(node->modelBBox & bbox))
    return false;

  BSphere3 localSphere0(localCylinderPoint0, radius);
  BSphere3 localSphere1(localCylinderPoint1, radius);

  if (!node->hasGeometry())
    return false; // degenerate-dropped: no geometry

  // Descend the node's quad-BLAS with the capsule's swept AABB instead of materialising + scanning
  // every face: only overlapping leaves are decoded and the first hit stops the walk. Leaf verts
  // arrive dequantised to node-local space, matching the local capsule.
  return walkNodeTrisInLocalBox(*this, nodeBlasData.data(), *node, node->nodeBlasOfs, v_ldu(&bbox[0].x), v_ldu(&bbox[1].x),
    [&](vec3f v0, vec3f v1, vec3f v2) -> bool {
      Point3_vec4 c0, c1, c2;
      v_st(&c0.x, v0);
      v_st(&c1.x, v1);
      v_st(&c2.x, v2);
      return test_triangle_sphere_intersection(c0, c1, c2, localSphere0) ||
             test_triangle_sphere_intersection(c0, c1, c2, localSphere1) ||
             test_triangle_cylinder_intersection(c0, c1, c2, localCylinderPoint0, localCylinderPoint1, radius);
    });
}

CollisionResource::Grid::Grid() = default;
void CollisionResource::Grid::reset()
{
  blasData.clear();
  blasData.shrink_to_fit();
  v_bbox3_init_empty(blasBBox);
  blasScale = v_zero();
  blasInvScale = v_zero(); // paired cache (1/blasScale): reset together or vert21 decode reads a stale inverse
  blasOfs = v_zero();
  blasTreeBytes = 0;
  blasRootRef = {};
  blasNodeRanges.clear();
  blasNodeRanges.shrink_to_fit();
  blasTwoSided = false;
}

int CollisionResource::getMemoryUsed() const
{
  auto blasMem = [](const Grid &g) -> int { return (int)(g.blasData.size() + g.blasNodeRanges.size() * sizeof(Grid::NodeRange)); };
  int mem = sizeof(*this);
  mem +=
    (int)(defaultInstance.nodeTm.size() * sizeof(TMatrix) + authoredNodeTm.size() * sizeof(TMatrix) +
          authoredNodeItm.size() * sizeof(TMatrix) + defaultInstance.poseMeta.size() * sizeof(CollisionResourceInstance::PoseMeta) +
          allNodesList.size() * sizeof(CollisionNode));
  mem += (int)nodeBlasData.size();
  mem += (int)names.size();
  mem += capsules.size() * sizeof(Capsule);
  mem += convexPlanes.size() * sizeof(plane3f);
  mem += blasMem(gridForTraceable) + blasMem(gridForCollidable);
  return mem;
}

vec4f CollisionResource::getWorldBoundingSphere(const mat44f &tm, const GeomNodeTree *geom_node_tree) const
{
  if (geom_node_tree && bsphereCenterNode)
    return geom_node_tree->getNodeWpos(bsphereCenterNode);
  else
    return v_mat44_mul_vec3p(tm, vBoundingSphere);
}

Point3 CollisionResource::getWorldBoundingSphere(const TMatrix &tm, const GeomNodeTree *geom_node_tree) const
{
  mat44f vTm;
  v_mat44_make_from_43cu_unsafe(vTm, tm.array);
  Point3 ret;
  v_stu_p3(&ret.x, getWorldBoundingSphere(vTm, geom_node_tree));
  return ret;
}

vec4f CollisionResource::getWorldBoundingSphere(const mat44f &tm, const CollisionResourceInstance &instance) const
{
  const CollisionResourceInstance *inst = resolveInstanceForTrace(instance);
  // A read-side query must not anchor the lazy refresh to its own tm (first caller would
  // mis-anchor the trace reject to a query tm); a stale tree-backed pose answers at the
  // bind center instead of refreshing.
  bool fresh = true;
  if (inst->getTree())
    fresh = interlocked_acquire_load(inst->poseGeneration) == inst->getTree()->getPoseGeneration() &&
            interlocked_acquire_load(inst->defaultPoseGenAtRefresh) == defaultPoseGen;
  // No selected center node: the BIND center by contract (matrix-posed instances included --
  // the das setBsphereCenterNode ordering relies on this fallback).
  if (!inst->hasBsphereCenterLocal || !fresh)
    return v_mat44_mul_vec3p(tm, vBoundingSphere);
  return v_mat44_mul_vec3p(tm, inst->bsphereCenterLocal);
}

Point3 CollisionResource::getWorldBoundingSphere(const TMatrix &tm, const CollisionResourceInstance &instance) const
{
  mat44f vTm;
  v_mat44_make_from_43cu_unsafe(vTm, tm.array);
  Point3 ret;
  v_stu_p3(&ret.x, getWorldBoundingSphere(vTm, instance));
  return ret;
}

bool CollisionResource::validateVerticesForJolt(const char *res_name, dag::ConstSpan<Point3_vec4> raw_verts,
  dag::ConstSpan<uint32_t> raw_indices)
{
  return validateVerticesForJolt(res_name, [](const uint32_t *, const CollisionNode *) {}, raw_verts, raw_indices);
}

dag::Vector<DegenerativeNodeData> CollisionResource::getDegenerativeNodes(const char *res_name)
{
  dag::Vector<DegenerativeNodeData> nodes;
  validateVerticesForJolt(res_name, [&nodes](const uint32_t *idx, const CollisionNode *node) {
    DegenerativeNodeData *result = eastl::find_if(nodes.begin(), nodes.end(),
      [&node](const DegenerativeNodeData &degenerative_node) { return degenerative_node.node == node; });
    if (result == nodes.end())
    {
      result = &nodes.emplace_back(DegenerativeNodeData(node));
    }
    result->indices.emplace_back(*idx);
    result->indices.emplace_back(*(idx + 1));
    result->indices.emplace_back(*(idx + 2));
  });
  return nodes;
}

bool CollisionResource::validateVerticesForJolt(const char *res_name, auto &&on_degenerate, dag::ConstSpan<Point3_vec4> raw_verts,
  dag::ConstSpan<uint32_t> raw_indices)
{
  bool passed = true;
  dag::Vector<Point3_vec4, framemem_allocator> vertsTmp;
  dag::Vector<uint32_t, framemem_allocator> idxTmp;
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &allNodesList[mi];
    if (!node->hasGeometry())
      continue;
    // Gather the node's verts exactly as the Jolt feeders do: the exporter passes its full-precision
    // raw_verts (its gate thus validates the exact floats that serialize); owning callers leave
    // raw_verts empty and the verts decode from vert21 (per-node chunk block, or the grid for
    // grid-resident nodes) -- the exact values any dacoll trimesh path hands Jolt at runtime.
    const Point3_vec4 *__restrict vertices;
    if (!raw_verts.empty())
      vertices = raw_verts.data() + node->verticesOfs;
    else
    {
      vertsTmp.resize((uint32_t)node->verticesCount);
      Point3_vec4 *dst = vertsTmp.data();
      iterateNodeVerts((int)node->nodeIndex, [&](int i, vec4f v) { v_st(&dst[i].x, v); });
      vertices = dst;
    }
    const uint32_t *__restrict idxBase;
    uint32_t idxCount = node->indicesCount;
    if (!raw_indices.empty()) // exporter: its external face-index workspace, node-local slice
      idxBase = raw_indices.data() + node->indicesOfs;
    else
    {
      // Runtime owning resource (no kept index list): recover the face list from the node's BLAS
      idxTmp.clear();
      idxTmp.reserve(node->indicesCount);
      iterateNodeFaces((int)node->nodeIndex, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
        idxTmp.push_back(i0);
        idxTmp.push_back(i1);
        idxTmp.push_back(i2);
      });
      idxBase = idxTmp.data();
      idxCount = (uint32_t)idxTmp.size();
    }
    const uint32_t *__restrict idxEnd = idxBase + idxCount;
    // Mirror JPH::MeshShapeSettings::Create() per node (the runtime feeds Jolt ONE MeshShape per mesh
    // node: collisionLib CST_MESH -> joltPhysics TYPE_TRIMESH). Create() rejects a triangle when EITHER
    //   1. IndexedTriangle::IsDegenerate -- cross(v1-v0, v2-v0) near zero on the raw float verts, or
    //   2. TriangleCodec::ValidationContext::IsDegenerate -- two of its verts land on the SAME 21-bit
    //      cell after round-to-nearest quantization against the bounds of the verts referenced by the
    //      triangle list (TriangleCodecIndexed8BitPackSOA4Flags.h).
    // Criterion 2 is vertex COINCIDENCE, not collinearity: Jolt accepts a triangle that quantizes to a
    // zero-area sliver as long as its three cells stay distinct. Testing the cross product of
    // DEQUANTIZED verts here (as this function used to) over-rejects huge near-collinear triangles that
    // every real Jolt path builds fine -- and did so on a grid (per-node modelBBox, truncation, cell
    // corners) that matches neither Jolt's rounding nor the runtime BLAS the exporter snaps to.
    bbox3f bb;
    v_bbox3_init_empty(bb);
    for (const uint32_t *__restrict cur = idxBase; cur < idxEnd; cur++)
      v_bbox3_add_pt(bb, v_ld(&vertices[*cur].x));
    // compress_scale = COMPONENT_MASK / max(size, 1e-20), quantize = trunc((v - bmin) * scale + 0.5) --
    // operation-for-operation Jolt's ValidationContext (Vec3::ToInt is the same cvttps truncation).
    const vec4f compressScale = v_div(v_splats((float)((1 << 21) - 1)), v_max(v_bbox3_size(bb), v_splats(1.0e-20f)));
    for (const uint32_t *__restrict cur = idxBase; cur < idxEnd;)
    {
      const uint32_t *triangleStartIdx = cur;
      vec3f v0 = v_ld(&vertices[*(cur++)].x);
      vec3f v1 = v_ld(&vertices[*(cur++)].x);
      vec3f v2 = v_ld(&vertices[*(cur++)].x);
      vec4f n = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
      const float inMaxDistSq = 1.0e-12f; // Jolt Vec3::IsNearZero default
      bool isDegenerate = v_extract_x(v_length3_sq_x(n)) <= inMaxDistSq;
      if (!isDegenerate)
      {
        vec4i q0 = v_cvti_vec4i(v_add(v_mul(v_sub(v0, bb.bmin), compressScale), V_C_HALF));
        vec4i q1 = v_cvti_vec4i(v_add(v_mul(v_sub(v1, bb.bmin), compressScale), V_C_HALF));
        vec4i q2 = v_cvti_vec4i(v_add(v_mul(v_sub(v2, bb.bmin), compressScale), V_C_HALF));
        // xyz lanes only: w holds Point3_vec4 padding
        isDegenerate = v_check_xyz_all_true(v_cast_vec4f(v_cmp_eqi(q0, q1))) ||
                       v_check_xyz_all_true(v_cast_vec4f(v_cmp_eqi(q1, q2))) || v_check_xyz_all_true(v_cast_vec4f(v_cmp_eqi(q0, q2)));
      }
      if (DAGOR_UNLIKELY(isDegenerate))
      {
        if (passed)
          logwarn("Degenerative triangles detected in collision res: %s", res_name);
        logwarn(" node '%s' " FMT_P3 FMT_P3 FMT_P3, getNodeNameStr(*node), V3D(v0), V3D(v1), V3D(v2));
        on_degenerate(triangleStartIdx, node);
        passed = false;
      }
    }
  }
  return passed;
}

CollisionResource::TraceMeshNodeLocalApi CollisionResource::traceMeshNodeLocalApi = {
  26 * 3, // check bounding for each 4 if triangles >= 26
  {&CollisionResource::traceRayMeshNodeLocalCullCCW<false>, &CollisionResource::rayHitMeshNodeLocalCullCCW<false>,
    &CollisionResource::traceRayMeshNodeLocalAllHits<false>},
  {&CollisionResource::traceRayMeshNodeLocalCullCCW<true>, &CollisionResource::rayHitMeshNodeLocalCullCCW<true>,
    &CollisionResource::traceRayMeshNodeLocalAllHits<true>},
};

void CollisionResource::check_avx_mesh_api_support()
{
  if (cpu_feature_fast_256bit_avx_checked && haveTraceMeshNodeLocalApi_AVX256)
    traceMeshNodeLocalApi = traceMeshNodeLocalApi_AVX256;
}

bool CollisionResource::getGridSize(uint8_t /*behavior_filter*/, IPoint3 & /*width*/, Point3 & /*leaf_size*/) const
{
  // FRT-era debug-viz of the leaf-grid extent. The SAH-built BLAS backend has no uniform leaf grid,
  // so this is permanently empty -- kept as a no-op so existing das bindings / debug overlays compile.
  return false;
}

int CollisionResource::getTrianglesCount(uint8_t behavior_filter) const
{
  unsigned count = 0;
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if (meshNode->checkBehaviorFlags(behavior_filter))
      count += meshNode->indicesCount;
  }
  return count / 3;
}

DAGOR_NOINLINE void CollisionResource::addTracesProfileTag(dag::Span<CollisionTrace> traces)
{
  unsigned activeTraces = 0;
  for (CollisionTrace &trace : traces)
  {
    if (trace.isHit || trace.isectBounding)
      activeTraces++;
  }
  DA_PROFILE_TAG(collres_traces, ": %u/%u", activeTraces, traces.size());
}

DAGOR_NOINLINE void CollisionResource::addMeshNodesProfileTag(const CollResProfileStats &profile_stats)
{
#if DA_PROFILER_ENABLED
  DA_PROFILE_TAG(mesh_nodes, ": %u/%u/%u; Tri: %u/%u", profile_stats.meshNodesBoxCheckPassed, profile_stats.meshNodesSphCheckPassed,
    profile_stats.meshNodesNum, profile_stats.meshTrianglesHits, profile_stats.meshTrianglesTraced);
#else
  G_UNUSED(profile_stats);
#endif
}
