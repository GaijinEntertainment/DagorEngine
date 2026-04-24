//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <math/dag_Point2.h>

#define BVH_TRACE_EPSILON 0.000004f

static constexpr int BVH_BLAS_NODE_SIZE = 16;
static constexpr uint32_t BLAS_LEAF_FLAG = QUAD_LEAF_FLAG;

struct RayData // -V730
{
  vec3f rayOrigin, rayDir;
  const uint8_t *data = nullptr;
  float t = 0;
  Point2 bCoord;
  int bestTriOffset = 0;
  int8_t bestSubTri = 0; // 0 = first tri / single, 1 = second tri of quad
  vec3f rayDirInv;
  void calc() { rayDirInv = v_rcp_safe(rayDir, V_C_MAX_VAL); }
  // 21-bit packed vertex: uint64 with x[20:0] | y[41:21] | z[62:42], stored as round(value * 32)
  // Uses 64-bit SIMD shifts to avoid scalar-to-SIMD transfers.
  // unpackVert21Raw returns integer-scaled values in [0, 2097120]; caller scales ray by 32 instead.
  // unpackVert21 returns values in [0, 65535] box space (divides by 32).
  static __forceinline vec3f unpackVert21Raw(const uint8_t *src)
  {
    vec4i raw = v_ldui_half(src);
    vec4i mask21 = v_splatsi(0x1FFFFF);
    vec4i x = v_andi(raw, mask21);
    vec4i y = v_andi(v_srli_64(raw, 21), mask21);
    vec4i z = v_srli_64(raw, 42);
    return v_cvt_vec4f(v_interleave_lo_i64(v_interleave_lo_i32(x, y), z));
  }
  static __forceinline vec3f unpackVert21(const uint8_t *src) { return v_mul(unpackVert21Raw(src), v_splats(1.f / 32.f)); }
};

struct DistData
{
  vec3f pos;
  const uint8_t *data;
  float bestDist2;
  int bestTriOffset;
};

struct BestHitCb
{
  bool operator()(RayData &r, int dataOffset) const
  {
    r.bestTriOffset = dataOffset;
    return false;
  }
};

// ============================================================================
// Geometric primitives (encoding-independent)
// ============================================================================

typedef uint32_t uint;
inline uint loadBuffer(const uint8_t *data, uint offset) { return *(uint *)(data + offset); }

inline bool RayIntersectsBoxT0T1(vec3f t0, vec3f t1, float ray_extent)
{
  const vec3f tmax = v_perm_xyzd(v_max(t0, t1), v_splats(ray_extent));
  const vec3f tmin = v_perm_xyzd(v_min(t0, t1), v_zero());
  vec3f tmaxMin = v_min(tmax, v_perm_zwxy(tmax));
  tmaxMin = v_min(tmaxMin, v_perm_yzwx(tmaxMin));
  vec3f tminMax = v_max(tmin, v_perm_zwxy(tmin));
  tminMax = v_max(tminMax, v_perm_yzwx(tminMax));
  return v_test_vec_x_le(tminMax, tmaxMin);
}
inline bool RayIntersectsBoxInf(vec3f t0, vec3f t1)
{
  const vec3f tmax = v_max(t0, t1);
  const vec3f tmin = v_min(t0, t1);
  vec3f tmaxMin = v_min(v_min(v_perm_zwxy(tmax), v_perm_yzwx(tmax)), tmax);
  vec3f tminMax = v_max(v_max(v_perm_zwxy(tmin), v_perm_yzwx(tmin)), tmin);
  return v_test_vec_x_le(tminMax, tmaxMin);
}

#define kEpsilon                    0.00001
#define BVH_TRACE_ONE_PLUS_EPSILON  (1.f + BVH_TRACE_EPSILON)
#define BVH_TRACE_ONE_PLUS_EPSILON2 (1.f + 2.f * BVH_TRACE_EPSILON)

template <bool CullCCW = false>
inline bool RayTriangleIntersect(vec3f orig, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float &t, Point2 &bc)
{
  vec3f e0 = v_sub(v1, v0), e1 = v_sub(v2, v0);
  vec3f s1 = v_cross3(dir, e1);
  vec3f det = v_dot3_x(s1, e0);
  if constexpr (CullCCW)
  {
    if (v_extract_x(det) < kEpsilon) // backface or degenerate
      return false;
  }
  vec3f invd = v_rcp_safe(det, V_C_MAX_VAL);
  vec3f d = v_sub(orig, v0);
  Point2 b;
  b.x = v_extract_x(v_mul_x(v_dot3_x(d, s1), invd));
  if (b.x < -BVH_TRACE_EPSILON || b.x > BVH_TRACE_ONE_PLUS_EPSILON)
    return false;
  const vec3f s2 = v_cross3(d, e0);
  b.y = v_extract_x(v_mul_x(v_dot3_x(dir, s2), invd));
  if (b.y < -BVH_TRACE_EPSILON || (b.x + b.y) > BVH_TRACE_ONE_PLUS_EPSILON2)
    return false;
  float ct = v_extract_x(v_mul_x(v_dot3_x(e1, s2), invd));
  if (ct < 0.0f || ct >= t)
    return false;
  bc = b;
  t = ct;
  return true;
}
template <bool CullCCW = false>
inline bool RayTriangleIntersectInf(vec3f orig, vec3f dir, vec3f v0, vec3f v1, vec3f v2, float &t, Point2 &bc)
{
  vec3f e0 = v_sub(v1, v0), e1 = v_sub(v2, v0);
  vec3f s1 = v_cross3(dir, e1);
  vec3f det = v_dot3_x(s1, e0);
  if constexpr (CullCCW)
  {
    if (v_extract_x(det) < kEpsilon)
      return false;
  }
  vec3f d = v_sub(orig, v0);
  Point2 b;
  b.x = v_extract_x(v_div_x(v_dot3_x(d, s1), det));
  if (b.x < -BVH_TRACE_EPSILON || b.x > BVH_TRACE_ONE_PLUS_EPSILON)
    return false;
  const vec3f s2 = v_cross3(d, e0);
  b.y = v_extract_x(v_div_x(v_dot3_x(dir, s2), det));
  if (b.y < -BVH_TRACE_EPSILON || (b.x + b.y) > BVH_TRACE_ONE_PLUS_EPSILON2)
    return false;
  t = v_extract_x(v_div_x(v_dot3_x(e1, s2), det));
  bc = b;
  return true;
}
inline bool RayTriangleIntersectInfXZ(vec3f orig, vec3f v0, vec3f v1, vec3f v2, float &t, Point2 &bc)
{
  vec3f a = v_perm_zxyw(v_sub(v0, orig));
  vec3f b = v_perm_zxyw(v_sub(v1, orig));
  vec3f c = v_perm_zxyw(v_sub(v2, orig));
  vec3f v012y = v_perm_ywbd(v_perm_xyab(v0, v1), v2);
  vec3f cab_x = v_perm_xyab(v_perm_xaxa(c, a), b);
  vec3f cab_y = v_perm_ywbd(v_perm_xyab(c, a), b);
  vec3f UVW = v_sub(v_mul(cab_x, v_perm_zxyw(cab_y)), v_mul(cab_y, v_perm_zxyw(cab_x)));
  if (v_check_xyz_any_true(v_cmp_lt(UVW, v_zero())) & v_check_xyz_any_true(v_cmp_lt(v_zero(), UVW))) // -V792
    return false;
  vec3f detV = v_hadd3_x(UVW);
  if (v_test_vec_x_lt(v_abs(detV), v_splats(1e-9f)))
    return false;
  UVW = v_div(UVW, v_splat_x(detV));
  bc.x = v_extract_y(UVW);
  bc.y = v_extract_z(UVW);
  t = v_extract_x(v_dot3_x(v012y, UVW));
  return true;
}

// closest point on triangle (barycentric projection + edge clamp)
inline vec3f closestPointOnTriVec(vec3f p, vec3f a, vec3f b, vec3f c)
{
  vec3f ba = v_sub(b, a), pa = v_sub(p, a);
  vec3f cb = v_sub(c, b), pb = v_sub(p, b);
  vec3f ac = v_sub(a, c), pc = v_sub(p, c);

  vec3f n = v_cross3(ba, ac);
  vec3f q = v_cross3(n, pa);
  float d = 1.f / v_extract_x(v_dot3_x(n, n));
  float u = d * v_extract_x(v_dot3_x(q, ac));
  float v = d * v_extract_x(v_dot3_x(q, ba));
  float w = 1.f - u - v;

#define DOT3(a, b)  v_extract_x(v_dot3_x(a, b))
#define SATURATE(x) ((x) < 0.f ? 0.f : ((x) > 1.f ? 1.f : (x)))
  if (u < 0.f)
  {
    w = SATURATE(DOT3(pc, ac) / DOT3(ac, ac));
    u = 0.f;
    v = 1.f - w;
  }
  else if (v < 0.f)
  {
    u = SATURATE(DOT3(pa, ba) / DOT3(ba, ba));
    v = 0.f;
    w = 1.f - u;
  }
  else if (w < 0.f)
  {
    v = SATURATE(DOT3(pb, cb) / DOT3(cb, cb));
    w = 0.f;
    u = 1.f - v;
  }
#undef SATURATE
#undef DOT3

  return v_add(v_add(v_mul(v_splats(u), b), v_mul(v_splats(v), c)), v_mul(v_splats(w), a));
}

inline void distBLASLeafTri(DistData &d, vec3f v0, vec3f v1, vec3f v2, int dataOffset)
{
  vec3f cp = closestPointOnTriVec(d.pos, v0, v1, v2);
  float dist2 = v_extract_x(v_length3_sq_x(v_sub(cp, d.pos)));
  if (dist2 < d.bestDist2)
  {
    d.bestDist2 = dist2;
    d.bestTriOffset = dataOffset;
  }
}

// ============================================================================
// Out-of-line BLAS traversal declarations (defined in bvhTraversalQuadOOL.cpp)
// ============================================================================

namespace bvh_traverse
{
bool rayBLASQuadOOL(RayData &r, int startOffset, int blasSize);
bool rayBLASQuadOOLCullCCW(RayData &r, int startOffset, int blasSize);
} // namespace bvh_traverse

// ============================================================================
// Quad-encoded BLAS traversal (UINT16 boxes, quad leaf encoding)
// Uses RayData with 21-bit packed vertices (8 bytes/vert in BVH box space)
// ============================================================================

template <bool CullCCW = false, int VertStride = 8>
struct BLASTraverse
{
  using RayDataType = RayData;
  static constexpr int LEAF_SIZE = 20;

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

  struct QuadLeafVerts
  {
    vec3f v0, v1, v2, v3;
    bool isSingle;
    bool isFan;

    template <class VL>
    __forceinline void decode(const uint8_t *data, int dataOffset, uint skip, const VL &vl)
    {
      int ofs1 = dataOffset + ((const int *)(data + dataOffset))[0];
      uint o1 = (skip & QUAD_O1_MASK) + 1;
      uint o2 = ((skip >> QUAD_O2_SHIFT) & QUAD_O2_MASK) + 1;
      uint o3 = ((skip >> QUAD_O3_SHIFT) & QUAD_O3_MASK) + 1;
      isSingle = (o3 == o2);
      isFan = !isSingle && (skip & QUAD_FAN_FLAG);
      v0 = vl(data, ofs1, 0);
      v1 = vl(data, ofs1, o1);
      v2 = vl(data, ofs1, o2);
      v3 = vl(data, ofs1, o3);
    }

    __forceinline void decode(const RayData &rd, int dataOffset, uint skip)
    {
      int ofs1 = dataOffset + ((const int *)(rd.data + dataOffset))[0];
      uint o1 = (skip & QUAD_O1_MASK) + 1;
      uint o2 = ((skip >> QUAD_O2_SHIFT) & QUAD_O2_MASK) + 1;
      uint o3 = ((skip >> QUAD_O3_SHIFT) & QUAD_O3_MASK) + 1;
      isSingle = (o3 == o2);
      isFan = !isSingle && (skip & QUAD_FAN_FLAG);
      v0 = loadVert(rd.data, ofs1, 0);
      v1 = loadVert(rd.data, ofs1, o1);
      v2 = loadVert(rd.data, ofs1, o2);
      v3 = loadVert(rd.data, ofs1, o3);
    }
    __forceinline vec3f tri2a() const { return isFan ? v0 : v2; }
    __forceinline vec3f tri2b() const { return isFan ? v2 : v1; }
    __forceinline vec3f tri2c() const { return v3; }

    // Decode a single triangle (subTri 0 = first/single, 1 = second of quad). Loads only 3 vertices.
    template <class VL>
    __forceinline void decodeTri(const uint8_t *data, int dataOffset, uint skip, int subTri, const VL &vl)
    {
      int ofs1 = dataOffset + ((const int *)(data + dataOffset))[0];
      uint o1 = (skip & QUAD_O1_MASK) + 1;
      uint o2 = ((skip >> QUAD_O2_SHIFT) & QUAD_O2_MASK) + 1;
      uint o3 = ((skip >> QUAD_O3_SHIFT) & QUAD_O3_MASK) + 1;
      bool single = (o3 == o2);
      if (single || subTri == 0)
      {
        v0 = vl(data, ofs1, 0);
        v1 = vl(data, ofs1, o1);
        v2 = vl(data, ofs1, o2);
      }
      else // subTri == 1, quad second triangle
      {
        uint o3 = ((skip >> QUAD_O3_SHIFT) & QUAD_O3_MASK) + 1;
        v2 = vl(data, ofs1, o3);           // v3 = always used
        vec3f shared = vl(data, ofs1, o2); // v2 = shared edge vertex
        if (skip & QUAD_FAN_FLAG)
        { // fan: (v0, v2, v3)
          v0 = vl(data, ofs1, 0);
          v1 = shared;
        }
        else
        { // strip: (v2, v1, v3)
          v0 = shared;
          v1 = vl(data, ofs1, o1);
        }
      }
    }
  };

  // Vertex loader adapters for iterateFiltered/decode
  struct RDVertexLoaderRaw // returns unscaled raw coords; caller scales ray by 32 instead
  {
    __forceinline vec3f operator()(const uint8_t *d, int baseOfs, uint vertIdx) const { return loadVertRaw(d, baseOfs, vertIdx); }
  };
  struct RDVertexLoader // returns box-space coords (divided by 32)
  {
    __forceinline vec3f operator()(const uint8_t *d, int baseOfs, uint vertIdx) const { return loadVert(d, baseOfs, vertIdx); }
  };

  // ---- Leaf intersection functions ----

  static inline bool rayLeaf(RayData &r, int dataOffset, uint skip)
  {
    QuadLeafVerts q;
    q.decode(r, dataOffset, skip);
    bool anyHit = false;
    if (q.isSingle)
    {
      anyHit = RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, q.v0, q.v1, q.v2, r.t, r.bCoord);
      if (anyHit)
        r.bestSubTri = 0;
    }
    else
    {
      if (RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, q.v0, q.v1, q.v2, r.t, r.bCoord))
      {
        anyHit = true;
        r.bestSubTri = 0;
      }
      if (RayTriangleIntersect<CullCCW>(r.rayOrigin, r.rayDir, q.tri2a(), q.tri2b(), q.tri2c(), r.t, r.bCoord))
      {
        anyHit = true;
        r.bestSubTri = 1;
      }
    }
    return anyHit;
  }

  static inline void distLeaf(DistData &d, int dataOffset, uint skip)
  {
    QuadLeafVerts q;
    q.decode(d.data, dataOffset, skip, RDVertexLoader{});
    if (q.isSingle)
    {
      distBLASLeafTri(d, q.v0, q.v1, q.v2, dataOffset);
    }
    else
    {
      vec3f mn1 = v_min(q.v0, v_min(q.v1, q.v2)), mx1 = v_max(q.v0, v_max(q.v1, q.v2));
      if (v_extract_x(v_distance_sq_to_bbox_x(mn1, mx1, d.pos)) < d.bestDist2)
        distBLASLeafTri(d, q.v0, q.v1, q.v2, dataOffset);
      vec3f a = q.tri2a(), b = q.tri2b(), c = q.tri2c();
      vec3f mn2 = v_min(a, v_min(b, c)), mx2 = v_max(a, v_max(b, c));
      if (v_extract_x(v_distance_sq_to_bbox_x(mn2, mx2, d.pos)) < d.bestDist2)
        distBLASLeafTri(d, a, b, c, dataOffset);
    }
  }

  // ---- Full BLAS traversal functions ----

  template <class HitCb = BestHitCb>
  static inline bool rayBLAS(RayData &r, int startOffset, int blasSize, const HitCb &cb = HitCb())
  {
    int dataOffset = startOffset;
    const int endOffset = startOffset + blasSize;
    vec3f rayOriginScaled = v_neg(v_mul(r.rayOrigin, r.rayDirInv));
    for (; dataOffset < endOffset;)
    {
      vec3f bboxMin, bboxMax;
      uint offsetToNextNode;
      decodeRaw(r.data, dataOffset, bboxMin, bboxMax, offsetToNextNode);
      bool collision =
        RayIntersectsBoxT0T1(v_madd(bboxMin, r.rayDirInv, rayOriginScaled), v_madd(bboxMax, r.rayDirInv, rayOriginScaled), r.t);
      dataOffset += BVH_BLAS_NODE_SIZE;
      const uint32_t isLeaf = offsetToNextNode & BLAS_LEAF_FLAG;
      if (!collision)
      {
        dataOffset += isLeaf ? LEAF_SIZE - BVH_BLAS_NODE_SIZE : offsetToNextNode;
      }
      else if (isLeaf)
      {
        if (rayLeaf(r, dataOffset, offsetToNextNode))
        {
          if (cb(r, dataOffset))
            break;
        }
        dataOffset += LEAF_SIZE - BVH_BLAS_NODE_SIZE;
      }
    }
    return (r.bestTriOffset > 0);
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
    iterateFiltered(
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
    iterateFiltered(
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
      bool collision = v_extract_x(v_distance_sq_to_bbox_x(bboxMin, bboxMax, d.pos)) < d.bestDist2;
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

  // Generic filtered BVH traversal: test each node, call leafCb with triangle vertices.
  // NodeTest: (vec3f bmin, vec3f bmax) -> bool (true = overlap, traverse children)
  // LeafCb:   (vec3f v0, vec3f v1, vec3f v2, int leafOffset) -> bool (true = stop all)
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
        if (leafCb(q.v0, q.v1, q.v2, dataOffset))
          return true;
        if (!q.isSingle)
        {
          if (leafCb(q.tri2a(), q.tri2b(), q.tri2c(), dataOffset))
            return true;
        }
        dataOffset += LEAF_SIZE - BVH_BLAS_NODE_SIZE;
      }
    }
    return false;
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
    int dataOffset = startOffset, lastInsideOffset = -1;
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
    return lastInsideOffset;
  }
};
