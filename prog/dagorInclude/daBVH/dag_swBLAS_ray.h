//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Stackless double-quad BLAS traversal: a linear memory-order stream of 16 B box nodes and 28 B
// double-quad leaves where a skip word jumps a rejected subtree. This is also the GPU upload
// format (swBLAS.dshl), so these walkers stay for GPU-format validation, tools and unified-memory
// configs. Shared leaf decode and ray/triangle primitives live in dag_swBLAS_leaf.h.

#include <daBVH/dag_swBLAS_leaf.h>

// ============================================================================
// Out-of-line BLAS traversal declarations (defined in bvhTraversalQuadOOL.cpp)
// ============================================================================

namespace bvh_traverse
{
bool rayBLASQuadOOL(RayData &r, int startOffset, int blasSize);
bool rayBLASQuadOOLCullCCW(RayData &r, int startOffset, int blasSize);
} // namespace bvh_traverse

// Forward decl needed before BLASTraverse so rayBLAS_Free can call back into BLASTraverse helpers.
template <bool CullCCW, int VertStride>
struct BLASTraverse;

// Canonical fast-path BLAS rayCast: free template function, calls into BLASTraverse static helpers
// for the leaf decode + 4-wide SoA ray-tri.
// Use this in preference to BLASTraverse<CullCCW>::rayBLAS(). Measured ~10% faster than the class-
// member equivalent in dagRayBench -- clang generates tighter code for templated free functions
// than for templated class members, even with __forceinline on both and identical bodies.
// VertStride defaults to 8 (vert21 packed); pass 12 for raw float3 BLAS data.
template <bool CullCCW, int VertStride = 8, class HitCb = BestHitCb>
__forceinline bool rayBLAS_Free(RayData &r, int startOffset, int blasSize, const HitCb &cb = HitCb())
{
  using B = BLASTraverse<CullCCW, VertStride>;
  int dataOffset = startOffset;
  const int endOffset = startOffset + blasSize;
  vec3f rayOriginScaled = v_neg(v_mul(r.rayOrigin, r.rayDirInv));
  for (; dataOffset < endOffset;)
  {
    vec3f bboxMin, bboxMax;
    uint offsetToNextNode;
    B::decodeRaw(r.data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
    bool collision =
      RayIntersectsBoxT0T1(v_madd(bboxMin, r.rayDirInv, rayOriginScaled), v_madd(bboxMax, r.rayDirInv, rayOriginScaled), r.t);
    dataOffset += BVH_BLAS_NODE_SIZE;
    const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
    if (!collision)
      dataOffset += isLeaf ? BVH_BLAS_LEAF_SIZE - BVH_BLAS_NODE_SIZE : offsetToNextNode;
    else if (isLeaf)
    {
      if (B::rayLeaf_SoA(r, dataOffset, offsetToNextNode))
        if (cb(r, dataOffset))
          break;
      dataOffset += BVH_BLAS_LEAF_SIZE - BVH_BLAS_NODE_SIZE;
    }
  }
  return (r.bestTriOffset > 0);
}

// ---- Nearest-first ("ordered") closest-hit traversal ----

// Nearest-first ("ordered") closest-hit BLAS rayCast over the SAME stackless buffer rayBLAS_Free
// walks: children are gathered in chunks of 4, sorted near->far by box entry distance (branchless
// network over packed uint64 keys) and visited nearest-first, so an early hit shrinks r.t sooner and
// prunes far subtrees; a stacked entry already at or beyond the shrunken r.t is dropped at pop with
// one 64-bit compare. Culling is always against the current r.t, so it finds the same closest hit as
// rayBLAS_Free -- order affects only speed (measured +6-13% closest-hit on collision meshes in
// dagRayBench). Any-hit gains nothing from ordering: keep rayBLAS_Free there.
// The HitCb contract matches rayBLAS_Free's, including callbacks that restore r.t on a rejected leaf
// (the prune key is refreshed after every reported leaf).
// Spans with more than 4 children (SAH-flattened unsplittable clusters) are handled by the chunking;
// if a pathological tree still fills the stack, the remainder of that span is traced in memory order
// via rayBLAS_Free, so correctness never depends on the stack bound.
// Cold overflow path, deliberately out of line so the __forceinline rayBLAS_Free body does not bloat
// the hot ordered loop it is virtually never needed in.
template <bool CullCCW, int VertStride, class HitCb>
DAGOR_NOINLINE inline void rayBLAS_orderedOverflow(RayData &r, int start_offset, int blas_size, const HitCb &cb)
{
  rayBLAS_Free<CullCCW, VertStride>(r, start_offset, blas_size, cb);
}

template <bool CullCCW, int VertStride = 8, class HitCb = BestHitCb>
__forceinline bool rayBLAS_OrderedFree(RayData &r, int startOffset, int blasSize, const HitCb &cb = HitCb())
{
  using B = BLASTraverse<CullCCW, VertStride>;
  const uint8_t *data = r.data;
  const vec3f dirInv = r.rayDirInv;
  const vec3f originScaled = v_neg(v_mul(r.rayOrigin, dirInv));
  uint64_t stack[512]; // ~3*BVH_MAX_BLAS_DEPTH+4 worst case for 4-ary trees; slack absorbs flattened spans
  constexpr int STACK_CAP = (int)(sizeof(stack) / sizeof(stack[0]));
  int sp = 0;

  // Gather the box-children in [childStart, childEnd) in chunks of <= 4, keep those the ray enters
  // within the current r.t, sort the chunk near->far and push far-first so the nearest pops first.
  auto gatherChildren = [&](int childStart, int childEnd) {
    for (int c = childStart; c < childEnd;)
    {
      if (DAGOR_UNLIKELY(sp > STACK_CAP - 4)) // nearly full (degenerate flattened cluster): finish unordered
      {
        rayBLAS_orderedOverflow<CullCCW, VertStride>(r, c, childEnd - c, cb);
        return;
      }
      uint64_t k4[4];
      int m = 0, nh = 0;
      while (c < childEnd && m < 4)
      {
        vec3f bmn, bmx;
        uint cskip;
        B::decodeRaw(data, c, bmn, bmx, cskip);
        float nearT;
        const bool hit = RayIntersectsBoxNearT(v_madd(bmn, dirInv, originScaled), v_madd(bmx, dirInv, originScaled), r.t, nearT);
        k4[m++] = hit ? swblas_packChildKey(nearT, c) : ~0ull; // miss -> sentinel, sinks in the sort
        nh += hit ? 1 : 0;
        c += (cskip & BLAS_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE + (int)cskip;
      }
      for (int i = m; i < 4; ++i)
        k4[i] = ~0ull;
      swblas_sort4net(k4[0], k4[1], k4[2], k4[3]);
      const int nhits = nh < 4 ? nh : 4;
      for (int i = nhits - 1; i >= 0 && sp < STACK_CAP; --i)
        stack[sp++] = k4[i];
    }
  };

  // The root's own box is suppressed in the buffer (the caller carries it), so the root's children
  // occupy the entire [startOffset, startOffset + blasSize) range.
  const int endOffset = startOffset + blasSize;
  gatherChildren(startOffset, endOffset);
  while (sp > 0)
  {
    const uint64_t e = stack[--sp];
    if (swblas_childKeyDist(e) >= r.t)
      continue; // deferred cull: a closer hit landed after this entry was pushed
    const int o = swblas_childKeyOfs(e);
    const uint32_t skip = loadBuffer(data, o + 12);
    if (skip & BLAS_LEAF_FLAG)
    {
      if (B::rayLeaf_SoA(r, o + BVH_BLAS_NODE_SIZE, skip))
        if (cb(r, o + BVH_BLAS_NODE_SIZE))
          return true;
      continue;
    }
    // Clamped so a corrupt skip degrades like rayBLAS_Free's bounded walk (subtree dropped), not reads
    // past the tree region.
    const int childEnd = o + BVH_BLAS_NODE_SIZE + (int)skip;
    gatherChildren(o + BVH_BLAS_NODE_SIZE, childEnd < endOffset ? childEnd : endOffset);
  }
  return r.bestTriOffset > 0;
}

// ============================================================================
// Quad-encoded BLAS traversal (UINT16 boxes, quad leaf encoding)
// Uses RayData with 21-bit packed vertices (8 bytes/vert in BVH box space)
// ============================================================================

template <bool CullCCW = false, int VertStride = 8>
struct BLASTraverse
{
  using RayDataType = RayData;
  static constexpr int LEAF_SIZE = BVH_BLAS_LEAF_SIZE;

  static __forceinline vec3f loadVertRaw(const uint8_t *d, int baseOfs, int vertIdx)
  {
    return RayData::unpackVert21Raw(d + baseOfs + vertIdx * VertStride);
  }
  static __forceinline vec3f loadVert(const uint8_t *d, int baseOfs, int vertIdx)
  {
    return RayData::unpackVert21(d + baseOfs + vertIdx * VertStride);
  }

  static inline void decodeRaw(const uint8_t *data, uint offset, vec3f &elem1, vec3f &elem2, uint &skip)
  {
    vec4i e12 = v_ldui((const int *)(data + offset));
    elem1 = v_cvt_vec4f(v_andi(e12, v_splatsi(0xFFFF)));
    elem2 = v_cvt_vec4f(v_srli(e12, 16));
    skip = v_extract_wi(e12);
  }

  // Vertex loader adapters for iterateFiltered/decode (aliases of the shared stride loader)
  using RDVertexLoaderRaw = Vert21StrideLoaderRaw<VertStride>; // unscaled raw coords; caller scales ray by 32 instead
  using RDVertexLoader = Vert21StrideLoader<VertStride>;       // box-space coords (divided by 32)

  // Decodes a double-quad leaf: up to two strip quads (= up to 4 triangles). triA = (v0,v1,v2),
  // triB = flipSecond ? (v2,v1,v3) : (v1,v2,v3). Offsets are signed; base = apex (see swBLASLeafDefs).
  struct QuadLeafVerts
  {
    vec3f v0, v1, v2, v3;     // quad A
    vec3f vb0, vb1, vb2, vb3; // quad B (valid when hasB)
    bool isSingle;            // quad A: 2nd tri degenerate (o3 == o2)
    bool flipSecond;          // quad A: triB winding (always decoded; consumers may compute normals)
    bool hasB;                // leaf carries a second quad
    bool isSingleB;           // quad B 2nd tri degenerate
    bool flipSecondB;         // quad B: triB winding (always decoded)

    template <class VL>
    static __forceinline void decodeQuad(const uint8_t *data, int baseByteOfs, int o1, int o2, int o3, const VL &vl, vec3f &a,
      vec3f &b, vec3f &c, vec3f &d)
    {
      a = vl(data, baseByteOfs, 0);
      b = vl(data, baseByteOfs, o1);
      c = vl(data, baseByteOfs, o2);
      d = vl(data, baseByteOfs, o3);
    }

    template <class VL>
    __forceinline void decode(const uint8_t *data, int dataOffset, uint skip, const VL &vl)
    {
      const QuadLeafFields f = decodeQuadLeafFields(skip, ((const uint *)(data + dataOffset))[0],
        ((const uint *)(data + dataOffset))[1], ((const uint *)(data + dataOffset))[2]);
      const int baseA = dataOffset + (int)f.relBaseBytes;
      isSingle = f.isSingle;
      // Always decode the 2nd-tri winding here: decode() hands raw verts to external consumers
      // (iterateFiltered's leafCb computes normals and runs its own cull; getNodeFaceVertsByRef
      // refetches face verts), which need correct winding regardless of this traversal's CullCCW.
      // Only the internal SoA hot path (rayLeaf_SoA), whose math is winding-independent, keeps the
      // cull-gated skip.
      flipSecond = f.flipSecond;
      decodeQuad(data, baseA, f.o1, f.o2, f.o3, vl, v0, v1, v2, v3);

      hasB = f.hasB;
      if (hasB)
      {
        isSingleB = f.isSingleB;
        flipSecondB = f.flipSecondB;
        decodeQuad(data, baseA + (int)f.deltaB * VertStride, f.o1b, f.o2b, f.o3b, vl, vb0, vb1, vb2, vb3);
      }
      else
      {
        isSingleB = true;
        flipSecondB = false;
        vb0 = vb1 = vb2 = vb3 = v0; // degenerate placeholder
      }
    }

    __forceinline void decode(const RayData &rd, int dataOffset, uint skip) { decode(rd.data, dataOffset, skip, RDVertexLoader{}); }

    // Corners of sub-triangle 0..3 (0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2).
    __forceinline void getTri(int subTri, vec3f &a, vec3f &b, vec3f &c) const
    {
      switch (subTri)
      {
        case 0:
          a = v0;
          b = v1;
          c = v2;
          break;
        case 1: swblas_secondTriCorners(flipSecond, v1, v2, v3, a, b, c); break;
        case 2:
          a = vb0;
          b = vb1;
          c = vb2;
          break;
        default: swblas_secondTriCorners(flipSecondB, vb1, vb2, vb3, a, b, c); break;
      }
    }

    // Decode just the 3 verts of one sub-triangle 0..3 into v0,v1,v2. Loads only what is needed.
    template <class VL>
    __forceinline void decodeTri(const uint8_t *data, int dataOffset, uint skip, int subTri, const VL &vl)
    {
      const QuadLeafFields f = decodeQuadLeafFields(skip, ((const uint *)(data + dataOffset))[0],
        ((const uint *)(data + dataOffset))[1], ((const uint *)(data + dataOffset))[2]);
      const int baseA = dataOffset + (int)f.relBaseBytes;
      int base, o1, o2, o3;
      bool flip; // set per-quad below; refetch/normal callers need correct winding (see decode())
      if (subTri < 2)
      {
        base = baseA;
        o1 = f.o1;
        o2 = f.o2;
        o3 = f.o3;
        flip = f.flipSecond;
      }
      else
      {
        base = baseA + (int)f.deltaB * VertStride;
        o1 = f.o1b;
        o2 = f.o2b;
        o3 = f.o3b;
        flip = f.flipSecondB;
      }
      bool single = (o3 == o2);
      bool firstTri = single || (subTri & 1) == 0;
      if (firstTri)
      {
        v0 = vl(data, base, 0);
        v1 = vl(data, base, o1);
        v2 = vl(data, base, o2);
      }
      else
      {
        const vec3f t1 = vl(data, base, o1), t2 = vl(data, base, o2), t3 = vl(data, base, o3);
        swblas_secondTriCorners(flip, t1, t2, t3, v0, v1, v2);
      }
    }
  };

  // ---- SoA 4-wide leaf decode + ray-tri (decodes both quads; lanes 0,1 = quad A tris, 2,3 = quad B) ----

  // Forwarders to the shared implementations in dag_swBLAS_leaf.h, kept for existing callers.
  static __forceinline void unpack4SoA(const uint8_t *data, int baseByteOfs, int o1, int o2, int o3, vec4f &xs, vec4f &ys, vec4f &zs)
  {
    swblas_unpack4SoA<VertStride>(data, baseByteOfs, o1, o2, o3, xs, ys, zs);
  }
  static __forceinline void quadCorners2(bool flip, vec4f xs, vec4f ys, vec4f zs, vec4f &c0x, vec4f &c0y, vec4f &c0z, vec4f &c1x,
    vec4f &c1y, vec4f &c1z, vec4f &c2x, vec4f &c2y, vec4f &c2z)
  {
    swblas_quadCorners2(flip, xs, ys, zs, c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z);
  }

  // SoA leaf processor: decode both quads, run one 4-wide ray-tri across all 4 sub-triangles.
  // Shared with the SoA4 traversal (full leaf bodies are byte-identical in both layouts).
  static __forceinline bool rayLeaf_SoA(RayData &r, int dataOffset, uint skip)
  {
    return swblas_rayLeaf_SoA<CullCCW, VertStride>(r, dataOffset, skip);
  }

  // ---- Leaf intersection functions ----

  static inline bool rayLeaf(RayData &r, int dataOffset, uint skip)
  {
    QuadLeafVerts q;
    q.decode(r, dataOffset, skip);
    bool anyHit = false;
    if (RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, q.v0, q.v1, q.v2, r.t, r.bCoord))
    {
      anyHit = true;
      r.bestSubTri = 0;
    }
    if (!q.isSingle)
    {
      vec3f a, b, c;
      q.getTri(1, a, b, c);
      if (RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, a, b, c, r.t, r.bCoord))
      {
        anyHit = true;
        r.bestSubTri = 1;
      }
    }
    if (q.hasB)
    {
      if (RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, q.vb0, q.vb1, q.vb2, r.t, r.bCoord))
      {
        anyHit = true;
        r.bestSubTri = 2;
      }
      if (!q.isSingleB)
      {
        vec3f a, b, c;
        q.getTri(3, a, b, c);
        if (RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, a, b, c, r.t, r.bCoord))
        {
          anyHit = true;
          r.bestSubTri = 3;
        }
      }
    }
    return anyHit;
  }

  static inline void distLeaf(DistData &d, int dataOffset, uint skip)
  {
    QuadLeafVerts q;
    q.decode(d.data, dataOffset, skip, RDVertexLoader{});
    // unscale verts to the query's world-metric space (see DistData)
    q.v0 = v_mul(q.v0, d.invScale);
    q.v1 = v_mul(q.v1, d.invScale);
    q.v2 = v_mul(q.v2, d.invScale);
    q.v3 = v_mul(q.v3, d.invScale);
    q.vb0 = v_mul(q.vb0, d.invScale);
    q.vb1 = v_mul(q.vb1, d.invScale);
    q.vb2 = v_mul(q.vb2, d.invScale);
    q.vb3 = v_mul(q.vb3, d.invScale);
    auto testTri = [&](vec3f a, vec3f b, vec3f c) {
      vec3f mn = v_min(a, v_min(b, c)), mx = v_max(a, v_max(b, c));
      if (v_extract_x(v_distance_sq_to_bbox_x(mn, mx, d.pos)) < d.bestDist2)
        distBLASLeafTri(d, a, b, c, dataOffset);
    };
    testTri(q.v0, q.v1, q.v2);
    if (!q.isSingle)
    {
      vec3f a, b, c;
      q.getTri(1, a, b, c);
      testTri(a, b, c);
    }
    if (q.hasB)
    {
      testTri(q.vb0, q.vb1, q.vb2);
      if (!q.isSingleB)
      {
        vec3f a, b, c;
        q.getTri(3, a, b, c);
        testTri(a, b, c);
      }
    }
  }

  // ---- Full BLAS traversal functions ----

  // PERF NOTE: callers that want maximum throughput should prefer rayBLAS_Free<CullCCW>(...) instead
  // of QuadBLAS::rayBLAS(...) -- clang generates ~10-15% worse code for templated class-member functions
  // than equivalent templated free functions (measured in dagRayBench). This member is kept for
  // backwards compatibility with existing callers.
  template <class HitCb = BestHitCb>
  static __forceinline bool rayBLAS(RayData &r, int startOffset, int blasSize, const HitCb &cb = HitCb())
  {
    return rayBLAS_Free<CullCCW, VertStride, HitCb>(r, startOffset, blasSize, cb);
  }

  // Out-of-line BLAS traversal (separate compilation unit to reduce i-cache pressure in TLAS lambda)
  static bool rayBLASOOL(RayData &r, int startOffset, int blasSize)
  {
    static_assert(VertStride == 8, "OOL traversal only supports default vertex stride (8)");
    if constexpr (CullCCW)
      return bvh_traverse::rayBLASQuadOOLCullCCW(r, startOffset, blasSize);
    else
      return bvh_traverse::rayBLASQuadOOL(r, startOffset, blasSize);
  }

  template <class HitCb = BestHitCb>
  static inline void rayBLASInf(RayData &r, int startOffset, int blasSize, const HitCb &cb = HitCb())
  {
    vec3f rayOriginScaled = v_neg(v_mul(r.rayOrigin, r.rayDirInv));
    vec3f rayDirInv = r.rayDirInv;
    iterateFilteredVerts(
      r.data, startOffset, blasSize,
      [rayDirInv, rayOriginScaled](vec3f bmin, vec3f bmax) {
        return RayIntersectsBoxInf(v_madd(bmin, rayDirInv, rayOriginScaled), v_madd(bmax, rayDirInv, rayOriginScaled));
      },
      [&](vec3f v0, vec3f v1, vec3f v2, int dataOffset) {
        if (RayTriangleIntersectInf<CullCCW>(r.rayOrigin, r.rayDir, v0, v1, v2, r.t, r.bCoord))
          cb(r, dataOffset);
        return false;
      },
      RDVertexLoader{});
  }

  template <class HitCb = BestHitCb>
  static inline void rayBLASXZInf(RayData &r, int startOffset, int blasSize, const HitCb &cb = HitCb())
  {
    vec3f rayOrigin = r.rayOrigin;
    iterateFilteredVerts(
      r.data, startOffset, blasSize,
      [rayOrigin](vec3f bmin, vec3f bmax) { return v_check_xz_all_true(v_and(v_cmp_ge(rayOrigin, bmin), v_cmp_ge(bmax, rayOrigin))); },
      [&](vec3f v0, vec3f v1, vec3f v2, int dataOffset) {
        if (RayTriangleIntersectInfXZ(r.rayOrigin, v0, v1, v2, r.t, r.bCoord))
          cb(r, dataOffset);
        return false;
      },
      RDVertexLoader{});
  }

  static inline bool distBLAS(DistData &d, int startOffset, int blasSize)
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(d.data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      // invScale is positive, so the unscaled box stays min/max ordered and axis-aligned
      bool collision =
        v_extract_x(v_distance_sq_to_bbox_x(v_mul(bboxMin, d.invScale), v_mul(bboxMax, d.invScale), d.pos)) < d.bestDist2;
      dataOffset += BVH_BLAS_NODE_SIZE;
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!collision)
      {
        dataOffset += isLeaf ? LEAF_SIZE - BVH_BLAS_NODE_SIZE : offsetToNextNode;
      }
      else if (isLeaf)
      {
        distLeaf(d, dataOffset, offsetToNextNode);
        dataOffset += LEAF_SIZE - BVH_BLAS_NODE_SIZE;
      }
    }
    return d.bestTriOffset >= 0;
  }

  // Leaf-granular, vertex-free sibling of iterateFiltered: tests each node and for an overlapping leaf
  // calls leafCb(bmin, bmax, leafOffset) ONCE -- no QuadLeafVerts decode, no per-sub-tri expansion. For
  // consumers that only need the leaf body offset (e.g. getNodeFaceVertsByRef, which then decodes once
  // via decodeQuadLeafFieldsAt + expandQuadLeafTris). leafCb returns true to stop all.
  template <class NodeTest, class LeafCb>
  static inline bool iterateLeafOffsets(const uint8_t *data, int startOffset, int blasSize, const NodeTest &nodeTest,
    const LeafCb &leafCb)
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      bool collision = nodeTest(bboxMin, bboxMax);
      dataOffset += BVH_BLAS_NODE_SIZE;
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!collision)
        dataOffset += isLeaf ? LEAF_SIZE - BVH_BLAS_NODE_SIZE : offsetToNextNode;
      else if (isLeaf)
      {
        if (leafCb(bboxMin, bboxMax, dataOffset))
          return true;
        dataOffset += LEAF_SIZE - BVH_BLAS_NODE_SIZE;
      }
    }
    return false;
  }

  // Generic filtered BVH traversal: test each node, call leafCb with triangle vertices.
  // NodeTest: (vec3f bmin, vec3f bmax) -> bool (true = overlap, traverse children)
  // LeafCb:   (vec3f v0, vec3f v1, vec3f v2, int leafOffset, int subTri) -> bool (true = stop all)
  //           subTri 0..3 identifies the sub-triangle (0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2).
  // VL:       vertex loader
  template <class NodeTest, class LeafCb, class VL>
  static inline bool iterateFiltered(const uint8_t *data, int startOffset, int blasSize, const NodeTest &nodeTest,
    const LeafCb &leafCb, const VL &vl)
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      bool collision = nodeTest(bboxMin, bboxMax);
      dataOffset += BVH_BLAS_NODE_SIZE;
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!collision)
      {
        dataOffset += isLeaf ? LEAF_SIZE - BVH_BLAS_NODE_SIZE : offsetToNextNode;
      }
      else if (isLeaf)
      {
        QuadLeafVerts q;
        q.decode(data, dataOffset, offsetToNextNode, vl);
        if (leafCb(q.v0, q.v1, q.v2, dataOffset, 0))
          return true;
        if (!q.isSingle)
        {
          vec3f a, b, c;
          q.getTri(1, a, b, c);
          if (leafCb(a, b, c, dataOffset, 1))
            return true;
        }
        if (q.hasB)
        {
          if (leafCb(q.vb0, q.vb1, q.vb2, dataOffset, 2))
            return true;
          if (!q.isSingleB)
          {
            vec3f a, b, c;
            q.getTri(3, a, b, c);
            if (leafCb(a, b, c, dataOffset, 3))
              return true;
          }
        }
        dataOffset += LEAF_SIZE - BVH_BLAS_NODE_SIZE;
      }
    }
    return false;
  }

  // Geometry-only sibling of iterateFiltered: the leaf callback gets only the triangle vertices and leaf
  // offset, never the sub-tri lane index, so geometry walkers (ray casts, candidate-triangle collection)
  // do not depend on double-quad lane numbering. Use the 5-arg iterateFiltered when a hit must be
  // attributed to its sub-triangle (e.g. tri_ref reporting).
  // LeafCb: (vec3f v0, vec3f v1, vec3f v2, int leafOffset) -> bool (true = stop all)
  template <class NodeTest, class LeafCb, class VL>
  static inline bool iterateFilteredVerts(const uint8_t *data, int startOffset, int blasSize, const NodeTest &nodeTest,
    const LeafCb &leafCb, const VL &vl)
  {
    return iterateFiltered(
      data, startOffset, blasSize, nodeTest,
      [&leafCb](vec3f v0, vec3f v1, vec3f v2, int dataOffset, int) { return leafCb(v0, v1, v2, dataOffset); }, vl);
  }

  static inline int firstBoxBLASLeaf(const uint8_t *data, vec3f check_bmin, vec3f check_bmax, int startOffset, int blasSize)
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      bool collision = v_check_xyz_all_true(v_and(v_cmp_ge(check_bmax, bboxMin), v_cmp_ge(bboxMax, check_bmin)));
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!collision)
      {
        dataOffset += isLeaf ? LEAF_SIZE : offsetToNextNode + BVH_BLAS_NODE_SIZE;
      }
      else if (isLeaf)
        return dataOffset;
      else
        dataOffset += BVH_BLAS_NODE_SIZE;
    }
    return -1;
  }

  static inline int firstBLASOffsetToCheckXZ(const uint8_t *data, vec3f check_bmin, vec3f check_bmax, int startOffset, int blasSize)
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!v_check_xz_all_true(v_and(v_cmp_ge(check_bmax, bboxMin), v_cmp_ge(bboxMax, check_bmin))))
      {
        dataOffset += isLeaf ? LEAF_SIZE : offsetToNextNode + BVH_BLAS_NODE_SIZE;
      }
      else
      {
        if (!isLeaf && v_check_xz_all_true(v_and(v_cmp_ge(check_bmin, bboxMin), v_cmp_ge(bboxMax, check_bmax))))
          dataOffset += BVH_BLAS_NODE_SIZE;
        else
          return dataOffset;
      }
    }
    return -1;
  }

  static inline int firstBLASOffsetToCheck(const uint8_t *data, vec3f check_bmin, vec3f check_bmax, int startOffset, int blasSize)
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!v_check_xyz_all_true(v_and(v_cmp_ge(check_bmax, bboxMin), v_cmp_ge(bboxMax, check_bmin))))
      {
        dataOffset += isLeaf ? LEAF_SIZE : offsetToNextNode + BVH_BLAS_NODE_SIZE;
      }
      else
      {
        if (!isLeaf && v_check_xyz_all_true(v_and(v_cmp_ge(check_bmin, bboxMin), v_cmp_ge(bboxMax, check_bmax))))
          dataOffset += BVH_BLAS_NODE_SIZE;
        else
          return dataOffset;
      }
    }
    return -1;
  }
};
