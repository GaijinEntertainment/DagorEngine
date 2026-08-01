//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Point and packet query walkers over the SoA4 BLAS layout (dag_swBLAS_soa4.h): vertical-line
// max-height queries (the landmesh getHeight/getHeightBelow shape), small coherent ray packets
// sharing one traversal, box-region leaf/triangle collection, and deepest-covering subtree refs
// for XZ narrowing grids. Acceleration here only ever over-includes, so results match the
// equivalent per-query traversals exactly. All walkers accept a start ref that is an internal node
// ref, a LeafRef, or the degenerate whole-BLAS-is-one-leaf root block; they all instantiate one
// shared stack-walk skeleton (maskWalk) with a per-query node-mask policy.

#include <daBVH/dag_swBLAS_soa4.h>
#include <math/dag_traceRayTriangle.h> // get4TrianglesHtWT: the shared water-tight height kernel

namespace soa4
{

// Vertical-line (XZ) max-height accumulator: track the highest surface at the query XZ, optionally
// only at or below `limit`. When wantNormal is set, normalBox holds the winning triangle's
// unnormalized geometric normal in box space; positions map world->box as b = w * scale, so the
// world normal is normalize(normalBox * scale) (diagonal-transform covariance).
// CULL_DOWN on the height walkers skips downward-facing triangles (the vertical-query equivalent of
// a backface cull); it is a template argument because every caller knows it statically.
struct HeightAcc
{
  float maxHt = -1e30f;
  float limit = 1e30f; // box-space Y
  bool wantNormal = false;
  bool hit = false;
  vec3f normalBox = v_zero();
};

// 4-wide vertical-line test of one double-quad leaf body: all up-to-4 sub-triangles in one SoA pass
// (the XZ sibling of swblas_rayLeaf_SoA). The height math is the shared water-tight kernel
// get4TrianglesHtWT (sign-exact, matching RayTriangleIntersectInfXZ acceptance); this wrapper only
// decodes the leaf into corner lanes and folds the accumulator policy (limit window, max reduce,
// winning-lane normal).
template <bool CULL_DOWN>
static __forceinline void heightLeaf_SoA(const uint8_t *data, const QuadLeafFields &f, int body_ofs, vec3f p, HeightAcc &acc)
{
  const int baseA = body_ofs + (int)f.relBaseBytes;
  vec4f xsA, ysA, zsA;
  swblas_unpack4SoA<8>(data, baseA, f.o1, f.o2, f.o3, xsA, ysA, zsA);
  vec4f xsB, ysB, zsB;
  bool flipB = false;
  if (f.hasB)
  {
    flipB = f.flipSecondB;
    swblas_unpack4SoA<8>(data, baseA + (int)f.deltaB * 8, f.o1b, f.o2b, f.o3b, xsB, ysB, zsB);
  }
  else
    xsB = ysB = zsB = v_zero(); // zero-area lanes 2,3 self-reject via det == 0
  vec4f a0x, a0y, a0z, a1x, a1y, a1z, a2x, a2y, a2z;
  vec4f b0x, b0y, b0z, b1x, b1y, b1z, b2x, b2y, b2z;
  swblas_quadCorners2(f.flipSecond, xsA, ysA, zsA, a0x, a0y, a0z, a1x, a1y, a1z, a2x, a2y, a2z);
  swblas_quadCorners2(flipB, xsB, ysB, zsB, b0x, b0y, b0z, b1x, b1y, b1z, b2x, b2y, b2z);
  mat43f p0, p1, p2;
  p0.row0 = v_perm_xyab(a0x, b0x), p0.row1 = v_perm_xyab(a0y, b0y), p0.row2 = v_perm_xyab(a0z, b0z);
  p1.row0 = v_perm_xyab(a1x, b1x), p1.row1 = v_perm_xyab(a1y, b1y), p1.row2 = v_perm_xyab(a1z, b1z);
  p2.row0 = v_perm_xyab(a2x, b2x), p2.row1 = v_perm_xyab(a2y, b2y), p2.row2 = v_perm_xyab(a2z, b2z);
  vec4f ht;
  vec4f valid = get4TrianglesHtWT<CULL_DOWN>(p, ht, p0, p1, p2);
  valid = v_and(valid, v_cmp_ge(v_splats(acc.limit), ht));
  ht = v_sel(v_splats(-1e30f), ht, valid);
  vec4f m = v_max(ht, v_perm_zwxy(ht));
  m = v_max(m, v_perm_yzwx(m));
  const float h = v_extract_x(m);
  if (h > acc.maxHt)
  {
    if (acc.wantNormal)
    {
      const vec4f e1x = v_sub(p1.row0, p0.row0), e1y = v_sub(p1.row1, p0.row1), e1z = v_sub(p1.row2, p0.row2);
      const vec4f e2x = v_sub(p2.row0, p0.row0), e2y = v_sub(p2.row1, p0.row1), e2z = v_sub(p2.row2, p0.row2);
      const vec4f nx = v_sub(v_mul(e1y, e2z), v_mul(e1z, e2y));
      const vec4f ny = v_sub(v_mul(e1z, e2x), v_mul(e1x, e2z));
      const vec4f nz = v_sub(v_mul(e1x, e2y), v_mul(e1y, e2x));
      const int lane = (int)__bsf_unsafe((unsigned)v_truemask(v_cmp_eq(ht, m)));
      alignas(16) float na[3][4];
      v_st(na[0], nx);
      v_st(na[1], ny);
      v_st(na[2], nz);
      acc.normalBox = v_make_vec4f(na[0][lane], na[1][lane], na[2][lane], 0);
    }
    acc.maxHt = h;
    acc.hit = true;
  }
}

// The shared stack-walk skeleton every query walker instantiates. node_mask(NodeSoA) returns the
// surviving-lane bitmask of a node's SoA child boxes (recomputed per node, so dynamic pruning
// against evolving query state is fine); leaf_cb(ref, loc) handles a surviving leaf and returns
// true to stop the whole walk. Returns true when a callback stopped it. A degenerate one-leaf
// root stores no box and is handed to leaf_cb unconditionally (over-inclusion only): walkers
// resolve it with their own leaf-level tests.
template <class NodeMask, class LeafCb>
inline bool maskWalk(const uint8_t *data, uint32_t start_ref, const NodeMask &node_mask, const LeafCb &leaf_cb)
{
  if (start_ref & LEAF_ENTRY_FLAG)
    return leaf_cb((LeafRef)start_ref, decodeLeafRef(data, start_ref));
  uint32_t stack[MAX_TREE_DEPTH * 4];
  int sp = 0;
  uint32_t cur = start_ref;
  for (;;)
  {
    if (cur & TAG_MASK) // internal node
    {
      const NodeSoA nd(data, cur);
      unsigned m = node_mask(nd) & ((1u << nd.N) - 1);
      if (m)
      {
        const uint32_t *w = nd.w();
        const unsigned leafAll = nd.leafMask();
        unsigned leafHit = m & leafAll;
        const int nodeOfs = (int)(cur & PTR_OFS_MASK);
        const unsigned shortMask = (cur >> PTR_SHORT_SHIFT) & 15u;
        while (leafHit)
        {
          const int i = (int)__bsf_unsafe(leafHit);
          leafHit &= leafHit - 1;
          const LeafLoc l{leafBodyOfs(nodeOfs, nd.N, leafAll, shortMask, i), w[i], ((shortMask >> i) & 1) != 0};
          if (leaf_cb(makeLeafRef(cur, i), l))
            return true;
        }
        unsigned mi = m & ~leafAll;
        if (mi)
        {
          G_ASSERT(sp + 3 <= (int)(sizeof(stack) / sizeof(stack[0]))); // corrupt trees must not overflow silently
          const unsigned first = __bsf_unsafe(mi);
          for (mi &= mi - 1; mi; mi &= mi - 1)
            stack[sp++] = w[__bsf_unsafe(mi)];
          cur = w[first];
          continue;
        }
      }
    }
    else // degenerate whole-BLAS-is-one-leaf root block
    {
      const int leaf = (int)(cur & PTR_OFS_MASK);
      const LeafLoc l{leaf + 4, *(const uint32_t *)(data + leaf), false};
      if (leaf_cb(makeRootLeafRef(cur & PTR_OFS_MASK), l))
        return true;
    }
    if (!sp)
      return false;
    cur = stack[--sp];
  }
}

// XZ point walk with the 4-wide point-in-box node test and Y pruning: a subtree topping out at or
// below the current best max cannot improve it, and one entirely above the limit has no acceptable
// surface (the landmesh gridHt analog).
template <bool CULL_DOWN>
inline void heightWalk(const uint8_t *data, uint32_t start_ref, vec3f p, HeightAcc &acc)
{
  const vec4f px = v_splat_x(p), pz = v_splat_z(p);
  maskWalk(
    data, start_ref,
    [&](const NodeSoA &nd) -> unsigned {
      const vec4f yOk = v_and(v_cmp_gt(nd.mxy, v_splats(acc.maxHt)), v_cmp_ge(v_splats(acc.limit), nd.mny));
      return (unsigned)v_truemask(
        v_and(v_and(v_and(v_cmp_ge(px, nd.mnx), v_cmp_ge(nd.mxx, px)), v_and(v_cmp_ge(pz, nd.mnz), v_cmp_ge(nd.mxz, pz))), yOk));
    },
    [&](LeafRef, const LeafLoc &l) {
      heightLeaf_SoA<CULL_DOWN>(data, leafFields(data, l), l.bodyOfs, p, acc);
      return false;
    });
}

// Bundled height queries: nearby points share one XZ-overlap traversal over the points' XZ union
// box. The union and the limit bound are derived here, not taken from the caller: a mismatched
// value could silently lose hits (under-covering box) or over-prune (small limit). Node pruning:
// a subtree can be dropped only when it cannot improve ANY point (top <= min best) or lies above
// ALL limits.
template <bool CULL_DOWN>
inline void heightWalkMulti(const uint8_t *data, uint32_t start_ref, const vec3f *p, HeightAcc *acc, int npts)
{
  if (npts <= 0)
    return;
  float max_limit = acc[0].limit;
  vec4f unx = v_splat_x(p[0]), uxx = unx, unz = v_splat_z(p[0]), uxz = unz;
  for (int k = 1; k < npts; ++k)
  {
    max_limit = max(max_limit, acc[k].limit);
    unx = v_min(unx, v_splat_x(p[k])), uxx = v_max(uxx, v_splat_x(p[k]));
    unz = v_min(unz, v_splat_z(p[k])), uxz = v_max(uxz, v_splat_z(p[k]));
  }
  float minBest = acc[0].maxHt; // monotone non-decreasing: refreshed after each leaf, O(1) per node
  for (int k = 1; k < npts; ++k)
    minBest = min(minBest, acc[k].maxHt);
  maskWalk(
    data, start_ref,
    [&](const NodeSoA &nd) -> unsigned {
      const vec4f yOk = v_and(v_cmp_gt(nd.mxy, v_splats(minBest)), v_cmp_ge(v_splats(max_limit), nd.mny));
      return (unsigned)v_truemask(
        v_and(v_and(v_and(v_cmp_ge(uxx, nd.mnx), v_cmp_ge(nd.mxx, unx)), v_and(v_cmp_ge(uxz, nd.mnz), v_cmp_ge(nd.mxz, unz))), yOk));
    },
    [&](LeafRef, const LeafLoc &l) {
      const QuadLeafFields f = leafFields(data, l);
      for (int k = 0; k < npts; ++k)
        heightLeaf_SoA<CULL_DOWN>(data, f, l.bodyOfs, p[k], acc[k]);
      minBest = acc[0].maxHt;
      for (int k = 1; k < npts; ++k)
        minBest = min(minBest, acc[k].maxHt);
      return false;
    });
}

// Bundled coherent traces: a small packet of rays (own origins and directions) traverses the tree
// ONCE -- node acceptance uses the packet's union box, leaf tests run per ray. Only pays while the
// packet box stays comparable to a leaf box (the caller gates); results always match independent
// traces exactly. Each RayData must be fully set up (origin/dir in box space, calc(), t, data,
// and bestTriOffset = 0 on entry, as for rayClosest/rayAnyHit). ub must be the union of every
// ray's segment AABB (origin + t * dir over t in [0, r.t], box space): an under-covering box
// silently prunes real hits.
template <bool CullCCW = false>
inline void rayPacketWalk(const uint8_t *data, uint32_t root_v, bbox3f ub, RayData *r, int nrays)
{
  const vec4f ubnx = v_splat_x(ub.bmin), ubny = v_splat_y(ub.bmin), ubnz = v_splat_z(ub.bmin);
  const vec4f ubxx = v_splat_x(ub.bmax), ubxy = v_splat_y(ub.bmax), ubxz = v_splat_z(ub.bmax);
  maskWalk(
    data, root_v,
    [&](const NodeSoA &nd) -> unsigned {
      return (unsigned)v_truemask(
        v_and(v_and(v_and(v_cmp_ge(ubxx, nd.mnx), v_cmp_ge(nd.mxx, ubnx)), v_and(v_cmp_ge(ubxy, nd.mny), v_cmp_ge(nd.mxy, ubny))),
          v_and(v_cmp_ge(ubxz, nd.mnz), v_cmp_ge(nd.mxz, ubnz))));
    },
    [&](LeafRef ref, const LeafLoc &l) {
      for (int k = 0; k < nrays; ++k)
        if (rayLeafAt<CullCCW>(r[k], l.bodyOfs, l.w0, l.isShort))
          r[k].bestTriOffset = (int)ref;
      return false;
    });
}

// Box-overlap walk with the 4-wide node test: leaf_cb(LeafLoc) for every leaf whose STORED box
// overlaps `qb` (box space); return true from the callback to stop the walk. The 4-wide sibling of
// iterateLeafRefs for pure box queries (triangle collection, region scans).
template <class LeafCb>
inline bool boxWalk(const uint8_t *data, uint32_t start_ref, bbox3f qb, const LeafCb &leaf_cb)
{
  const vec4f qnx = v_splat_x(qb.bmin), qny = v_splat_y(qb.bmin), qnz = v_splat_z(qb.bmin);
  const vec4f qxx = v_splat_x(qb.bmax), qxy = v_splat_y(qb.bmax), qxz = v_splat_z(qb.bmax);
  return maskWalk(
    data, start_ref,
    [&](const NodeSoA &nd) -> unsigned {
      return (unsigned)v_truemask(
        v_and(v_and(v_and(v_cmp_ge(qxx, nd.mnx), v_cmp_ge(nd.mxx, qnx)), v_and(v_cmp_ge(qxy, nd.mny), v_cmp_ge(nd.mxy, qny))),
          v_and(v_cmp_ge(qxz, nd.mnz), v_cmp_ge(nd.mxz, qnz))));
    },
    [&](LeafRef, const LeafLoc &l) { return leaf_cb(l); });
}

// Per-triangle box query on top of boxWalk: tri_cb(v0, v1, v2) (box space, source winding) for
// every sub-triangle whose own bbox overlaps `qb`, of every leaf whose box does; return true from
// the callback to stop. Returns true when stopped -- the triangle-collection shape (TraceMeshFaces
// style caches, region scans over collision/land BLASes).
template <class TriCb>
inline bool boxTriWalk(const uint8_t *data, uint32_t start_ref, bbox3f qb, const TriCb &tri_cb)
{
  return boxWalk(data, start_ref, qb, [&](const LeafLoc &l) {
    const QuadLeafFields f = leafFields(data, l);
    const int nTri = (int)quadLeafTriCount(f);
    for (int st = 0, emitted = 0; emitted < nTri; ++st)
    {
      if (st == 1 && f.isSingle)
        continue; // quad A single: sub-tri 1 absent
      vec3f a, b, c;
      fetchLeafTri(data, l, st, Vert21Loader{}, a, b, c);
      emitted++;
      bbox3f tb;
      v_bbox3_init(tb, a);
      v_bbox3_add_pt(tb, b);
      v_bbox3_add_pt(tb, c);
      if (!v_bbox3_test_box_intersect(tb, qb))
        continue;
      if (tri_cb(a, b, c))
        return true;
    }
    return false;
  });
}

// Deepest node/leaf whose subtree covers every leaf overlapping the given box-space XZ range (for
// per-texel narrowing grids). Returns 0 when nothing overlaps (0 is never a valid ref). A
// degenerate one-leaf root is returned as is: the whole tree is the deepest cover.
inline uint32_t deepestCoveringXZRef(const uint8_t *data, uint32_t root_v, float bx_min, float bx_max, float bz_min, float bz_max)
{
  if (root_v & LEAF_ENTRY_FLAG)
    return root_v;
  if ((root_v & TAG_MASK) == 0) // degenerate whole-BLAS-is-one-leaf root block (see maskWalk)
    return makeRootLeafRef(root_v & PTR_OFS_MASK);
  const vec4f bxm = v_splats(bx_min), bxM = v_splats(bx_max), bzm = v_splats(bz_min), bzM = v_splats(bz_max);
  uint32_t cur = root_v;
  for (;;)
  {
    const NodeSoA nd(data, cur);
    const unsigned m = (unsigned)v_truemask(v_and(v_and(v_cmp_ge(bxM, nd.mnx), v_cmp_ge(nd.mxx, bxm)), //
                         v_and(v_cmp_ge(bzM, nd.mnz), v_cmp_ge(nd.mxz, bzm)))) &
                       ((1u << nd.N) - 1);
    if (!m)
      return 0;
    if (m & (m - 1)) // several children overlap: this node is the deepest cover
      return cur;
    const int lane = (int)__bsf_unsafe(m);
    if (nd.w()[lane] & QUAD_LEAF_FLAG)
      return makeLeafRef(cur, lane);
    cur = nd.w()[lane];
  }
}

} // namespace soa4
