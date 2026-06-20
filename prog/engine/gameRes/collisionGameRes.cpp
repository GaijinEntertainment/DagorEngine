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
#include <perfMon/dag_statDrv.h>
#include <util/dag_finally.h>
#include <debug/dag_debug.h>
#include <daBVH/dag_swBLAS_ray.h>
#include "collisionTraceOOL.h"

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

// Cull-agnostic filtered BLAS walk: iterateFiltered runs the caller's node-box test and emits leaf
// triangles to the caller's leaf callback, which picks the cull mode (RayTriangleIntersect<CullCCW>).
// iterateFiltered ignores the BLASTraverse CullCCW arg, so <false> is arbitrary.
using QuadBLASWalk = BLASTraverse<false>;

// Dequantize a vert21 leaf vertex back to resource-local coordinates. Packed value is box-space
// [0,65535]; inverse is `local = q * (1/blasScale) + blasBBox.bmin`. This matches the trace
// dispatch frame (vLocalFrom/vLocalDir), so the per-triangle ray test needs no further conversion.
struct BlasLocalUnquant
{
  vec3f bmin, invScale;
  __forceinline vec3f operator()(const uint8_t *d, int baseOfs, uint vertIdx) const
  {
    vec3f q = RayData::unpackVert21(d + baseOfs + vertIdx * BVH_BLAS_VERT21_STRIDE);
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

// Recover a leaf's source CollisionNode from its leaf body offset. buildBLAS keeps each node's vert21
// contiguous (drops the global fetch-remap, keeps QUAD_O1 dups inside the owning node's block), so a
// leaf's verts all live in one source node's sub-range: read the leaf's first vert byte offset, turn
// it into a vert21 index, and upper-bound the sorted blasNodeRanges to the containing range. Used by
// the BLAS trace dispatch (directly in the all-hits walk, via the OOL accept thunk for SoA rayCast).
static inline uint16_t blas_src_node_for_leaf(const CollisionResource::Grid &g, uint32_t blas_offset)
{
  const uint8_t *d = g.blasData.data();
  // First 4 bytes at blas_offset hold the relative offset to the leaf's vertex data.
  const int ofs1Rel = ((const int *)(d + blas_offset))[0];
  const uint32_t v0ByteOfs = (uint32_t)((int)blas_offset + ofs1Rel);
  G_ASSERTF(v0ByteOfs >= g.blasVertsOfs(), "BLAS leaf vert byte offset %u is inside the tree region (vertsOfs=%u, blasOffset=%u)",
    v0ByteOfs, g.blasVertsOfs(), blas_offset);
  const uint32_t v0Idx = (v0ByteOfs - g.blasVertsOfs()) / BVH_BLAS_VERT21_STRIDE;
  const auto *first = g.blasNodeRanges.data();
  const auto *last = first + g.blasNodeRanges.size();
  auto it =
    eastl::upper_bound(first, last, v0Idx, [](uint32_t v, const CollisionResource::Grid::NodeRange &r) { return v < r.verticesOfs; });
  G_ASSERTF(it != first, "BLAS leaf vert21 index %u precedes all blasNodeRanges entries", v0Idx);
  --it;
  G_ASSERTF(v0Idx < it->verticesEnd, "BLAS leaf vert21 index %u falls into gap between blasNodeRanges entries", v0Idx);
  return it->nodeIndex;
}


void CollisionResource::bakeNodeTransform(int node_id)
{
  // Single-node variant of the bake block in collapseAndOptimize (collisionGameResLoad.cpp): transforms vertices, transforms convex
  // planes via inverse-transpose, rebuilds bbox/bsphere from the transformed vertices, flips triangle winding when the TM is mirrored
  // (det<0), and normalizes transform metadata (tm/flags/cachedMaxTmScale). Capsule/box/sphere primitives store auxiliary shape data
  // outside vertices/indices, so they are out of scope for this helper.
  CollisionNode *n = getNode(node_id);
  if (!n)
    return;
  if (n->type != COLLISION_NODE_TYPE_MESH && n->type != COLLISION_NODE_TYPE_CONVEX)
    return;
  if ((n->flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT || n->indicesCount == 0)
    return;

  // Bake mutates verts/indices in place, so we must mutate ownVertices/ownIndices directly
  // (meshVertsBase/meshIndicesBase may point at FRT data which is read-only from here).
  G_ASSERT(meshVertsBase == ownVertices.data());
  Point3_vec4 *nodeVerts = ownVertices.data() + n->verticesOfs;
  uint16_t *nodeIdx = ownIndices.data() + n->indicesOfs;
  const uint32_t vCount = (uint32_t)n->verticesCount + 1u;

  mat44f nodeTm;
  v_mat44_make_from_43cu(nodeTm, n->tm[0]);
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

  if (n->tm.det() < 0.f)
    for (uint32_t i = 0, e = n->indicesCount; i + 2 < e; i += 3)
      eastl::swap(nodeIdx[i + 0], nodeIdx[i + 2]);
  n->tm = TMatrix::IDENT;
  n->flags =
    (n->flags & ~(CollisionNode::TRANSLATE | CollisionNode::ORTHOUNIFORM)) | CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED;
  n->cachedMaxTmScale = 1.f;
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

template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, typename filter_t,
  typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir,
  float len, bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
  TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  CollisionTrace in{
    .vFrom = from,
    .vDir = dir,
    .vTo = v_madd(dir, v_splats(len), from),
    .t = len,
  };

  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<trace_mode, trace_type, true /*is_single_ray*/>(tm, geom_node_tree, traces, calc_normal, bsphere_scale,
    behavior_filter, filter, callback, out_stats, force_no_cull);
}

// Heavy all-in-one node iterator, calculations unused by caller and deadcode will be automatically stripped by compiler
template <CollisionResource::IterationMode trace_mode, CollisionResource::CollisionTraceType trace_type, bool is_single_ray,
  typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(mat44f original_tm, const GeomNodeTree *geom_node_tree,
  dag::Span<CollisionTrace> traces, bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter,
  const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  TIME_PROFILE_DEV(collres_trace);

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
  vec3f xyzzScaleSq = v_mat44_scale43_sq(tm);

  // Check bounding
  bool anyTraceIntersectsBounding = false;
  {
    vec3f vBsphCenter;
    if (geom_node_tree && bsphereCenterNode)
      vBsphCenter = v_add(geom_node_tree->getNodeWposRel(bsphereCenterNode), v_sub(geom_node_tree->getWtmOfs(), woffset));
    else
      vBsphCenter = v_mat44_mul_vec3p(tm, vBoundingSphere);
    vec4f vBsphR2 = v_mul_x(v_mul_x(maxScaleSq, v_splat_w(vBoundingSphere)), v_set_x(bsphere_scale * bsphere_scale));
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
    // 0.025 * sqrt(3) of absolute error max
    const float eps = eastl::min(0.008f, 0.025f / getBoundingSphereRad());
    // Scaled tm requires more complicated norm calculation code and additional T conversion from world to local basis and back
    vec3f otmMask = v_and(v_cmp_gt(xyzzScaleSq, v_splats(1.f - eps)), v_cmp_lt(xyzzScaleSq, v_splats(1.f + eps)));
    bool bIsOrthonormalizedTm = v_check_xyzw_all_true(otmMask);

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
      res = forEachIntersectedNode<true, trace_mode, trace_type, is_single_ray>(tm, 1.f /* max_tm_scale_sq */, woffset, geom_node_tree,
        traces, calc_normal, behavior_filter, filter, cb_wrapper, out_stats, force_no_cull);
    else
      res = forEachIntersectedNode<false, trace_mode, trace_type, is_single_ray>(tm, v_extract_x(maxScaleSq), woffset, geom_node_tree,
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

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4701) // potentially uninitialized local variable 'XXX' used
#endif

template <bool orthonormalized_instance_tm, CollisionResource::IterationMode trace_mode,
  CollisionResource::CollisionTraceType trace_type, bool is_single_ray, typename filter_t, typename callback_t>
__forceinline bool CollisionResource::forEachIntersectedNode(const mat44f tm, float max_tm_scale_sq, vec3f woffset,
  const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces, bool calc_normal, uint8_t behavior_filter,
  const filter_t &filter, const callback_t &callback, TraceCollisionResourceStats *out_stats, bool force_no_cull) const
{
  bool hasCollision = false;
  bool isTraceByCapsule = trace_type == CollisionTraceType::TRACE_CAPSULE || trace_type == CollisionTraceType::CAPSULE_HIT;
  calc_normal &= trace_type == CollisionTraceType::TRACE_RAY || trace_type == CollisionTraceType::TRACE_CAPSULE;
  // The BLAS bakes BLAS-eligible mesh-node triangles in resource-local (bind) space and is walked using
  // only the instance tm -- it never consults the GeomNodeTree. A tree-driven node hit via the BLAS
  // would therefore be tested at its bind pose, not its animated pose. The invariant (asserted in
  // initializeWithGeomNodeTree) is that tree-bound resources are never grid-optimized, so hasBlas() is
  // already false for them; the explicit !geom_node_tree guard makes the trace path honour that contract
  // in release builds too -- when a tree is supplied we fall through to the per-node loop, which applies
  // the animated per-node tm via getMeshNodeTmInline.
  const bool useBlas = hasBlas(behavior_filter) && !isTraceByCapsule && !geom_node_tree;
  // Per-node trace of the mesh list (mesh + convex nodes). A BLAS covers only BLAS-eligible (IDENT)
  // mesh nodes; those are traced by the BLAS walk below and skipped here. Convex and non-IDENT mesh
  // nodes are never in the BLAS, so they must still be traced per-node or they'd be silently dropped.
  if (meshNodesHead != CollisionNode::INVALID_IDX)
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
    for (uint16_t mi = meshNodesHead; DAGOR_LIKELY(mi != CollisionNode::INVALID_IDX); mi = allNodesList[mi].nextNode)
    {
      const CollisionNode *meshNode = &allNodesList[mi];
      if (!filter(meshNode))
        continue;
      // The BLAS holds every BLAS-eligible (IDENT) mesh node; skip those here to avoid double-tracing
      // (the BLAS walk below handles them). Mirrors buildBLAS's isEligibleForBlas; convex and
      // non-IDENT mesh nodes fall through.
      if (useBlas && meshNode->type == COLLISION_NODE_TYPE_MESH && (meshNode->flags & CollisionNode::IDENT) &&
          meshNode->checkBehaviorFlags(behavior_filter) && meshNode->indicesCount > 0)
        continue;

      alignas(EA_CACHE_LINE_SIZE) mat44f nodeTm, nodeItm;
      nodeTm = getMeshNodeTmInline(meshNode, tm, woffset, geom_node_tree);
      vec3f bsphCenter = v_mat44_mul_vec3p(nodeTm, v_ldu(&meshNode->boundingSphere.c.x));
      float bsphR2 = get_bsphere_r2(meshNode->boundingSphere.r) * max_tm_scale_sq * sqr(meshNode->cachedMaxTmScale);
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
      bool isOrthonormalizedTm = orthonormalized_instance_tm && (meshNode->flags & CollisionNode::ORTHONORMALIZED);
      bool isOrthouniformTm =
        orthonormalized_instance_tm && (meshNode->flags & (CollisionNode::ORTHOUNIFORM | CollisionNode::ORTHONORMALIZED));

      if (DAGOR_LIKELY(isOrthouniformTm))
      {
        v_mat44_orthonormal_inverse43(nodeItm, nodeTm);
        if (DAGOR_UNLIKELY(!isOrthonormalizedTm))
        {
          vec4f scale = v_splat_x(v_rcp_x(v_set_x(meshNode->cachedMaxTmScale)));
          nodeItm.col0 = v_mul(nodeItm.col0, scale);
          nodeItm.col1 = v_mul(nodeItm.col1, scale);
          nodeItm.col2 = v_mul(nodeItm.col2, scale);
        }
      }
      else
        v_mat44_inverse43(nodeItm, nodeTm);

      // BLAS-resident materialisation: this per-node fallback is the only forEachIntersectedNode path
      // for capsule traces (useBlas is false when isTraceByCapsule), and ray traces land here when the
      // target grid is empty. The pfn / traceCapsule* / capsuleHit* helpers read `verts_base +
      // node.verticesOfs` / `idx_base + node.indicesOfs`, but BLAS-resident MESH nodes dropped both
      // ownVertices and ownIndices. resolveNodeVertsForCall materialises both into framemem scratches
      // and rebases the node copy's verticesOfs/indicesOfs to 0 so the helpers index unchanged.
      dag::Vector<Point3_vec4, framemem_allocator> blasResidentVScratch;
      dag::Vector<uint16_t, framemem_allocator> blasResidentIScratch;
      CollisionNode rebasedNodeCopy;
      const Point3_vec4 *vertsForCall = nullptr;
      const uint16_t *idxForCall = nullptr;
      const CollisionNode *nodeForCall = nullptr;
      resolveNodeVertsForCall(*meshNode, blasResidentVScratch, blasResidentIScratch, rebasedNodeCopy, vertsForCall, idxForCall,
        nodeForCall);
      if (DAGOR_UNLIKELY((meshNode->flags & CollisionNode::BLAS_RESIDENT) && getBlasGridForResidentNode(*meshNode).blasData.empty()))
        continue; // defensive: BLAS_RESIDENT was stamped but grid wasn't built. Skip silently.

      for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
      {
        CollisionTrace &trace = traces[traceId];
        if (!trace.isectBounding)
          continue;

        vec3f vNodeLocalFrom = v_mat44_mul_vec3p(nodeItm, trace.vFrom);
        vec3f vNodeLocalDir;
        float localT;

        if (isOrthonormalizedTm)
        {
          vNodeLocalDir = v_mat44_mul_vec3v(nodeItm, trace.vDir);
          localT = trace.t;
        }
        else
        {
          vec3f vNodeLocalTo = v_mat44_mul_vec3p(nodeItm, trace.vTo);
          vec3f vNodeLocalFullDir = v_sub(vNodeLocalTo, vNodeLocalFrom);
          vec4f vNodeLocalT = v_length3(vNodeLocalFullDir);
          vNodeLocalDir = v_div(vNodeLocalFullDir, vNodeLocalT);
          localT = v_extract_x(vNodeLocalT);
          if (DAGOR_UNLIKELY(localT < VERY_SMALL_NUMBER))
            continue;
        }
        float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * (localT / trace.t) : 0.f;

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
          switch (trace_type)
          {
            case CollisionTraceType::TRACE_RAY:
              isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalCullCCW
                                   : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalCullCCW)(vertsForCall, idxForCall,
                *nodeForCall, vNodeLocalFrom, vNodeLocalDir, inOutLocalT, normPtr);
              break;
            case CollisionTraceType::TRACE_CAPSULE:
              isHit = traceCapsuleMeshNodeLocalCullCCW(vertsForCall, idxForCall, *nodeForCall, vNodeLocalFrom, vNodeLocalDir,
                inOutLocalT, localCapsuleRadius, vNodeLocalNorm, vNodeLocalCapsuleHitPos);
              break;
            case CollisionTraceType::RAY_HIT:
              isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnRayHitMeshNodeLocalCullCCW
                                   : traceMeshNodeLocalApi.heavy.pfnRayHitMeshNodeLocalCullCCW)(vertsForCall, idxForCall, *nodeForCall,
                vNodeLocalFrom, vNodeLocalDir, inOutLocalT);
              break;
            case CollisionTraceType::CAPSULE_HIT:
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
              if (DAGOR_LIKELY(isOrthouniformTm))
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
            callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos,
              tri_ref::makeForNonTri(meshNode->nodeIndex));
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
          all_collres_tri_indices_t retTriIndices;
          bool isLightNode = meshNode->indicesCount < (uint32_t)traceMeshNodeLocalApi.threshold;
          bool isHit = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalAllHits
                                    : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalAllHits)(vertsForCall, idxForCall,
            *nodeForCall, vNodeLocalFrom, vNodeLocalDir, localT, calc_normal, force_no_cull, ret, retTriIndices);
          if (isHit)
          {
            hasCollision = true;
            mat33f titm33;
            if (!isOrthouniformTm && calc_normal)
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
                if (DAGOR_UNLIKELY(!isOrthouniformTm))
                  normTm = titm33;
                vIntersectionNorm = v_norm3(v_mat33_mul_vec3(normTm, n_t));
              }
              callback(traceId, meshNode, intersectionT, vIntersectionNorm, vIntersectionPos,
                tri_ref::make(meshNode->nodeIndex, (uint32_t)retTriIndices[j], false));
            }
            profileStats.meshTrianglesHits += ret.size();
          }
        } // allow multiple intersection of one node or not
      } // traces loop
    } // mesh nodes loop

    // Grid-less early-out only: with useBlas set we still need the BLAS walk and the box/sphere/
    // capsule loops below, so don't return here.
    if (!useBlas && DAGOR_LIKELY(boxNodesHead == CollisionNode::INVALID_IDX && sphereNodesHead == CollisionNode::INVALID_IDX &&
                                 capsuleNodesHead == CollisionNode::INVALID_IDX))
      return hasCollision;
  } // mesh-list per-node pass

  alignas(EA_CACHE_LINE_SIZE) mat44f itm;
  if (orthonormalized_instance_tm)
    v_mat44_orthonormal_inverse43(itm, tm);
  else
    v_mat44_inverse43(itm, tm);

  // for normals
  mat33f titm33;
  bool titmCalculated = false;

  for (int traceId = 0, traceEnd = traces.size(); traceId < traceEnd; traceId++)
  {
    CollisionTrace &trace = traces[traceId];
    if (!trace.isectBounding)
      continue;

    vec3f vLocalFrom = v_mat44_mul_vec3p(itm, trace.vFrom);
    vec3f vLocalTo, vLocalDir;
    typename eastl::conditional_t<orthonormalized_instance_tm, float &, float> localT = trace.t;

    if (orthonormalized_instance_tm)
    {
      vLocalDir = v_mat44_mul_vec3v(itm, trace.vDir);
      vLocalTo = v_madd(vLocalDir, v_splats(trace.t), vLocalFrom);
      localT = trace.t;
    }
    else
    {
      vLocalTo = v_mat44_mul_vec3p(itm, trace.vTo);
      vLocalDir = v_sub(vLocalTo, vLocalFrom);
      vec4f vLocalT = v_length3(vLocalDir);
      vLocalDir = v_div(vLocalDir, vLocalT);
      localT = v_extract_x(vLocalT);
      if (DAGOR_UNLIKELY(localT < VERY_SMALL_NUMBER))
        continue;
    }
    float localCapsuleRadius = isTraceByCapsule ? trace.capsuleRadius * (localT / trace.t) : 0.f;

    if (useBlas)
    {
      TIME_PROFILE_DEV(collres_trace_mesh_grid);
#if DA_PROFILER_ENABLED
      int tris = getTrianglesCount(behavior_filter);
      DA_PROFILE_TAG(tris, ": %u", tris);
#endif

      // moved it in (BLAS) block for rendinsts only, because in most cases we have not more than one not-mesh node
      // don't forget to extend vFullBBox by localCapsuleRadius if you want to change it
      if (!v_test_segment_box_intersection(vLocalFrom, vLocalTo, vFullBBox))
        continue;

      const Grid &blasGrid = getBlasGrid(behavior_filter);
      const bool isCollidableGrid = isCollidableGridForTrace(behavior_filter);
      {
        // BLAS path: walk the combined-per-behavior BLAS once per trace. The bbox check is box-space
        // (the BLAS quantization frame); the per-triangle ray test is resource-local on verts
        // dequantized via BlasLocalUnquant. blas_src_node_for_leaf (file-static above) maps a leaf's
        // first-vert vert21 index to its CollisionNode so we can apply filter() and stamp the tri_ref
        // with that node + the leaf's blasOffset.
        const BlasLocalUnquant unquantVL = BlasLocalUnquant::make(blasGrid.blasBBox.bmin, blasGrid.blasScale);
        const uint8_t *bData = blasGrid.blasData.data();
        // Traversal bound is the tree region only ([0, blasTreeBytes)); the packed vert21 array lives
        // after it in the same buffer and must not be walked as nodes.
        const int bSize = (int)blasGrid.blasTreeBytes;

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
            const Grid *grid;
            const CollisionNode *nodes;
            const filter_t *filter;
          } actx{&blasGrid, allNodesList.data(), &filter};
          const collision_blas::LeafAccept acceptLeaf = [](void *ctx, int dataOfs) -> bool {
            const AcceptCtx *c = (const AcceptCtx *)ctx;
            return (*c->filter)(&c->nodes[blas_src_node_for_leaf(*c->grid, (uint32_t)dataOfs)]);
          };

          constexpr bool anyHit = (trace_mode == ANY_ONE_INTERSECTION) || (trace_type == CollisionTraceType::RAY_HIT);
          int bestBlasOfs = 0, bestSubTri = 0;
          bool foundHit;
          IF_CONSTEXPR (anyHit)
            foundHit = blasGrid.blasTwoSided
                         ? collision_blas::rayBLASAnyHitFilteredOOL(rd, 0, bSize, acceptLeaf, &actx, bestBlasOfs, bestSubTri)
                         : collision_blas::rayBLASAnyHitFilteredOOLCullCCW(rd, 0, bSize, acceptLeaf, &actx, bestBlasOfs, bestSubTri);
          else
            foundHit = blasGrid.blasTwoSided
                         ? collision_blas::rayBLASClosestFilteredOOL(rd, 0, bSize, acceptLeaf, &actx, bestBlasOfs, bestSubTri)
                         : collision_blas::rayBLASClosestFilteredOOLCullCCW(rd, 0, bSize, acceptLeaf, &actx, bestBlasOfs, bestSubTri);

          if (foundHit)
          {
            hasCollision = true;
            const uint16_t bestSrcNode = blas_src_node_for_leaf(blasGrid, (uint32_t)bestBlasOfs);
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
              QuadBLASWalk::QuadLeafVerts q;
              const uint32_t leafSkip = ((const uint32_t *)(bData + bestBlasOfs))[-1];
              q.decodeTri(bData, bestBlasOfs, leafSkip, bestSubTri, unquantVL);
              vec3f vLocalNorm = v_norm3(v_cross3(v_sub(q.v1, q.v0), v_sub(q.v2, q.v0)));
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
              tri_ref::make_blas(bestSrcNode, (uint32_t)bestBlasOfs, (bestSubTri != 0), isCollidableGrid));
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
          // All-hits: cull CCW unless this grid replaces a two-sided FRT or the caller forces no-cull.
          // Mirrors traceRayMeshNodeLocalAllHits's `noCull = force_no_cull || SOLID`; SOLID nodes never
          // enter a BLAS, so force_no_cull is the only no-cull source on top of the grid's two-sided
          // mode. Constant for the whole walk, bound once to a compile-time RayTriangleIntersect below.
          int lastDataOfs = -1;
          auto walkAll = [&]<bool CullCCW>() {
            QuadBLASWalk::iterateFiltered(
              bData, 0, bSize,
              [fullLocalT, &bRay](vec3f bmn, vec3f bmx) {
                return RayIntersectsBoxT0T1(v_madd(bmn, bRay.dirInv, bRay.originScaled), v_madd(bmx, bRay.dirInv, bRay.originScaled),
                  fullLocalT);
              },
              [&](vec3f v0, vec3f v1, vec3f v2, int dataOfs) -> bool {
                // All-hits walker never early-exits: the bool return is the iterateFiltered protocol;
                // always return false (continue) here.
                const bool isSecondSubTri = (dataOfs == lastDataOfs);
                lastDataOfs = dataOfs;
                float t = fullLocalT;
                Point2 bc;
                if (RayTriangleIntersect<CullCCW>(vLocalFrom, vLocalDir, v0, v1, v2, t, bc))
                {
                  const uint16_t srcNode = blas_src_node_for_leaf(blasGrid, (uint32_t)dataOfs);
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
                      tri_ref::make_blas(srcNode, (uint32_t)dataOfs, isSecondSubTri, isCollidableGrid));
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
      if (!filter(boxNode))
        continue;
      float atMin = 1.f, atMax = 1.f; // [0; 1]
      int side = 0;
      bool isHit = false;
      bbox3f modelBBox = v_ldu_bbox3(boxNode->modelBBox);
      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          side = v_segment_box_intersection_side(vLocalFrom, vLocalTo, modelBBox, atMin, atMax);
          isHit = side != -1;
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          G_ASSERTF(false, "CollisionResource trace failed: capsule trace for box nodes is not implemented yet"); //-V1037
          // Very complex when done correct for all theoretically possible cases
          // Try to use traceRay for collres with box nodes
          break;
        case CollisionTraceType::RAY_HIT: isHit = v_test_segment_box_intersection(vLocalFrom, vLocalTo, modelBBox); break;
        case CollisionTraceType::CAPSULE_HIT:
          G_ASSERTF(false, "CollisionResource trace failed: capsule rayhit for box nodes is not implemented yet"); //-V1037
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * atMin;
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          localT *= atMin; // warning: localT is ref to trace.t if (orthonormalized_instance_tm==true)
          trace.t = intersectionT;
          vLocalTo = v_madd(vLocalDir, v_splats(localT), vLocalFrom);
        }
        if (calc_normal)
        {
          switch (side % 3) // better than index
          {
            case 0: vIntersectionNorm = tm.col0; break;
            case 1: vIntersectionNorm = tm.col1; break;
            case 2: vIntersectionNorm = tm.col2; break;
          }
          if (!orthonormalized_instance_tm)
            vIntersectionNorm = v_norm3(vIntersectionNorm);
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
      if (!filter(sphereNode))
        continue;
      bool isHit = false;
      vec3f bsphPos = v_ldu(&sphereNode->boundingSphere.c.x);
      vec4f bsphR2 = v_set_x(get_bsphere_r2(sphereNode->boundingSphere.r));
      vec4f bsphCapsuleR = v_set_x(sphereNode->boundingSphere.r + localCapsuleRadius);
      vec4f bsphCapsuleR2 = v_mul(bsphCapsuleR, bsphCapsuleR);
      vec4f inOutLocalT = v_set_x(localT);

      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY:
          isHit = v_ray_sphere_intersection(vLocalFrom, vLocalDir, inOutLocalT, bsphPos, bsphR2);
          break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit = v_ray_sphere_intersection(vLocalFrom, vLocalDir, inOutLocalT, bsphPos, bsphCapsuleR2);
          break;
        case CollisionTraceType::RAY_HIT:
          isHit = v_test_ray_sphere_intersection(vLocalFrom, vLocalDir, v_splat_x(inOutLocalT), bsphPos, bsphR2);
          break;
        case CollisionTraceType::CAPSULE_HIT:
          isHit = v_test_ray_sphere_intersection(vLocalFrom, vLocalDir, v_splat_x(inOutLocalT), bsphPos, bsphCapsuleR2);
          break;
        default: G_ASSERTF(false, "CollisionResource trace failed: unsupported trace_type");
      }
      if (isHit)
      {
        hasCollision = true;
        float intersectionT = trace.t * (v_extract_x(inOutLocalT) / localT);
        vec3f vIntersectionPos = v_madd(trace.vDir, v_splats(intersectionT), trace.vFrom);
        vec3f vIntersectionNorm = v_zero();
        if (trace_mode == FIND_BEST_INTERSECTION)
        {
          trace.t = intersectionT;
          localT = v_extract_x(inOutLocalT);
        }
        if (calc_normal || trace_type == CollisionTraceType::TRACE_CAPSULE)
        {
          vec3f vLocalHitPos = v_madd(vLocalDir, v_splat_x(inOutLocalT), vLocalFrom);
          vec3f vLocalNorm = v_sub(vLocalHitPos, v_ldu(&sphereNode->boundingSphere.c.x));

          if (calc_normal)
          {
            if (orthonormalized_instance_tm)
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
            vec3f vLocalPos = v_madd(vLocalNorm, v_splats(sphereNode->boundingSphere.r), v_ldu(&sphereNode->boundingSphere.c.x));
            vIntersectionPos = v_mat44_mul_vec3p(tm, vLocalPos);
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
      if (!filter(capsuleNode))
        continue;
      float inOutLocalT = localT;
      bool isHit = false;
      vec3f vOutLocalNorm = v_zero();
      vec3f vOutLocalPos = v_zero();

      const Capsule &nodeCapsule = capsules[capsuleNode->capsuleIndex];
      switch (trace_type)
      {
        case CollisionTraceType::TRACE_RAY: isHit = nodeCapsule.traceRay(vLocalFrom, vLocalDir, inOutLocalT, vOutLocalNorm); break;
        case CollisionTraceType::TRACE_CAPSULE:
          isHit = nodeCapsule.traceCapsule(vLocalFrom, vLocalDir, inOutLocalT, localCapsuleRadius, vOutLocalNorm, vOutLocalPos);
          break;
        case CollisionTraceType::RAY_HIT: isHit = nodeCapsule.rayHit(vLocalFrom, vLocalDir, localT); break;
        case CollisionTraceType::CAPSULE_HIT: isHit = nodeCapsule.capsuleHit(vLocalFrom, vLocalDir, localT, localCapsuleRadius); break;
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
          localT = inOutLocalT;
        }
        if (calc_normal)
        {
          if (orthonormalized_instance_tm)
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
          vIntersectionPos = v_mat44_mul_vec3p(tm, vOutLocalPos);
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

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, int &out_node_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/, tri_ref_t /*tri_ref*/) {
    in_out_t = t;
    if (out_normal)
      v_stu_p3(&out_normal->x, normal);
    out_mat_id = node->physMatId;
    out_node_id = node->nodeIndex;
  };

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x),
    v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id, const CollisionNodeFilter &filter, int ray_mat_id,
  uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!filter || filter(node->nodeIndex)) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/, tri_ref_t /*tri_ref*/) {
    in_out_t = t;
    if (out_normal)
      v_stu_p3(&out_normal->x, normal);
    if (out_mat_id)
      out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x),
    v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const Point3 &from, const Point3 &dir, float &in_out_t,
  Point3 *out_normal, int &out_mat_id, int ray_mat_id, uint8_t behavior_filter) const
{
  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) &&
           (ray_mat_id == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(ray_mat_id, node->physMatId));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f /*pos*/, tri_ref_t /*tri_ref*/) {
    in_out_t = t;
    if (out_normal)
      v_stu_p3(&out_normal->x, normal);
    out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, nullptr /*geom_node_tree*/, v_ldu(&from.x),
    v_ldu(&dir.x), in_out_t, out_normal != nullptr, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter,
  const CollisionNodeMask *collision_node_mask, bool force_no_cull) const
{
  intersected_nodes_list.clear();

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode * /*node*/, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    IntersectedNode *inode = (IntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->triRef = tri_ref;
  };

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    true /*calc_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*out_stats*/, force_no_cull);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  const CollisionNodeFilter &filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  intersected_nodes_list.clear();
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behaviorFilter) && (!filter || filter(node->nodeIndex));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode * /*node*/, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    IntersectedNode *inode = (IntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->triRef = tri_ref;
  };

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    true /*calc_normal*/, 1.f, behaviorFilter, nodeFilter, callback, nullptr, false /*force_no_cull*/);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

static const float bsphereScale = sqrtf(3.f); // moving nodes can extend outside of bound box
DAGOR_NOINLINE bool CollisionResource::traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  const CollisionNodeMask &collision_node_mask, TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  intersected_nodes_list.clear();
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behaviorFilter) && collision_node_mask.test(node->nodeIndex, true);
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode * /*node*/, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    IntersectedNode *inode = (IntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->triRef = tri_ref;
  };

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x), in_t,
    true /*calc_normal*/, bsphereScale, behaviorFilter, nodeFilter, callback, out_stats, false /*force_no_cull*/);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

DAGOR_NOINLINE bool CollisionResource::traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir,
  float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
  TraceCollisionResourceStats *out_stats) const
{
  intersected_nodes_list.clear();
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behaviorFilter) && collision_node_mask.test(node->nodeIndex, true);
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode * /*node*/, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    IntersectedNode *inode = (IntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->triRef = tri_ref;
  };

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, from, dir, in_t, true /*calc_normal*/,
    bsphereScale, behaviorFilter, nodeFilter, callback, out_stats, false /*force_no_cull*/);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const CollisionNodeFilter &filter, const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_t), in.vFrom);
  in.t = in_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;
  float closest = VERY_BIG_NUMBER;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!filter || filter(node->nodeIndex));
  };

  auto callback = [&](int /*trace_id*/, const CollisionNode * /*node*/, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    vec3f cl = closest_point_on_line(pos, in.vFrom, in.vDir);
    float dist = v_extract_x(v_length3_sq_x(v_sub(cl, pos)));
    if (dist < closest)
    {
      closest = dist;
      v_stu_p3(&intersected_node.normal.x, normal);
      intersected_node.intersectionT = t;
      v_stu_p3(&intersected_node.intersectionPos.x, pos);
      intersected_node.triRef = tri_ref;
    }
  };

  constexpr bool is_single_ray = true;
  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<ALL_NODES_INTERSECTIONS, CollisionTraceType::TRACE_CAPSULE, is_single_ray>(tm, geom_node_tree, traces,
    true /*calc_normal*/, bsphere_scale, behavior_filter, nodeFilter, callback, nullptr /*out_stats*/, false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.m[0]);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float t, vec3f normal, vec3f pos, tri_ref_t /*tri_ref*/) {
    in_out_t = t;
    v_stu_p3(&out_normal.x, normal);
    v_stu_p3(&out_pos.x, pos);
    out_mat_id = node->physMatId;
  };

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_out_t), in.vFrom);
  in.t = in_out_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;

  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_CAPSULE, true /*is_single_ray*/>(tm, geom_node_tree,
    traces, true /*out_normal*/, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale,
  const uint8_t behavior_filter) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_t), in.vFrom);
  in.t = in_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;
  float closest = VERY_BIG_NUMBER;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behavior_filter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode * /*node*/, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    vec3f cl = closest_point_on_line(pos, in.vFrom, in.vDir);
    float dist = v_extract_x(v_length3_sq_x(v_sub(cl, pos)));
    if (dist < closest)
    {
      closest = dist;
      v_stu_p3(&intersected_node.normal.x, normal);
      intersected_node.intersectionT = t;
      v_stu_p3(&intersected_node.intersectionPos.x, pos);
      intersected_node.triRef = tri_ref;
    }
  };

  constexpr bool is_single_ray = true;
  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<ALL_NODES_INTERSECTIONS, CollisionTraceType::TRACE_CAPSULE, is_single_ray>(tm, geom_node_tree, traces,
    true /*calc_normal*/, bsphere_scale, behavior_filter, nodeFilter, callback, nullptr /*out_stats*/, false /*force_no_cull*/);
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

  return forEachIntersectedNode<FIND_BEST_INTERSECTION, CollisionTraceType::TRACE_RAY>(tm, nullptr /*geom_node_tree*/, traces,
    true /*calc_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::traceMultiRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  dag::Span<CollisionTrace> traces, MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
  float bsphere_scale, uint8_t behavior_filter, const CollisionNodeMask *collision_node_mask,
  TraceCollisionResourceStats *out_stats) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  intersected_nodes_list.clear();

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behavior_filter) && (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [&](int trace_id, const CollisionNode *node, float t, vec3f normal, vec3f pos, tri_ref_t tri_ref) {
    traces[trace_id].outMatId = node->physMatId;
    traces[trace_id].outNodeId = node->nodeIndex;
    traces[trace_id].isHit = true;

    MultirayIntersectedNode *inode = (MultirayIntersectedNode *)intersected_nodes_list.push_back_uninitialized();
    v_stu_p3(&inode->normal.x, normal);
    inode->intersectionT = t;
    v_stu_p3(&inode->intersectionPos.x, pos);
    inode->rayId = trace_id;
    inode->triRef = tri_ref;
  };

  for (CollisionTrace &trace : traces)
  {
    trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
    trace.isectBounding = false;
    trace.outMatId = PHYSMAT_INVALID;
    trace.outNodeId = -1;
    trace.isHit = false;
  }

  forEachIntersectedNode<ALL_INTERSECTIONS, CollisionTraceType::TRACE_RAY>(tm, geom_node_tree, traces, true /*calc_normal*/,
    bsphere_scale, behavior_filter, nodeFilter, callback, out_stats, false /*force_no_cull*/);

  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);
  return !intersected_nodes_list.empty();
}

// Materialise a BLAS-resident node's verts (and indices, dropped for MESH residents) into the
// supplied framemem scratches and produce a CollisionNode copy with verticesOfs/indicesOfs rebased to
// zero. Non-resident nodes pass through as (meshVertsBase, meshIndicesBase, &node). Lets the per-node
// trace helpers (traceRayMeshNodeLocal*, test_*_node_intersection, traceQuad, etc.) keep their
// (verts_base + node.verticesOfs) / (idx_base + node.indicesOfs) indexing unchanged. BLAS residents
// are always MESH (Convex never enters the BLAS); idxScratch is filled by decoding node-local indices
// from each leaf via iterateNodeFaces.
void CollisionResource::resolveNodeVertsForCall(const CollisionNode &node, dag::Vector<Point3_vec4, framemem_allocator> &vertsScratch,
  dag::Vector<uint16_t, framemem_allocator> &idxScratch, CollisionNode &node_copy, const Point3_vec4 *&out_verts_base,
  const uint16_t *&out_idx_base, const CollisionNode *&out_node) const
{
  if (!(node.flags & CollisionNode::BLAS_RESIDENT))
  {
    out_verts_base = meshVertsBase;
    out_idx_base = meshIndicesBase;
    out_node = &node;
    return;
  }
  const Grid &g = getBlasGridForResidentNode(node);
  if (DAGOR_UNLIKELY(g.blasData.empty()))
  {
    // Defensive: BLAS_RESIDENT stamped but the resolved grid has no BLAS (shouldn't happen). Fall back
    // to the non-resident path instead of dereferencing empty blasData (release-safe; the prior
    // G_ASSERTF was compiled out in shipping, leaving an out-of-bounds read).
    out_verts_base = meshVertsBase;
    out_idx_base = meshIndicesBase;
    out_node = &node;
    return;
  }
  const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)node.verticesOfs * BVH_BLAS_VERT21_STRIDE;
  const vec3f invScale = g.blasInvScale;
  const vec3f bmin = g.blasBBox.bmin;
  const uint32_t vc = (uint32_t)node.verticesCount + 1u;
  vertsScratch.resize(vc);
  for (uint32_t i = 0; i < vc; ++i)
  {
    vec3f q = RayData::unpackVert21(vbase + i * BVH_BLAS_VERT21_STRIDE);
    v_st(&vertsScratch[i].x, v_madd(q, invScale, bmin));
  }
  // ownIndices was dropped; materialise from the BLAS leaf walk and rebase indicesOfs to 0 so
  // callers keep their (idx_base + indicesOfs) idiom.
  idxScratch.reserve(node.indicesCount);
  idxScratch.clear();
  iterateNodeFaces((int)node.nodeIndex, [&](int, uint16_t i0, uint16_t i1, uint16_t i2) {
    idxScratch.push_back(i0);
    idxScratch.push_back(i1);
    idxScratch.push_back(i2);
  });
  node_copy = node;
  node_copy.verticesOfs = 0;
  node_copy.indicesOfs = 0;
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

  dag::Vector<Point3_vec4, framemem_allocator> vScratch;
  dag::Vector<uint16_t, framemem_allocator> iScratch;
  CollisionNode rebased;
  const Point3_vec4 *vbase = nullptr;
  const uint16_t *ibase = nullptr;
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

  dag::Vector<Point3_vec4, framemem_allocator> vScratch;
  dag::Vector<uint16_t, framemem_allocator> iScratch;
  CollisionNode rebased;
  const Point3_vec4 *vbase = nullptr;
  const uint16_t *ibase = nullptr;
  const CollisionNode *nodeForCall = nullptr;
  resolveNodeVertsForCall(node, vScratch, iScratch, rebased, vbase, ibase, nodeForCall);

  all_collres_nodes_t allNodes;
  all_collres_tri_indices_t allTriIndices;
  const bool isLightNode = node.indicesCount < (uint32_t)traceMeshNodeLocalApi.threshold;
  // -V::601 for some reason PVS glitches and fails with "The 'true' value becomes a class object" on calc_normal parameter
  const bool res = (isLightNode ? traceMeshNodeLocalApi.light.pfnTraceRayMeshNodeLocalAllHits
                                : traceMeshNodeLocalApi.heavy.pfnTraceRayMeshNodeLocalAllHits)(vbase, ibase, *nodeForCall, vLocalFrom,
    vLocalDir, in_t, true, force_no_cull, allNodes, allTriIndices);

  intersected_nodes_list.reserve(allNodes.size());
  for (int k = 0, cnt = allNodes.size(); k < cnt; k++)
  {
    const vec4f &normAndT = allNodes[k];
    auto &n = intersected_nodes_list.push_back();
    n.intersectionT = v_extract_w(normAndT);
    v_stu_p3(&n.intersectionPos.x, v_madd(vLocalDir, v_splat_w(normAndT), vLocalFrom));
    v_stu_p3(&n.normal.x, normAndT);
    n.triRef = tri_ref::make(node.nodeIndex, (uint32_t)allTriIndices[k], false);
  }
  if (sort_intersections)
    sort_collres_intersections(intersected_nodes_list);

  return res;
}

bool CollisionResource::testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth, int &out_node_id) const
{
  vec4f bsph = v_ldu_bsphere3(sphere);
  if (!v_test_bsph_bsph_intersection(bsph, getBoundingSphereXYZR()))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if ((filter && !filter(meshNode->nodeIndex)) || (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE)))
      continue;
    Point3 nodeNorm;
    float nodeDepth;
    if (test_sphere_node_intersection(sphere, meshNode, dir_norm, nodeNorm, nodeDepth))
    {
      out_norm = meshNode->tm % nodeNorm;
      out_depth = nodeDepth;
      out_node_id = meshNode->nodeIndex;
      return true;
    }
  }
  return false;
}

bool CollisionResource::testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius) const
{
  bbox3f bbox;
  vec3f radV = v_splats(radius);
  v_bbox3_init_by_bsph(bbox, v_ldu(&p0.x), radV);
  v_bbox3_add_pt(bbox, v_sub(v_ldu(&p1.x), radV));
  v_bbox3_add_pt(bbox, v_add(v_ldu(&p1.x), radV));

  if (!v_bbox3_test_sph_intersect(bbox, vBoundingSphere, v_splat_w(vBoundingSphere)))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    if (test_capsule_node_intersection(p0, p1, radius, meshNode))
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

  if (!v_bbox3_test_sph_intersect(bbox, vBoundingSphere, v_splat_w(vBoundingSphere)))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    if (!meshNode->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    mat44f nodeTm, nodeItm;
    v_mat44_make_from_43cu_unsafe(nodeTm, meshNode->tm.array);
    bbox3f nodeBox;
    v_bbox3_init(nodeBox, nodeTm, v_ldu_bbox3(meshNode->modelBBox));
    if (!v_bbox3_test_box_intersect(nodeBox, bbox))
      continue;
    dag::Vector<Point3_vec4, framemem_allocator> vScratch;
    dag::Vector<uint16_t, framemem_allocator> iScratch;
    CollisionNode rebased;
    const Point3_vec4 *vertsBase = nullptr;
    const uint16_t *idxBase = nullptr;
    const CollisionNode *nodeForCall = nullptr;
    resolveNodeVertsForCall(*meshNode, vScratch, iScratch, rebased, vertsBase, idxBase, nodeForCall);
    const uint16_t *__restrict cur = idxBase + nodeForCall->indicesOfs;
    PREFETCH_DATA(0, cur);
    const uint16_t *__restrict end = cur + nodeForCall->indicesCount;
    const Point3_vec4 *__restrict vertices = vertsBase + nodeForCall->verticesOfs;

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

    while (cur < end)
    {
      vec3f corner0 = v_ld(&vertices[*(cur++)].x);
      vec3f corner1 = v_ld(&vertices[*(cur++)].x);
      vec3f corner2 = v_ld(&vertices[*(cur++)].x);

      bbox3f triBox;
      v_bbox3_init(triBox, corner0);
      v_bbox3_add_pt(triBox, corner1);
      v_bbox3_add_pt(triBox, corner2);
      if (!v_bbox3_test_box_intersect(triBox, bbox))
        continue;

      vec3f triangleUnnormal = v_cross3(v_sub(corner2, corner0), v_sub(corner1, corner0));
      if (v_extract_x(v_dot3_x(moveDirUnnorm, triangleUnnormal)) > 0.0f)
        continue;

      bool isIntersected = v_test_triangle_triangle_intersection(corner0, corner1, corner2, localA00, localA01, localA11) ||
                           v_test_triangle_triangle_intersection(corner0, corner1, corner2, localA00, localA11, localA10);

      if (isIntersected)
      {
        vec3f trgCenter = v_mul(v_add(v_add(corner0, corner1), corner2), v_splats(0.333333f));
        v_stu_p3(&out_point.x, v_mat44_mul_vec3p(nodeTm, trgCenter));
        out_node_index = meshNode->nodeIndex;
        return true;
      }
    }
  }
  return false;
}

template <bool check_bounding>
DAGOR_NOINLINE bool CollisionResource::traceRayMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
  const CollisionNode &node,
  const vec4f &v_local_from, // better to hold it in memory
  const vec4f &v_local_dir,  // it prevents inefficient loop optimizations
  float &in_out_t, vec4f *v_out_norm)
{
  int resultIdx = -1;
  const uint16_t *__restrict indices = indicesBase + node.indicesOfs;
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

DAGOR_NOINLINE bool CollisionResource::traceCapsuleMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint16_t *idx_base,
  const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm,
  vec4f &v_out_pos) const
{
  bool ret = false;
  const uint16_t *__restrict indices = idx_base + node.indicesOfs;
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
    float hitRadius = v_extract_x(distance_to_line_x(pos, v_local_from, v_local_dir));
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
DAGOR_NOINLINE bool CollisionResource::traceRayMeshNodeLocalAllHits(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
  const CollisionNode &node,
  const vec4f &v_local_from, // better to hold it in memory
  const vec4f &v_local_dir,  // it prevents inefficient loop optimizations
  float in_t, bool calc_normal, bool force_no_cull, all_collres_nodes_t &ret_array, all_collres_tri_indices_t &tri_indices)
{
  const uint16_t *__restrict indices = indicesBase + node.indicesOfs;
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
DAGOR_NOINLINE bool CollisionResource::rayHitMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
  const CollisionNode &node, const vec3f &v_local_from, const vec3f &v_local_dir, float in_t)
{
  const uint16_t *__restrict indices = indicesBase + node.indicesOfs;
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

DAGOR_NOINLINE bool CollisionResource::capsuleHitMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint16_t *idx_base,
  const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, float radius) const
{
  const uint16_t *__restrict indices = idx_base + node.indicesOfs;
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

// Decode a node's per-face triangle into resource-local vert positions. Two paths:
//   - BLAS-resident (always MESH): walks the node's BLAS leaves (walkBlasResidentNodeLeavesForFaces)
//     to the face_idx-th sub-triangle and decodes its verts from the grid's vert21 array. For
//     low-frequency callers (aiTargetES random face, damage-debug, AssetViewer); hot per-face paths
//     should use iterateNodeFacesVerts.
//   - Non-resident: the original direct-index path. CONVEX nodes always land here.
bool CollisionResource::getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const
{
  const CollisionNode *n = getNode(node_id);
  if (!n || face_idx < 0 || (uint32_t)(face_idx * 3 + 2) >= n->indicesCount)
    return false;
  if (n->flags & CollisionNode::BLAS_RESIDENT)
  {
    const Grid &g = getBlasGridForResidentNode(*n);
    if (g.blasData.empty())
      return false;
    const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)n->verticesOfs * BVH_BLAS_VERT21_STRIDE;
    const vec3f invScale = g.blasInvScale;
    const vec3f bmin = g.blasBBox.bmin;
    bool found = false;
    walkBlasResidentNodeLeavesForFaces(*n, [&](int fi, uint16_t i0, uint16_t i1, uint16_t i2) {
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
  const uint16_t *idx = meshIndicesBase + n->indicesOfs;
  const Point3_vec4 *verts = meshVertsBase + n->verticesOfs;
  v0 = verts[idx[face_idx * 3 + 0]];
  v1 = verts[idx[face_idx * 3 + 1]];
  v2 = verts[idx[face_idx * 3 + 2]];
  return true;
}

// Decode a tri_ref_t back to the three source-triangle vertices.
// - BLAS refs (type=1): grid bit selects gridForTraceable/gridForCollidable; walk the quad leaf at
//   blasOffset directly, pick the sub-triangle per the sub-tri bit, unquantize via the grid's
//   blasScale/blasBBox. No side table -- the leaf carries enough on its own.
// - Non-BLAS refs (type=0): dispatch to getNodeFaceVerts via the encoded per-node srcFace.
// Returns false for invalid / non-tri refs or out-of-range indices.
bool CollisionResource::getNodeFaceVertsByRef(tri_ref_t ref, Point3 &v0, Point3 &v1, Point3 &v2) const
{
  if (!tri_ref::hasTri(ref))
    return false;
  if (!tri_ref::isBlas(ref))
    return getNodeFaceVerts((int)tri_ref::nodeIndex(ref), (int)tri_ref::faceIndex(ref), v0, v1, v2);

  const Grid &g = tri_ref::isCollidableGrid(ref) ? gridForCollidable : gridForTraceable;
  if (g.blasData.empty())
    return false;
  const uint64_t blasOfs = tri_ref::blasOffset(ref);
  if (blasOfs < BVH_BLAS_NODE_SIZE || blasOfs >= g.blasData.size())
    return false;
  const uint8_t *d = g.blasData.data();
  // Skip word is the 4th uint32 of the leaf's box header, immediately before the leaf body
  // iterateFiltered passes to leafCb (dataOfs = leafBoxStart + BVH_BLAS_NODE_SIZE, skip at dataOfs-4).
  const BlasLeafLayout L = decodeBlasLeafLayout(d, (uint32_t)blasOfs);
  const bool subTri = tri_ref::subTri(ref);
  // Sub-tri 0 (single or first of a quad): (v[0], v[o1], v[o2])
  // Sub-tri 1 of a quad: fan -> (v[0], v[o2], v[o3]); strip -> (v[o2], v[o1], v[o3]).
  uint32_t k0, k1, k2;
  if (L.isSingle || !subTri)
  {
    k0 = 0;
    k1 = L.o1;
    k2 = L.o2;
  }
  else if (L.isFan)
  {
    k0 = 0;
    k1 = L.o2;
    k2 = L.o3;
  }
  else
  {
    k0 = L.o2;
    k1 = L.o1;
    k2 = L.o3;
  }
  const uint8_t *vbase = d + L.vertBytesOfs;
  const vec3f invScale = g.blasInvScale;
  const vec3f bmin = g.blasBBox.bmin;
  v_stu_p3(&v0.x, v_madd(RayData::unpackVert21(vbase + k0 * BVH_BLAS_VERT21_STRIDE), invScale, bmin));
  v_stu_p3(&v1.x, v_madd(RayData::unpackVert21(vbase + k1 * BVH_BLAS_VERT21_STRIDE), invScale, bmin));
  v_stu_p3(&v2.x, v_madd(RayData::unpackVert21(vbase + k2 * BVH_BLAS_VERT21_STRIDE), invScale, bmin));
  return true;
}

bool CollisionResource::checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list) const
{
  intersected_nodes_list.clear();
  vec4f vPos = v_ldu(&pos.x);
  if (!v_test_vec_x_le(v_length3_sq(v_sub(vPos, vBoundingSphere)), v_splat_w(vBoundingSphere)))
    return false;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *meshNode = &allNodesList[mi];
    // Test only for bbox
    TMatrix itm = meshNode->getInverseTmFlags();
    Point3 localPos = itm * pos;
    if (meshNode->modelBBox & localPos)
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(meshNode->nodeIndex);
  }

  for (uint16_t bi = boxNodesHead; bi != CollisionNode::INVALID_IDX; bi = allNodesList[bi].nextNode)
  {
    const CollisionNode *boxNode = &allNodesList[bi];
    if (boxNode->modelBBox & pos)
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(boxNode->nodeIndex);
  }

  for (uint16_t si = sphereNodesHead; si != CollisionNode::INVALID_IDX; si = allNodesList[si].nextNode)
  {
    const CollisionNode *sphNode = &allNodesList[si];
    if (lengthSq(pos - sphNode->boundingSphere.c) <= get_bsphere_r2(sphNode->boundingSphere.r))
      ((IntersectedNode *)intersected_nodes_list.push_back_uninitialized())->triRef = tri_ref::makeForNonTri(sphNode->nodeIndex);
  }

  for (uint16_t ci = capsuleNodesHead; ci != CollisionNode::INVALID_IDX; ci = allNodesList[ci].nextNode)
  {
    const CollisionNode *capNode = &allNodesList[ci];
    if (capsules[capNode->capsuleIndex].isInside(pos))
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
  TMatrix finalTm = node_to_check.getInverseTmFlags() * tm1;
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
    reserve_and_resize(vertices, (uint32_t)node_to_move.verticesCount + 1u);
    // BLAS-resident path: decode vert21 instead of dereferencing meshVertsBase (the slice was
    // dropped at load time). iterateNodeVerts handles both cases transparently.
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
  TMatrix finalTm = node_to_check.getInverseTmFlags() * tm1;
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
    reserve_and_resize(vertices, (uint32_t)node_to_move.verticesCount + 1u);
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

  TMatrix testTm;
  if (test_node_tree)
    getCollisionNodeTm(node_to_test, tm_test, test_node_tree, testTm);
  else
    testTm = tm_test * node_to_test->tm;

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
    BSphere3 localSph = finalTm * make_node_bsphere(node_to_test->boundingSphere.c, node_to_test->boundingSphere.r);
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
    // BLAS-resident path: decode via iterateNodeVerts instead of meshVertsBase dereference.
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

  TMatrix restrainTm;
  if (restrain_node_tree)
    restraining_resource->getCollisionNodeTm(restraining_node, tm_restrain, restrain_node_tree, restrainTm);
  else
    restrainTm = tm_restrain * restraining_node->tm;

  return testInclusion(test_node_index, tm_test, restraining_resource->getNodeConvexPlanes(restraining_node_index), restrainTm,
    test_node_tree);
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

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT>(tm, nullptr /*geom_node_tree*/, v_ldu(&from.x),
    v_ldu(&dir.x), in_t, false /*out_normal*/, 1.f /*bsphere_scale*/, behavior_filter, nodeFilter, callback, nullptr /*stats*/,
    false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float bsphere_scale, const CollisionNodeMask *collision_node_mask, int *out_mat_id) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool {
    return node->checkBehaviorFlags(behaviorFilter) && (!collision_node_mask || collision_node_mask->test(node->nodeIndex, true));
  };

  auto callback = [out_mat_id](int /*trace_id*/, const CollisionNode *node, float /*t*/, vec3f /*normal*/, vec3f /*pos*/,
                    tri_ref_t /*tri_ref*/) {
    if (out_mat_id)
      *out_mat_id = node->physMatId;
  };

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT>(tm, geom_node_tree, v_ldu(&from.x), v_ldu(&dir.x),
    in_t, false /*out_normal*/, bsphere_scale, behaviorFilter, nodeFilter, callback, nullptr /*stats*/, false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::capsuleHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from,
  const Point3 &dir, float in_t, float radius, CollResHitNodesType &nodes_hit) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  CollisionTrace in;
  in.vFrom = v_ldu(&from.x);
  in.vDir = v_ldu(&dir.x);
  in.vTo = v_madd(in.vDir, v_splats(in_t), in.vFrom);
  in.t = in_t;
  in.capsuleRadius = radius;
  in.isectBounding = false;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int /*trace_id*/, const CollisionNode *node, float, vec3f, vec3f, tri_ref_t) {
    nodes_hit.push_back(node->nodeIndex);
  };

  constexpr bool is_single_ray = true;
  dag::Span<CollisionTrace> traces(&in, 1);
  return forEachIntersectedNode<ALL_NODES_INTERSECTIONS, CollisionTraceType::CAPSULE_HIT, is_single_ray>(tm, geom_node_tree, traces,
    false /*calc_normal*/, 1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*out_stats*/,
    false /*force_no_cull*/);
}

DAGOR_NOINLINE bool CollisionResource::multiRayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree,
  dag::Span<CollisionTrace> traces) const
{
  alignas(EA_CACHE_LINE_SIZE) mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
  uint8_t behaviorFilter = CollisionNode::TRACEABLE;

  auto nodeFilter = [&](const CollisionNode *node) -> bool { return node->checkBehaviorFlags(behaviorFilter); };

  auto callback = [&](int trace_id, const CollisionNode *node, float, vec3f, vec3f, tri_ref_t) {
    traces[trace_id].isHit = true;
    traces[trace_id].outMatId = node->physMatId;
    traces[trace_id].outNodeId = node->nodeIndex;
  };

  for (CollisionTrace &trace : traces)
  {
    trace.vTo = v_madd(trace.vDir, v_splats(trace.t), trace.vFrom);
    trace.isectBounding = false;
    trace.outMatId = PHYSMAT_INVALID;
    trace.outNodeId = -1;
    trace.isHit = false;
  }

  return forEachIntersectedNode<ANY_ONE_INTERSECTION, CollisionTraceType::RAY_HIT>(tm, geom_node_tree, traces, false /*calc_normal*/,
    1.f /*bsphere_scale*/, behaviorFilter, nodeFilter, callback, nullptr /*out_stats*/, false /*force_no_cull*/);
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
  for (auto &node : allNodesList)
    node.geomNodeId = geom_node_tree.findNodeIndex(getNodeNameStr(node));
}

CollisionResource *CollisionResource::deepCopy(void *inplace_ptr) const
{
  auto *collRes = new (inplace_ptr ? inplace_ptr : midmem->alloc(sizeof(CollisionResource)), _NEW_INPLACE) CollisionResource();
  collRes->vFullBBox = vFullBBox;
  collRes->vBoundingSphere = vBoundingSphere;
  collRes->boundingBox = boundingBox;
  collRes->boundingSphereRad = boundingSphereRad;
  collRes->collisionFlags = collisionFlags;
  collRes->bsphereCenterNode = bsphereCenterNode;
  collRes->allNodesList = allNodesList;
  collRes->relGeomNodeTms = relGeomNodeTms;
  collRes->names = names;
  collRes->capsules = capsules;
  collRes->convexPlanes = convexPlanes;
  // If source's mesh data lives in its own dense storage, deep-copy that storage.
  // Otherwise the data lives somewhere external (FRT or caller-owned span), and we
  // share the same base pointer; lifetime is the caller's problem.
  if (meshVertsBase == ownVertices.data())
  {
    collRes->ownVertices = ownVertices;
    collRes->meshVertsBase = collRes->ownVertices.data();
  }
  else
    collRes->meshVertsBase = meshVertsBase;
  if (meshIndicesBase == ownIndices.data())
  {
    collRes->ownIndices = ownIndices;
    collRes->meshIndicesBase = collRes->ownIndices.data();
  }
  else
    collRes->meshIndicesBase = meshIndicesBase;

  // compactOwnVertices stamps BLAS_RESIDENT on nodes whose verts/indices live in these grids'
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
    dst.blasNodeRanges = src.blasNodeRanges;
    dst.blasTwoSided = src.blasTwoSided; // cull mode must survive the clone or a two-sided asset reverts to CullCCW
  };
  copyGrid(collRes->gridForTraceable, gridForTraceable);
  copyGrid(collRes->gridForCollidable, gridForCollidable);

  collRes->rebuildNodesLL();
  return collRes;
}

void CollisionResource::sortNodesList()
{
  const char *namesData = names.empty() ? "" : names.data();
  stlsort::sort(allNodesList.begin(), allNodesList.end(), [namesData](const CollisionNode &left, const CollisionNode &right) {
    float szL = v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(left.modelBBox))));
    float szR = v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(right.modelBBox))));
    if (szL == szR)
      return strcmp(namesData + left.nameOfs, namesData + right.nameOfs) > 0;
    return szL > szR; // larger first
  });

  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
  {
    // We must rearrange relGeomNodeTms according to new order after sort
    Tab<TMatrix> relTms(framemem_ptr());
    reserve_and_resize(relTms, allNodesList.size());
    G_ASSERTF_RETURN(relGeomNodeTms.size() == allNodesList.size(), , "relGeomNodeTms.size()=%d allNodesList.size()=%d",
      relGeomNodeTms.size(), allNodesList.size());
    for (size_t nodeNo = 0; nodeNo < allNodesList.size(); nodeNo++)
    {
      const uint16_t nodeIdx = allNodesList[nodeNo].nodeIndex;
      G_ASSERT(nodeIdx < relGeomNodeTms.size());
      relTms[nodeNo] = relGeomNodeTms[nodeIdx];
    }
    mem_copy_from(relGeomNodeTms, relTms.data());
  }

  for (size_t nodeNo1 = 0; nodeNo1 < allNodesList.size(); nodeNo1++)
  {
    for (size_t nodeNo2 = nodeNo1 + 1; nodeNo2 < allNodesList.size(); nodeNo2++)
    {
      bbox3f bbox1 = v_ldu_bbox3(allNodesList[nodeNo1].modelBBox);
      bbox3f bbox2 = v_ldu_bbox3(allNodesList[nodeNo2].modelBBox);
      if (v_bbox3_test_box_inside(bbox1, bbox2)) // bbox2 inside of bbox1.
      {
        float bboxLenSq = v_extract_x(v_length3_sq_x(v_bbox3_size(bbox1)));
        uint16_t n2ins = allNodesList[nodeNo2].insideOfNode;
        if (allNodesList[nodeNo2].insideOfNode == 0xffff ||
            bboxLenSq < v_extract_x(v_length3_sq_x(v_bbox3_size(v_ldu_bbox3(allNodesList[n2ins].modelBBox)))))
        {
          allNodesList[nodeNo2].insideOfNode = nodeNo1;
        }
      }
    }
  }
}
void CollisionResource::rebuildNodesLL()
{
  // Re-stamping nodeIndex = position below invalidates every nodeIndex-keyed acceleration side
  // table when the list ORDER changed -- the real caller is the ECS collres__desc_add path, which
  // appends nodes to a deep-copied LOADED resource and then runs sortNodesList() + this. The
  // grids' blasNodeRanges map combined-BLAS leaves (and BLAS-resident vert blocks) to their source
  // node by nodeIndex, so rebase them through the old->new permutation; pre-stamp indices are
  // dense [0, size) for every caller (load/collapse renumber before sorting, add*Node stamps
  // size-1).
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
        // verticesCount uses count-minus-one encoding so 0 stored == 1 actual; gate on
        // indicesCount instead, which uses raw 0-means-empty semantics.
        G_ASSERT(node.indicesCount);
        allNodesList[meshNodesTail].nextNode = idx;
        meshNodesTail = idx;
      }
      else
        meshNodesHead = meshNodesTail = idx;
      ++numMeshNodes;
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

  if (DAGOR_UNLIKELY(needRemap && orderChanged))
  {
    for (Grid *g : {&gridForTraceable, &gridForCollidable})
      for (Grid::NodeRange &r : g->blasNodeRanges)
        if (r.nodeIndex < oldToNew.size())
          r.nodeIndex = oldToNew[r.nodeIndex];
  }
}

struct ITestIntersectionAlgo
{
  // a_head_idx / b_head_idx index into a_all / b_all and address the head of the per-type linked
  // list; CollisionNode::INVALID_IDX means the list is empty. Walk via allNodesList[i].nextNode.
  // res_a / res_b let the algo route BLAS_RESIDENT mesh nodes through the grid's vert21 array
  // (ownVertices was dropped at load time). Non-resident nodes index a_verts_base / b_verts_base.
  virtual bool apply(uint16_t a_head_idx, dag::ConstSpan<CollisionNode> a_all, const mat44f &tm_a, uint16_t b_head_idx,
    dag::ConstSpan<CollisionNode> b_all, const mat44f &tm_b, float a_max_scale, float b_max_scale, bool checkOnlyPhysNodes,
    const CollisionResource *res_a, const Point3_vec4 *a_verts_base, const uint16_t *a_idx_base, const CollisionResource *res_b,
    const Point3_vec4 *b_verts_base, const uint16_t *b_idx_base) = 0;
  virtual ~ITestIntersectionAlgo() = 0;

  Point3 collisionPointA;
  Point3 collisionPointB;
};

ITestIntersectionAlgo::~ITestIntersectionAlgo() {}

// Per-node verts source resolver for the testIntersection algos. Returns a pointer such that
// p[idx[k]] yields the k-th vert of `node` (already offset by node.verticesOfs):
//   - non-resident: fallback_verts_base + node.verticesOfs unchanged.
//   - BLAS_RESIDENT: decodes vert21 into scratch and returns scratch.data() (zero-based; node
//     indices 0..verticesCount map directly into the materialised scratch).
// Returns nullptr if BLAS_RESIDENT but the resolved grid has empty blasData (defensive).
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
    const CollisionResource *res_a, const Point3_vec4 * /*a_verts_base*/, const uint16_t * /*a_idx_base*/,
    const CollisionResource *res_b, const Point3_vec4 * /*b_verts_base*/, const uint16_t * /*b_idx_base*/) final
  {
    aResource = res_a;
    bResource = res_b;
    outsideOfBounding.clear();
    reserve_and_resize(outsideOfBounding, a_nodes_count);
    aWtms.clear();
    aWtms.reserve(a_nodes_count);
    aWtms.resize(a_nodes_count);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      mat44f vWtmA;
      v_mat44_make_from_43cu_unsafe(vWtmA, nodeA->tm.array);
      v_mat44_mul43(vWtmA, tm_a, vWtmA);
      vec4f aNodeWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x)), v_splats(nodeA->boundingSphere.r));
      outsideOfBounding.set(nodeA->nodeIndex, !isBoundingsIntersect(aNodeWbsph, v_set_x(a_max_scale), b_wbsph, v_set_x(b_max_scale)));
      aWtms[nodeA->nodeIndex] = vWtmA;
    }

    for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
    {
      const CollisionNode *nodeB = &b_all[ib];
      if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;

      mat44f vWtmB, vIWtmB;
      v_mat44_make_from_43cu_unsafe(vWtmB, nodeB->tm.array);
      v_mat44_mul43(vWtmB, tm_b, vWtmB);
      vec3f bNodeBoundingCenter = v_mat44_mul_vec3p(vWtmB, v_ldu(&nodeB->boundingSphere.c.x));
      if (!isBoundingsIntersect(bNodeBoundingCenter, v_set_x(b_max_scale), a_wbsph, v_set_x(a_max_scale)))
        continue;
      v_mat44_inverse43(vIWtmB, vWtmB);
      vec3f sphereCenterNodeB = v_mat44_mul_vec3p(vWtmB, v_ldu(&nodeB->boundingSphere.c.x));

      for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
      {
        const CollisionNode *nodeA = &a_all[ia];
        if (outsideOfBounding[nodeA->nodeIndex])
          continue;
        if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;

        mat44f vWtmA = aWtms[nodeA->nodeIndex];
        vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmA, tmAtoB, tmBtoA;
        v_mat44_inverse43(vIWtmA, vWtmA);
        v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);
        v_mat44_mul43(tmBtoA, vIWtmA, vWtmB);

        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = v_ldu_bbox3(nodeB->modelBBox);
        if (v_bbox3_test_trasformed_box_intersect_rel_tm(bboxA, tmAtoB, bboxB, tmBtoA) == false)
          continue;

        if (isMeshNodeIntersectedWithMeshNode(nodeA, nodeB, tmAtoB))
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
    const uint32_t treeBytes = b_grid->blasTreeBytes;
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
    QuadBLASWalk::iterateFiltered(
      bData, 0, (int)treeBytes,
      [boxMinQ, boxMaxQ](vec3f bmn, vec3f bmx) { return v_check_xyz_all_true(v_and(v_cmp_ge(boxMaxQ, bmn), v_cmp_ge(bmx, boxMinQ))); },
      // -V657: always returns false by design -- collects every overlapping candidate with no
      // early-out; false means "continue iterating" per the iterateFiltered contract.
      [this, bData, vertsOfs, nodeVOfs, nodeVEnd](vec3f v0, vec3f v1, vec3f v2, int dataOfs) -> bool { //-V657
        // Recover the leaf's owning node (first-vert byte offset -> vert21 index, tested against
        // node_b's range); leaves owned by sibling nodes are skipped.
        const int ofs1Rel = ((const int *)(bData + dataOfs))[0];
        const uint32_t v0ByteOfs = (uint32_t)((int)dataOfs + ofs1Rel);
        const uint32_t v0Idx = (v0ByteOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
        if (v0Idx < nodeVOfs || v0Idx >= nodeVEnd)
          return false; // not this node's leaf
        candidateTris.push_back({v0, v1, v2});
        return false; // continue
      },
      unquantVL);
    return true;
  }

  bool isMeshNodeIntersectedWithMeshNode(const CollisionNode *node_a, const CollisionNode *node_b, const mat44f &tm_a_to_b)
  {
    bool useBlas = false;
    if (b_grid && !b_grid->blasData.empty())
    {
      bbox3f bbox;
      v_bbox3_init(bbox, tm_a_to_b, v_ldu_bbox3(node_a->modelBBox));
      useBlas = collectBlasCandidates(bbox, node_b);
    }

    // iterateNodeFacesVerts dispatches on BLAS_RESIDENT internally (non-resident: ownIndices +
    // ownVertices; resident: walk the grid's BLAS, decode vert21), always yielding resource-local face
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
      mat44f tmA, tmB;
      v_mat44_make_from_43cu_unsafe(tmA, node_a->tm.array);
      v_mat44_make_from_43cu_unsafe(tmB, node_b->tm.array);
      vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
      vec3f bc = v_mul(v_add(b0, v_add(b1, b2)), v_splats(1 / 3.f));
      v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(tmA, ac));
      v_stu_p3(&collisionPointB.x, v_mat44_mul_vec3p(tmB, bc));
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
    const CollisionResource *res_a, const Point3_vec4 * /*a_verts_base*/, const uint16_t * /*a_idx_base*/,
    const CollisionResource * /*res_b*/, const Point3_vec4 * /*b_verts_base*/, const uint16_t * /*b_idx_base*/) final
  {
    aResource = res_a;
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, vIWtmA, tmAtoB, tmBtoA;
      v_mat44_make_from_43cu_unsafe(vWtmA, nodeA->tm.array);
      v_mat44_mul43(vWtmA, tm_a, vWtmA);
      v_mat44_mul43(tmAtoB, vIWtmB, vWtmA);
      v_mat44_inverse43(vIWtmA, vWtmA);
      v_mat44_mul43(tmBtoA, vIWtmA, tm_b);
      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));

      for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&nodeB->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        bbox3f bboxA = v_ldu_bbox3(nodeA->modelBBox);
        bbox3f bboxB = v_ldu_bbox3(nodeB->modelBBox);
        if (v_bbox3_test_trasformed_box_intersect_rel_tm(bboxA, tmAtoB, bboxB, tmBtoA) == false)
          continue;

        // iterateNodeFacesVerts dispatches on BLAS_RESIDENT internally; ownIndices may have been
        // dropped (compactOwnVertices), so the prior aIdxBase + nodeA->indicesOfs read was stale.
        bool found = false;
        aResource->iterateNodeFacesVerts(nodeA->nodeIndex, [&](int, vec4f a0, vec4f a1, vec4f a2) {
          if (found)
            return;
          Point3_vec4 a0b, a1b, a2b;
          v_st(&a0b.x, v_mat44_mul_vec3p(tmAtoB, a0));
          v_st(&a1b.x, v_mat44_mul_vec3p(tmAtoB, a1));
          v_st(&a2b.x, v_mat44_mul_vec3p(tmAtoB, a2));
          if (test_triangle_box_intersection(a0b, a1b, a2b, nodeB->modelBBox))
          {
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1 / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
            collisionPointB = nodeB->boundingSphere.c;
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
    const CollisionResource * /*res_a*/, const Point3_vec4 * /*a_verts_base*/, const uint16_t * /*a_idx_base*/,
    const CollisionResource * /*res_b*/, const Point3_vec4 * /*b_verts_base*/, const uint16_t * /*b_idx_base*/) final
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmA, vIWtmB, tmBToA, tmAToB;
    v_mat44_inverse43(vIWtmA, tm_a);
    v_mat44_inverse43(vIWtmB, tm_b);
    v_mat44_mul43(tmBToA, vIWtmA, tm_b);
    v_mat44_mul43(tmAToB, vIWtmB, tm_a);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(tm_a, v_ldu(&nodeA->boundingSphere.c.x));
      for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&nodeB->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        if (v_bbox3_test_trasformed_box_intersect_rel_tm(v_ldu_bbox3(nodeA->modelBBox), tmAToB, v_ldu_bbox3(nodeB->modelBBox), tmBToA))
        {
          collisionPointA = nodeA->boundingSphere.c;
          collisionPointB = nodeB->boundingSphere.c;
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
    const CollisionResource *res_a, const Point3_vec4 * /*a_verts_base*/, const uint16_t * /*a_idx_base*/,
    const CollisionResource * /*res_b*/, const Point3_vec4 * /*b_verts_base*/, const uint16_t * /*b_idx_base*/) final
  {
    aResource = res_a;
    alignas(EA_CACHE_LINE_SIZE) mat44f vIWtmB;
    v_mat44_inverse43(vIWtmB, tm_b);

    for (uint16_t ia = a_head_idx; ia != CollisionNode::INVALID_IDX; ia = a_all[ia].nextNode)
    {
      const CollisionNode *nodeA = &a_all[ia];
      if (checkOnlyPhysNodes && !nodeA->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
        continue;
      alignas(EA_CACHE_LINE_SIZE) mat44f vWtmA, tmAToB;
      v_mat44_make_from_43cu_unsafe(vWtmA, nodeA->tm.array);
      v_mat44_mul43(vWtmA, tm_a, vWtmA);
      v_mat44_mul43(tmAToB, vIWtmB, vWtmA);

      vec3f sphereCenterNodeA = v_mat44_mul_vec3p(vWtmA, v_ldu(&nodeA->boundingSphere.c.x));
      for (uint16_t ib = b_head_idx; ib != CollisionNode::INVALID_IDX; ib = b_all[ib].nextNode)
      {
        const CollisionNode *nodeB = &b_all[ib];
        if (checkOnlyPhysNodes && !nodeB->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
          continue;
        vec3f sphereCenterNodeB = v_mat44_mul_vec3p(tm_b, v_ldu(&nodeB->boundingSphere.c.x));
        float sumRad = nodeA->boundingSphere.r * a_max_scale + nodeB->boundingSphere.r * b_max_scale;
        if (v_extract_x(v_length3_sq_x(v_sub(sphereCenterNodeA, sphereCenterNodeB))) > sumRad * sumRad)
          continue;

        bool found = false;
        aResource->iterateNodeFacesVerts(nodeA->nodeIndex, [&](int, vec4f a0, vec4f a1, vec4f a2) {
          if (found)
            return;
          vec3f a0b = v_mat44_mul_vec3p(tmAToB, a0);
          vec3f a1b = v_mat44_mul_vec3p(tmAToB, a1);
          vec3f a2b = v_mat44_mul_vec3p(tmAToB, a2);
          if (v_test_triangle_sphere_intersection(a0b, a1b, a2b, v_ldu(&nodeB->boundingSphere.c.x),
                v_set_x(get_bsphere_r2(nodeB->boundingSphere.r))))
          {
            vec3f ac = v_mul(v_add(a0, v_add(a1, a2)), v_splats(1.f / 3.f));
            v_stu_p3(&collisionPointA.x, v_mat44_mul_vec3p(vWtmA, ac));
            collisionPointB = nodeB->boundingSphere.c;
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

  alignas(EA_CACHE_LINE_SIZE) mat44f vTmA, vTmB;
  v_mat44_make_from_43cu_unsafe(vTmA, tm_a.array);
  v_mat44_make_from_43cu_unsafe(vTmB, tm_b.array);
  float maxScaleA = v_extract_x(v_mat44_max_scale43_x(vTmA));
  float maxScaleB = v_extract_x(v_mat44_max_scale43_x(vTmB));

  // world xyz|r
  vec4f aWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vTmA, res_a->vBoundingSphere), v_splats(res_a->boundingSphereRad));
  vec4f bWbsph = v_perm_xyzd(v_mat44_mul_vec3p(vTmB, res_b->vBoundingSphere), v_splats(res_b->boundingSphereRad));
  if (!TestMeshNodeMeshNodesIntersectionAlgo::isBoundingsIntersect(aWbsph, v_set_x(maxScaleA), bWbsph, v_set_x(maxScaleB)))
    return false;

  TestMeshNodeMeshNodesIntersectionAlgo testMeshNodeMeshNodesIntersectionAlgo(aWbsph, res_a->allNodesList.size(), bWbsph,
    useTraceFaces ? &res_b->gridForTraceable : nullptr);
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
          maxScaleB, checkOnlyPhysNodes, res_a, res_a->meshVertsBase, res_a->meshIndicesBase, res_b, res_b->meshVertsBase,
          res_b->meshIndicesBase);

        collisionPointA = testAB->collisionPointA;
        collisionPointB = testAB->collisionPointB;
      }
      else if (ITestIntersectionAlgo *testBA = arrIntersectionCall[nodeTypeIxB][nodeTypeIxA])
      {
        result = testBA->apply(res_b->nodeLists[nodeTypeIxB], bAll, vTmB, res_a->nodeLists[nodeTypeIxA], aAll, vTmA, maxScaleB,
          maxScaleA, checkOnlyPhysNodes, res_b, res_b->meshVertsBase, res_b->meshIndicesBase, res_a, res_a->meshVertsBase,
          res_a->meshIndicesBase);

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
// res2's BLAS (when populated) prunes node2's faces by node1's bbox in res2 frame AND filters to
// leaves owned by node2 (the combined BLAS interleaves leaves from every IDENT MESH node). Without a
// BLAS, falls back to brute-force tri-tri over node2's faces via iterateNodeFacesVerts.
// Both branches feed verts in their natural source frame (BLAS: res2 resource-local; brute: node-local
// from ownVertices) into the inner `tm2to1 * v2` -- consistent because the BLAS holds only
// IDENT-transform nodes, so node-local == res2-local.
bool CollisionResource::testMeshNodePair(const CollisionNode *node1, dag::ConstSpan<Point3_vec4> node1Faces,
  const CollisionResource *res2, const CollisionNode *node2, const TMatrix &tm1ToWorld, const TMatrix &tm2, const TMatrix &tm2to1,
  Point3 &cp1, Point3 &cp2, uint16_t *node_index1, uint16_t *node_index2)
{
  dag::Vector<Point3_vec4, framemem_allocator> res2Faces;
  const CollisionResource::Grid &res2Blas = res2->getBlasGrid(CollisionNode::TRACEABLE);
  const CollisionResource::Grid::NodeRange *nr2 = nullptr;
  if (!res2Blas.blasData.empty())
    for (const auto &r : res2Blas.blasNodeRanges)
      if (r.nodeIndex == node2->nodeIndex)
      {
        nr2 = &r;
        break;
      }
  if (!nr2)
  {
    // node2 isn't in res2's traceable BLAS (res2 has no BLAS, or node2 is non-IDENT /
    // PHYS_COLLIDABLE-only / sub-gate and never entered gridForTraceable). Brute-force its faces;
    // iterateNodeFacesVerts dispatches on BLAS_RESIDENT internally (gridForCollidable residents still
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
    const TMatrix tm1to2 = inverse(tm2) * tm1ToWorld;
    bbox3f bboxRes2v = v_ldu_bbox3(tm1to2 * node1->modelBBox);
    const vec3f boxMinQ = v_madd(bboxRes2v.bmin, res2Blas.blasScale, res2Blas.blasOfs);
    const vec3f boxMaxQ = v_madd(bboxRes2v.bmax, res2Blas.blasScale, res2Blas.blasOfs);
    const BlasLocalUnquant unquantVL = BlasLocalUnquant::make(res2Blas.blasBBox.bmin, res2Blas.blasScale);
    const uint8_t *bData = res2Blas.blasData.data();
    const uint32_t treeBytes = res2Blas.blasTreeBytes;
    const uint32_t vertsOfs = res2Blas.blasVertsOfs();
    const uint32_t nodeVOfs = nr2->verticesOfs;
    const uint32_t nodeVEnd = nr2->verticesEnd;
    QuadBLASWalk::iterateFiltered(
      bData, 0, (int)treeBytes,
      [boxMinQ, boxMaxQ](vec3f bmn, vec3f bmx) { return v_check_xyz_all_true(v_and(v_cmp_ge(boxMaxQ, bmn), v_cmp_ge(bmx, boxMinQ))); },
      // -V657: always returns false by design -- collects every overlapping candidate with no
      // early-out; false means "continue iterating" per the iterateFiltered contract.
      [&res2Faces, bData, vertsOfs, nodeVOfs, nodeVEnd](vec3f v0, vec3f v1, vec3f v2, int dataOfs) -> bool { //-V657
        const int ofs1Rel = ((const int *)(bData + dataOfs))[0];
        const uint32_t v0ByteOfs = (uint32_t)((int)dataOfs + ofs1Rel);
        const uint32_t v0Idx = (v0ByteOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
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
        cp1 = node1->tm * ((v1_0 + v1_1 + v1_2) * 0.333333f);
        cp2 = node2->tm * ((v2_0 + v2_1 + v2_2) * 0.333333f);
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

  float scale1 = tm1.getcol(1).length();
  float scale2 = tm2.getcol(1).length();

  TMatrix modelTm2to1 = inverse(tm1) * tm2;

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

  // Per-node1 face materialisation feeds the per-pair mesh/box/sphere sub-loops below.
  // iterateNodeFacesVerts dispatches on BLAS_RESIDENT internally; cache once per node1 because a
  // BLAS-resident walk is O(total_leaves) (no per-node side table) and would otherwise rerun per node2.
  for (uint16_t mi1 = res1->meshNodesHead; mi1 != CollisionNode::INVALID_IDX; mi1 = res1->allNodesList[mi1].nextNode)
  {
    const CollisionNode *node1 = &res1->allNodesList[mi1];
    if (filter1 && !filter1(node1->nodeIndex))
      continue;

    Point3 sphereCenter1 = tm1 * (node1->tm * node1->boundingSphere.c);
    TMatrix invTm1, tm1ToWorld;
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

    for (uint16_t mi2 = res2->meshNodesHead; mi2 != CollisionNode::INVALID_IDX; mi2 = res2->allNodesList[mi2].nextNode)
    {
      const CollisionNode *node2 = &res2->allNodesList[mi2];
      _INC(numNodesDebug);

      if (filter2 && !filter2(node2->nodeIndex))
        continue;

      if (node2->insideOfNode != 0xffff && boxOutside[node2->insideOfNode])
      {
        boxOutside[node2->nodeIndex] = true;
        continue;
      }

      _INC(numBoxesInsideDebug);

      Point3 sphereCenter2 = tm2 * (node2->tm * node2->boundingSphere.c);
      float sumRad = node1->boundingSphere.r * scale1 + node2->boundingSphere.r * scale2;
      if (lengthSq(sphereCenter1 - sphereCenter2) >= sumRad * sumRad)
        continue;
      _INC(numSpheresDebug);

      if (!invTm1ready)
      {
        tm1ToWorld = tm1 * node1->tm;
        invTm1 = inverse(tm1ToWorld);
        invTm1ready = true;
      }
      TMatrix tm2to1 = invTm1 * (tm2 * node2->tm);

      if (!test_box_box_intersection(node1->modelBBox, node2->modelBBox, tm2to1))
      {
        boxOutside[node2->nodeIndex] = true;
        continue;
      }
      _INC(numBoxesDebug);

      if (testMeshNodePair(node1, make_span_const(node1FaceVerts), res2, node2, tm1ToWorld, tm2, tm2to1, collisionPoint1,
            collisionPoint2, nodeIndex1, nodeIndex2))
      {
        if (!node_indices1)
          return true;
        node_indices1->push_back(node1->nodeIndex);
      }
    }

    TMatrix tm1to2;
    bool tm1to2ready = false;

    auto faceCentroidWorld = [&](size_t i1) {
      return node1->tm * ((node1FaceVerts[i1] + node1FaceVerts[i1 + 1] + node1FaceVerts[i1 + 2]) * 0.333333f);
    };

    for (uint16_t bi2 = res2->boxNodesHead; bi2 != CollisionNode::INVALID_IDX; bi2 = res2->allNodesList[bi2].nextNode)
    {
      const CollisionNode *node2 = &res2->allNodesList[bi2];
      Point3 sphereCenter2 = tm2 * (node2->tm * node2->boundingSphere.c);

      float sumRad = node1->boundingSphere.r * scale1 + node2->boundingSphere.r * scale2;
      if (lengthSq(sphereCenter1 - sphereCenter2) >= sumRad * sumRad)
        continue;
      if (!test_box_box_intersection(node1->modelBBox, node2->modelBBox, modelTm2to1))
        continue;
      if (!tm1to2ready)
      {
        tm1to2 = inverse(tm2) * tm1 * node1->tm;
        tm1to2ready = true;
      }

      for (size_t i1 = 0; i1 + 2 < node1FaceVerts.size(); i1 += 3)
      {
        if (!test_triangle_box_intersection(tm1to2 * node1FaceVerts[i1], tm1to2 * node1FaceVerts[i1 + 1],
              tm1to2 * node1FaceVerts[i1 + 2], node2->modelBBox))
          continue;
        collisionPoint1 = faceCentroidWorld(i1);
        collisionPoint2 = node2->boundingSphere.c;
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

    for (uint16_t si2 = res2->sphereNodesHead; si2 != CollisionNode::INVALID_IDX; si2 = res2->allNodesList[si2].nextNode)
    {
      const CollisionNode *node2 = &res2->allNodesList[si2];
      Point3 sphereCenter2 = tm2 * (node2->tm * node2->boundingSphere.c);

      float sumRad = node1->boundingSphere.r * scale1 + node2->boundingSphere.r * scale2;
      if (lengthSq(sphereCenter1 - sphereCenter2) >= sqr(sumRad))
        continue;
      if (!tm1to2ready)
      {
        tm1to2 = inverse(tm2) * tm1 * node1->tm;
        tm1to2ready = true;
      }

      const auto node2BSphere = make_node_bsphere(node2->boundingSphere.c, node2->boundingSphere.r);
      for (size_t i1 = 0; i1 + 2 < node1FaceVerts.size(); i1 += 3)
      {
        if (!test_triangle_sphere_intersection(tm1to2 * node1FaceVerts[i1], tm1to2 * node1FaceVerts[i1 + 1],
              tm1to2 * node1FaceVerts[i1 + 2], node2BSphere))
          continue;
        collisionPoint1 = faceCentroidWorld(i1);
        collisionPoint2 = node2->boundingSphere.c;
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
    out_tm = getMeshNodeTmInline(node, instance_tm, v_zero(), geom_node_tree);
  else
    out_tm = instance_tm;
}

__forceinline mat44f CollisionResource::getMeshNodeTmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
  const GeomNodeTree *geom_node_tree) const
{
  mat44f outTm;
  if (auto idx = (geom_node_tree && node->geomNodeId.index() < geom_node_tree->nodeCount()) ? node->geomNodeId : dag::Index16())
  {
    outTm = geom_node_tree->getNodeWtmRel(idx); //-V1004
    outTm.col3 = v_add(outTm.col3, v_sub(geom_node_tree->getWtmOfs(), instance_woffset));
    if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    {
      mat44f relGeomNodeTm;
      v_mat44_make_from_43cu_unsafe(relGeomNodeTm, relGeomNodeTms[node->nodeIndex].array);
      v_mat44_mul43(outTm, outTm, relGeomNodeTm);
    }
  }
  else
  {
    mat44f nodeTm;
    v_mat44_make_from_43cu_unsafe(nodeTm, node->tm.m[0]);
    v_mat44_mul43(outTm, instance_tm, nodeTm);
  }
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
  const bool useBlas = !blasGrid.blasData.empty();
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

    QuadBLASWalk::iterateFiltered(
      blasGrid.blasData.data(), 0, (int)blasGrid.blasTreeBytes,
      [vCapMinBox, vCapMaxBox](vec3f bmin, vec3f bmax) {
        return v_check_xyz_all_true(v_and(v_cmp_ge(vCapMaxBox, bmin), v_cmp_ge(bmax, vCapMinBox)));
      },
      [&](vec3f v0, vec3f v1, vec3f v2, int /*dataOfs*/) -> bool {
        clipTri(v0, v1, v2, haveMoveDir);
        return false;
      },
      unquantVL);
  }

  // Per-node fallback for collidable nodes the BLAS branch misses: CONVEX nodes (never in the BLAS),
  // non-IDENT mesh nodes (kept out by isEligibleForBlas), and -- when no BLAS was built (small or
  // SOLID resource) -- every collidable mesh node. Without this they silently stop contributing to
  // capsule clipping (e.g. convex collision in rendInstGenCollision.cpp). Skips the BLAS-eligible
  // IDENT mesh nodes the BLAS branch already clipped. Triangles are read node-local
  // (iterateNodeFacesVerts) and transformed by node->tm into resource-local (the capsule/BLAS frame).
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &allNodesList[mi];
    if (!node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE) || node->indicesCount == 0)
      continue;
    if (useBlas && node->type == COLLISION_NODE_TYPE_MESH && (node->flags & CollisionNode::IDENT))
      continue; // already handled by the BLAS walk above (mirrors isEligibleForBlas)

    // SOLID nodes trace without back-face culling (never in the BLAS -- a SOLID node aborts buildBLAS),
    // so disable the movedir cull to match FRT/per-node semantics.
    const bool cull = haveMoveDir && !node->checkBehaviorFlags(CollisionNode::SOLID);
    mat44f nodeTm;
    v_mat44_make_from_43cu_unsafe(nodeTm, node->tm.m[0]); // node-local -> resource-local
    iterateNodeFacesVerts(mi, [&](int, vec4f lv0, vec4f lv1, vec4f lv2) {
      clipTri(v_mat44_mul_vec3p(nodeTm, lv0), v_mat44_mul_vec3p(nodeTm, lv1), v_mat44_mul_vec3p(nodeTm, lv2), cull);
    });
  }
}

bool CollisionResource::test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm,
  Point3 &out_norm, float &out_depth) const
{
  TMatrix itm = inverse(node->tm);

  BSphere3 localSphere(itm * sphere.c, sphere.r);
  if (!(node->modelBBox & localSphere))
    return false;

  // Resolve verts + indices: ownVertices/ownIndices for non-resident; decoded vert21 +
  // re-materialised indices for BLAS-resident. The helper handles both internally.
  dag::Vector<Point3_vec4, framemem_allocator> vScratch;
  dag::Vector<uint16_t, framemem_allocator> iScratch;
  CollisionNode rebased;
  const Point3_vec4 *vertsBase = nullptr;
  const uint16_t *idxBase = nullptr;
  const CollisionNode *nodeForCall = nullptr;
  resolveNodeVertsForCall(*node, vScratch, iScratch, rebased, vertsBase, idxBase, nodeForCall);

  const uint16_t *__restrict cur = idxBase + nodeForCall->indicesOfs;
  PREFETCH_DATA(0, cur);
  const uint16_t *__restrict end = cur + nodeForCall->indicesCount;
  const Point3_vec4 *__restrict vertices = vertsBase + nodeForCall->verticesOfs;
  const Point3 localDirNorm = itm % dir_norm;

  PREFETCH_DATA(128, cur);
  while (cur < end)
  {
    const Point3_vec4 &corner0 = vertices[*(cur++)];
    const Point3_vec4 &corner1 = vertices[*(cur++)];
    const Point3_vec4 &corner2 = vertices[*(cur++)];

    if (test_triangle_sphere_intersection(corner0, corner1, corner2, localSphere))
    {
      const Point3 cross = (corner1 - corner0) % (corner2 - corner0);

      if (localDirNorm * cross < -VERY_SMALL_NUMBER)
        continue;

      const float crossLen = cross.length();
      out_norm = cross * safeinv(crossLen);
      out_depth = -out_norm * (corner0 - localSphere.c) - sphere.r;
      return true;
    }
  }
  return false;
}

bool CollisionResource::test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius,
  const CollisionNode *node) const
{
  TMatrix itm = inverse(node->tm);
  Point3 localCylinderPoint0 = itm * p0;
  Point3 localCylinderPoint1 = itm * p1;

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

  dag::Vector<Point3_vec4, framemem_allocator> vScratch;
  dag::Vector<uint16_t, framemem_allocator> iScratch;
  CollisionNode rebased;
  const Point3_vec4 *vertsBase = nullptr;
  const uint16_t *idxBase = nullptr;
  const CollisionNode *nodeForCall = nullptr;
  resolveNodeVertsForCall(*node, vScratch, iScratch, rebased, vertsBase, idxBase, nodeForCall);

  const uint16_t *__restrict cur = idxBase + nodeForCall->indicesOfs;
  PREFETCH_DATA(0, cur);
  const uint16_t *__restrict end = cur + nodeForCall->indicesCount;
  const Point3_vec4 *__restrict vertices = vertsBase + nodeForCall->verticesOfs;

  PREFETCH_DATA(128, cur);
  while (cur < end)
  {
    const Point3_vec4 &corner0 = vertices[*(cur++)];
    const Point3_vec4 &corner1 = vertices[*(cur++)];
    const Point3_vec4 &corner2 = vertices[*(cur++)];

    if (test_triangle_sphere_intersection(corner0, corner1, corner2, localSphere0) ||
        test_triangle_sphere_intersection(corner0, corner1, corner2, localSphere1) ||
        test_triangle_cylinder_intersection(corner0, corner1, corner2, localCylinderPoint0, localCylinderPoint1, radius))
      return true;
  }

  return false;
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
  blasNodeRanges.clear();
  blasNodeRanges.shrink_to_fit();
  blasTwoSided = false;
}

int CollisionResource::getMemoryUsed() const
{
  auto blasMem = [](const Grid &g) -> int { return (int)(g.blasData.size() + g.blasNodeRanges.size() * sizeof(Grid::NodeRange)); };
  int mem = sizeof(*this);
  mem += (int)(ownVertices.size() * sizeof(Point3_vec4));
  mem += (int)(ownIndices.size() * sizeof(uint16_t));
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

bool CollisionResource::validateVerticesForJolt(const char *res_name)
{
  return validateVerticesForJolt(res_name, [](const uint16_t *, const CollisionNode *) {});
}

dag::Vector<DegenerativeNodeData> CollisionResource::getDegenerativeNodes(const char *res_name)
{
  dag::Vector<DegenerativeNodeData> nodes;
  validateVerticesForJolt(res_name, [&nodes](const uint16_t *idx, const CollisionNode *node) {
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
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &allNodesList[mi];
    // BLAS-resident verts are already 21-bit-quantised in vert21; the Point3_vec4 source the
    // pre-encoding precision check needs is gone. That check flags content that won't survive Jolt's
    // quantisation -- vert21 verts already survived it at build time. Skip them.
    if (node->flags & CollisionNode::BLAS_RESIDENT)
      continue;
    const uint16_t *__restrict idxBase = meshIndicesBase + node->indicesOfs;
    const uint16_t *__restrict idxEnd = idxBase + node->indicesCount;
    const Point3_vec4 *__restrict vertices = meshVertsBase + node->verticesOfs;
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
    for (const uint16_t *__restrict cur = idxBase; cur < idxEnd; cur++)
      v_bbox3_add_pt(bb, v_ld(&vertices[*cur].x));
    // compress_scale = COMPONENT_MASK / max(size, 1e-20), quantize = trunc((v - bmin) * scale + 0.5) --
    // operation-for-operation Jolt's ValidationContext (Vec3::ToInt is the same cvttps truncation).
    const vec4f compressScale = v_div(v_splats((float)((1 << 21) - 1)), v_max(v_bbox3_size(bb), v_splats(1.0e-20f)));
    for (const uint16_t *__restrict cur = idxBase; cur < idxEnd;)
    {
      const uint16_t *triangleStartIdx = cur;
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
