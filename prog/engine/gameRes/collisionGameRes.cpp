// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <stdio.h>
#include <math/dag_geomTree.h>
#include <math/dag_traceRayTriangle.h>
#include <math/dag_plane3.h>
#include <generic/dag_sort.h>
#include <math/dag_mathUtils.h>
#include <math/dag_intrin.h>
#include <math/dag_capsuleTriangle.h>
#include <supp/dag_prefetch.h>
#include <math/dag_math3d.h>
#include <scene/dag_physMat.h>
#include <osApiWrappers/dag_cpuFeatures.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_finally.h>
#include <util/dag_hash.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <daBVH/dag_swBLAS_ray.h>
#include <EASTL/algorithm.h> // eastl::unique
#include <daBVH/dag_swBLAS_soa4.h>
#include <daBVH/swCommon.h>
#include <daBVH/dag_swBLAS_soa4Validate.h>
#include "collisionTraceOOL.h"
#include <EASTL/bitvector.h>
#include "collisionGameResInternal.h"

// Largest singular value, used where column lengths under-bound shear. The rounding below does
// NOT reach a strict upper bound: the closed form carries about 1e-4 where the two largest
// singular values nearly coincide, against the 2e-6 applied here. Only the call sites that add
// the shear-class 1.0002f stamp on top are actually covered.
float mat33_spectral_norm(mat44f_cref m_in)
{
  // v_mat44_spectral_norm43_x needs a finite basis: the band is that check, and a nonzero s
  // outside it is the degenerate scale the branch below reports as FLT_MAX.
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
  return v_extract_x(v_mat44_spectral_norm43_x(m_in)) * SPECTRAL_NORM_UPWARD;
}

// Shared ownership/freshness gate for updates and dispatch.
bool check_instance_owned_and_fresh(const CollisionResource *res, const CollisionResourceInstance &instance, const char *site)
{
  const bool ok = instance.getResource() == res && instance.nodeCount() == (int)res->getAllNodes().size();
  G_ASSERTF(ok, "%s: CollisionResourceInstance is foreign or stale (res %p vs %p, %d nodes vs %d)", site, instance.getResource(), res,
    instance.nodeCount(), (int)res->getAllNodes().size());
  if (DAGOR_UNLIKELY(!ok))
    LOGERR_ONCE("%s: CollisionResource %p used with a foreign/stale instance (res %p, %d nodes vs %d)", site, res,
      instance.getResource(), instance.nodeCount(), (int)res->getAllNodes().size());
  return ok;
}

// #define VERIFY_TRACE_RESULTS 1 // May affect performance

template <class pose_t>
bool CollisionResource::traceReadsInstanceInverse(const pose_t &instance, bool orthonormal_tm, bool capsule, bool single_ray) const
{
  return !boxNodes().empty() || !sphereNodes().empty() || !capsuleNodes().empty() || (!instance.isMetaAliased() && single_ray) ||
         meshNodes().empty() ||
         (pose_t::mayUseTlas && orthonormal_tm && !capsule && hasAllNodesTLAS() &&
           (instance.getTree() != nullptr || instance.hasTlas()));
}

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


// Decoded per-node BLAS chunk frame (chunk = [NodeBlasChunkHeader][quad tree][pad][vert21 stream]):
// the tree span plus the two frames every chunk descent needs -- the q-space transform (scale/qOfs)
// for tree traversal, and the decode frame (bmin/invScale) for vert/normal recovery, both from the
// chunk header. Reads the chunk layout in one place.
struct NodeChunkFrame
{
  const uint8_t *tree;
  soa4::RootRef rootRef;
  int treeBytes;          // dev-only token validation (getNodeFaceVertsByRef); the walks are root-driven
  vec3f scale;            // exact pack scale: node-local -> chunk q-space (ray/box transform)
  vec3f qOfs;             // -bmin * scale
  Vert21MetricLoader unq; // q-space -> node-local, the trace dispatch frame of vLocalFrom/vLocalDir (vert/normal decode)
};
static NodeChunkFrame decode_node_chunk_frame(const uint8_t *chunk)
{
  const CollisionResource::NodeBlasChunkHeader *hdr = (const CollisionResource::NodeBlasChunkHeader *)chunk;
  NodeChunkFrame f;
  f.tree = chunk + sizeof(CollisionResource::NodeBlasChunkHeader);
  f.rootRef = hdr->rootRef;
  f.treeBytes = (int)hdr->treeBytes;
  f.scale = hdr->scaleV();
  f.unq.invScale = hdr->invScaleV();
  f.unq.bmin = hdr->bminV();
  f.qOfs = v_neg(v_mul(f.unq.bmin, f.scale));
  return f;
}
// The chunk's SoA4 tree base alone: all a per-leaf accept needs, without building the rest of the frame.
// Same chunk pointer as decode_node_chunk_frame, which owns this layout.
static inline const uint8_t *node_chunk_tree(const uint8_t *chunk) { return chunk + sizeof(CollisionResource::NodeBlasChunkHeader); }
// A node-local query box lifted to chunk q-space, padded by one cell: a box folded from DECODED
// verts carries up to half a source cell of quantization error, and an exact-touch contact must
// not fall out of the prune (the callers run exact tests behind it). Every chunk box walk shares
// this one home of the pad rule.
static inline bbox3f padded_q_box(const NodeChunkFrame &fr, vec3f local_bmin, vec3f local_bmax)
{
  // Subtract before scaling: the madd form (local * scale + qOfs) cancels two big products, and
  // at a far-from-origin node that rounding is whole cells, past the pad (non-FMA builds).
  bbox3f qb;
  qb.bmin = v_sub(v_mul(v_sub(local_bmin, fr.unq.bmin), fr.scale), V_C_ONE);
  qb.bmax = v_add(v_mul(v_sub(local_bmax, fr.unq.bmin), fr.scale), V_C_ONE);
  return qb;
}
// The inclusive overlap test every chunk box walk pairs with padded_q_box: one named home,
// reading the library primitive.
static __forceinline bool q_box_overlaps(bbox3f_cref qb, vec3f bmn, vec3f bmx)
{
  bbox3f b;
  b.bmin = bmn;
  b.bmax = bmx;
  return v_bbox3_test_box_intersect(qb, b);
}

// tri_ref leaf tokens are minted by our own traversals over loader-built trees -- correct by
// construction, so decoding one back needs no checks; the gate itself lives in
// collResTriRefValidate.h, shared with the unit test's crafted-token battery.
#include "collResTriRefValidate.h"


// ===== the trace-time material filter =====
// Whole-node early-out; PHYSMAT_INVALID means no filter.
// The walk applies it to every node, so callers pass ray_mat_id and nothing else. Exact only while a node holds ONE material.
static __forceinline bool node_passes_ray_mat(const CollisionResource &res, const CollisionNode *node, int ray_mat_id)
{
  // No-material geometry cannot be filtered by material: PHYSMAT_INVALID as isMaterialsCollide's COLUMN would read a neighbor row's
  // cell (index -1), so it is defined as always passing here.
  return ray_mat_id == PHYSMAT_INVALID || res.anyNodeMaterialPasses(*node, [ray_mat_id](int mat_id) {
    return mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, mat_id);
  });
}
// A set-holding node passes the early-out when ANY material does, yet still holds faces the ray
// must skip: its leaves are asked one at a time by the chunk accept callback.
static __forceinline bool node_needs_leaf_mat_gate(const CollisionResource &res, const CollisionNode &node, int ray_mat_id)
{
  return ray_mat_id != PHYSMAT_INVALID && !res.isNodeOwnMaterialAuthority(node);
}
// The leaf's 6 user bits index the material set of the node that owns the leaf.
static __forceinline int chunk_leaf_mat_id(const CollisionResource &res, const CollisionNode &node, const uint8_t *tree,
  soa4::LeafRef ref)
{
  return res.getNodePhysMatId(node.nodeIndex, (int)soa4::leafUserBits(tree, soa4::decodeLeafRef(tree, ref)));
}
static __forceinline bool leaf_passes_ray_mat(const CollisionResource &res, const CollisionNode &node, const uint8_t *tree,
  soa4::LeafRef ref, int ray_mat_id)
{
  // A no-material entry always passes, as in node_passes_ray_mat.
  const int m = chunk_leaf_mat_id(res, node, tree, ref);
  return m == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, m);
}

// Leaf accept for a per-node chunk walk, in the shape the chunk entry points take.
// The ctx is built per node by the trace loop, only for a node the whole-node early-out could not settle.
struct ChunkLeafMatCtx
{
  const CollisionResource *res = nullptr;
  const CollisionNode *node = nullptr;
  const uint8_t *tree = nullptr; // the node's chunk SoA4 tree, i.e. NodeChunkFrame::tree
  int rayMatId = PHYSMAT_INVALID;
};
static bool chunk_leaf_mat_accept(void *ctx, soa4::LeafRef ref)
{
  const ChunkLeafMatCtx *c = (const ChunkLeafMatCtx *)ctx;
  return leaf_passes_ray_mat(*c->res, *c->node, c->tree, ref, c->rayMatId);
}

// Visit each triangle of one mesh/convex node whose node-local AABB overlaps [local_box_min,
// local_box_max], with verts dequantised to node-local space, until leaf_test(v0,v1,v2) returns true
// (any-hit). Spatial-culled through the node's per-node quad-BLAS chunk. Returns true iff a
// leaf_test hit. chunk_or_null = CollisionResource::nodeChunkPtr(node). Shared by the sphere /
// capsule overlap tests so each pays BLAS-log cost, not O(verts+faces).
template <class LeafTest>
static bool walkNodeTrisInLocalBox(const uint8_t *chunk_or_null, vec4f local_box_min, vec4f local_box_max, const LeafTest &leaf_test)
{
  // chunk_or_null = CollisionResource::nodeChunkPtr(node): the resource resolves its own storage.
  if (chunk_or_null)
  {
    const NodeChunkFrame fr = decode_node_chunk_frame(chunk_or_null);
    const bbox3f qb = padded_q_box(fr, local_box_min, local_box_max);
    return soa4::iterateFilteredVerts(
      fr.tree, fr.rootRef, [qb](vec3f bmn, vec3f bmx) { return q_box_overlaps(qb, bmn, bmx); },
      [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef, int) -> bool { return leaf_test(v0, v1, v2); }, fr.unq);
  }
  return false;
}

uint64_t CollisionResource::Data::blockBytesFor(const uint32_t counts[ARRAY_COUNT], uint64_t out_ofs[ARRAY_COUNT])
{
  // Each array starts at its element's alignment. An EMPTY array takes no pad: nothing addresses
  // it, so a resource pays alignment only for the arrays it actually has (an empty one is the header).
  uint64_t total = sizeof(Data);
  for (int i = 0; i < ARRAY_COUNT; ++i)
  {
    if (counts[i])
      total = (total + ELEM_ALIGN[i] - 1) & ~uint64_t(ELEM_ALIGN[i] - 1);
    if (out_ofs)
      out_ofs[i] = total;
    total += (uint64_t)counts[i] * ELEM_SIZE[i];
  }
  return total;
}

CollisionResource::Data *CollisionResource::Data::build(const uint32_t counts[ARRAY_COUNT])
{
  G_STATIC_ASSERT(blockAlignServesEveryArray());
  const uint32_t empty[ARRAY_COUNT] = {};
  if (!counts)
    counts = empty;
  uint64_t ofs[ARRAY_COUNT];
  const uint64_t total = blockBytesFor(counts, ofs);
  if (DAGOR_UNLIKELY(total > 0xFFFFFFFFu))
    DAG_FATAL("collision Data block of %llu bytes exceeds its 32-bit offsets", (unsigned long long)total);
  void *mem = midmem->allocAligned((size_t)total, BLOCK_ALIGN);
  Data *d = new (mem, _NEW_INPLACE) Data();
  for (int i = 0; i < ARRAY_COUNT; ++i)
  {
    d->arrays[i].byteOfs = (uint32_t)ofs[i];
    d->arrays[i].elemCount = counts[i];
    char *dst = (char *)d + ofs[i];
    if (i == NODES)
      for (uint32_t k = 0; k < counts[i]; ++k)
        new (dst + (size_t)k * sizeof(CollisionNode), _NEW_INPLACE) CollisionNode();
    else if (i == AUTHORED_TM || i == AUTHORED_ITM || i == REL_GEOM_TMS)
      for (uint32_t k = 0; k < counts[i]; ++k)
        new (dst + (size_t)k * sizeof(TMatrix), _NEW_INPLACE) TMatrix(TMatrix::IDENT);
    else
      memset(dst, 0, (size_t)counts[i] * ELEM_SIZE[i]);
  }
  return d;
}

CollisionResource::Data *CollisionResource::Data::clone(const Data &src)
{
  // The block is relocatable (self-relative offsets): one byte copy, then the sharer count starts over.
  const uint32_t bytes = src.blockBytes();
  Data *d = (Data *)midmem->allocAligned(bytes, BLOCK_ALIGN);
  memcpy((void *)d, &src, bytes);
  d->refCnt = 0;
  return d;
}

void CollisionResource::Data::destroy()
{
#if DAGOR_DBGLEVEL > 0
  const uint32_t bytes = blockBytes();
#endif
  this->~Data();
#if DAGOR_DBGLEVEL > 0
  memset((void *)this, 0xDD, bytes); // a block-interior pointer kept across a reshape reads poison, not the old bytes
#endif
  midmem->freeAligned(this);
}

const CollisionNode *CollisionResource::getNode(uint32_t index) const
{
  return index < data->allNodesList().size() ? data->allNodesList().data() + index : nullptr;
}

CollisionNode *CollisionResource::getNode(uint32_t index)
{
  return index < data->allNodesList().size() ? data->allNodesList().data() + index : nullptr;
}

int CollisionResource::getNodeIndexByName(const char *name) const
{
  if (name && *name && !data->names().empty())
    for (int i = 0; i < data->allNodesList().size(); i++)
      if (strcmp(data->names().data() + data->allNodesList()[i].nameOfs, name) == 0)
        return i;
  return -1;
}

CollisionNode *CollisionResource::getNodeByName(const char *name) { return getNode(getNodeIndexByName(name)); }

const CollisionNode *CollisionResource::getNodeByName(const char *name) const { return getNode(getNodeIndexByName(name)); }

template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, bool pose_may_refresh,
  typename pose_t, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f tm, const pose_t &instance, vec3f from, vec3f dir, float len,
  bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
  TraceCollisionResourceStats *out_stats, bool force_no_cull, int ray_mat_id, TraceTmCache *tm_cache, bool force_cull) const
{
  CollisionTrace in{
    .vFrom = from,
    .vDir = dir,
    .vTo = v_madd(dir, v_splats(len), from),
    .t = len,
  };

  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<trace_mode, trace_type, true /*is_single_ray*/, pose_may_refresh>(tm, instance, traces, calc_normal,
    bsphere_scale, behavior_filter, filter, callback, out_stats, force_no_cull, ray_mat_id, tm_cache, force_cull);
}


// Heavy all-in-one node iterator, calculations unused by caller and deadcode will be automatically stripped by compiler
template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, bool is_single_ray,
  bool pose_may_refresh, typename pose_t, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f original_tm, const pose_t &instance,
  dag::Span<CollisionTrace> traces, bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter,
  const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull, int ray_mat_id, TraceTmCache *tm_cache,
  bool force_cull) const
{
  TIME_PROFILE_DEV(collres_trace);
#if DAGOR_DBGLEVEL > 0
  instance.noteDispatchByCand(false); // ANY early return (bounds reject included) must not leave a previous call's value
#endif
  // A tree-backed temp view never refreshes (bind meta, no rootBBox); tree-less legacy calls
  // read the default instance, and owned-matrix forms are mutator-maintained except for the
  // live flags, which follow the default.
  if constexpr (pose_may_refresh)
  {
    if (DAGOR_UNLIKELY(instance.getTree() != nullptr))
      instance.refreshIfStale(original_tm, pose_t::mayUseTlas && trace_type != CollisionTraceType::TRACE_CAPSULE &&
                                             trace_type != CollisionTraceType::CAPSULE_HIT && hasAllNodesTLAS());
    else
      instance.mirrorLiveFlagsIfStale();
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

  // A caller's entry (see TraceTmCache) holding this basis reads its class, its reject scale and,
  // below, its inverse; the columns' w lanes never enter those, so xyz decides.
  TraceTmCache *tmc = tm_cache;
  const bool tmCached =
    tmc && tmc->eps == traceTmEps &&
    v_check_xyz_all_true(v_and(v_and(v_cmp_eq(tm.col0, tmc->col0), v_cmp_eq(tm.col1, tmc->col1)), v_cmp_eq(tm.col2, tmc->col2)));
  vec4f maxScaleSq;
  bool bIsOrthonormalizedTm = tmCached && tmc->ortho; // a miss decides after the bounds pass
  vec4f xyzScaleSq = v_zero(), offDiag = v_zero();    // that decision's inputs, from the miss branch
  if (tmCached)
    maxScaleSq = v_splats(tmc->maxScaleSq);
  else
  {
    // The Gershgorin bound on the squared largest singular value holds for every class, so the
    // sphere reject below runs on it before the tm is classified: a rejected ray never classifies.
    // Rounded to nearest the row sum can land a few ulp under sigma_max^2, which the spectral
    // path's upward factor covers.
    v_mat33_gram(xyzScaleSq, offDiag, tm.col0, tm.col1, tm.col2);
    maxScaleSq = v_mul(v_mat33_gram_bound_x(xyzScaleSq, offDiag), v_splats(SPECTRAL_NORM_UPWARD * SPECTRAL_NORM_UPWARD));
  }

  // Check bounding
  bool anyTraceIntersectsBounding = false;
  {
    vec3f vBsphCenter;
    vec4f vBsphR2;
    if (DAGOR_LIKELY(!instance.isMetaAliased()))
    {
      if (DAGOR_LIKELY(!instance.isPosedSinceBind()))
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
    if (!tmCached)
    {
      // Classify the survivors' tm: unit column lengths and pairwise dots within eps (a unit-column
      // SHEAR passes a length-only band). The class picks the dispatch below; the non-orthonormal
      // class also tightens the per-node reject scale from the Gershgorin bound to the spectral
      // norm (exact for uniform scale, conservative for shear).
      bIsOrthonormalizedTm = collres_gram_in_band(xyzScaleSq, offDiag, traceTmEps);
      if (DAGOR_UNLIKELY(!bIsOrthonormalizedTm))
        maxScaleSq = v_max(v_hmax3(xyzScaleSq), v_splats(sqr(mat33_spectral_norm(tm))));
      if (tmc) // the tightened scale: a repeat rejects on the bound the per-node pass uses
      {
        tmc->col0 = tm.col0, tmc->col1 = tm.col1, tmc->col2 = tm.col2;
        tmc->eps = traceTmEps;
        tmc->maxScaleSq = v_extract_x(maxScaleSq);
        tmc->ortho = bIsOrthonormalizedTm;
        tmc->itmValid = false;
      }
    }
    // The inverse only for a trace that reads it, so a run over tree-backed mesh-only
    // resources pays no inverse it never paid; the first reader of the basis fills it.
    const bool readsItm =
      tmc != nullptr &&
      traceReadsInstanceInverse(instance, bIsOrthonormalizedTm,
        trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT, is_single_ray);
    if (readsItm && !tmc->itmValid)
    {
      v_mat44_inverse43(tmc->itm, tm);
      tmc->itmValid = true;
    }
    const mat44f *cachedItm = readsItm ? &tmc->itm : nullptr;
#if VERIFY_TRACE_RESULTS
    dag::Vector<CollisionTrace> initialTraces(traces.begin(), traces.end());
#endif

    // The hit callbacks are small, yet the compiler leaves them out of line (two hops per hit, each
    // vec4f argument through a hidden pointer on the Windows ABI); the attribute inlines them.
    auto cb_wrapper = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos,
                        tri_ref_t tri_ref) DAGOR_FORCE_INLINE_LAMBDA {
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
      res = forEachIntersectedNodePrepared<true, trace_mode, trace_type, is_single_ray>(tm, v_extract_x(maxScaleSq), woffset, instance,
        traces, calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull, ray_mat_id, cachedItm, force_cull);
    else
      res =
        forEachIntersectedNodePrepared<false, trace_mode, trace_type, is_single_ray>(tm, v_extract_x(maxScaleSq), woffset, instance,
          traces, calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull, ray_mat_id, cachedItm, force_cull);
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
  const float g2 = v_extract_x(v_mat44_max_scale43_sq(m));
  const float g = sqrtf(g2);
  return fabsf(out_ndet) > 1e-6f * g * g * g;
}

static inline vec4f sigma_max_bound_sq4(const mat44f *const m[4])
{
  vec4f x[3], y[3], z[3];
  for (int j = 0; j < 3; ++j)
  {
    vec4f a0 = (&m[0]->col0)[j], a1 = (&m[1]->col0)[j], a2 = (&m[2]->col0)[j], a3 = (&m[3]->col0)[j];
    v_mat44_transpose(a0, a1, a2, a3);
    x[j] = a0;
    y[j] = a1;
    z[j] = a2;
  }
  auto dot = [&](int i, int j) { return v_madd(y[i], y[j], v_madd(z[i], z[j], v_mul(x[i], x[j]))); };
  const vec4f a01 = v_abs(dot(0, 1)), a12 = v_abs(dot(1, 2)), a20 = v_abs(dot(2, 0));
  const vec4f row0 = v_add(dot(0, 0), v_add(a20, a01)), row1 = v_add(dot(1, 1), v_add(a01, a12)),
              row2 = v_add(dot(2, 2), v_add(a12, a20));
  return v_mul(v_max(row0, v_max(row1, row2)), v_splats(SPECTRAL_NORM_UPWARD * SPECTRAL_NORM_UPWARD));
}

uint64_t CollisionResource::preCullTreeMeshSpheres(const CollisionTrace &trace, const CollisionResourceInstance &meta_src,
  const GeomNodeTree &tree, vec3f woffset, uint32_t first, bool authored_meta, bool live_bound) const
{
  mat44f ident;
  v_mat44_ident(ident);
  const dag::ConstSpan<uint16_t> list = meshNodes();
  const uint32_t end = min<uint32_t>(first + 64, list.size());
  const CollisionNode *nodes = data->allNodesList().data();
  const CollisionResourceInstance::PoseMeta *meta = meta_src.metaData();
  const uint32_t treeCount = tree.nodeCount();
  const vec3f ofs = v_sub(tree.getWtmOfs(), woffset);
  const vec4f fx = v_splat_x(trace.vFrom), fy = v_splat_y(trace.vFrom), fz = v_splat_z(trace.vFrom);
  const vec4f dx = v_splat_x(trace.vDir), dy = v_splat_y(trace.vDir), dz = v_splat_z(trace.vDir);
  const vec4f len = v_splats(trace.t);
  const float bandPad = authored_meta ? COLLRES_RIGID_SIGMA_MAX : 1.f;
  // The scalar test rounds differently under fast math: a pad of 1e-5 of the coordinate magnitudes keeps this a superset.
  const vec4f tolK = v_splats(1e-5f), tolFrom = v_mul(v_splat_x(v_hmax3(v_abs(trace.vFrom))), tolK);
  uint64_t bits = 0;
  for (uint32_t g = first; g < end; g += 4)
  {
    vec4f c[4];
    alignas(16) float r2[4], rb2[4];
    const mat44f *wtm[4];
    uint32_t liveLanes = 0;
    for (uint32_t l = 0; l < 4; ++l)
    {
      const uint16_t mi = list[min(g + l, end - 1)];
      const CollisionNode &node = nodes[mi];
      rb2[l] = get_bsphere_r2(node.radiusAroundBoxCenter);
      r2[l] = rb2[l] * sqr(meta[mi].maxTmScale * bandPad);
      // an unbound node reads the default pose: centered on the ray start, it always reaches the scalar test
      c[l] = trace.vFrom;
      wtm[l] = &ident;
      if (node.geomNodeId.index() < treeCount)
      {
        wtm[l] = &tree.getNodeWtmRel(node.geomNodeId);
        mat44f posed = *wtm[l]; // the offset joins col3 first, as in getPosedNodeWtmInline
        posed.col3 = v_add(posed.col3, ofs);
        c[l] = v_mat44_mul_vec3p(posed, node.vBsphereCenter());
        liveLanes |= (live_bound && isLiveBoundNode(mi)) << l;
      }
    }
    vec4f rr2 = v_ld(r2);
    if (liveLanes)
      rr2 = v_sel(rr2, v_mul(v_ld(rb2), sigma_max_bound_sq4(wtm)),
        v_cast_vec4f(v_make_vec4i(liveLanes & 1 ? -1 : 0, liveLanes & 2 ? -1 : 0, liveLanes & 4 ? -1 : 0, liveLanes & 8 ? -1 : 0)));
    v_mat44_transpose(c[0], c[1], c[2], c[3]);
    const vec4f ax = v_sub(c[0], fx), ay = v_sub(c[1], fy), az = v_sub(c[2], fz);
    const vec4f t = v_min(v_max(v_madd(az, dz, v_madd(ay, dy, v_mul(ax, dx))), v_zero()), len);
    const vec4f ex = v_sub(v_mul(dx, t), ax), ey = v_sub(v_mul(dy, t), ay), ez = v_sub(v_mul(dz, t), az);
    const vec4f d2 = v_madd(ez, ez, v_madd(ey, ey, v_mul(ex, ex)));
    const vec4f cMag = v_max(v_max(v_abs(c[0]), v_abs(c[1])), v_abs(c[2]));
    const vec4f apLen = v_sqrt(v_madd(az, az, v_madd(ay, ay, v_mul(ax, ax))));
    const vec4f r = v_add(v_madd(v_add(apLen, cMag), tolK, tolFrom), v_sqrt(v_max(rr2, v_zero())));
    bits |= (uint64_t)v_truemask(v_cmp_le(d2, v_mul(r, r))) << (g - first);
  }
  return bits;
}

// Conservative caller-tm stretch for the pair-test radius culls: column lengths are exact for
// the near-orthonormal class (the common case), so only scale or shear outside the band pays
// the spectral bound.
static float conservative_outer_scale(mat44f_cref tm)
{
  vec3f xyzScaleSq, offDiag;
  v_mat33_gram(xyzScaleSq, offDiag, tm.col0, tm.col1, tm.col2);
  if (DAGOR_LIKELY(collres_gram_in_band(xyzScaleSq, offDiag, 0.008f)))
    // The accepted off-diagonals are NOT free: the bare column max would under-bound an in-band
    // shear by up to sqrt(1 + 2 * 0.008), and these values feed ungated pair culls.
    return v_extract_x(v_sqrt_x(v_mat33_gram_bound_x(xyzScaleSq, offDiag)));
  return mat33_spectral_norm(tm);
}

// The nodes one call can reach, off the SoA4 walk: a conservative superset of what the passes
// can hit (leaf boxes enclose the posed geometry). The batch path sorts back to ascending
// nodeIndex and stays bit-identical to the linear walk; the single-ray path keeps walk order.
using TlasCandList = CollResTlasCandidates;

static __forceinline void tlas_seg_ray(RayData &r, const uint8_t *tree, vec4f q_from, vec4f q_to)
{
  r.data = tree;
  r.rayOrigin = q_from;
  r.rayDir = v_sub(q_to, q_from);
  // Just past the segment end: the ordered walk's deferred cull is strict, so a leaf entered
  // at exactly 1.0 would be dropped while the boundary-inclusive box test reports the hit.
  r.t = 1.f + 1e-6f;
  r.bestTriOffset = 0;
  r.calc();
}

// Single ray: leaves arrive roughly near-to-far (depth-first per subtree), so the consuming
// pass usually shrinks its t early. dense_at terminates a walk whose list is already doomed
// to the density fallback.
static void collect_tlas_candidates_ordered(const uint8_t *tree, uint32_t root_ref, vec4f q_from, vec4f q_to, TlasCandList &out,
  uint32_t dense_at)
{
  RayData r;
  tlas_seg_ray(r, tree, q_from, q_to);
  struct Collect
  {
    TlasCandList &out;
    uint32_t denseAt;
    __forceinline bool operator()(RayData &, const uint8_t *data, uint32_t ptr, uint64_t &, vec4f &) const
    {
      if (ptr & soa4::LEAF_ENTRY_FLAG) // a TLAS has no degenerate root-leaf block; the emitter rejects one
        out.push_back((uint16_t)(soa4::decodeLeafRef(data, ptr).w0 & ~0x80000000u));
      return (uint32_t)out.size() >= denseAt;
    }
  };
  const Collect collect{out, dense_at};
  soa4::RootRef root;
  root.v = (int32_t)root_ref;
  soa4::traverseOrdered(r, root, collect);
}

// Ray batch: unordered, the caller sorts and uniques. dense_at bounds THIS ray's tail; one
// walk emits each leaf at most once, so a tail that dense proves the density fallback.
static void collect_tlas_candidates_any(const uint8_t *tree, uint32_t root_ref, vec4f q_from, vec4f q_to, TlasCandList &out,
  uint32_t dense_at)
{
  RayData r;
  tlas_seg_ray(r, tree, q_from, q_to);
  const uint32_t stopAt = (uint32_t)out.size() + dense_at;
  struct Collect
  {
    TlasCandList &out;
    uint32_t stopAt;
    __forceinline bool operator()(RayData &, const uint8_t *, uint32_t, const soa4::NodeSoA *nd, int lane) const
    {
      if (nd)
        out.push_back((uint16_t)(nd->w()[lane] & ~0x80000000u));
      return (uint32_t)out.size() >= stopAt;
    }
  };
  const Collect collect{out, stopAt};
  soa4::RootRef root;
  root.v = (int32_t)root_ref;
  soa4::traverseAny(r, root, collect);
}

// Box twin of the segment collectors: child boxes against a quantized query box, leaves collect
// their node ids (unordered). The serve/decline decision is the caller's clone gate.
static void collect_tlas_candidates_box(const uint8_t *tree, uint32_t root_ref, vec4f q_min, vec4f q_max, TlasCandList &out)
{
  const vec4f qnx = v_splat_x(q_min), qny = v_splat_y(q_min), qnz = v_splat_z(q_min);
  const vec4f qxx = v_splat_x(q_max), qxy = v_splat_y(q_max), qxz = v_splat_z(q_max);
  int stack[soa4::MAX_TREE_DEPTH * 4];
  int sp = 0;
  uint32_t cur = root_ref;
  for (;;)
  {
    const soa4::NodeSoA nd(tree, cur);
    const uint32_t *w = nd.w();
    const vec4f ox = v_and(v_cmp_ge(qxx, nd.mnx), v_cmp_ge(nd.mxx, qnx));
    const vec4f oy = v_and(v_cmp_ge(qxy, nd.mny), v_cmp_ge(nd.mxy, qny));
    const vec4f oz = v_and(v_cmp_ge(qxz, nd.mnz), v_cmp_ge(nd.mxz, qnz));
    unsigned m = (unsigned)v_truemask(v_and(ox, v_and(oy, oz))) & ((1u << nd.N) - 1);
    const unsigned leafAll = nd.leafMask();
    unsigned leafHit = m & leafAll;
    while (leafHit)
    {
      const int i = (int)__bsf_unsafe(leafHit);
      leafHit &= leafHit - 1;
      out.push_back((uint16_t)(w[i] & ~0x80000000u));
    }
    unsigned mi = m & ~leafAll;
    if (mi)
    {
      const unsigned first = __bsf_unsafe(mi);
      G_ASSERT(sp + 3 <= (int)(sizeof(stack) / sizeof(stack[0])));
      for (mi &= mi - 1; mi; mi &= mi - 1)
        stack[sp++] = (int)w[__bsf_unsafe(mi)];
      cur = w[first];
      continue;
    }
    if (!sp)
      break;
    cur = (uint32_t)stack[--sp];
  }
}

// Read-only over the clone bytes, so it carries the clone readers' duty: call under the same
// serialization as traces (pose writes rewrite the clone in place).
bool CollisionResource::tlasBoxCandidates(const CollisionResourceInstance &inst, bbox3f_cref box, CollResTlasCandidates &out) const
{
  // Owned form only: a tree-backed clone is gated by the dispatch's generation stamps this
  // read-only helper does not check, so it declines to the caller's linear walk.
  // tlasCloneCurrent is the rest of the gate: a reset clears the clone and bumps the generation
  // together. `out` comes back holding exactly this query's candidates.
  out.clear();
#if DAGOR_DBGLEVEL > 0
  // The dispatch witness (lastDispatchByCand) stamps here for every consumer: false first, so a
  // skipped or stale arm reads as the linear walk, true only when candidates serve.
  // Dev-only, like the trace dispatch's stamps.
  inst.noteDispatchByCand(false);
#endif
  if (inst.getTree() != nullptr || !inst.tlasCloneCurrent(data->tlasGeneration))
    return false;
  // One quant bin of pad eats the query's own quantization rounding; the leaf boxes carry their
  // build-time pad already.
  const vec4f qMin = v_sub(inst.tlasQuant(box.bmin), V_C_ONE);
  const vec4f qMax = v_add(inst.tlasQuant(box.bmax), V_C_ONE);
  collect_tlas_candidates_box(inst.tlasTree(), data->tlasRootRef, qMin, qMax, out);
  // Sorted ascending, like the batch ray dispatch: candidate order then matches the linear walk.
  stlsort::sort(out.begin(), out.end());
#if DAGOR_DBGLEVEL > 0
  inst.noteDispatchByCand(true);
#endif
  return true;
}

bool CollisionResource::visitTrianglesInBox(bbox3f_cref box, uint16_t behavior_filter, BoxTriVisitor visitor) const
{
  // Garbage input (NaN or infinite bounds) answers false up front; it must not reach the
  // non-IDENT logerr below.
  if (!v_test_xyz_finite(v_add(box.bmin, box.bmax)))
    return false;
  const auto visitNode = [&](uint16_t mi) -> bool {
    const CollisionNode &node = data->allNodesList()[mi];
    // Live flags, like every query gate (getTrianglesCount, the trace accept).
    if (!node.hasGeometry() || !checkNodeBehaviorFlags(node.nodeIndex, behavior_filter))
      return false;
    // The LIVE-identity gate runs BEFORE the bbox cull: modelBBox is node-local, so a posed node
    // would cull in the wrong frame - and its silent skip would hide the contract logerr.
    if (!isIdentNode(node.nodeIndex))
    {
      LOGERR_ONCE("visitTrianglesInBox: node <%s>#%u is not IDENT; skipped", getNodeNameStr(node), (unsigned)node.nodeIndex);
      return false;
    }
    // modelBBox holds the SOURCE verts; a decoded chunk vert can overhang it by the vert21
    // round-trip. One q-cell of pad keeps the node cull as conservative as the leaf prune.
    bbox3f nodeBox = v_ldu_bbox3(node.modelBBox);
    v_bbox3_extend(nodeBox, v_mul(v_bbox3_size(nodeBox), v_splats(1.f / 65535.f)));
    if (!v_bbox3_test_box_intersect(nodeBox, box))
      return false;
    const uint8_t *chunk = nodeChunkPtr(node);
    if (!chunk)
      return false;
    const NodeChunkFrame fr = decode_node_chunk_frame(chunk);
    // Each visited triangle then passes an exact test of its own decoded bbox against the box.
    const bbox3f qb = padded_q_box(fr, box.bmin, box.bmax);
    const bool ownMat = isNodeOwnMaterialAuthority(node);
    const int ni = node.nodeIndex;
    const int inlineMat = ownMat ? getNodePhysMatId(ni, 0) : PHYSMAT_INVALID;
    return soa4::iterateFilteredVerts(
      fr.tree, fr.rootRef, [qb](vec3f bmn, vec3f bmx) { return q_box_overlaps(qb, bmn, bmx); },
      [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef ref, int) -> bool {
        bbox3f tb;
        v_bbox3_init(tb, v0);
        v_bbox3_add_pt(tb, v1);
        v_bbox3_add_pt(tb, v2);
        if (!v_bbox3_test_box_intersect(tb, box))
          return false;
        const int mat = ownMat ? inlineMat : chunk_leaf_mat_id(*this, node, fr.tree, ref);
        return visitor(v0, v1, v2, mat, ni);
      },
      fr.unq);
  };
  // TLAS arm: the same candidate set the trace dispatch prunes by, then the identical per-node
  // gates, so the TLAS only replaces the enumeration. The default instance's clone serves this
  // bind-frame query (a finalize point materialized it); a missing or stale clone falls back to
  // the linear list.
  TlasCandList cands;
  if (tlasBoxCandidates(defaultInstance, box, cands))
  {
    // A primitive candidate falls out at visitNode's hasGeometry gate, before any logerr.
    for (uint16_t mi : cands)
      if (visitNode(mi))
        return true;
    return false;
  }
  for (uint16_t mi : meshNodes())
    if (visitNode(mi))
      return true;
  return false;
}

int CollisionResource::countTlasCandidates(const CollisionResourceInstance &inst, vec3f local_from, vec3f local_to) const
{
  // The dispatch refreshes tree-backed stamps first (refreshTlasIfStale); this helper never
  // refreshes, so any stale stamp declines - counting the previous pose's leaves would let a
  // narrowing pin pass against bytes the dispatch would have re-encoded.
  // The query-magnitude gate is NOT mirrored (it reads caller-space endpoint magnitudes this
  // resource-local signature cannot know): counts are the reachable set past refresh and clip,
  // and the SELECTED path stays lastDispatchByCand's to assert.
  if (!inst.hasTlas() || !hasAllNodesTLAS() || !inst.tlasCloneCurrent(data->tlasGeneration))
    return -1;
  if (inst.getTree() && (interlocked_acquire_load(inst.tlasPoseGeneration) != inst.getTree()->getPoseGeneration() ||
                          interlocked_acquire_load(inst.tlasBlueprintGenSeen) != data->tlasGeneration ||
                          interlocked_acquire_load(inst.tlasDefaultPoseGenSeen) != defaultPoseGen))
    return -1;
  TlasCandList cands;
  vec4f qf, qt;
  const int seg = inst.tlasQuantSeg(local_from, local_to, qf, qt);
  if (seg < 0)
    return -1; // unclippable segment: the dispatch would fall back to the linear walk
  if (seg > 0) // uncapped: this test helper reports the full reachable set
    collect_tlas_candidates_ordered(inst.tlasTree(), data->tlasRootRef, qf, qt, cands, (uint32_t)data->tlasLeafCount + 1u);
  return (int)cands.size();
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4701) // potentially uninitialized local variable 'XXX' used
#endif

template <bool orthonormalized_instance_tm, CollisionResource::IterationMode trace_mode,
  CollisionResource::CollisionTraceType trace_type, bool is_single_ray, typename pose_t, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNodePrepared(const mat44f tm, float max_tm_scale_sq, vec3f woffset,
  const pose_t &instance, dag::Span<CollisionTrace> traces, bool calc_normal, uint8_t behavior_filter, const filter_t &filter,
  const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull, int ray_mat_id, const mat44f *cached_itm,
  bool force_cull) const
{
  bool hasCollision = false;
  // The behavior mask and the whole-node material early-out are fused with the caller's filter here, so every arm of the walk gets
  // all three and a caller cannot lose a gate by leaving it out of its own filter. The mask reads the LIVE flags of the pose.
  auto acceptNode = [&](const CollisionNode *n, const CollisionResourceInstance::PoseMeta &pm) DAGOR_FORCE_INLINE_LAMBDA {
    return pm.checkBehaviorFlags(behavior_filter) && filter(n) && node_passes_ray_mat(*this, n, ray_mat_id);
  };
  bool isTraceByCapsule = trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT;
  calc_normal &= trace_type == CollisionTraceType::TRACE_RAY || trace_type == CollisionTraceType::TRACE_CAPSULE;

  // The instance inverse serves the whole-resource reject, the box/sphere/capsule primitive pass
  // at the end, and identity-pose mesh nodes, which reuse it as their node inverse. A mesh-only
  // resource with no box/sphere/capsule nodes returns right after the mesh per-node pass (early-out
  // below) and reaches neither tail consumer, so the eager compute gates on the same test; the mesh
  // loop derives the inverse lazily on first identity-pose use when the gate skipped it.
  const bool hasNonMeshNodes = !boxNodes().empty() || !sphereNodes().empty() || !capsuleNodes().empty();

  // Always the full affine inverse: the orthonormal epsilon class's transpose maps segments
  // by s instead of 1/s; tm * inv(tm) = I keeps the parameter scale for the t semantics below.
  // The TLAS term mirrors the candidate gate's STABLE preconditions only: the clone's
  // currency can change inside the gate chain after this guard ran.
  // A caller's entry hands the inverse in (see TraceTmCache): a copy, not a reference, since a
  // nested trace inside a callback may refill the entry mid-walk.
  alignas(EA_CACHE_LINE_SIZE) mat44f itm;
  bool itmComputed = cached_itm != nullptr;
  if (itmComputed)
    itm = *cached_itm;
  else
  {
    itmComputed = traceReadsInstanceInverse(instance, orthonormalized_instance_tm, isTraceByCapsule, is_single_ray);
    if (itmComputed)
      v_mat44_inverse43(itm, tm);
  }

  // Every traceable node box nearly fills the root box and nothing else walks: each node's own
  // box test settles what the root box and node sphere rejects would. A capsule keeps its node
  // sphere: its arm widens the local radius conservatively under a non-uniform scale, so the
  // world-space sphere is the tighter reject there.
  const bool nodesCoverRoot = (collisionFlags & COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX) && !hasNonMeshNodes;

  // A single non-capsule ray may reject against the posed node union. Only the tree-backed
  // temp view skips it (no rootBBox); the tree-less legacy form reads the default instance's.
  if (!instance.isMetaAliased() && is_single_ray && !isTraceByCapsule && !nodesCoverRoot)
  {
    const CollisionTrace &tr = traces.front();
    if (!v_test_segment_box_intersection(v_mat44_mul_vec3p(itm, tr.vFrom), v_mat44_mul_vec3p(itm, tr.vTo), instance.getRootBBox()))
      return false;
  }

  // Per-trace quantized segments plus the mapped from-point the trace loop below reuses on the candidate path
  // (bit-identical arithmetic by construction, so reuse changes nothing).
  struct SegQ
  {
    vec4f qf, qt, lFrom;
    int seg;
  };
  // constraint: declaration order is framemem free order
  dag::Vector<SegQ, framemem_allocator> segs;
  // Instance-TLAS prefilter: one walk per trace marks the nodes this call can reach, and the
  // per-node loops skip the rest -- a prefilter over the existing loop bodies, not a
  // leaf-driven dispatch, so the batch path stays bit-identical to the linear walk. Capsule
  // traces stay linear: a swept radius under a scaled pose maps to an ellipsoid the
  // axis-aligned leaf boxes do not bound.
  TlasCandList tlasCands;
  dag::Vector<uint8_t, framemem_allocator> candSeen; // dense-batch dedup markers; empty until first overflow
  bool byCand = false;
  bool candUnsafe = false;
  float qMagLimit = 0.f;
  // Cheap availability gates first, then the transform gate. Candidate mode runs only in the
  // orthonormal specialization: the general branch's per-node inverse divergence has no
  // bounded budget. getTree() before hasTlas(): a tree-backed clone mutates under concurrent
  // traces, so its storage must not be read before the refresh's acquire.
  bool candGate = pose_t::mayUseTlas && orthonormalized_instance_tm && hasAllNodesTLAS() && !isTraceByCapsule &&
                  (instance.getTree() != nullptr || instance.hasTlas());
  float dev = 0.f;
  if (candGate)
  {
    // The classified tm maps the segment up to 3 * dev * |p| off the per-node inverses; the
    // frame-aware tolerance below keeps that inside half the leaf pad.
    dev = collres_mat33_ortho_dev(tm);
    // Lazy tree-backed freshness: the frame-aware checks below read what this refreshes.
    const mat44f entityTm = {tm.col0, tm.col1, tm.col2, woffset};
    instance.refreshTlasIfStale(entityTm);
    candGate = instance.tlasCloneCurrent(data->tlasGeneration);
  }
  if (candGate)
  {
    // Baked-basis and baked-translation mismatches spend the SAME displacement budget as the
    // tm classification (first-order additive).
    dev += instance.tlasEntityBasisDev(tm) + instance.tlasEntityPosDev(woffset);
    candGate = dev < instance.tlasSegTmTolerance();
    if (candGate)
      qMagLimit = instance.tlasQueryMagLimit(dev);
  }
  if (candGate)
  {
    // Preflight every trace before any traversal: one unsafe trace sends the whole call to
    // the linear walk, and walks performed before discovering that are wasted.
    segs.resize(traces.size());
    const bbox3f clipFrame = instance.tlasClipFrame(); // frame state is call-constant: derive once
    uint32_t si = 0;
    for (const CollisionTrace &trace : traces)
    {
      SegQ &sq = segs[si++];
      sq.seg = 0;
      if (!trace.isectBounding)
        continue;
      // Recompose with the consuming passes' arithmetic: a differently rounded line could
      // miss a narrow leaf.
      sq.lFrom = v_mat44_mul_vec3p(itm, trace.vFrom);
      const vec3f lFrom = sq.lFrom;
      const vec3f lTo = v_madd(v_mat44_mul_vec3v(itm, trace.vDir), v_splats(trace.t), lFrom);
      // Segment and per-node transforms round independently at query magnitude, which the
      // geometry-derived pad does not cover; the caller magnitude restores the pre-offset
      // rounding. NaN endpoints fail too.
      const vec4f cMag = v_max(v_hmax3(v_abs(v_add(trace.vFrom, woffset))), v_hmax3(v_abs(v_add(trace.vTo, woffset))));
      const vec4f qMag = v_max(cMag, v_max(v_hmax3(v_abs(lFrom)), v_hmax3(v_abs(lTo))));
      if (!(v_extract_x(qMag) < qMagLimit))
      {
        candUnsafe = true;
        break;
      }
      sq.seg = instance.tlasQuantSeg(clipFrame, lFrom, lTo, sq.qf, sq.qt);
      if (DAGOR_UNLIKELY(sq.seg < 0))
      {
        candUnsafe = true; // unclippable segment: the whole call keeps the linear walk
        break;
      }
    }
    // O(1) node id -> leaf ordinal, prebuilt by the blueprint for the marker scratch.
    const dag::ConstSpan<uint16_t> nodeLeafOrd = tlasNodeLeafOrd();
    auto leafOrd = [&](uint16_t id) { return (uint32_t)nodeLeafOrd[id]; };
    auto compactCands = [&] {
      stlsort::sort(tlasCands.begin(), tlasCands.end());
      tlasCands.resize(eastl::unique(tlasCands.begin(), tlasCands.end()) - tlasCands.begin());
    };
    bool candDense = false;
    // Past this the density fallback is certain: collectors terminate instead of finishing
    // walks whose lists are guaranteed discarded.
    const uint32_t denseAt = ((uint32_t)data->tlasLeafCount + 1u) / 2u;
    if (!candUnsafe)
      for (uint32_t ti = 0, te = (uint32_t)traces.size(); ti < te && !candDense; ++ti)
      {
        if (segs[ti].seg <= 0)
          continue; // segment misses the frame box: no leaf reachable
        if (is_single_ray)
          collect_tlas_candidates_ordered(instance.tlasTree(), data->tlasRootRef, segs[ti].qf, segs[ti].qt, tlasCands, denseAt);
        else
        {
          const uint32_t tailAt = (uint32_t)tlasCands.size();
          collect_tlas_candidates_any(instance.tlasTree(), data->tlasRootRef, segs[ti].qf, segs[ti].qt, tlasCands, denseAt);
          // The first overflow compacts by sort and mints leaf-ordinal markers; later rays
          // dedup their tails in O(tail) instead of re-sorting the union.
          if (DAGOR_UNLIKELY(!candSeen.empty()))
          {
            uint32_t w = tailAt;
            for (uint32_t r = tailAt, e2 = (uint32_t)tlasCands.size(); r < e2; ++r)
            {
              const uint32_t ord = leafOrd(tlasCands[r]);
              if (!candSeen[ord])
              {
                candSeen[ord] = 1;
                tlasCands[w++] = tlasCands[r];
              }
            }
            tlasCands.resize(w);
            if (tlasCands.size() * 2u >= (uint32_t)data->tlasLeafCount) // the union only grows
              candDense = true;
          }
          else if (DAGOR_UNLIKELY((uint32_t)tlasCands.size() - tailAt >= denseAt))
            candDense = true; // one walk emits each leaf once: this ray's tail alone proves the fallback
          else if (DAGOR_UNLIKELY(tlasCands.size() > 2u * data->tlasLeafCount))
          {
            compactCands();
            candSeen.resize(data->tlasLeafCount, 0);
            for (uint16_t id : tlasCands)
              candSeen[leafOrd(id)] = 1;
            if (tlasCands.size() * 2u >= (uint32_t)data->tlasLeafCount)
              candDense = true;
          }
        }
      }
    if (!is_single_ray && !candDense && tlasCands.size() > 1) // one leaf can be named by several rays
      compactCands();
    // Past half the leaves the linear type walks are the cheaper iteration; leaves, not
    // nodes, so leafless POINTS do not dilute the ratio.
    byCand = !candUnsafe && !candDense && tlasCands.size() * 2u < (uint32_t)data->tlasLeafCount;
  }
#if DAGOR_DBGLEVEL > 0
  instance.noteDispatchByCand(byCand); // tests assert the SELECTED path, not just result parity
#endif

  // The per-type passes walk the candidate list instead of their node list; a candidate can
  // name a node of any type, hence the per-pass type test. An empty type list stays free of
  // the candidate scan.
  auto passNodes = [&](dag::ConstSpan<uint16_t> list) -> dag::ConstSpan<uint16_t> {
    return byCand && !list.empty() ? make_span_const(tlasCands) : list;
  };

  // Per-node trace of the mesh list (mesh + convex nodes): each node descends its own chunk.
  if (!meshNodes().empty() && !(byCand && tlasCands.empty()))
  {
    TIME_PROFILE_DEV(collres_trace_mesh_per_node);
    CollResProfileStats profileStats;
    profileStats.meshNodesNum = (int)meshNodes().size();
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
    const GeomNodeTree *instTree = instance.getTree();
    // An owned pose composes stored matrices; a tree-backed one reads live wtms and cannot take
    // the identity shortcut below.
    const bool ownedPose = instTree == nullptr;
    const dag::ConstSpan<uint16_t> meshList = passNodes(meshNodes());
    const uint32_t meshCount = meshList.size();
    // Most nodes miss a single ray's sphere test: the four-wide cull skips them before any per-node work. It culls
    // 64 nodes at a time, so an any-hit trace stops early and a closest trace culls later nodes by its shortened ray.
    const bool preCulled = is_single_ray && !isTraceByCapsule && !nodesCoverRoot && !byCand && instTree &&
                           !(collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) &&
                           instance.primPoseSource().metaCount() >= data->allNodesList().size();
    const bool authoredMeta = instance.isMetaAliased();
    const bool liveBound = authoredMeta && hasLiveBoundNodes();
    uint64_t sphereBits = 0;
    auto nextMesh = [&](uint32_t li) -> uint32_t {
      if (!preCulled)
        return li;
      for (; li < meshCount; li = (li | 63u) + 1)
      {
        if ((li & 63u) == 0) // the loop enters each word at its first entry
          sphereBits =
            preCullTreeMeshSpheres(traces.front(), instance.primPoseSource(), *instTree, woffset, li, authoredMeta, liveBound);
        if (const uint64_t b = sphereBits >> (li & 63u))
          return li + __ctz_unsafe(b);
      }
      return meshCount;
    };
    for (uint32_t li = nextMesh(0); li < meshCount; li = nextMesh(li + 1))
    {
      const uint16_t mi = meshList[li];
      const CollisionNode *meshNode = &data->allNodesList()[mi];
      if (byCand && !collres_is_mesh_list_node(meshNode->type))
        continue; // a candidate of another type; its own pass takes it
      // Every walk below descends the node's chunk. Geometry without one exists only in exporter
      // raw workspaces (never traced) and after a rejected chunk build (geometry dropped).
      if (DAGOR_UNLIKELY(meshNode->nodeBlasOfs == ~0u))
        continue;
      // Tree-backed poses read the live wtm and the meta their refresh classified (temp views
      // alias the bind meta verbatim). One fetch feeds the hide gate, the traceable gate
      // and the pose class: each per-node accessor otherwise re-bounds-checks its slot (temp views
      // through the PoseView delegation), and the absent-slot fallback already encodes exactly
      // the answers the individual accessors give out of range (enabled, not traceable).
      const CollisionResourceInstance::PoseMeta &pm = instance.getPoseMeta(meshNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue; // structurally hidden or untraceable pose: skipped before any filter
      if (!acceptNode(meshNode, pm))
        continue;
      const bool nodeNoCull = force_no_cull || pm.checkBehaviorFlags(CollisionNode::SOLID); // all-hits arm only

      const float nodeMaxTmScale = pm.maxTmScale;
      const uint8_t nodeTmFlags = pm.flags;
      // A stored identity composes to the entity tm bit-for-bit, so referencing it directly skips
      // the 3x4 load, the mat44 conversion, the compose and the 64-byte copy for every node placed
      // at the origin -- which is most of them in a real resource. Same for its inverse below.
      alignas(EA_CACHE_LINE_SIZE) mat44f posedTm, posedItm;
      const bool poseIsIdent = ownedPose && pm.isPoseIdentity();
      if (DAGOR_UNLIKELY(!poseIsIdent))
        posedTm = getPosedNodeWtmInline(meshNode, tm, woffset, instance);
      const mat44f &nodeTm = poseIsIdent ? tm : posedTm;
      if (isTraceByCapsule || !is_single_ray || !nodesCoverRoot)
      {
        vec3f bsphCenter = v_mat44_mul_vec3p(nodeTm, meshNode->vBsphereCenter());
        // A driven node's wtm lives in tree world: the trace tm never composes into it, so its
        // scale must not size the reject (an entity scale < 1 would under-cover the sphere).
        const bool treeDriven = instTree && meshNode->geomNodeId.index() < instTree->nodeCount();
        const bool authoredDriven = treeDriven && authoredMeta;
        const float nodeScaleSq = treeDriven && liveBound && isLiveBoundNode(meshNode->nodeIndex)
                                    ? collres_sigma_max_bound_sq(nodeTm)
                                    : sqr(nodeMaxTmScale * (authoredDriven ? COLLRES_RIGID_SIGMA_MAX : 1.f));
        float bsphR2 = get_bsphere_r2(meshNode->radiusAroundBoxCenter) * (treeDriven ? 1.f : max_tm_scale_sq) * nodeScaleSq;
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
      }
      else
        v_mat44_inverse43(posedItm, nodeTm);
      const mat44f &nodeItm = poseIsIdent ? itm : posedItm;

      vec3f chunkScale = v_zero(), chunkQOfs = v_zero();
      const uint8_t *chunkTree = nullptr;
      soa4::RootRef chunkRootRef;
      Vert21MetricLoader chunkUnquant = {};
      // These locals feed only the traceChunkRay lambda (ray closest / any-hit). All-hits TRACE_RAY
      // descends via traceAllHitsNodeChunk, which decodes the chunk frame itself, so skip the
      // pre-decode there instead of decoding the frame twice per node.
      const bool needsChunkRayDecode = trace_mode != ALL_INTERSECTIONS || trace_type == CollisionTraceType::RAY_HIT;
      if (!isTraceByCapsule && needsChunkRayDecode)
      {
        // q-space transform (chunk header scale) for the tree walk; decode frame (block bmin/invScale)
        // for hit normals.
        const NodeChunkFrame fr = decode_node_chunk_frame(nodeChunkPtr(*meshNode));
        chunkScale = fr.scale;
        chunkQOfs = fr.qOfs;
        chunkTree = fr.tree;
        chunkRootRef = fr.rootRef;
        chunkUnquant = fr.unq;
      }
      // Per-leaf material gate for this node's chunk walks, built once per node; a node the whole-node early-out settles gets no hook
      // and walks as before.
      ChunkLeafMatCtx leafMatCtx;
      leaf_accept_t leafMatAccept = nullptr;
      if (DAGOR_UNLIKELY(node_needs_leaf_mat_gate(*this, *meshNode, ray_mat_id)))
      {
        leafMatCtx = {this, meshNode, node_chunk_tree(nodeChunkPtr(*meshNode)), ray_mat_id};
        leafMatAccept = chunk_leaf_mat_accept;
      }
      // One convention for every chunk walk below: the context is null exactly when the accept is, so no callee is handed a context it
      // must know not to read.
      void *const acceptCtxOrNull = leafMatAccept ? (void *)&leafMatCtx : nullptr;

      // |nodeItm d| under-bounds the radial widening of a mixed-scale node frame; capsules need
      // the full inverse stretch: exact for uniform scale, conservative for shear, and it covers
      // the class tolerance bands without a fixed pad (a down-scale inside the band needs MORE
      // than any constant). A bit-exact rigid basis has inverse stretch exactly 1: skip the
      // spectral path for that common population instead of paying it per node per capsule call.
      const float nodeItmCapsuleScale = !isTraceByCapsule || v_mat44_is_signed_perm33(nodeItm) ? 1.f : mat33_spectral_norm(nodeItm);

      for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
      {
        CollisionTrace &trace = traces[traceId];
        if (!trace.isectBounding)
          continue;

        vec3f vNodeLocalFrom = v_mat44_mul_vec3p(nodeItm, trace.vFrom);
        // Linear-part direction map (endpoint differences cancel far from the origin). The closest
        // and any-hit arms keep the caller's parameter along it and map a hit back by ratio; a unit
        // direction matters only to the capsule arms (a local radius measured against t) and to the
        // all-hits arm (its triangle kernel's fixed tolerance sits on direction-scaled numerators).
        // Those renormalize only when the mapping changed the length (re-division adds ULP noise).
        vec3f vNodeLocalDir = v_mat44_mul_vec3v(nodeItm, trace.vDir);
        float localT = trace.t;
        if (isTraceByCapsule || trace_mode == ALL_INTERSECTIONS)
        {
          const float nodeDirLen = v_extract_x(v_length3_x(vNodeLocalDir));
          if (fabsf(nodeDirLen - 1.f) > 1e-6f)
          {
            vNodeLocalDir = v_div(vNodeLocalDir, v_splats(nodeDirLen));
            localT = trace.t * nodeDirLen;
          }
        }
        if (DAGOR_UNLIKELY(!(localT >= VERY_SMALL_NUMBER)))
          continue;
        float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * nodeItmCapsuleScale : 0.f;

        // The node box, padded on every axis by the BVH backend's blas_size_eps: a flat node (a
        // ground plane, a curtain mesh) would otherwise reject a ray that starts on its surface and
        // hits a triangle. A superset of the old flat-axis-only inflation; the walk decides the hit.
        bbox3f bbox = v_ldu_bbox3(meshNode->modelBBox);
        v_bbox3_extend(bbox, v_splats(0.00005f));
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
          // Ray chunk hits carry the leaf identity. The capsule arms stay non-tri BY CHOICE: a
          // fused node's capsule material answers the dominant (getHitPhysMat's non-tri rule);
          // the per-face capsule answer is deferred.
          tri_ref_t hitRef = tri_ref::makeForNonTri(meshNode->nodeIndex);
          // Per-node chunk descent for ray closest/any-hit: the node-local ray transforms into the
          // chunk's q-space with an unnormalized direction, so the parametric t carries over
          // unchanged. A TRACE_TWO_SIDED node walks uncull, the contract the legacy CULL_BOTH FRT
          // gave these arms; everything else culls CCW like the scalar arm it replaces.
          // Out of line the closure costs a call and 27 captured references stored per ray.
          const auto traceChunkRay = [&](bool any_hit) DAGOR_FORCE_INLINE_LAMBDA {
            RayData rd;
            rd.data = chunkTree;
            rd.rayOrigin = v_madd(vNodeLocalFrom, chunkScale, chunkQOfs);
            rd.rayDir = v_mul(vNodeLocalDir, chunkScale);
            rd.t = inOutLocalT;
            rd.calc();
            // The OOL takes an accept either way, so the material gate rides the call the accept-all lambda already made -- no leaf
            // costs more than it did.
            const collision_blas::LeafAcceptRef acceptAll = [](void *, soa4::LeafRef) -> bool { return true; };
            const collision_blas::LeafAcceptRef accept = leafMatAccept ? leafMatAccept : acceptAll;
            void *acceptCtx = acceptCtxOrNull;
            soa4::LeafRef bestRef = 0;
            int bestSub = 0;
            const bool nodeTwoSided = !force_cull && (meshNode->flags & CollisionNode::TRACE_TWO_SIDED) != 0;
            const bool hit =
              any_hit ? (nodeTwoSided
                            ? collision_blas::raySoa4AnyHitFilteredOOL(rd, chunkRootRef, accept, acceptCtx, bestRef, bestSub)
                            : collision_blas::raySoa4AnyHitFilteredOOLCullCCW(rd, chunkRootRef, accept, acceptCtx, bestRef, bestSub))
                      : (nodeTwoSided
                            ? collision_blas::raySoa4ClosestFilteredOOL(rd, chunkRootRef, accept, acceptCtx, bestRef, bestSub)
                            : collision_blas::raySoa4ClosestFilteredOOLCullCCW(rd, chunkRootRef, accept, acceptCtx, bestRef, bestSub));
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
            hitRef = tri_ref::make_node_blas(meshNode->nodeIndex, bestRef, (uint32_t)bestSub, data->nodeBlasBuildId);
            return true;
          };
          switch (trace_type)
          {
            case CollisionTraceType::TRACE_RAY: isHit = traceChunkRay(false /*any_hit*/); break;
            case CollisionTraceType::TRACE_CAPSULE:
              isHit = traceCapsuleNodeChunkCullCCW(*meshNode, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, localCapsuleRadius,
                vNodeLocalNorm, vNodeLocalCapsuleHitPos, leafMatAccept, acceptCtxOrNull);
              break;
            case CollisionTraceType::RAY_HIT: isHit = traceChunkRay(true /*any_hit*/); break;
            case CollisionTraceType::CAPSULE_HIT:
              isHit = capsuleHitNodeChunkCullCCW(*meshNode, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, localCapsuleRadius,
                leafMatAccept, acceptCtxOrNull);
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
              if (DAGOR_LIKELY(isOrthonormalizedTm) && v_mat44_is_signed_perm33(nodeTm))
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

          all_collres_hits_t ret;
          const bool isHit = traceAllHitsNodeChunk(*meshNode, vNodeLocalFrom, vNodeLocalDir, localT, calc_normal, nodeNoCull,
            force_cull, ret, leafMatAccept, acceptCtxOrNull);
          if (isHit)
          {
            hasCollision = true;
            mat33f titm33;
            // Forward rotation is the inverse transpose only for bit-exact rigid bases; the
            // epsilon classes admit shear that tilts a forward-mapped normal off the plane.
            const bool exactRigidNormals = isOrthonormalizedTm && v_mat44_is_signed_perm33(nodeTm);
            if (!exactRigidNormals && calc_normal)
            {
              mat33f itm33;
              v_mat33_from_mat44(itm33, nodeItm);
              v_mat33_transpose(titm33, itm33);
            }
            for (const CollResChunkHit &hit : ret)
            {
              const vec4f n_t = hit.normT;
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
              callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos, hit.triRef);
            }
            profileStats.meshTrianglesHits += ret.size();
          }
        } // allow multiple intersection of one node or not
      } // traces loop
    } // mesh nodes loop

    if (DAGOR_LIKELY(!hasNonMeshNodes))
      return hasCollision;
  } // mesh-list per-node pass

  // Every primitive node carries a leaf, so an empty candidate set rules them all out and the
  // per-node loops below would only re-derive that one node at a time.
  if (byCand && tlasCands.empty())
    return hasCollision;

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
    const mat44f world = getPosedNodeWtmInline(&data->allNodesList()[node_index], tm, woffset, primPose);
    mat44f posed;
    v_mat44_mul43(posed, itm, world);
    return geometryTmFromPosed(node_index, posed, primPose.getPoseMeta(node_index));
  };

  // Sparse per-batch cache of non-identity prim inverses, keyed by nodeIndex: sized by the
  // posed prims actually visited, not the node list.
  struct PrimInvCache //-V730 invT is set under the computed gate; eager init is per-trace waste
  {
    mat44f invT;
    float maxInvScale = 1.f; // must stay: MSVC x86 hoists the fld out of the capsule branch, SNaN bytes raise #IA
    int nodeIndex = -1;
    bool computed = false;
  };
  // Node-indexed slot map, BATCH scope like the cache it indexes (a per-ray map would re-append
  // the same node every ray and overflow narrow slots): a scan-based find-or-add is quadratic
  // across the first ray's appends. Sized lazily so plain-IDENT batches never touch it.
  // constraint: declaration order is framemem free order
  dag::Vector<int, framemem_allocator> primSlotOfNode;
  dag::Vector<PrimInvCache, framemem_allocator> primInvCache;

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
    vec3f vLocalFrom = byCand ? segs[traceId].lFrom : v_mat44_mul_vec3p(itm, trace.vFrom);
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
          primSlotOfNode.assign(data->allNodesList().size(), -1);
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


    for (uint16_t bi : passNodes(boxNodes()))
    {
      const CollisionNode *boxNode = &data->allNodesList()[bi];
      if (byCand && boxNode->type != COLLISION_NODE_TYPE_BOX)
        continue;
      const CollisionResourceInstance::PoseMeta &pm = primPose.getPoseMeta(boxNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      if (!acceptNode(boxNode, pm))
        continue;
      // A box is an OOBB, so a placed one costs a ray transform into its frame -- the same work
      // the pre-instance dispatch did. An UNPLACED one cost nothing back then, because its stored
      // geometry was already resource-local, and it must cost nothing now: no frame pull, no ray
      // struct, no per-node branch on the result -- just the per-trace ray as-is.
      const bool posed = !(pm.flags & CollisionNode::IDENT);
      vec3f boxFrom = vLocalFrom, boxTo = vLocalTo, boxDir = vLocalDir;
      float primT = localT, primR = localCapsuleRadius;
      if (DAGOR_UNLIKELY(posed))
      {
        const PrimRay pr = makePrimRay(boxNode);
        if (pr.t < VERY_SMALL_NUMBER)
          continue;
        boxFrom = pr.from;
        boxTo = v_madd(pr.dir, v_splats(pr.t), pr.from);
        boxDir = pr.dir;
        primT = pr.t;
        primR = pr.capsuleRadius;
      }
      float atMin = 1.f, atMax = 1.f; // [0; 1]
      float inOutLocalT = primT;
      int side = 0;
      bool isHit = false;
      vec3f vCapsuleLocalPos = v_zero(), vCapsuleLocalNorm = v_zero();
      bbox3f modelBBox = v_ldu_bbox3(boxNode->modelBBox);

      // implement capsule hit as test against mesh
      CollisionNode tmpNode;
      Point3_vec4 boxVerts[8];
      // clang-format off
      constexpr uint32_t boxIndices[36] = {
        0, 6, 4,  0, 2, 6,
        0, 5, 1,  0, 4, 5,
        0, 1, 3,  0, 3, 2,
        1, 7, 3,  1, 5, 7,
        2, 3, 7,  2, 7, 6,
        4, 7, 5,  4, 6, 7,
      };
      // clang-format on
      IF_CONSTEXPR (trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT)
      {
        vec3f bsize = v_sub(modelBBox.bmax, modelBBox.bmin);
        vec3f inflate = v_mul(v_sub(v_max(bsize, v_splats(0.0001f)), bsize), V_C_HALF);
        modelBBox.bmin = v_sub(modelBBox.bmin, inflate);
        modelBBox.bmax = v_add(modelBBox.bmax, inflate);
        for (int i = 0; i < 8; i++)
          v_stu(&boxVerts[i], v_bbox3_point(modelBBox, i));
        tmpNode.indicesCount = 36;
      }

      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          side = v_segment_box_intersection_side(boxFrom, boxTo, modelBBox, atMin, atMax);
          isHit = side != -1;
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit = traceCapsuleMeshNodeLocalCullCCW(boxVerts, boxIndices, tmpNode, boxFrom, boxDir, inOutLocalT, primR,
            vCapsuleLocalNorm, vCapsuleLocalPos);
          break;
        case CollisionTraceType::RAY_HIT: isHit = v_test_segment_box_intersection(boxFrom, boxTo, modelBBox); break;
        case CollisionTraceType::CAPSULE_HIT:
          isHit = capsuleHitMeshNodeLocalCullCCW(boxVerts, boxIndices, tmpNode, boxFrom, boxDir, inOutLocalT, primR);
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        vec4f vIntersectionPos, vIntersectionNorm = v_zero();
        float intersectionT;
        IF_CONSTEXPR (trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT)
        {
          atMin = inOutLocalT / primT;
          intersectionT = trace.t * atMin;
          vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
          mat44f fullTm;
          if (DAGOR_UNLIKELY(posed) && (calc_normal || trace_type == CollisionTraceType::TRACE_CAPSULE))
            v_mat44_mul43(fullTm, tm, primGeometryTm(boxNode->nodeIndex));
          if (calc_normal)
            vIntersectionNorm = posedPrimNormal(posed ? fullTm : tm, vCapsuleLocalNorm);
          IF_CONSTEXPR (trace_type == CollisionTraceType::TRACE_CAPSULE)
            vIntersectionPos = v_mat44_mul_vec3p(posed ? fullTm : tm, vCapsuleLocalPos);
        }
        else
        {
          intersectionT = trace.t * atMin;
          vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
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
        }
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          localT *= atMin; // keep the local length consistent with the tightened trace.t below
          trace.t = intersectionT;
          vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
        }
        callback(traceId, boxNode, intersectionT, vIntersectionNorm, vIntersectionPos, tri_ref::makeForNonTri(boxNode->nodeIndex));
        if (trace_mode == ANY_ONE_INTERSECTION)
          return hasCollision;
        if (trace_mode == FIND_BEST_INTERSECTION && intersectionT < VERY_SMALL_NUMBER)
          goto next_trace;
      }
    } // box nodes loop

    for (uint16_t si : passNodes(sphereNodes()))
    {
      const CollisionNode *sphereNode = &data->allNodesList()[si];
      if (byCand && sphereNode->type != COLLISION_NODE_TYPE_SPHERE)
        continue;
      const CollisionResourceInstance::PoseMeta &pm = primPose.getPoseMeta(sphereNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      // Zero-vert marker: adding a sweep radius to the negative marker would resurrect it as
      // positive geometry in the capsule arms.
      if (DAGOR_UNLIKELY(sphereNode->radiusAroundBoxCenter < 0.f))
        continue;
      if (!acceptNode(sphereNode, pm))
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
      vec3f bsphPos = sphereNode->vBsphereCenter();
      vec4f bsphR2 = v_set_x(get_bsphere_r2(sphereNode->radiusAroundBoxCenter));
      vec4f bsphCapsuleR = v_set_x(sphereNode->radiusAroundBoxCenter + primR);
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
          vec3f vLocalNorm = v_sub(vLocalHitPos, sphereNode->vBsphereCenter());
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
            vec3f vLocalPos = v_madd(v_norm3(vLocalNorm), v_splats(sphereNode->radiusAroundBoxCenter), sphereNode->vBsphereCenter());
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

    for (uint16_t ci : passNodes(capsuleNodes()))
    {
      const CollisionNode *capsuleNode = &data->allNodesList()[ci];
      // The TLAS gate is on the trace SHAPE, so a ray tested against capsule nodes still prefilters.
      if (byCand && capsuleNode->type != COLLISION_NODE_TYPE_CAPSULE)
        continue;
      const CollisionResourceInstance::PoseMeta &pm = primPose.getPoseMeta(capsuleNode->nodeIndex);
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      // Zero-vert marker: a pose write re-classifies it traceable, and the degenerate stored
      // capsule plus a sweep radius would fake a hit at the pose origin.
      if (DAGOR_UNLIKELY(capsuleNode->radiusAroundBoxCenter < 0.f))
        continue;
      if (!acceptNode(capsuleNode, pm))
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

      const Capsule &nodeCapsule = data->capsules()[capsuleNode->capsuleIndex];
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

    bool posed; // a live tree pose never licenses bind shortcuts

    const GeomNodeTree *getTree() const { return treePtr; }
    // A temp view over a tree carries no rootBBox and never refreshes; the tree-less form is the
    // default instance itself, so it is not aliased.
    bool isMetaAliased() const { return treePtr != nullptr; }
    bool isPosedSinceBind() const { return posed; }
    // Primitive source for the trace core: both legacy forms read primitives from the
    // default instance the view wraps.
    const CollisionResourceInstance &primPoseSource() const { return *owned; }
    // No clone on this path; compile-time false folds the candidate gate out of these
    // instantiations.
    static constexpr bool mayUseTlas = false;
    bool hasTlas() const { return false; }
    bool tlasCloneCurrent(uint32_t) const { return false; }
    float tlasEntityBasisDev(mat44f_cref) const { return 0.f; }
    float tlasEntityPosDev(vec3f) const { return 0.f; }
    const uint8_t *tlasTree() const { return nullptr; }
    // Dead by mayUseTlas, so both state the FALLBACK a reached gate must take: -1 sends the call
    // to the linear walk, where 0 would claim a frame miss and drop every hit of that call.
    vec4f tlasQuant(vec3f p) const { return p; }
    bbox3f tlasClipFrame() const { return bbox3f{}; }
    int tlasQuantSeg(bbox3f_cref, vec3f, vec3f, vec4f &, vec4f &) const { return -1; }
    float tlasSegTmTolerance() const { return 0.f; }
    float tlasQueryMagLimit(float) const { return 0.f; }
    void refreshTlasIfStale(mat44f_cref) const {}
    void noteDispatchByCand(bool) const {}
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
    int ray_mat_id, uint8_t behavior_filter, CollisionResource::TraceTmCache *tm_cache = nullptr, bool force_cull = false)
  {
    auto nodeFilter = [&](const CollisionNode *node) -> bool { return !filter || !*filter || (*filter)(node->nodeIndex); };

    auto callback = [&](int, const CollisionNode *node, float t, vec3f normal, vec3f, tri_ref_t tri_ref) DAGOR_FORCE_INLINE_LAMBDA {
      in_out_t = t;
      if (out_normal)
        v_stu_p3(&out_normal->x, normal);
      out_mat_id = res.getHitPhysMat(tri_ref);
      if (out_node_id)
        *out_node_id = node->nodeIndex;
    };

    return res.forEachIntersectedNode<CollisionResource::FIND_BEST_INTERSECTION, CollisionResource::CollisionTraceType::TRACE_RAY,
      pose_t::mayRefresh>(tm, pose.pose(), v_ldu(&from.x), v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f, behavior_filter,
      nodeFilter, callback, nullptr, false, ray_mat_id, tm_cache, force_cull);
  }

  template <typename pose_t>
  static __forceinline bool traceRayAll(const CollisionResource &res, const mat44f &tm, const pose_t &pose, vec3f from, vec3f dir,
    float in_t, CollResIntersectionsType &intersections, bool sort_intersections, uint8_t behavior_filter,
    const CollisionNodeFilter *filter, const CollisionNodeMask *mask, float bsphere_scale, TraceCollisionResourceStats *out_stats,
    bool force_no_cull, CollisionResource::TraceTmCache *tm_cache = nullptr)
  {
    intersections.clear();
    auto nodeFilter = [&](const CollisionNode *node) -> bool {
      return (!filter || !*filter || (*filter)(node->nodeIndex)) && (!mask || mask->test(node->nodeIndex, true));
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
      force_no_cull, PHYSMAT_INVALID, tm_cache);
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
    const vec3f worldFrom = trace.vFrom;
    float closest = FLT_MAX;

    auto nodeFilter = [&](const CollisionNode *node) -> bool { return !filter || !*filter || (*filter)(node->nodeIndex); };
    auto callback = [&](int, const CollisionNode *, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      const vec3f line_pos = v_closest_point_on_line(pos, worldFrom, trace.vDir);
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
    auto nodeFilter = [](const CollisionNode *) -> bool { return true; }; // the core applies the TRACEABLE mask
    auto callback = [&](int, const CollisionNode *, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      in_out_t = t;
      v_stu_p3(&out_normal.x, normal);
      v_stu_p3(&out_pos.x, pos);
      out_mat_id = res.getHitPhysMat(tri_ref);
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
    auto nodeFilter = [&](const CollisionNode *node) -> bool { return !mask || mask->test(node->nodeIndex, true); };
    auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
      traces[trace_id].outMatId = res.getHitPhysMat(tri_ref);
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
    const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *mask, int *out_mat_id,
    CollisionResource::TraceTmCache *tm_cache = nullptr)
  {
    auto nodeFilter = [&](const CollisionNode *node) -> bool { return !mask || mask->test(node->nodeIndex, true); };
    auto callback = [out_mat_id, &res](int, const CollisionNode *, float, vec3f, vec3f, tri_ref_t tri_ref) {
      if (out_mat_id)
        *out_mat_id = res.getHitPhysMat(tri_ref);
    };

    return res.forEachIntersectedNode<CollisionResource::ANY_ONE_INTERSECTION, CollisionResource::CollisionTraceType::RAY_HIT,
      pose_t::mayRefresh>(tm, pose.pose(), v_ldu(&from.x), v_ldu(&dir.x), in_t, false, bsphere_scale, CollisionNode::TRACEABLE,
      nodeFilter, callback, nullptr, false, PHYSMAT_INVALID, tm_cache);
  }

  template <typename pose_t>
  static __forceinline bool capsuleHit(const CollisionResource &res, const mat44f &tm, const pose_t &pose, const Point3 &from,
    const Point3 &dir, float in_t, float radius, CollResHitNodesType &nodes_hit)
  {
    CollisionTrace trace;
    initCapsuleTrace(trace, from, dir, in_t, radius);
    auto nodeFilter = [](const CollisionNode *) -> bool { return true; }; // the core applies the TRACEABLE mask
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
    auto nodeFilter = [](const CollisionNode *) -> bool { return true; }; // the core applies the TRACEABLE mask
    auto callback = [&](int trace_id, const CollisionNode *node, float, vec3f, vec3f, tri_ref_t tri_ref) {
      traces[trace_id].isHit = true;
      traces[trace_id].outMatId = res.getHitPhysMat(tri_ref);
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
  Point3 *out_normal, int &out_mat_id, int ray_mat_id, uint8_t behavior_filter, TraceTmCache *tm_cache, bool force_cull) const
{
  return CollisionResourceTraceAdapter::traceRayClosest(*this, tm, CollisionResourceTraceAdapter::pose(*this), from, dir, in_out_t,
    out_normal, out_mat_id, nullptr, nullptr, ray_mat_id, behavior_filter, tm_cache, force_cull);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter,
  const CollisionNodeMask *collision_node_mask, bool force_no_cull, TraceTmCache *tm_cache) const
{
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    v_ldu(&from.x), v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, behavior_filter, nullptr, collision_node_mask,
    1.f, nullptr, force_no_cull, tm_cache);
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
  const CollisionNodeMask &collision_node_mask, TraceCollisionResourceStats *out_stats, TraceTmCache *tm_cache) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree),
    v_ldu(&from.x), v_ldu(&dir.x), in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, nullptr,
    &collision_node_mask, bsphereScale, out_stats, false, tm_cache);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
  TraceCollisionResourceStats *out_stats, TraceTmCache *tm_cache) const
{
  return CollisionResourceTraceAdapter::traceRayAll(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from, dir,
    in_t, intersected_nodes_list, sort_intersections, CollisionNode::TRACEABLE, nullptr, &collision_node_mask, bsphereScale, out_stats,
    false, tm_cache);
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
  auto nodeFilter = [](const CollisionNode *) -> bool { return true; }; // the core applies behavior_filter

  auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/, tri_ref_t tri_ref) {
    CollisionTrace &trace = traces[trace_id];
    v_stu_p3(&trace.norm.x, normal);
    trace.t = t;
    trace.outMatId = getHitPhysMat(tri_ref);
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
    1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/, ray_mat_id);
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


bool CollisionResource::traceRayMeshNodeLocal(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
  float &in_out_t, vec4f *v_out_norm) const
{
  if (!checkNodeBehaviorFlags(node.nodeIndex, CollisionNode::TRACEABLE))
    return false;

  bbox3f bbox = v_ldu_bbox3(node.modelBBox);
  if (!v_test_ray_box_intersection_unsafe(v_local_from, v_local_dir, v_set_x(in_out_t), bbox))
    return false;

  // Per-node quad-BLAS descent; callers that loop nodes (InteractiveObject::traceRay) pay only
  // the tree walk. Geometry without a chunk exists only in never-traced exporter workspaces.
  if (DAGOR_UNLIKELY(node.nodeBlasOfs == ~0u))
    return false;
  return traceRayNodeChunkCullCCW(node, v_local_from, v_local_dir, in_out_t, v_out_norm, nullptr, nullptr); // no material filter
}


bool CollisionResource::traceRayMeshNodeLocalAllHits(const CollisionNode &node, const Point3 &from, const Point3 &dir, float in_t,
  CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, bool force_no_cull) const
{
  if (!checkNodeBehaviorFlags(node.nodeIndex, CollisionNode::TRACEABLE))
    return false;
  force_no_cull |= checkNodeBehaviorFlags(node.nodeIndex, CollisionNode::SOLID); // the kernels no longer read SOLID
  const vec4f vLocalFrom = v_ldu_p3(&from.x);
  const vec4f vLocalDir = v_ldu_p3(&dir.x);
  const bbox3f bbox = v_ldu_bbox3(node.modelBBox);
  if (!v_test_ray_box_intersection_unsafe(vLocalFrom, vLocalDir, v_set_x(in_t), bbox))
    return false;

  all_collres_hits_t allHits;
  if (DAGOR_UNLIKELY(node.nodeBlasOfs == ~0u))
    return false; // no chunk = no traceable geometry (exporter raw workspaces never trace)
  const bool res = traceAllHitsNodeChunk(node, vLocalFrom, vLocalDir, in_t, true, force_no_cull, false /*force_cull*/, allHits,
    nullptr, nullptr); // no material filter

  IntersectedNode *dst = intersected_nodes_list.append_default(allHits.size());
  for (const CollResChunkHit &hit : allHits)
  {
    IntersectedNode &n = *dst++;
    n.intersectionT = v_extract_w(hit.normT);
    v_stu_p3(&n.intersectionPos.x, v_madd(vLocalDir, v_splat_w(hit.normT), vLocalFrom));
    v_stu_p3(&n.normal.x, hit.normT);
    n.triRef = hit.triRef;
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

  const auto testNode = [&](const CollisionNode *meshNode) -> bool {
    if ((filter && !filter(meshNode->nodeIndex)) || !inst->checkNodeBehaviorFlags(meshNode->nodeIndex, CollisionNode::TRACEABLE))
      return false;
    if (!inst->isNodeEnabled(meshNode->nodeIndex))
      return false;
    Point3 nodeNorm;
    float nodeDepth;
    if (!test_sphere_node_intersection(sphere, meshNode, *inst, dir_norm, nodeNorm, nodeDepth))
      return false;
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
  };
  // TLAS arm: the dispatch's candidate set replaces the node enumeration; every per-node gate
  // runs unchanged, and the mesh list stays the fallback. Any-hit: the walk is unordered, and
  // which of several overlapping nodes answers stays unspecified, as before.
  bbox3f sphBox;
  v_bbox3_init_by_bsph(sphBox, bsph, v_splat_w(bsph));
  CollResTlasCandidates cands;
  if (tlasBoxCandidates(*inst, sphBox, cands))
  {
    // A primitive candidate falls out at the kernel's hasGeometry gate, as in visitTrianglesInBox.
    for (uint16_t mi : cands)
      if (testNode(&data->allNodesList()[mi]))
        return true;
    return false;
  }
  for (uint16_t mi : meshNodes())
    if (testNode(&data->allNodesList()[mi]))
      return true;
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

  const auto testNode = [&](const CollisionNode *meshNode) -> bool {
    if (!inst->checkNodeBehaviorFlags(meshNode->nodeIndex, CollisionNode::TRACEABLE))
      return false;
    if (!inst->isNodeEnabled(meshNode->nodeIndex))
      return false;
    return test_capsule_node_intersection(p0, p1, radius, meshNode, *inst);
  };
  // TLAS candidates, same contract as the sphere arm (the annotated one).
  CollResTlasCandidates cands;
  if (tlasBoxCandidates(*inst, bbox, cands))
  {
    for (uint16_t mi : cands)
      if (testNode(&data->allNodesList()[mi]))
        return true;
    return false;
  }
  for (uint16_t mi : meshNodes())
    if (testNode(&data->allNodesList()[mi]))
      return true;
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

  const auto testNode = [&](const CollisionNode *meshNode) -> bool {
    if (!meshNode->hasGeometry() || !checkNodeBehaviorFlags(meshNode->nodeIndex, CollisionNode::TRACEABLE))
      return false;
    // same structural contract as every other trace: disabled or singular-posed nodes never test
    if (!defaultInstance.isNodeEnabled(meshNode->nodeIndex) || !defaultInstance.isNodeTraceable(meshNode->nodeIndex))
      return false;
    mat44f nodeTm, nodeItm;
    v_mat44_make_from_43cu_unsafe(nodeTm, defaultInstance.getNodeTmRef(meshNode->nodeIndex).array);
    bbox3f nodeBox;
    v_bbox3_init(nodeBox, nodeTm, v_ldu_bbox3(meshNode->modelBBox));
    if (!v_bbox3_test_box_intersect(nodeBox, bbox))
      return false;
    v_mat44_inverse43(nodeItm, nodeTm);
    vec3f localA00 = v_mat44_mul_vec3p(nodeItm, a00);
    vec3f localA01 = v_mat44_mul_vec3p(nodeItm, a01);
    vec3f localA10 = v_mat44_mul_vec3p(nodeItm, a10);
    vec3f localA11 = v_mat44_mul_vec3p(nodeItm, a11);

    vec3f localQuadUnnormal = v_cross3(v_sub(localA10, localA00), v_sub(localA11, localA10));
    if (v_extract_x(v_length3_sq_x(localQuadUnnormal)) < 1.0e-3f)
      return false; // a degenerate quad in this node's frame tests nowhere; the walk continues
    vec3f localQuadNormal = v_norm3(localQuadUnnormal);
    vec3f moveDirUnnorm = v_cross3(v_sub(localA11, localA10), localQuadNormal);

    // Walk the node's quad-BLAS filtered by the quad's node-local AABB, so only candidate leaves are
    // tested, instead of materialising the node and brute-forcing every face.
    bbox3f localQuadBox;
    v_bbox3_init(localQuadBox, localA00);
    v_bbox3_add_pt(localQuadBox, localA01);
    v_bbox3_add_pt(localQuadBox, localA10);
    v_bbox3_add_pt(localQuadBox, localA11);

    return walkNodeTrisInLocalBox(nodeChunkPtr(*meshNode), localQuadBox.bmin, localQuadBox.bmax,
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
      });
  };
  // TLAS candidates, same contract as the sphere arm (the annotated one).
  CollResTlasCandidates cands;
  if (tlasBoxCandidates(defaultInstance, bbox, cands))
  {
    for (uint16_t mi : cands)
      if (testNode(&data->allNodesList()[mi]))
        return true;
    return false;
  }
  for (uint16_t mi : meshNodes())
    if (testNode(&data->allNodesList()[mi]))
      return true;
  return false;
}


DAGOR_NOINLINE bool CollisionResource::traceCapsuleMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
  const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm,
  vec4f &v_out_pos) const
{
  bool ret = false;
  const uint32_t *__restrict indices = idx_base;
  const Point3_vec4 *__restrict vertices = verts_base;
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


DAGOR_NOINLINE bool CollisionResource::capsuleHitMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
  const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, float radius) const
{
  const uint32_t *__restrict indices = idx_base;
  const Point3_vec4 *__restrict vertices = verts_base;
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
// accept_leaf (CollisionResource::leaf_accept_t, spelled out because that one is private; null = accept all) drops whole leaves before
// any triangle math.
template <class TriCb>
static inline void walk_capsule_node_chunk(const uint8_t *chunk, vec4f v_local_from, vec4f v_local_dir, const float *seg_t,
  const float *radius, bool (*accept_leaf)(void *, soa4::LeafRef), void *accept_ctx, TriCb per_tri)
{
  const NodeChunkFrame fr = decode_node_chunk_frame(chunk);
  const vec3f lTo = v_madd(v_local_dir, v_splats(*seg_t), v_local_from);
  const bbox3f swept =
    padded_q_box(fr, v_sub(v_min(v_local_from, lTo), v_splats(*radius)), v_add(v_max(v_local_from, lTo), v_splats(*radius)));
  soa4::iterateFilteredVerts(
    fr.tree, fr.rootRef, [&](vec3f bmn, vec3f bmx) { return q_box_overlaps(swept, bmn, bmx); },
    [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef ref, int) -> bool {
      if (DAGOR_UNLIKELY(accept_leaf != nullptr) && !accept_leaf(accept_ctx, ref))
        return false;
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
  const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos, leaf_accept_t accept_leaf,
  void *accept_ctx) const
{
  bool ret = false;
  walk_capsule_node_chunk(nodeChunkPtr(node), v_local_from, v_local_dir, &in_out_t, &radius, accept_leaf, accept_ctx,
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
  const vec4f &v_local_dir, float in_t, float radius, leaf_accept_t accept_leaf, void *accept_ctx) const
{
  bool hit = false;
  walk_capsule_node_chunk(nodeChunkPtr(node), v_local_from, v_local_dir, &in_t, &radius, accept_leaf, accept_ctx,
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
  const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm, leaf_accept_t accept_leaf, void *accept_ctx) const
{
  // Mirrors the traceChunkRay(any_hit=false) lambda in forEachIntersectedNode: node-local ray into the
  // chunk's q-space (unnormalized scaled direction, so parametric t carries over), CCW cull.
  const NodeChunkFrame fr = decode_node_chunk_frame(nodeChunkPtr(node));
  RayData rd;
  rd.data = fr.tree;
  rd.rayOrigin = v_madd(v_local_from, fr.scale, fr.qOfs);
  rd.rayDir = v_mul(v_local_dir, fr.scale);
  rd.t = in_out_t;
  rd.calc();
  // The OOL takes an accept either way, so a caller's leaf gate rides the call the accept-all lambda already made.
  const collision_blas::LeafAcceptRef acceptAll = [](void *, soa4::LeafRef) -> bool { return true; };
  soa4::LeafRef bestRef = 0;
  int bestSub = 0;
  if (!collision_blas::raySoa4ClosestFilteredOOLCullCCW(rd, fr.rootRef, accept_leaf ? accept_leaf : acceptAll,
        accept_leaf ? accept_ctx : nullptr, bestRef, bestSub))
    return false;
  in_out_t = rd.t; // q-space t == node-local t (unnormalized scaled direction)
  if (v_out_norm)
  {
    vec3f a, b, c;
    soa4::fetchLeafTri(fr.tree, bestRef, bestSub, fr.unq, a, b, c);
    // unnormalized face cross product (the walk's normal contract)
    *v_out_norm = v_cross3(v_sub(b, a), v_sub(c, a));
  }
  return true;
}

DAGOR_NOINLINE bool CollisionResource::traceAllHitsNodeChunk(const CollisionNode &node, const vec4f &v_local_from,
  const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, bool force_cull, all_collres_hits_t &ret_hits,
  leaf_accept_t accept_leaf, void *accept_ctx) const
{
  const NodeChunkFrame fr = decode_node_chunk_frame(nodeChunkPtr(node));
  const uint8_t *tree = fr.tree;
  const Vert21MetricLoader unq = fr.unq;
  const vec3f scale = fr.scale;
  const vec3f qOfs = fr.qOfs;
  // Slab-prune the descent by the ray (t is scale-invariant in q-space, so in_t bounds both
  // frames); a segment-AABB filter would visit nearly every leaf of a long diagonal ray.
  const BlasBoxRay bRay = BlasBoxRay::make(v_local_from, v_local_dir, scale, qOfs);
  // SOLID arrives in force_no_cull (live flags)
  const bool noCull = !force_cull && (force_no_cull || (node.flags & CollisionNode::TRACE_TWO_SIDED));
  bool any = false;
  soa4::iterateFiltered(
    tree, fr.rootRef, [in_t, &bRay](vec3f bmn, vec3f bmx) { return bRay.admits(bmn, bmx, in_t); },
    [&](vec3f v0, vec3f v1, vec3f v2, soa4::LeafRef ref, int subTri, int) -> bool {
      // Same per-tri predicate as traceRayMeshNodeLocalAllHits (shared 4-wide kernel);
      // no t-pruning -- every crossing within [0, in_t].
      // The leaf accept runs AFTER the triangle test: per hit, not per overlapping sub-triangle (each quad leaf is visited twice).
      float t = in_t;
      if (traceray1Triangle(v_local_from, v_local_dir, t, v0, v1, v2, noCull))
      {
        if (DAGOR_UNLIKELY(accept_leaf != nullptr) && !accept_leaf(accept_ctx, ref))
          return false;
        const vec3f vNorm = calc_normal ? v_cross3(v_sub(v1, v0), v_sub(v2, v0)) : v_zero();
        ret_hits.push_back(
          {v_perm_xyzd(vNorm, v_splats(t)), tri_ref::make_node_blas(node.nodeIndex, ref, (uint32_t)subTri, data->nodeBlasBuildId)});
        any = true;
      }
      return false; // collect across all leaves
    },
    unq);
  return any;
}

// Decode a node's per-face triangle into resource-local vert positions: walk the node's chunk
// leaves to the face_idx-th sub-triangle and decode from its vert21 block. For low-frequency
// callers (aiTargetES random face, damage-debug, AssetViewer); hot per-face paths should use
// iterateNodeFacesVerts.
bool CollisionResource::getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n || face_idx < 0 || (uint32_t)(face_idx * 3 + 2) >= n->indicesCount)
    return false;
  const PackedVerts21 p = getPackedNodeVerts21(*n);
  bool found = false;
  walkNodeChunkLeavesForFaces(*n, [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) {
    if (found || fi != face_idx)
      return;
    v_stu_p3(&v0.x, p.vert(i0));
    v_stu_p3(&v1.x, p.vert(i1));
    v_stu_p3(&v2.x, p.vert(i2));
    found = true;
  });
  return found;
}

// The leaf's own material behind a hit, addressed like getNodeFaceVertsByRef below (same ref kinds and token gates) but reading only
// the user bits, which index the OWNING node's set.
int CollisionResource::getLeafPhysMat(tri_ref_t ref) const
{
  if (!tri_ref::hasTri(ref))
    return PHYSMAT_INVALID;
  const CollisionNode *n = getNode((int)tri_ref::nodeIndex(ref));
  if (!n)
    return PHYSMAT_INVALID;
  if (tri_ref::isNodeBlas(ref))
  {
    if (n->nodeBlasOfs == ~0u)
      return PHYSMAT_INVALID;
    // Same gate as getNodeFaceVertsByRef: a forged generation-zero ref would otherwise read a
    // stale-but-in-bounds material through this node's leaf.
    if (tri_ref::nodeBlasGeneration(ref) != (data->nodeBlasBuildId & ((1u << tri_ref::NODE_BLAS_GEN_BITS) - 1u)))
      return PHYSMAT_INVALID;
    const NodeChunkFrame fr = decode_node_chunk_frame(nodeChunkPtr(*n));
    soa4::LeafLoc l;
#if VALIDATE_TRI_REF_TOKENS
    if (!soa4::validateLeafToken(fr.tree, (uint32_t)fr.treeBytes, tri_ref::nodeBlasToken(ref), l))
      return PHYSMAT_INVALID; // stale/forged token
#else
    l = soa4::decodeLeafRef(fr.tree, (soa4::LeafRef)tri_ref::nodeBlasToken(ref));
#endif
    return getNodePhysMatId(n->nodeIndex, (int)soa4::leafUserBits(fr.tree, l));
  }
  // source-face refs and stale grid-era refs: no leaf to read the bits from
  return PHYSMAT_INVALID;
}

// Decode a tri_ref_t back to the three source-triangle vertices.
// - BLAS refs (type=1): walk the node chunk's quad leaf at the token, pick the sub-triangle
//   per the sub-tri index (0..3). No side table -- the leaf carries enough on its own.
// - Non-BLAS refs (type=0): dispatch to getNodeFaceVerts via the encoded per-node srcFace.
// Returns false for non-tri refs, missing geometry and stale node-chunk generations. Leaf tokens
// are trusted (correct by construction); only VALIDATE_TRI_REF_TOKENS builds reject a forged one.
bool CollisionResource::getNodeFaceVertsByRef(tri_ref_t ref, Point3 &v0, Point3 &v1, Point3 &v2) const
{
  if (!tri_ref::hasTri(ref))
    return false;
  if (tri_ref::isNodeBlas(ref))
  {
    // Per-node chunk hit: quad-leaf decode addressed from the node's chunk tree, dequantized
    // through the embedded block's exact frame.
    const CollisionNode *n = getNode((int)tri_ref::nodeIndex(ref));
    if (!n || n->nodeBlasOfs == ~0u)
      return false;
    // Reject a forged generation-zero ref: its leaf offset would otherwise decode
    // stale-but-in-bounds geometry. Generation is the low NODE_BLAS_GEN_BITS, so compare
    // data->nodeBlasBuildId in that width.
    if (tri_ref::nodeBlasGeneration(ref) != (data->nodeBlasBuildId & ((1u << tri_ref::NODE_BLAS_GEN_BITS) - 1u)))
      return false;
    const NodeChunkFrame fr = decode_node_chunk_frame(nodeChunkPtr(*n));
    soa4::LeafLoc l;
#if VALIDATE_TRI_REF_TOKENS
    if (!soa4::validateLeafToken(fr.tree, (uint32_t)fr.treeBytes, tri_ref::nodeBlasToken(ref), l))
      return false; // stale/forged token
    const QuadLeafFields f = soa4::leafFields(fr.tree, l);
    if (!soa4::leafEmitsSubTri(f, (uint32_t)tri_ref::subTriIndex(ref)))
      return false; // lane this leaf does not emit: stale/forged token
    const uint32_t chunkVertsOfs = alignVert21StreamOfs((uint32_t)fr.treeBytes);
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
  // A retired grid-era ref answers faceIndex() == -1 and fails in getNodeFaceVerts.
  return getNodeFaceVerts((int)tri_ref::nodeIndex(ref), (int)tri_ref::faceIndex(ref), v0, v1, v2);
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

  // One per-node test dispatching by type, so the TLAS candidate arm and the per-type list walks
  // apply identical rules; each type keeps its stored-geometry-frame probe.
  const auto testNode = [&](const CollisionNode *node) {
    if (!nodeTestable(node->nodeIndex))
      return;
    switch (node->type)
    {
      case COLLISION_NODE_TYPE_MESH:
      case COLLISION_NODE_TYPE_CONVEX:
        if (!node->hasGeometry())
          return; // degenerate-dropped node: no collision surface, its modelBBox is stale
        [[fallthrough]];
      case COLLISION_NODE_TYPE_BOX:
        if (node->modelBBox & (invInstNodeTm(*inst, node->nodeIndex) * pos))
          ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(node->nodeIndex);
        return;
      case COLLISION_NODE_TYPE_SPHERE:
        if (
          lengthSq(invInstNodeTm(*inst, node->nodeIndex) * pos - node->bsphereCenter()) <= get_bsphere_r2(node->radiusAroundBoxCenter))
          ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(node->nodeIndex);
        return;
      case COLLISION_NODE_TYPE_CAPSULE:
        if (data->capsules()[node->capsuleIndex].isInside(invInstNodeTm(*inst, node->nodeIndex) * pos))
          ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(node->nodeIndex);
        return;
      default: return; // POINTS: never an inclusion target
    }
  };

  // TLAS arm: the dispatch's candidate set covers every leaf-bearing type, so one walk replaces
  // the four per-type list walks; the collected set is identical, its order is not (callers read
  // the list as a set). The list walks stay the fallback.
  CollResTlasCandidates cands;
  bbox3f posBox;
  posBox.bmin = posBox.bmax = vPos;
  if (tlasBoxCandidates(*inst, posBox, cands))
  {
    for (uint16_t ni : cands)
      testNode(&data->allNodesList()[ni]);
    return !intersected_nodes_list.empty();
  }
  for (uint16_t mi : meshNodes())
    testNode(&data->allNodesList()[mi]);
  // Pull primitive probes into their stored geometry frames.
  for (uint16_t bi : boxNodes())
    testNode(&data->allNodesList()[bi]);
  for (uint16_t si : sphereNodes())
    testNode(&data->allNodesList()[si]);
  for (uint16_t ci : capsuleNodes())
    testNode(&data->allNodesList()[ci]);

  return !intersected_nodes_list.empty();
}

bool CollisionResource::calcOffsetForIntersection(const TMatrix &tm1, const CollisionNode &node_to_move,
  const CollisionNode &node_to_check, Point3 &offset) const
{
  offset.zero();

  G_ASSERTF(node_to_check.type == COLLISION_NODE_TYPE_CONVEX, "final node #%u is not of convex type, only convex is supported",
    (unsigned)node_to_check.nodeIndex);
  TMatrix finalTm = invNodeTm(node_to_check.nodeIndex) * tm1;
  const plane3f *checkPlanes = data->convexPlanes().data() + node_to_check.planesOfs;
  const int checkPlanesCount = node_to_check.planesCount;
  const int numIterations = 5;
  vec4f vOffset = v_zero();
  if (node_to_move.type == COLLISION_NODE_TYPE_BOX)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    bbox3f modelBox = v_ldu_bbox3(node_to_move.modelBBox);
    carray<vec4f, 8> boundPoints;
    for (int i = 0; i < boundPoints.size(); ++i)
      boundPoints[i] = v_mat44_mul_vec3p(vFinalTm, v_bbox3_point(modelBox, i));
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
    BSphere3 localSph = finalTm * make_node_bsphere(node_to_move.bsphereCenter(), node_to_move.radiusAroundBoxCenter);
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
    // Decode vert21 via iterateNodeVerts: the resource holds no raw Point3_vec4 array (verts
    // live in the node's chunk vert21 block).
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
  const plane3f *checkPlanes = data->convexPlanes().data() + node_to_check.planesOfs;
  const int checkPlanesCount = node_to_check.planesCount;
  const int numIterations = 5;
  vec4f zero = v_zero();
  vec4f vOffset = v_zero();
  vec4f axis_v = v_ldu(&axis.x);
  if (node_to_move.type == COLLISION_NODE_TYPE_BOX)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    bbox3f modelBox = v_ldu_bbox3(node_to_move.modelBBox);
    carray<vec4f, 8> boundPoints;
    for (int i = 0; i < boundPoints.size(); ++i)
      boundPoints[i] = v_mat44_mul_vec3p(vFinalTm, v_bbox3_point(modelBox, i));
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
    BSphere3 localSph = finalTm * make_node_bsphere(node_to_move.bsphereCenter(), node_to_move.radiusAroundBoxCenter);
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
  if (node_to_test->radiusAroundBoxCenter < 0.f)
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
  if (node_to_test->radiusAroundBoxCenter < 0.f)
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
  TMatrix finalTm = collres_inverse(tm_restrain) * testTm;
  if (node_to_test->type == COLLISION_NODE_TYPE_BOX)
  {
    mat44f vFinalTm;
    v_mat44_make_from_43cu_unsafe(vFinalTm, finalTm.array);
    bbox3f modelBox = v_ldu_bbox3(node_to_test->modelBBox);
    for (int i = 0; i < 8; ++i)
    {
      vec4f vert = v_mat44_mul_vec3p(vFinalTm, v_bbox3_point(modelBox, i));
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
      make_node_bsphere(finalTm * node_to_test->bsphereCenter(), node_to_test->radiusAroundBoxCenter * mat33_spectral_norm(vFinalTm));
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
  int &out_mat_id, uint8_t behavior_filter, TraceTmCache *tm_cache) const
{
  auto nodeFilter = [](const CollisionNode *) -> bool { return true; }; // the core applies behavior_filter
  auto callback = [&](int /*trace_id*/, const CollisionNode *, float /*t*/, vec3f /*normal*/, vec3f /*pos*/, tri_ref_t tri_ref)
                    DAGOR_FORCE_INLINE_LAMBDA { out_mat_id = getHitPhysMat(tri_ref); };

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT,
    false /*pose_may_refresh: the default instance is never tree-backed*/>(tm, defaultInstance, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    false /*out_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/,
    ray_mat_id, tm_cache);
}

DAGOR_NOINLINE bool CollisionResource::rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *collision_node_mask, int *out_mat_id,
  TraceTmCache *tm_cache) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  return CollisionResourceTraceAdapter::rayHit(*this, tm, CollisionResourceTraceAdapter::pose(*this, geom_node_tree), from, dir, in_t,
    bsphere_scale, collision_node_mask, out_mat_id, tm_cache);
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
  stampGeomNodeIds(geom_node_tree, nullptr);
  geomNodeTreeBound = true;
}

CollisionResource *CollisionResource::deepCopy(void *inplace_ptr) const
{
  auto *collRes =
    new (inplace_ptr ? inplace_ptr : midmem->alloc(sizeof(CollisionResource)), _NEW_INPLACE) CollisionResource(data.get());
  collRes->vFullBBox = vFullBBox;
  collRes->vBoundingSphere = vBoundingSphere;
  collRes->vBindTraceSphere = vBindTraceSphere;
  collRes->boundingBox = boundingBox;
  collRes->setBoundingSphereRad(boundingSphereRad);
  collRes->collisionFlags = collisionFlags;
  collRes->bsphereCenterNode = bsphereCenterNode;
  collRes->geomNodeTreeBound = geomNodeTreeBound;
  if (const uint32_t *words = interlocked_acquire_load_ptr(liveBoundWords))
  {
    collRes->liveBoundWords = new uint32_t[liveBoundWordCount()];
    for (uint32_t i = 0; i < liveBoundWordCount(); ++i)
      collRes->liveBoundWords[i] = interlocked_relaxed_load(words[i]);
  }
#if DAGOR_DBGLEVEL > 0
  collRes->gameResId = gameResId;
#endif

  // Preserve the source's current pose.
  collRes->defaultInstance.nodeTm = defaultInstance.nodeTm;
  collRes->defaultInstance.poseMeta = defaultInstance.poseMeta; // status bits (hides) travel with the pose
  collRes->defaultInstance.rootBBox = defaultInstance.rootBBox;
  collRes->defaultInstance.posedSinceBind = defaultInstance.posedSinceBind;
  collRes->defaultInstance.bsphereCenterLocal = defaultInstance.bsphereCenterLocal;
  collRes->defaultInstance.hasBsphereCenterLocal = defaultInstance.hasBsphereCenterLocal;
  // Same gate as seedPose: owned instances have no read-side refresh, so the copy refits here
  // (not memcpy - the source clone may be mid-generation), and a posed-but-DECLINED source has
  // EMPTY clone bytes - the refit re-derives the envelope and the decline latch on the copy.
  if (collRes->defaultInstance.posedSinceBind && collRes->hasAllNodesTLAS())
    collRes->defaultInstance.refitAllTlasLeaves();
  return collRes;
}

bool CollisionResource::ownsData() const { return interlocked_acquire_load(data->refCnt) == 1; }

bool CollisionResource::setNodeBehaviorFlags(int node_index, uint16_t flags)
{
  if ((uint32_t)node_index >= defaultInstance.metaCount())
    return false;
  defaultInstance.poseMeta[node_index].behaviorFlags = flags;
  interlocked_increment(liveFlagsGen); // release-publish AFTER the store: see liveFlagsGen
  return true;
}

bool CollisionResource::setNodeGeomNodeId(int node_index, dag::Index16 id)
{
  if (node_index < 0 || node_index >= (int)data->allNodesList().size())
    return false;
  if (!ownsData())
  {
    logerr("collres: setNodeGeomNodeId refused: the immutable Data block is shared (refcount %d)",
      interlocked_acquire_load(data->refCnt));
    return false;
  }
  data->allNodesList()[node_index].geomNodeId = id;
  markLiveBoundNode(node_index);
  return true;
}

void CollisionResource::markLiveBoundNode(uint32_t node_index)
{
  uint32_t *words = interlocked_acquire_load_ptr(liveBoundWords);
  if (!words)
  {
    uint32_t *fresh = new uint32_t[liveBoundWordCount()]();
    if ((words = interlocked_compare_exchange_ptr(liveBoundWords, fresh, (uint32_t *)nullptr)) != nullptr)
      delete[] fresh;
    else
      words = fresh;
  }
  interlocked_or(words[node_index / 32], 1u << (node_index % 32));
}

// The claim is a claim, not a first valid id: a disjoint tree that stamps nothing still
// claims the block. An unshared block (the cached resource with several object-level
// consumers) keeps the older contract, one layout per object, last bind wins - the
// violation the mimic note above admits.
void CollisionResource::stampGeomNodeIds(const GeomNodeTree &geom_node_tree, const char *name_prefix)
{
  // One open-addressed table over the tree's names per bind (a slot holds tree index + 1): a hash and
  // a strcmp per node instead of findNodeIndex's strcmp scan. Linear probing in index order keeps
  // findNodeIndex's first-match rule for a name the tree carries twice.
  const uint32_t treeNodes = geom_node_tree.nodeCount();
  uint32_t mask = 16;
  while (mask < 2 * treeNodes)
    mask *= 2;
  --mask;
  dag::Vector<uint32_t, framemem_allocator> table(mask + 1, 0u);
  for (uint32_t i = 0; i < treeNodes; ++i)
    for (uint32_t h = str_hash_fnv1a(geom_node_tree.getNodeName(dag::Index16(i))) & mask;; h = (h + 1) & mask)
      if (!table[h])
      {
        table[h] = i + 1;
        break;
      }
  auto lookup = [&](const CollisionNode &node) -> dag::Index16 {
    const char *name = getNodeNameStr(node);
    char fullNameBuff[128];
    if (name_prefix)
    {
      snprintf(fullNameBuff, sizeof(fullNameBuff), "%s%s", name_prefix, name);
      name = fullNameBuff;
    }
    for (uint32_t h = str_hash_fnv1a(name) & mask;; h = (h + 1) & mask)
    {
      if (!table[h])
        return {};
      if (strcmp(geom_node_tree.getNodeName(dag::Index16(table[h] - 1)), name) == 0)
        return dag::Index16(table[h] - 1);
    }
  };
  auto markScaledBindNodes = [&] {
    dag::ConstSpan<CollisionNode> nodes = getAllNodes();
    for (uint16_t i : meshNodes())
    {
      if (isLiveBoundNode(i) || nodes[i].geomNodeId.index() >= geom_node_tree.nodeCount())
        continue;
      const mat44f &wtm = geom_node_tree.getNodeWtmRel(nodes[i].geomNodeId);
      vec3f gramDiag, gramOff;
      v_mat33_gram(gramDiag, gramOff, wtm.col0, wtm.col1, wtm.col2);
      if (!collres_gram_in_band(gramDiag, gramOff, COLLRES_RIGID_GRAM_EPS))
        markLiveBoundNode(i);
    }
  };
  if (!ownsData() && data->treeLayoutClaimed)
  {
    bool anyChange = false;
    for (auto &node : data->allNodesList())
      if ((anyChange = lookup(node) != node.geomNodeId))
        break;
    if (!anyChange) // the multi-entity steady state: skip the value-identical stores
      return markScaledBindNodes();
    data = Data::clone(*data); // a private copy on the SAME resource: live instance pointers stay valid
  }
  data->treeLayoutClaimed = true;
  for (auto &node : data->allNodesList())
    node.geomNodeId = lookup(node);
  markScaledBindNodes();
}

uint8_t CollisionResource::classifyNodeTmFlags(mat44f_cref tm, float &out_max_scale)
{
  uint8_t flags = 0;
  float dot01 = v_extract_x(v_dot3_x(tm.col0, tm.col1));
  float dot02 = v_extract_x(v_dot3_x(tm.col0, tm.col2));
  float dot12 = v_extract_x(v_dot3_x(tm.col1, tm.col2));
  vec3f lenSq = v_mat44_scale43_sq(tm);
  float len0sq = v_extract_x(lenSq);
  float len1sq = v_extract_y(lenSq);
  float len2sq = v_extract_z(lenSq);
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
    vec3f c = node.vBsphereCenter();
    vec3f r = v_splats(node.radiusAroundBoxCenter);
    box.bmin = v_sub(c, r);
    box.bmax = v_add(c, r);
  }
  else if (node.type == COLLISION_NODE_TYPE_CAPSULE)
  {
    const Capsule &c = data->capsules()[node.capsuleIndex];
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
  if (n->radiusAroundBoxCenter < 0 || n->type == COLLISION_NODE_TYPE_POINTS ||
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

  G_ASSERT((uint32_t)node_index < data->authoredNodeTm().size());
  mat44f authored;
  v_mat44_make_from_43cu_unsafe(authored, data->authoredNodeTm()[node_index].array);
  if (v_mat44_is_equal43(posed, authored))
  {
    mat44f identity;
    v_mat44_ident(identity);
    return identity;
  }
  mat44f invAuthored, geometryTm;
  if (DAGOR_LIKELY((uint32_t)node_index < data->authoredNodeItm().size()))
    v_mat44_make_from_43cu_unsafe(invAuthored, data->authoredNodeItm()[node_index].array);
  else
    v_mat44_inverse43(invAuthored, authored);
  v_mat44_mul43(geometryTm, posed, invAuthored);
  return geometryTm;
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
  inst.tlasDeclined = false; // full-refit boundary: a re-seeded pose re-evaluates the envelope
  inst.tree = tree;
  inst.poseGeneration = 0;
  // A DIFFERENT tree can carry the same numeric generation (they start at 1): without this reset
  // the lazy TLAS refresh would treat the old clone's refit as current and never re-run.
  inst.tlasPoseGeneration = 0;
  inst.tlasDefaultPoseGenSeen = 0;
  inst.tlasBlueprintGenSeen = 0;
  inst.treeNodeCountAtBind = tree ? (uint32_t)tree->nodeCount() : 0;
  inst.layoutDriftAsserted = false; // a rebind re-arms the once-per-binding drift assert
  // The clone holds leaf boxes for the pose being replaced. A tree-backed instance would re-refit on
  // the next trace (poseGeneration 0), but the owned form has no refresh hook, so drop it here and
  // let the next pose write or full refit rebuild it.
  inst.tlas.data.clear();
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
  if (n->radiusAroundBoxCenter < 0)
    return n->modelBBox;
  // A non-composable pose (non-finite or inverse-overflowing) must not compose: NaN/Inf bounds
  // would leak. A finite mirrored pose is hidden from traces but composes fine.
  if (DAGOR_UNLIKELY(!defaultInstance.isNodeComposable(node_id)))
    return n->modelBBox;
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, defaultInstance.getNodeGeometryTm(node_id));
  if (n->type == COLLISION_NODE_TYPE_SPHERE) // exact: corner-mapping would inflate under rotation
    return composed_sphere_box(tm, n->bsphereCenter(), n->radiusAroundBoxCenter);
  BBox3 out;
  for (int k = 0; k < 8; k++)
    out += tm * n->modelBBox.point(k);
  return out;
}

BSphere3 CollisionResource::getNodeBSphere(int node_id) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n || n->radiusAroundBoxCenter < 0)
    return BSphere3(); // empty: r = r2 = -1
  // A RETAINED bake is a valid poseable sphere: it composes through the compatibility
  // transform like the bbox accessor, only a SINGULAR bake stays pinned at its baked frame.
  if (n->type != COLLISION_NODE_TYPE_SPHERE || (defaultInstance.poseMeta[node_id].flags & CollisionNode::IDENT) ||
      (defaultInstance.poseMeta[node_id].isGeometryBaked() && !defaultInstance.poseMeta[node_id].isRetainedBake()))
    return BSphere3(n->bsphereCenter(), n->radiusAroundBoxCenter);
  // A non-composable pose (non-finite or inverse-overflowing) must not compose (NaN/Inf
  // center); a finite mirrored pose is hidden from traces but composes fine.
  if (DAGOR_UNLIKELY(!defaultInstance.isNodeComposable(node_id)))
    return BSphere3(n->bsphereCenter(), n->radiusAroundBoxCenter);
  TMatrix tm;
  v_mat_43cu_from_mat44(tm.array, defaultInstance.getNodeGeometryTm(node_id));
  return BSphere3(tm * n->bsphereCenter(), n->radiusAroundBoxCenter * defaultInstance.poseMeta[node_id].maxTmScale);
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
  return collres_inverse(data->authoredNodeTm()[node_id]);
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
    ret = collres_inverse(tm);
  return ret;
}

const CollisionResourceInstance &CollisionResource::instanceOrDefaultFallback(const CollisionResourceInstance *instance) const
{
  if (DAGOR_UNLIKELY(instance && instance->getResource()))
  {
    G_ASSERTF(false, "collision instance of res %p <%s> used with res %p <%s>", instance->getResource(),
      instance->getResource()->getGameResName().str(), this, getGameResName().str());
    LOGERR_ONCE("collision instance of res %p <%s> used with res %p <%s>; using the current pose", instance->getResource(),
      instance->getResource()->getGameResName().str(), this, getGameResName().str());
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
    inst = &defaultInstance;
  }
  inst->mirrorLiveFlagsIfStale(); // secondary queries answer by the live flags, like traces
  return inst;
}

static int collres_node_list(CollisionResourceNodeType type)
{
  return collres_is_mesh_list_node(type) ? COLLISION_NODE_TYPE_MESH : (int)type;
}

void CollisionResource::rebuildNodeLists()
{
  // A counting sort by list keeps each group in node order. A degenerate-dropped node
  // (indicesCount == 0) stays listed and indexable: the walks early-out on indicesCount.
  dag::Span<CollisionNode> nodes = data->allNodesList();
  uint16_t count[NUM_COLLISION_NODE_TYPES] = {};
  for (size_t nodeNo = 0; nodeNo < nodes.size(); nodeNo++)
  {
    nodes[nodeNo].nodeIndex = (uint16_t)nodeNo;
    count[collres_node_list(nodes[nodeNo].type)]++;
  }
  dag::Span<uint16_t> order = data->nodeOrder();
  if (order.empty())
    return; // a node-less resource: every list is empty, and nodeList() answers that from the array
  G_ASSERT(order.size() == Data::LIST_BOUNDS + nodes.size());
  order[0] = 0;
  for (int list = 0; list < NUM_COLLISION_NODE_TYPES; list++)
    order[list + 1] = order[list] + count[list];
  uint16_t next[NUM_COLLISION_NODE_TYPES];
  for (int list = 0; list < NUM_COLLISION_NODE_TYPES; list++)
    next[list] = order[list];
  for (size_t nodeNo = 0; nodeNo < nodes.size(); nodeNo++)
    order[Data::LIST_BOUNDS + next[collres_node_list(nodes[nodeNo].type)]++] = (uint16_t)nodeNo;
}

struct ITestIntersectionAlgo
{
  // a_list / b_list are the per-type node index slices into a_all / b_all.
  // Resources select geometry storage; instances supply placements.
  virtual bool apply(dag::ConstSpan<uint16_t> a_list, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a,
    dag::ConstSpan<uint16_t> b_list, dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale,
    bool checkOnlyPhysNodes, const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource *res_b,
    const CollisionResourceInstance *inst_b) = 0;
  virtual ~ITestIntersectionAlgo() = 0;

  // list is always the arm's type list or its subset
  struct InnerNodes
  {
    CollResTlasCandidates cands;
    dag::ConstSpan<uint16_t> list;
    bool dense = false; // latched for the rest of apply()
  };
  // below this, the list beats pruning
  static constexpr uint32_t MIN_PAIR_TLAS_LIST = 8;

  static void collectInner(InnerNodes &out, const CollisionResource *res, const CollisionResourceInstance *inst, bbox3f_cref query_box,
    int list_ix)
  {
    const dag::ConstSpan<uint16_t> type_list = res->nodeList(list_ix);
    out.list = type_list;
    if (out.dense || type_list.size() < MIN_PAIR_TLAS_LIST || !res->tlasBoxCandidates(*inst, query_box, out.cands))
      return;
    uint32_t kept = 0;
    for (uint16_t ni : out.cands)
      if (collres_node_list(res->getAllNodes()[ni].type) == list_ix)
        out.cands[kept++] = ni;
    out.cands.resize(kept);
    // the trace dispatch's density rule
    out.dense = kept * 2u >= (uint32_t)type_list.size();
    if (out.dense)
      return;
    out.list = dag::ConstSpan<uint16_t>(out.cands.data(), out.cands.size());
  }

  // decoded verts overhang modelBBox by a q-cell
  static bbox3f paddedMeshNodeBox(const BBox3 &model_box)
  {
    bbox3f box = v_ldu_bbox3(model_box);
    v_bbox3_extend(box, v_mul(v_bbox3_size(box), v_splats(1.f / 65535.f)));
    return box;
  }

  // far placements round past the quant pad
  static vec4f placementRoundingMag(mat44f_cref world_to_frame, mat44f_cref tm_a, mat44f_cref tm_b)
  {
    const vec4f rowAbs = v_add(v_add(v_abs(world_to_frame.col0), v_abs(world_to_frame.col1)), v_abs(world_to_frame.col2));
    const vec4f worldMag = v_add(v_hmax3(v_abs(tm_a.col3)), v_hmax3(v_abs(tm_b.col3)));
    return v_add(v_mul(v_hmax3(rowAbs), worldMag), v_hmax3(v_abs(world_to_frame.col3)));
  }
  static bbox3f nodeQueryBox(mat44f_cref node_to_frame, bbox3f_cref node_box, vec4f placement_mag)
  {
    bbox3f box;
    v_bbox3_init(box, node_to_frame, node_box);
    const vec4f rowAbs = v_add(v_add(v_abs(node_to_frame.col0), v_abs(node_to_frame.col1)), v_abs(node_to_frame.col2));
    const vec4f nodeMag = v_max(v_hmax3(v_abs(node_box.bmin)), v_hmax3(v_abs(node_box.bmax)));
    const vec4f outMag = v_max(v_hmax3(v_abs(box.bmin)), v_hmax3(v_abs(box.bmax)));
    const vec4f mag = v_add(v_add(v_mul(v_hmax3(rowAbs), nodeMag), outMag), placement_mag);
    v_bbox3_extend(box, v_mul(mag, v_splats(16.f * 1.19209290e-07f)));
    return box;
  }

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

  // Composed box/sphere-node geometry in its historical resource frame. Pair tests keep the legacy
  // conservative AABB: corners in BBox3::point order, folded from the empty BBox3() with the tie
  // rule of BBox3::operator+=, so the box equals the scalar corner mapping bit for bit.
  static bbox3f composedNodeBox(const CollisionResourceInstance *inst, int node_index, const BBox3 &local_box)
  {
    bbox3f local = v_ldu_bbox3(local_box);
    if (inst->getPoseMeta(node_index).flags & CollisionNode::IDENT)
      return local;
    mat44f tm = inst->getNodeGeometryTm(node_index);
    // Literal corner indices: from a loop counter MSVC builds the lane mask at run time.
    vec4f corners[8] = {v_bbox3_point(local, 0), v_bbox3_point(local, 1), v_bbox3_point(local, 2), v_bbox3_point(local, 3),
      v_bbox3_point(local, 4), v_bbox3_point(local, 5), v_bbox3_point(local, 6), v_bbox3_point(local, 7)};
    bbox3f out;
    out.bmin = v_splats(MAX_REAL / 4);
    out.bmax = v_splats(MIN_REAL / 4);
    for (vec4f c : corners)
    {
      // TMatrix::operator* order; v_mat44_mul_vec3p groups differently and can round differently.
      vec4f p = v_mul(tm.col0, v_splat_x(c));
      p = v_madd(tm.col1, v_splat_y(c), p);
      p = v_madd(tm.col2, v_splat_z(c), p);
      v_bbox3_add_pt(out, v_add(p, tm.col3));
    }
    return out;
  }

  Point3 collisionPointA;
  Point3 collisionPointB;
};

ITestIntersectionAlgo::~ITestIntersectionAlgo() {}

class TestMeshNodeMeshNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestMeshNodeMeshNodesIntersectionAlgo(vec4f a_wbsph, vec4f b_wbsph) : a_wbsph(a_wbsph), b_wbsph(b_wbsph) {}
  ~TestMeshNodeMeshNodesIntersectionAlgo() final = default;

  // a_list is unused: collectInner sources the same type list, pruned to B's box where it can.
  bool apply(dag::ConstSpan<uint16_t> /*a_list*/, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a,
    dag::ConstSpan<uint16_t> b_list, dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale,
    bool checkOnlyPhysNodes, const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource *res_b,
    const CollisionResourceInstance *inst_b) final
  {
    aResource = res_a;
    bResource = res_b;
    aInst = inst_a;
    bInst = inst_b;
    alignas(EA_CACHE_LINE_SIZE) mat44f vITmA;
    v_mat44_inverse43(vITmA, tm_a);
    const vec4f placementMag = placementRoundingMag(vITmA, tm_a, tm_b);
    InnerNodes innerA;

    for (uint16_t ib : b_list)
    {
      const CollisionNode *nodeB = &b_all[ib];
      if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
        continue;
      if (checkOnlyPhysNodes && !inst_b->checkNodeBehaviorFlags(nodeB->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        continue;

      mat44f vWtmB, vIWtmB;
      v_mat44_mul43(vWtmB, tm_b, inst_b->getNodeTm(nodeB->nodeIndex));
      // The transformed vector's w lane is not a radius.
      vec4f bNodeBoundingCenter = v_perm_xyzd(v_mat44_mul_vec3p(vWtmB, nodeB->vBsphereCenter()),
        v_splats(nodeB->radiusAroundBoxCenter * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale));
      if (!isBoundingsIntersect(bNodeBoundingCenter, v_set_x(b_max_scale), a_wbsph, v_set_x(a_max_scale)))
        continue;
      v_mat44_inverse43(vIWtmB, vWtmB);
      vec3f sphereCenterNodeB = v_mat44_mul_vec3p(vWtmB, nodeB->vBsphereCenter());

      {
        mat44f bToAres;
        v_mat44_mul43(bToAres, vITmA, vWtmB);
        collectInner(innerA, res_a, inst_a, nodeQueryBox(bToAres, paddedMeshNodeBox(nodeB->modelBBox), placementMag),
          COLLISION_NODE_TYPE_MESH);
      }

      for (uint16_t ia : innerA.list)
      {
        const CollisionNode *nodeA = &a_all[ia];
        if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
          continue;
        if (checkOnlyPhysNodes && !inst_a->checkNodeBehaviorFlags(nodeA->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
          continue;

        // The placement is composed where it is used. The list is B's box candidates where the
        // TLAS served them, and the whole type list where it declined, was dense or too short.
        mat44f vWtmA;
        v_mat44_mul43(vWtmA, tm_a, inst_a->getNodeTm(nodeA->nodeIndex));
        const float aNodeRad = nodeA->radiusAroundBoxCenter * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale;
        vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, nodeA->vBsphereCenter());
        if (!isBoundingsIntersect(v_perm_xyzd(sphereCenterNodeA, v_splats(aNodeRad)), v_set_x(a_max_scale), b_wbsph,
              v_set_x(b_max_scale)))
          continue;
        float sumRad =
          aNodeRad * a_max_scale + nodeB->radiusAroundBoxCenter * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        alignas(EA_CACHE_LINE_SIZE) mat44f tmAtoB;
        v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);

        // SAT overlap is frame-independent, so test in B's frame with tmAtoB, the same matrix the
        // mesh test below carries A's triangles through.
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
  // Both sides prune through their own chunk tree, so the pair costs the overlap region and not
  // the product of the two face counts: A's node box taken into B's frame selects B's candidates,
  // and the bounds of what came back, taken into A's frame, select A's.
  bool isMeshNodeIntersectedWithMeshNode(const CollisionNode *node_a, const CollisionNode *node_b, const mat44f &tm_a_to_b,
    const mat44f &v_wtm_a, const mat44f &v_wtm_b)
  {
    // pad in A's cells, before the transform
    bbox3f aBoxInB;
    v_bbox3_init(aBoxInB, tm_a_to_b, paddedMeshNodeBox(node_a->modelBBox));

    // The candidates are materialised because the A walk below re-reads them per face; the walk
    // itself would be a second chunk descent each time. Chunk pruning bounds the candidate set,
    // but within it the walk is still all-pairs, so each face carries its box for a per-pair
    // reject.
    struct CachedFace
    {
      vec4f v0, v1, v2;
      vec4f bmin, bmax;
    };
    dag::Vector<CachedFace, framemem_allocator> bFaceCache; // no reserve: the empty reject is the common answer
    bbox3f bCandBox;
    v_bbox3_init_empty(bCandBox);
    walkNodeTrisInLocalBox(bResource->nodeChunkPtr(*node_b), aBoxInB.bmin, aBoxInB.bmax, [&](vec3f b0, vec3f b1, vec3f b2) {
      vec3f fmin = v_min(b0, v_min(b1, b2));
      vec3f fmax = v_max(b0, v_max(b1, b2));
      bFaceCache.push_back({b0, b1, b2, fmin, fmax});
      v_bbox3_add_pt(bCandBox, fmin);
      v_bbox3_add_pt(bCandBox, fmax);
      return false; // collect every candidate
    });
    if (bFaceCache.empty())
      return false;

    mat44f tmBtoA;
    v_mat44_inverse43(tmBtoA, tm_a_to_b);
    bbox3f bCandBoxInA;
    v_bbox3_init(bCandBoxInA, tmBtoA, bCandBox);

    auto emitHit = [&](vec3f a0, vec3f a1, vec3f a2, vec3f b0, vec3f b1, vec3f b2) {
      vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
      vec3f bc = v_mul(v_add(b0, v_add(b1, b2)), v_splats(1 / 3.f));
      // Contacts use full world transforms.
      v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(v_wtm_a, ac));
      v_stu_p3(&collisionPointB.x, v_mat44_mul_vec3p(v_wtm_b, bc));
    };
    uint32_t aFaces = 0; // reached, not total: a hit stops the walk
    const bool hit =
      walkNodeTrisInLocalBox(aResource->nodeChunkPtr(*node_a), bCandBoxInA.bmin, bCandBoxInA.bmax, [&](vec3f a0, vec3f a1, vec3f a2) {
        aFaces++;
        vec3f a0b = v_mat44_mul_vec3p(tm_a_to_b, a0);
        vec3f a1b = v_mat44_mul_vec3p(tm_a_to_b, a1);
        vec3f a2b = v_mat44_mul_vec3p(tm_a_to_b, a2);
        vec3f aMin = v_min(a0b, v_min(a1b, a2b));
        vec3f aMax = v_max(a0b, v_max(a1b, a2b));
        for (const auto &bf : bFaceCache)
        {
          if (v_check_xyz_any_true(v_or(v_cmp_gt(aMin, bf.bmax), v_cmp_gt(bf.bmin, aMax))))
            continue;
          if (v_test_triangle_triangle_intersection(a0b, a1b, a2b, bf.v0, bf.v1, bf.v2))
          {
            emitHit(a0, a1, a2, bf.v0, bf.v1, bf.v2);
            return true;
          }
        }
        return false;
      });
#if DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
    // well past a pruned pair's cost
    constexpr uint64_t PAIR_TRI_TAG_AT = 1000;
    if ((uint64_t)aFaces * bFaceCache.size() > PAIR_TRI_TAG_AT && (::da_profiler::get_active_mode() & ::da_profiler::TAGS))
    {
      Point3 at;
      v_stu_p3(&at.x, v_mat44_mul_vec3p(v_wtm_b, v_madd(bCandBox.bmin, V_C_HALF, v_mul(bCandBox.bmax, V_C_HALF))));
      // five args need a format of <= 33 chars
      DA_PROFILE_TAG(collres_pair_tris, ": A%d B%d tris (%.0f %.0f %.0f)", (int)aFaces, (int)bFaceCache.size(), at.x, at.y, at.z);
    }
#else
    G_UNUSED(aFaces);
#endif
    return hit;
  }

  vec4f a_wbsph; // pos|r
  vec4f b_wbsph; // pos|r
  // Set on every apply() entry; isMeshNodeIntersectedWithMeshNode reads them for nodeChunkPtr, so
  // no separate vert/idx base or scratch members are kept here.
  const CollisionResource *aResource = nullptr;
  const CollisionResource *bResource = nullptr;
  const CollisionResourceInstance *aInst = nullptr;
  const CollisionResourceInstance *bInst = nullptr;
};


class TestMeshNodeBoxNodesIntersectionAlgo final : public ITestIntersectionAlgo
{
public:
  TestMeshNodeBoxNodesIntersectionAlgo() = default;
  ~TestMeshNodeBoxNodesIntersectionAlgo() final = default;

  bool apply(dag::ConstSpan<uint16_t> a_list, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a,
    dag::ConstSpan<uint16_t> /*b_list*/, dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale,
    bool checkOnlyPhysNodes, const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource *res_b,
    const CollisionResourceInstance *inst_b) final
  {
    aResource = res_a;
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);
    const vec4f placementMag = placementRoundingMag(vIWtmB, tm_a, tm_b);
    InnerNodes innerB;

    for (uint16_t ia : a_list)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !inst_a->checkNodeBehaviorFlags(nodeA->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, tmAtoB, tmBtoA;
      v_mat44_mul43(vWtmA, tm_a, inst_a->getNodeTm(nodeA->nodeIndex));
      v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);
      v_mat44_inverse43(tmBtoA, tmAtoB); // loop-invariant: every box node prunes A's walk through it
      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, nodeA->vBsphereCenter());

      collectInner(innerB, res_b, inst_b, nodeQueryBox(tmAtoB, paddedMeshNodeBox(nodeA->modelBBox), placementMag),
        COLLISION_NODE_TYPE_BOX);

      for (uint16_t ib : innerB.list)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
          continue;
        if (checkOnlyPhysNodes && !inst_b->checkNodeBehaviorFlags(nodeB->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
          continue;
        // Box bounding-sphere centers are stored node-local.
        vec3f sphereCenterNodeB =
          v_mat44_mul_vec3p(tm_b, v_mat44_mul_vec3p(inst_b->getNodeTm(nodeB->nodeIndex), nodeB->vBsphereCenter()));
        float sumRad = nodeA->radiusAroundBoxCenter * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale +
                       nodeB->radiusAroundBoxCenter * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        // SAT overlap is frame-independent, so test in B's frame with tmAtoB, the same matrix that
        // carries A's triangles below. Only the prune needs the reverse, hence tmBtoA above.
        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = composedNodeBox(inst_b, nodeB->nodeIndex, nodeB->modelBBox);
        if (v_bbox3_test_trasformed_box_likely_intersect(bboxB, bboxA, tmAtoB) == false)
          continue;

        // B's box back in A's frame prunes A's chunk walk to the overlap; a full face scan would
        // cost A's whole node however small the box is.
        bbox3f boxBInA;
        v_bbox3_init(boxBInA, tmBtoA, bboxB);

        if (walkNodeTrisInLocalBox(aResource->nodeChunkPtr(*nodeA), boxBInA.bmin, boxBInA.bmax, [&](vec3f a0, vec3f a1, vec3f a2) {
              if (!v_test_triangle_box_intersection(v_mat44_mul_vec3p(tmAtoB, a0), v_mat44_mul_vec3p(tmAtoB, a1),
                    v_mat44_mul_vec3p(tmAtoB, a2), bboxB))
                return false;
              vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
              v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
              v_stu_p3(&collisionPointB.x, sphereCenterNodeB); // posed center: raw c would report bind
              return true;
            }))
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

  bool apply(dag::ConstSpan<uint16_t> a_list, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a,
    dag::ConstSpan<uint16_t> /*b_list*/, dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale,
    bool checkOnlyPhysNodes, const CollisionResource * /*res_a*/, const CollisionResourceInstance *inst_a,
    const CollisionResource *res_b, const CollisionResourceInstance *inst_b) final
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmA, vIWtmB, tmBToA, tmAToB;
    v_mat44_inverse43(vIWtmA, tm_a);
    v_mat44_mul43(tmBToA, vIWtmA, tm_b);
    v_mat44_inverse43(vIWtmB, tm_b);
    v_mat44_mul43(tmAToB, vIWtmB, tm_a);
    const vec4f placementMag = placementRoundingMag(vIWtmB, tm_a, tm_b);
    InnerNodes innerB;

    for (uint16_t ia : a_list)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !inst_a->checkNodeBehaviorFlags(nodeA->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        continue;
      // Box bounding-sphere centers are stored node-local.
      vec3f sphereCenterNodeA =
        v_mat44_mul_vec3p(tm_a, v_mat44_mul_vec3p(inst_a->getNodeTm(nodeA->nodeIndex), nodeA->vBsphereCenter()));
      bbox3f composedBoxA = composedNodeBox(inst_a, nodeA->nodeIndex, nodeA->modelBBox);

      collectInner(innerB, res_b, inst_b, nodeQueryBox(tmAToB, composedBoxA, placementMag), COLLISION_NODE_TYPE_BOX);

      for (uint16_t ib : innerB.list)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
          continue;
        if (checkOnlyPhysNodes && !inst_b->checkNodeBehaviorFlags(nodeB->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB =
          v_mat44_mul_vec3p(tm_b, v_mat44_mul_vec3p(inst_b->getNodeTm(nodeB->nodeIndex), nodeB->vBsphereCenter()));
        float sumRad = nodeA->radiusAroundBoxCenter * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale +
                       nodeB->radiusAroundBoxCenter * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        if (v_bbox3_test_trasformed_box_likely_intersect(composedBoxA, composedNodeBox(inst_b, nodeB->nodeIndex, nodeB->modelBBox),
              tmBToA))
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

  bool apply(dag::ConstSpan<uint16_t> a_list, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a,
    dag::ConstSpan<uint16_t> /*b_list*/, dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale,
    bool checkOnlyPhysNodes, const CollisionResource *res_a, const CollisionResourceInstance *inst_a, const CollisionResource *res_b,
    const CollisionResourceInstance *inst_b) final
  {
    aResource = res_a;
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);
    const vec4f placementMag = placementRoundingMag(vIWtmB, tm_a, tm_b);
    InnerNodes innerB;

    for (uint16_t ia : a_list)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (!ITestIntersectionAlgo::poseCollidable(inst_a, nodeA))
        continue;
      if (checkOnlyPhysNodes && !inst_a->checkNodeBehaviorFlags(nodeA->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, tmAToB;
      v_mat44_mul43(vWtmA, tm_a, inst_a->getNodeTm(nodeA->nodeIndex));
      v_mat44_mul43(tmAToB, vIWtmB, vWtmA);

      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, nodeA->vBsphereCenter());

      collectInner(innerB, res_b, inst_b, nodeQueryBox(tmAToB, paddedMeshNodeBox(nodeA->modelBBox), placementMag),
        COLLISION_NODE_TYPE_SPHERE);

      for (uint16_t ib : innerB.list)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (!ITestIntersectionAlgo::poseCollidable(inst_b, nodeB))
          continue;
        if (checkOnlyPhysNodes && !inst_b->checkNodeBehaviorFlags(nodeB->nodeIndex, CollisionNode::PHYS_COLLIDABLE))
          continue;
        // maxTmScale conservatively encloses a non-uniformly posed sphere.
        const bool identTB = (inst_b->getPoseMeta(nodeB->nodeIndex).flags & CollisionNode::IDENT) != 0;
        vec3f composedCB =
          identTB ? nodeB->vBsphereCenter() : v_mat44_mul_vec3p(inst_b->getNodeGeometryTm(nodeB->nodeIndex), nodeB->vBsphereCenter());
        // A BAKED sphere's stored radius is already in the tested frame (its geometry tm is
        // identity): scaling it again would shrink or inflate the narrow phase.
        const float composedRB =
          (nodeB->radiusAroundBoxCenter < 0.f || identTB ||
            (inst_b->getPoseMeta(nodeB->nodeIndex).isGeometryBaked() && !inst_b->getPoseMeta(nodeB->nodeIndex).isRetainedBake()))
            ? nodeB->radiusAroundBoxCenter
            : nodeB->radiusAroundBoxCenter * inst_b->getPoseMeta(nodeB->nodeIndex).maxTmScale;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, composedCB);
        float sumRad =
          nodeA->radiusAroundBoxCenter * inst_a->getPoseMeta(nodeA->nodeIndex).maxTmScale * a_max_scale + composedRB * b_max_scale;
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
        const vec3f storedCB = nodeB->vBsphereCenter();
        const vec4f storedRB2 = v_set_x(get_bsphere_r2(nodeB->radiusAroundBoxCenter));

        // The sphere's own AABB, taken back through triToSphere, prunes A's chunk walk. The radius
        // here is the stored one the narrow phase uses, so the box encloses what it tests
        // (poseCollidable already refused the zero-vert marker, so it is never negative).
        alignas(EA_CACHE_LINE_SIZE) mat44f sphereToTri;
        v_mat44_inverse43(sphereToTri, triToSphere);
        bbox3f sphereBoxInA;
        {
          const vec3f vStoredRB = v_splats(nodeB->radiusAroundBoxCenter);
          bbox3f sphereBox;
          sphereBox.bmin = v_sub(storedCB, vStoredRB);
          sphereBox.bmax = v_add(storedCB, vStoredRB);
          v_bbox3_init(sphereBoxInA, sphereToTri, sphereBox);
        }

        if (walkNodeTrisInLocalBox(aResource->nodeChunkPtr(*nodeA), sphereBoxInA.bmin, sphereBoxInA.bmax,
              [&](vec3f a0, vec3f a1, vec3f a2) {
                vec3f a0b = v_mat44_mul_vec3p(triToSphere, a0);
                vec3f a1b = v_mat44_mul_vec3p(triToSphere, a1);
                vec3f a2b = v_mat44_mul_vec3p(triToSphere, a2);
                if (!v_test_triangle_sphere_intersection(a0b, a1b, a2b, storedCB, storedRB2))
                  return false;
                vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1.f / 3.f));
                v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
                v_stu_p3(&collisionPointB.x, sphereCenterNodeB);
                return true;
              }))
          return true;
      }
    }

    return false;
  }

  const CollisionResource *aResource = nullptr;
};

bool CollisionResource::testIntersection(const CollisionResource *res_a, const TMatrix &tm_a, const CollisionResource *res_b,
  const TMatrix &tm_b, Point3 &collisionPointA, Point3 &collisionPointB, bool checkOnlyPhysNodes /* = false*/)
{
  G_ASSERT(res_a);
  G_ASSERT(res_b);
  return testIntersection(res_a, tm_a, res_a->getDefaultInstance(), res_b, tm_b, res_b->getDefaultInstance(), collisionPointA,
    collisionPointB, checkOnlyPhysNodes);
}

bool CollisionResource::testIntersection(const CollisionResource *res_a, const TMatrix &tm_a,
  const CollisionResourceInstance &instance_a, const CollisionResource *res_b, const TMatrix &tm_b,
  const CollisionResourceInstance &instance_b, Point3 &collisionPointA, Point3 &collisionPointB, bool checkOnlyPhysNodes)
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
      // The stamped sphere is the serialized one widened (rotated boxes, eps-IDENT mesh frames):
      // the loader refuses a smaller stamp and the landing never makes one.
      return v_perm_xyzd(v_mat44_mul_vec3p(tm, res->vBindTraceSphere), v_sqrt(v_splat_w(res->vBindTraceSphere)));
    }
    const bbox3f rootBox = inst->getRootBBox();
    const vec3f rootCenter = v_madd(rootBox.bmin, V_C_HALF, v_mul(rootBox.bmax, V_C_HALF)); // overflow-safe midpoint
    return v_perm_xyzd(v_mat44_mul_vec3p(tm, rootCenter), v_length3(v_sub(rootBox.bmax, rootCenter)));
  };
  vec4f aWbsph = worldBsph(vTmA, res_a, instA);
  vec4f bWbsph = worldBsph(vTmB, res_b, instB);
  if (!TestMeshNodeMeshNodesIntersectionAlgo::isBoundingsIntersect(aWbsph, v_set_x(maxScaleA), bWbsph, v_set_x(maxScaleB)))
    return false;

  TestMeshNodeMeshNodesIntersectionAlgo testMeshNodeMeshNodesIntersectionAlgo(aWbsph, bWbsph);
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
    if (res_a->nodeList(nodeTypeIxA).empty())
      continue;

    for (int nodeTypeIxB = COLLISION_NODE_TYPE_MESH; nodeTypeIxB < COLLISION_NODE_TYPE_CAPSULE; nodeTypeIxB++)
    {
      if (res_b->nodeList(nodeTypeIxB).empty())
        continue;

      bool result = false;
      if (ITestIntersectionAlgo *testAB = arrIntersectionCall[nodeTypeIxA][nodeTypeIxB])
      {
        result = testAB->apply(res_a->nodeList(nodeTypeIxA), aAll, vTmA, res_b->nodeList(nodeTypeIxB), bAll, vTmB, maxScaleA,
          maxScaleB, checkOnlyPhysNodes, res_a, instA, res_b, instB);

        collisionPointA = testAB->collisionPointA;
        collisionPointB = testAB->collisionPointB;
      }
      else if (ITestIntersectionAlgo *testBA = arrIntersectionCall[nodeTypeIxB][nodeTypeIxA])
      {
        result = testBA->apply(res_b->nodeList(nodeTypeIxB), bAll, vTmB, res_a->nodeList(nodeTypeIxA), aAll, vTmA, maxScaleB,
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

// Test the mesh-vs-mesh pair (node1, node2) given node1's pre-materialised faces.
// Returns true on hit; writes cp1/cp2 (world-space centroids) and *nodeIndex1/2 if non-null.
// node2's chunk walk emits node-local verts and is pruned by node1's box mapped into that
// frame; tm2to1 composes each CANDIDATE tri into node1's frame once, ahead of the pair loop.
bool CollisionResource::testMeshNodePair(const CollisionNode *node1, const MeshNodeFaces &node1_faces, const CollisionResource *res2,
  const CollisionNode *node2, const TMatrix &node2_wtm, const TMatrix &tm1ToWorld, const TMatrix &tm2to1, Point3 &cp1, Point3 &cp2,
  uint16_t *node_index1, uint16_t *node_index2)
{
  dag::ConstSpan<Point3_vec4> node1Faces = node1_faces.tris;
  const bbox3f &n1Box = node1_faces.box;
  mat44f vTm2to1, vTm1to2;
  v_mat44_make_from_43cu_unsafe(vTm2to1, tm2to1.array);
  v_mat44_inverse43(vTm1to2, vTm2to1);
  bbox3f n1InN2;
  v_bbox3_init(n1InN2, vTm1to2, n1Box);
  bool hit = false;
  // node1 face boxes, so the pair loop rejects before the tri-tri kernel. Filled on the first
  // candidate only: the chunk walk is box-pruned and often emits nothing at all.
  dag::Vector<bbox3f, framemem_allocator> n1FaceBox;
  walkNodeTrisInLocalBox(res2->nodeChunkPtr(*node2), n1InN2.bmin, n1InN2.bmax, [&](vec3f w0, vec3f w1, vec3f w2) -> bool {
    const vec3f a2 = v_mat44_mul_vec3p(vTm2to1, w0);
    const vec3f b2 = v_mat44_mul_vec3p(vTm2to1, w1);
    const vec3f c2 = v_mat44_mul_vec3p(vTm2to1, w2);
    if (n1FaceBox.empty())
    {
      n1FaceBox.reserve(node1Faces.size() / 3);
      for (size_t i = 0; i + 2 < node1Faces.size(); i += 3)
      {
        vec3f f0 = v_ld(&node1Faces[i + 0].x), f1 = v_ld(&node1Faces[i + 1].x), f2 = v_ld(&node1Faces[i + 2].x);
        n1FaceBox.push_back({v_min(f0, v_min(f1, f2)), v_max(f0, v_max(f1, f2))});
      }
    }
    const vec3f cMin = v_min(a2, v_min(b2, c2));
    const vec3f cMax = v_max(a2, v_max(b2, c2));
    for (size_t i1 = 0, fi = 0; i1 + 2 < node1Faces.size(); i1 += 3, fi++)
    {
      if (v_check_xyz_any_true(v_or(v_cmp_gt(cMin, n1FaceBox[fi].bmax), v_cmp_gt(n1FaceBox[fi].bmin, cMax))))
        continue;
      if (!v_test_triangle_triangle_intersection(v_ld(&node1Faces[i1 + 0].x), v_ld(&node1Faces[i1 + 1].x), v_ld(&node1Faces[i1 + 2].x),
            a2, b2, c2))
        continue;
      cp1 = tm1ToWorld * ((node1Faces[i1 + 0] + node1Faces[i1 + 1] + node1Faces[i1 + 2]) * 0.333333f);
      Point3 l0, l1, l2;
      v_stu_p3(&l0.x, w0);
      v_stu_p3(&l1.x, w1);
      v_stu_p3(&l2.x, w2);
      cp2 = node2_wtm * ((l0 + l1 + l2) * 0.333333f);
      if (node_index1)
        *node_index1 = node1->nodeIndex;
      if (node_index2)
        *node_index2 = node2->nodeIndex;
      hit = true;
      return true; // any-hit: stop the walk
    }
    return false;
  });
  return hit;
}

bool CollisionResource::testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
  const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2, Point3 &collisionPoint1,
  Point3 &collisionPoint2, uint16_t *nodeIndex1, uint16_t *nodeIndex2, CollResNodeIndicesType *node_indices1)
{
  G_ASSERT(res1);
  G_ASSERT(res2);
  return testIntersection(res1, tm1, filter1, res1->getDefaultInstance(), res2, tm2, filter2, res2->getDefaultInstance(),
    collisionPoint1, collisionPoint2, nodeIndex1, nodeIndex2, node_indices1);
}

bool CollisionResource::testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
  const CollisionResourceInstance &instance1, const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2,
  const CollisionResourceInstance &instance2, Point3 &collisionPoint1, Point3 &collisionPoint2, uint16_t *nodeIndex1,
  uint16_t *nodeIndex2, CollResNodeIndicesType *node_indices1)
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
  reserve_and_resize(boxOutside, res2->data->allNodesList().size());

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
  mesh2s.reserve(res2->meshNodes().size());
  box2s.reserve(res2->boxNodes().size());
  sph2s.reserve(res2->sphereNodes().size());
  for (uint16_t mi2 : res2->meshNodes())
  {
    const CollisionNode *node2 = &res2->data->allNodesList()[mi2];
    if (!ITestIntersectionAlgo::poseCollidable(inst2, node2))
      continue;
    if (filter2 && !filter2(node2->nodeIndex))
      continue;
    Mesh2Data &m2 = mesh2s.push_back();
    m2.node = node2;
    m2.wtm2 = tm2 * nodePlacement(inst2, node2);
    m2.wCenter = m2.wtm2 * node2->bsphereCenter();
    m2.rTerm = node2->radiusAroundBoxCenter * inst2->getPoseMeta(node2->nodeIndex).maxTmScale;
  }
  for (uint16_t bi2 : res2->boxNodes())
  {
    const CollisionNode *node2 = &res2->data->allNodesList()[bi2];
    if (!ITestIntersectionAlgo::poseCollidable(inst2, node2))
      continue;
    if (filter2 && !filter2(node2->nodeIndex))
      continue;
    Box2Data &b2 = box2s.push_back();
    b2.node = node2;
    v_stu_bbox3(b2.composedBox, ITestIntersectionAlgo::composedNodeBox(inst2, node2->nodeIndex, node2->modelBBox));
    b2.wCenter = tm2 * (nodePlacement(inst2, node2) * node2->bsphereCenter());
    b2.rTerm = node2->radiusAroundBoxCenter * inst2->getPoseMeta(node2->nodeIndex).maxTmScale;
  }
  for (uint16_t si2 : res2->sphereNodes())
  {
    const CollisionNode *node2 = &res2->data->allNodesList()[si2];
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
    s2.composedC = identT2 ? node2->bsphereCenter() : geometryTm2 * node2->bsphereCenter();
    s2.pullToStored = !identT2;
    s2.invGtm = identT2 ? TMatrix::IDENT : collres_inverse(geometryTm2);
    // BAKED radii are already in the tested frame; see the sphere-vs-sphere arm.
    s2.composedR =
      (node2->radiusAroundBoxCenter < 0.f || identT2 ||
        (inst2->getPoseMeta(node2->nodeIndex).isGeometryBaked() && !inst2->getPoseMeta(node2->nodeIndex).isRetainedBake()))
        ? node2->radiusAroundBoxCenter
        : node2->radiusAroundBoxCenter * inst2->getPoseMeta(node2->nodeIndex).maxTmScale;
    // broadphase center in the same single-composed frame the narrow phase tests
    s2.wCenter = tm2 * s2.composedC;
  }

  // Per-node1 face materialisation feeds the per-pair mesh/box/sphere sub-loops below; cache once
  // per node1 so the chunk-leaf walk does not rerun per node2.
  for (uint16_t mi1 : res1->meshNodes())
  {
    const CollisionNode *node1 = &res1->data->allNodesList()[mi1];
    if (!ITestIntersectionAlgo::poseCollidable(inst1, node1))
      continue;
    if (filter1 && !filter1(node1->nodeIndex))
      continue;

    const TMatrix node1Tm = nodePlacement(inst1, node1);
    Point3 sphereCenter1 = tm1 * (node1Tm * node1->bsphereCenter());
    const float r1Term = node1->radiusAroundBoxCenter * inst1->getPoseMeta(node1->nodeIndex).maxTmScale * scale1;
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
    // Folded once per node1: the pair test prunes each partner's chunk walk by the folded box.
    const MeshNodeFaces node1Folded(make_span_const(node1FaceVerts));

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
        invTm1 = collres_inverse(tm1ToWorld);
        invTm1ready = true;
      }
      TMatrix tm2to1 = invTm1 * m2.wtm2;

      if (!test_box_box_intersection(node1->modelBBox, node2->modelBBox, tm2to1))
      {
        boxOutside[node2->nodeIndex] = true;
        continue;
      }
      _INC(numBoxesDebug);

      if (testMeshNodePair(node1, node1Folded, res2, node2, m2.wtm2, tm1ToWorld, tm2to1, collisionPoint1, collisionPoint2, nodeIndex1,
            nodeIndex2))
      {
        if (!node_indices1)
          return true;
        node_indices1->push_back(node1->nodeIndex);
      }
    }

    TMatrix tm1to2 = TMatrix::IDENT; // initialized for the same C4701 reason as invTm1
    bool tm1to2ready = false;
    // node1 verts in node2 space. tm1to2 does not depend on the partner box, so transforming them
    // inside the box loop would redo the same work for every box; filled on the first survivor.
    dag::Vector<vec4f, framemem_allocator> node1VertsIn2;

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
        invTm1 = collres_inverse(tm1ToWorld);
        invTm1ready = true;
      }
      // Map the composed box into node1's local frame.
      if (!test_box_box_intersection(node1->modelBBox, b2.composedBox, invTm1 * tm2))
        continue;
      if (!tm1to2ready)
      {
        tm1to2 = collres_inverse(tm2) * tm1 * node1Tm;
        tm1to2ready = true;
      }
      if (node1VertsIn2.empty() && !node1FaceVerts.empty())
      {
        mat44f vTm1to2;
        v_mat44_make_from_43cu_unsafe(vTm1to2, tm1to2.array);
        node1VertsIn2.resize(node1FaceVerts.size());
        for (size_t i = 0; i < node1FaceVerts.size(); i++)
          node1VertsIn2[i] = v_mat44_mul_vec3p(vTm1to2, v_ld(&node1FaceVerts[i].x));
      }
      bbox3f vComposedBox2 = v_ldu_bbox3(b2.composedBox);

      for (size_t i1 = 0; i1 + 2 < node1VertsIn2.size(); i1 += 3)
      {
        if (!v_test_triangle_box_intersection(node1VertsIn2[i1 + 0], node1VertsIn2[i1 + 1], node1VertsIn2[i1 + 2], vComposedBox2))
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
        tm1to2 = collres_inverse(tm2) * tm1 * node1Tm;
        tm1to2ready = true;
      }

      // Narrow phase in the sphere's STORED frame: the widened enclosing sphere above is only
      // the cull, and testing it directly would invent contacts outside a non-uniform pose's
      // short axes.
      if (node2->radiusAroundBoxCenter < 0.f)
        continue; // no stored sphere: the enclosing-sphere cull was all there is to test
      const TMatrix tri2sphere = s2.pullToStored ? s2.invGtm * tm1to2 : tm1to2;
      mat44f vTri2sphere;
      v_mat44_make_from_43cu_unsafe(vTri2sphere, tri2sphere.array);
      const Point3 storedC = node2->bsphereCenter();
      const vec4f vStoredC = v_ldu_p3(&storedC.x);
      const vec4f vStoredR2 = v_splats(sqr(node2->radiusAroundBoxCenter));
      for (size_t i1 = 0; i1 + 2 < node1FaceVerts.size(); i1 += 3)
      {
        const vec3f v0 = v_mat44_mul_vec3p(vTri2sphere, v_ld(&node1FaceVerts[i1].x));
        const vec3f v1 = v_mat44_mul_vec3p(vTri2sphere, v_ld(&node1FaceVerts[i1 + 1].x));
        const vec3f v2 = v_mat44_mul_vec3p(vTri2sphere, v_ld(&node1FaceVerts[i1 + 2].x));
        if (!v_test_triangle_sphere_intersection(v0, v1, v2, vStoredC, vStoredR2))
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
      v_mat44_make_from_43cu_unsafe(relGeomNodeTm, data->relGeomNodeTms()[node->nodeIndex].array);
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
  TMatrix itm = collres_inverse(instance_tm);

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
  const vec3f vA = v_ldu(&c.a.x);
  const vec3f vB = v_ldu(&c.b.x);
  const vec4f vR = v_splats(c.r);
  vec3f vCp1 = v_zero(), vCp2 = v_zero();
  vec4f vMd = v_splats(md);
  bool changed = false;

  // Per-triangle capsule-clip kernel of the chunk walk. Mirrors FRT
  // clipCapsule: recompute the face normal from resource-local verts, optionally skip back-faces vs
  // capsule travel, then accumulate the deepest penetration via clipCapsuleTriangle (keeps the min
  // over cp1/cp2/md). Tolerance > 0 keeps near-coplanar tris so a grazed wall still clips.
  auto clipTri = [&](vec3f v0, vec3f v1, vec3f v2, bool cull) {
    vec3f e1 = v_sub(v1, v0), e2 = v_sub(v2, v0);
    vec3f n = v_cross3(e1, e2);
    vec4f nLen = v_length3_x(n);
    if (v_extract_x(nLen) < 1e-9f)
      return;
    n = v_mul(n, v_splat_x(v_rcp_x(nLen)));
    if (cull && v_extract_x(v_dot3_x(n, vMoveDir)) > 1e-3f)
      return;
    changed |= v_clip_capsule_triangle(vA, vB, vR, v0, v1, v2, n, vCp1, vCp2, vMd);
  };

  // Each node's chunk walk is culled by the capsule's bbox mapped into node-local space; triangles
  // come out node-local and are transformed by the instance's live node tm into resource-local
  // (the capsule frame), so a posed instance clips at its pose.
  bbox3f capBox;
  capBox.bmin = v_sub(v_min(vA, vB), vR);
  capBox.bmax = v_add(v_max(vA, vB), vR);
  const auto clipNode = [&](const CollisionNode *node) {
    if (!inst->checkNodeBehaviorFlags(node->nodeIndex, CollisionNode::PHYS_COLLIDABLE) || !node->hasGeometry())
      return;
    if (!ITestIntersectionAlgo::poseCollidable(inst, node))
      return; // structurally hidden or untraceable pose

    // SOLID nodes trace without back-face culling: disable the movedir cull to match per-node semantics.
    const bool cull = haveMoveDir && !inst->checkNodeBehaviorFlags(node->nodeIndex, CollisionNode::SOLID);
    const mat44f nodeTm = inst->getNodeTm(node->nodeIndex); // node-local -> resource-local
    mat44f vITm;
    v_mat44_inverse43(vITm, nodeTm); // poseCollidable above rejected singular poses
    bbox3f nodeLocalBox;
    v_bbox3_init(nodeLocalBox, vITm, capBox);
    walkNodeTrisInLocalBox(nodeChunkPtr(*node), nodeLocalBox.bmin, nodeLocalBox.bmax, [&](vec3f lv0, vec3f lv1, vec3f lv2) {
      clipTri(v_mat44_mul_vec3p(nodeTm, lv0), v_mat44_mul_vec3p(nodeTm, lv1), v_mat44_mul_vec3p(nodeTm, lv2), cull);
      return false;
    });
  };
  // TLAS candidates, same contract as the sphere arm (the annotated one).
  CollResTlasCandidates cands;
  if (tlasBoxCandidates(*inst, capBox, cands))
  {
    for (uint16_t mi : cands)
      clipNode(&data->allNodesList()[mi]);
  }
  else
    for (uint16_t mi : meshNodes())
      clipNode(&data->allNodesList()[mi]);
  // the contact points reach the caller only when some triangle beat the incoming md
  if (changed)
  {
    md = v_extract_x(vMd);
    v_stu_p3(&cp1.x, vCp1);
    v_stu_p3(&cp2.x, vCp2);
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
  const vec3f vLocalDirNorm = v_ldu_p3(&localDirNorm.x);
  const vec4f vR = v_splats(sphere.r);
  const vec4f vR2 = v_splats(sphere.r * sphere.r);
  const vec4f vSphereC = v_ldu(&localSphere.c.x);

  // Descend the node's quad-BLAS with the sphere's AABB instead of materialising + scanning every
  // face: only overlapping leaves are decoded, and the first front-facing hit stops the walk. Leaf
  // verts arrive dequantised to node-local space (the same frame as localSphere).
  return walkNodeTrisInLocalBox(nodeChunkPtr(*node), v_sub(vSphereC, vR), v_add(vSphereC, vR),
    [&](vec3f v0, vec3f v1, vec3f v2) -> bool {
      if (!v_test_triangle_sphere_intersection(v0, v1, v2, vSphereC, vR2))
        return false;
      const vec3f cross = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
      const vec4f crossLenSq = v_length3_sq_x(cross);
      // a collapsed face has no plane: the sphere kernel accepts it at any distance, and its
      // normal is rounding noise
      if (v_test_vec_x_le(crossLenSq, v_splats(VERY_SMALL_NUMBER)))
        return false;
      if (v_extract_x(v_dot3_x(vLocalDirNorm, cross)) < -VERY_SMALL_NUMBER)
        return false; // wrong-facing triangle: keep walking
      const vec3f norm = v_mul(cross, v_splat_x(v_rsqrt_x(crossLenSq)));
      v_stu_p3(&out_norm.x, norm);
      out_depth = -v_extract_x(v_dot3_x(norm, v_sub(v0, vSphereC))) - sphere.r;
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

  if (!node->hasGeometry())
    return false; // degenerate-dropped: no geometry

  const vec4f vR2 = v_splats(radius * radius);
  const vec3f vCylP0 = v_ldu_p3(&localCylinderPoint0.x);
  const vec3f vCylP1 = v_ldu_p3(&localCylinderPoint1.x);

  // Descend the node's quad-BLAS with the capsule's swept AABB instead of materialising + scanning
  // every face: only overlapping leaves are decoded and the first hit stops the walk. Leaf verts
  // arrive dequantised to node-local space, matching the local capsule.
  return walkNodeTrisInLocalBox(nodeChunkPtr(*node), v_ldu(&bbox[0].x), v_ldu(&bbox[1].x), [&](vec3f v0, vec3f v1, vec3f v2) -> bool {
    if (v_test_triangle_sphere_intersection(v0, v1, v2, vCylP0, vR2) || v_test_triangle_sphere_intersection(v0, v1, v2, vCylP1, vR2))
    {
      // a collapsed face has no plane and the sphere kernel accepts it at any distance;
      // only the cylinder test, which works on the edges, can decide it
      if (v_test_vec_x_gt(v_length3_sq_x(v_cross3(v_sub(v1, v0), v_sub(v2, v0))), v_splats(VERY_SMALL_NUMBER)))
        return true;
    }
    return v_test_triangle_cylinder_intersection(v0, v1, v2, vCylP0, vCylP1, vR2);
  });
}


int CollisionResource::getMemoryUsed() const
{
  // The whole immutable block (header, arrays, alignment pads, the pending build's TLAS reservation),
  // heap-held so sizeof(*this) misses it.
  int mem = (int)data->bytesPerHolder();
  // This holder's own bytes: the object, its poses, its mark words and the clone the embedded instance
  // materializes once posed. Caller-owned instances hold their own clone and are not counted
  // here, like their poses. The clone counts CAPACITY: the decline and drift arms clear it
  // without freeing (the stale-reader defense), so the retained buffer stays resident.
  mem += (int)(sizeof(*this) + defaultInstance.nodeTm.size() * sizeof(TMatrix) +
               defaultInstance.poseMeta.size() * sizeof(CollisionResourceInstance::PoseMeta) + defaultInstance.tlas.data.capacity() +
               (interlocked_acquire_load_ptr(liveBoundWords) ? liveBoundWordCount() * sizeof(uint32_t) : 0));
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

bool CollisionResource::validateVerticesForJolt(const char *res_name)
{
  return validateVerticesForJolt(res_name, [](const uint32_t *, const CollisionNode *) {});
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

bool CollisionResource::validateVerticesForJolt(const char *res_name, auto &&on_degenerate)
{
  bool passed = true;
  for (uint16_t mi : meshNodes())
  {
    const CollisionNode *node = &data->allNodesList()[mi];
    if (!node->hasGeometry())
      continue;
    // The verts decode from the node's chunk vert21 block and the faces from its tree: the exact
    // values every dacoll trimesh path hands Jolt.
    // not hoisted: framemem needs LIFO frees
    dag::Vector<Point3_vec4, framemem_allocator> vertsTmp;
    vertsTmp.resize_noinit((uint32_t)node->verticesCount);
    Point3_vec4 *dst = vertsTmp.data();
    iterateNodeVerts((int)node->nodeIndex, [&](int i, vec4f v) { v_st(&dst[i].x, v); });
    const Point3_vec4 *__restrict vertices = dst;
    dag::Vector<uint32_t, framemem_allocator> idxTmp;
    idxTmp.reserve(node->indicesCount);
    iterateNodeFaces((int)node->nodeIndex, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
      idxTmp.push_back(i0);
      idxTmp.push_back(i1);
      idxTmp.push_back(i2);
    });
    const uint32_t *__restrict idxBase = idxTmp.data();
    const uint32_t *__restrict idxEnd = idxBase + idxTmp.size();
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


int CollisionResource::getTrianglesCount(uint8_t behavior_filter) const
{
  unsigned count = 0;
  for (uint16_t mi : meshNodes())
  {
    const CollisionNode *meshNode = &data->allNodesList()[mi];
    if (checkNodeBehaviorFlags(meshNode->nodeIndex, behavior_filter))
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
