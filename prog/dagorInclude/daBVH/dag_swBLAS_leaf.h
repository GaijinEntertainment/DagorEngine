//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Shared double-quad BLAS leaf decode and ray/geometry primitives, independent of the tree layout
// around the leaves. Tree traversal lives in dag_swBLAS_ray.h (stackless format); include this
// header alone when only leaf decode or triangle tests are needed (MOC quad rendering, BLAS IO,
// face walkers). Like the rest of daBVH CPU code, uses vecmath (vec3f/v_*) without including it.

#include <stdint.h>
#include <string.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <math/dag_Point2.h>
#include <math/dag_bits.h> // __bsf_unsafe (best-lane pick in swblas_rayLeaf_SoA)
#include <util/dag_compilerDefs.h>

#define BVH_TRACE_EPSILON 0.000004f

static constexpr uint32_t BLAS_LEAF_FLAG = QUAD_LEAF_FLAG;
// Packed vert21 stride (bytes per vertex) in a collision BLAS vert stream; vertex index <-> byte
// offset conversions go through this. The stream stays stride-aligned (see the MOC walkers' index recovery).
static constexpr uint32_t BVH_BLAS_VERT21_STRIDE = 8;

struct RayData // -V730
{
  vec3f rayOrigin, rayDir;
  const uint8_t *data = nullptr;
  float t = 0;
  Point2 bCoord;
  int bestTriOffset = 0;
  int8_t bestSubTri = 0; // sub-triangle hit, 0..3: 0/1 = quad A tri 1/2, 2/3 = quad B tri 1/2
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

// Vertex loader adapters shared by the walker surfaces (vertIdx is a SIGNED offset from the apex
// base, Stride the packed vertex stride in bytes) -- one authority so the stackless and SoA4
// walkers cannot fork per vertex format.
template <int Stride>
struct Vert21StrideLoader // returns box-space coords (divided by 32)
{
  __forceinline vec3f operator()(const uint8_t *d, int baseOfs, int vertIdx) const
  {
    return RayData::unpackVert21(d + baseOfs + vertIdx * Stride);
  }
};
template <int Stride>
struct Vert21StrideLoaderRaw // returns unscaled raw coords; caller scales its query by 32 instead
{
  __forceinline vec3f operator()(const uint8_t *d, int baseOfs, int vertIdx) const
  {
    return RayData::unpackVert21Raw(d + baseOfs + vertIdx * Stride);
  }
};

// The distance query runs in UNSCALED (rotated world-metric) leaf-local space: the encoded
// space is anisotropic (per-axis scale), where both candidate selection and the closest point
// come out wrong. pos must be unscaled; invScale converts encoded node boxes/verts to that
// space (identity for callers whose data is already world-metric).
struct DistData // -V730 (pos/data/bestDist2/bestTriOffset are caller-filled before the walk, same contract as RayData)
{
  vec3f pos;
  vec3f bestPos; // closest point (in unscaled query space), valid once bestTriOffset >= 0
  vec3f invScale = V_C_ONE;
  const uint8_t *data;
  float bestDist2; // world-metric
  int bestTriOffset;
  // pos is behind the winning triangle's front face AND the closest point lies on that face
  // (edge/vertex-closest points cannot be classified by one plane and stay outside). Valid once
  // bestTriOffset >= 0. Reflected (mirrored) instances flip winding and thus this sign; same
  // limitation as the GPU path.
  bool bestInside = false;
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
// Same slab math as RayIntersectsBoxT0T1, but also returns the box entry distance (near t, clamped
// to >= 0). True iff the box overlaps [0, ray_extent].
inline bool RayIntersectsBoxNearT(vec3f t0, vec3f t1, float ray_extent, float &near_t)
{
  const vec3f tmax = v_perm_xyzd(v_max(t0, t1), v_splats(ray_extent));
  const vec3f tmin = v_perm_xyzd(v_min(t0, t1), v_zero());
  vec3f tmaxMin = v_min(tmax, v_perm_zwxy(tmax));
  tmaxMin = v_min(tmaxMin, v_perm_yzwx(tmaxMin));
  vec3f tminMax = v_max(tmin, v_perm_zwxy(tmin));
  tminMax = v_max(tminMax, v_perm_yzwx(tminMax));
  near_t = v_extract_x(tminMax);
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

// Jolt-style 4-wide SoA ray-vs-triangle. Tests 4 triangles in parallel; returns 4 distances
// (FLT_MAX for misses). u,v bary coords per lane in outU/outV. Lanes the caller wants to ignore
// (e.g. degenerate padding) should have all-zero vertex data so det == 0 -> miss.
// CullCCW = true rejects lanes where det <= eps (backfacing or degenerate), matching the
// behavior of RayTriangleIntersect<true>.
template <bool CullCCW>
__forceinline vec4f rayTriangle4_SoA(vec3f orig, vec3f dir, vec4f v0x, vec4f v0y, vec4f v0z, vec4f v1x, vec4f v1y, vec4f v1z,
  vec4f v2x, vec4f v2y, vec4f v2z, vec4f &outU, vec4f &outV)
{
  vec4f e1x = v_sub(v1x, v0x), e1y = v_sub(v1y, v0y), e1z = v_sub(v1z, v0z);
  vec4f e2x = v_sub(v2x, v0x), e2y = v_sub(v2y, v0y), e2z = v_sub(v2z, v0z);
  vec4f dx = v_splat_x(dir), dy = v_splat_y(dir), dz = v_splat_z(dir);
  vec4f ox = v_splat_x(orig), oy = v_splat_y(orig), oz = v_splat_z(orig);
  vec4f px = v_sub(v_mul(dy, e2z), v_mul(dz, e2y));
  vec4f py = v_sub(v_mul(dz, e2x), v_mul(dx, e2z));
  vec4f pz = v_sub(v_mul(dx, e2y), v_mul(dy, e2x));
  vec4f det = v_add(v_add(v_mul(e1x, px), v_mul(e1y, py)), v_mul(e1z, pz));
  vec4f signBit = v_cast_vec4f(v_splatsi(0x80000000));
  vec4f detSign = v_and(det, signBit);
  vec4f absDet = v_xor(det, detSign);
  // CullCCW: reject signed det < kEpsilon (matches scalar RayTriangleIntersect<true>'s `det < kEpsilon` cull).
  // !CullCCW: reject |det| < V_C_VERY_SMALL_VAL (~sqrt(FLT_MIN), 4e-19f). Scalar uses
  // v_rcp_safe(det, V_C_MAX_VAL) which substitutes 1e32 for the inverse when |det| < V_C_VERY_SMALL_VAL,
  // so subsequent bary checks reject. We reject explicitly with the same threshold for equivalent behavior.
  vec4f detTooSmall = CullCCW ? v_cmp_lt(det, v_splats(float(kEpsilon))) : v_cmp_lt(absDet, V_C_VERY_SMALL_VAL);
  vec4f detSafe = v_sel(absDet, v_splats(1.f), detTooSmall);
  vec4f sx = v_sub(ox, v0x), sy = v_sub(oy, v0y), sz = v_sub(oz, v0z);
  vec4f u = v_xor(v_add(v_add(v_mul(sx, px), v_mul(sy, py)), v_mul(sz, pz)), detSign);
  vec4f qx = v_sub(v_mul(sy, e1z), v_mul(sz, e1y));
  vec4f qy = v_sub(v_mul(sz, e1x), v_mul(sx, e1z));
  vec4f qz = v_sub(v_mul(sx, e1y), v_mul(sy, e1x));
  vec4f vp = v_xor(v_add(v_add(v_mul(dx, qx), v_mul(dy, qy)), v_mul(dz, qz)), detSign);
  vec4f t = v_xor(v_add(v_add(v_mul(e2x, qx), v_mul(e2y, qy)), v_mul(e2z, qz)), detSign);
  vec4f zero = v_zero();
  // Edge-grazing tolerance in det-space (matches scalar RayTriangleIntersect's BVH_TRACE_EPSILON
  // applied to barycentric coords): accept u in [-eps*|det|, |det|*(1+eps)], v in [-eps*|det|, ...],
  // and u+v in [..., |det|*(1+2*eps)]. Without this, edge rays on external/cross-leaf edges that the
  // scalar path would have hit can fall through both lanes' strict u<0/v<0/u+v>det rejects.
  vec4f epsAbsDet = v_mul(absDet, v_splats(BVH_TRACE_EPSILON));
  vec4f negEpsAbsDet = v_xor(epsAbsDet, signBit); // -eps*|det|
  vec4f detPlusEps = v_add(absDet, epsAbsDet);
  vec4f detPlus2Eps = v_add(detPlusEps, epsAbsDet);
  vec4f noHit = v_or(v_or(detTooSmall, v_or(v_cmp_lt(u, negEpsAbsDet), v_cmp_gt(u, detPlusEps))),
    v_or(v_or(v_cmp_lt(vp, negEpsAbsDet), v_cmp_gt(v_add(u, vp), detPlus2Eps)), v_cmp_lt(t, zero)));
  vec4f invDet = v_rcp(detSafe);
  outU = v_mul(u, invDet);
  outV = v_mul(vp, invDet);
  vec4f tDist = v_mul(t, invDet);
  return v_sel(tDist, v_splats(3.4e38f), noHit);
}

inline void distBLASLeafTri(DistData &d, vec3f v0, vec3f v1, vec3f v2, int dataOffset)
{
  vec3f cp = closestPointOnTriVec(d.pos, v0, v1, v2);
  float dist2 = v_extract_x(v_length3_sq_x(v_sub(cp, d.pos)));
  if (dist2 < d.bestDist2)
  {
    d.bestDist2 = dist2;
    d.bestPos = cp;
    d.bestTriOffset = dataOffset;
    // Plane side of the winning triangle, but ONLY when the closest point lies on the face
    // (|planeD| == dist): one plane cannot classify edge/vertex-closest points, and open soups
    // would leak negatives (same guard as the GPU distBlas). Positive anisotropic scale
    // preserves sidedness.
    vec3f n = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
    float planeD = v_extract_x(v_dot3_x(n, v_sub(d.pos, v0)));
    d.bestInside = planeD < 0.f && planeD * planeD >= dist2 * v_extract_x(v_length3_sq_x(n)) * 0.99f;
  }
}

// ============================================================================
// Double-quad leaf decode (shared bit-layout authority)
// ============================================================================

// Sign-extend the low `bits` of v (the quad-leaf offset/base fields are stored 2's-complement).
__forceinline int swblas_sx(uint32_t v, int bits)
{
  int s = 32 - bits;
  return (int)(v << s) >> s;
}

// Quantize a box-space AABB onto the u16 box lattice: floor the min, ceil the max, clamp to
// [0, 65535]. THE rule every stored u16 leaf/node box is written with; box rebuilds (bvhIO,
// rootLeafBox) must use the same rule so filtering stays aligned with stored boxes.
__forceinline void swblas_quantize_box_u16(vec4f bmin, vec4f bmax, vec4i &bmin_i, vec4i &bmax_i)
{
  bmin_i = v_clampi(v_cvt_floori(bmin), v_zeroi(), v_splatsi(65535));
  bmax_i = v_clampi(v_cvt_ceili(bmax), v_zeroi(), v_splatsi(65535));
}

// Decoded bit fields of a double-quad BLAS leaf body (skip word W0 plus W1/W2/W3), in vertex-index
// units and independent of the vertex stride. This is the single authority for the leaf bit layout
// defined in swBLASLeafDefs.hlsli -- CPU traversal, collision face walking and MOC all decode through
// it, so a layout change touches one place. Turning relBaseBytes/deltaB into byte offsets (needs the
// consumer's vertex stride) and loading verts is left to the caller.
struct QuadLeafFields
{
  uint32_t relBaseBytes; // quad A apex byte offset relative to the leaf body offset; always >= 0
  uint32_t deltaB;       // quad B apex - quad A apex, in vertex-index units (0 when !hasB)
  int o1, o2, o3;        // quad A signed vertex-index offsets from its apex
  int o1b, o2b, o3b;     // quad B signed vertex-index offsets from its apex
  bool isSingle;         // quad A 2nd sub-tri degenerate (o3 == o2)
  bool flipSecond;       // quad A 2nd sub-tri winding (cull/closest only)
  bool hasB;             // leaf carries a 2nd quad (sub-tris 2,3); sentinel o1b != o2b
  bool isSingleB;        // quad B 2nd sub-tri degenerate (o3b == o2b)
  bool flipSecondB;      // quad B 2nd sub-tri winding
  uint32_t user;         // per-leaf user bits, opaque here (0 unless the producer stamped them)
};

__forceinline QuadLeafFields decodeQuadLeafFields(uint32_t skip, uint32_t w1, uint32_t w2, uint32_t w3)
{
  QuadLeafFields f;
  f.relBaseBytes = (w1 & QUAD_BASE_MASK) << QUAD_BASE_ALIGN_SHIFT;
  f.deltaB = w2 & QUADB_BASE_MASK;
  f.o1 = swblas_sx(skip, QUAD_O_BITS);
  f.o2 = swblas_sx(skip >> QUAD_O2_SHIFT, QUAD_O_BITS);
  f.o3 = swblas_sx(((skip >> QUAD_O3_LO_SHIFT) & QUAD_O3_LO_MASK) | ((w1 >> QUAD_O3_HI_SHIFT) << QUAD_O3_LO_BITS), QUAD_O_BITS);
  f.o1b = swblas_sx(w2 >> QUADB_O1_SHIFT, QUAD_O_BITS);
  f.o2b = swblas_sx(w3, QUAD_O_BITS);
  f.o3b = swblas_sx(w3 >> QUADB_O3_SHIFT, QUAD_O_BITS);
  f.isSingle = (f.o3 == f.o2);
  f.flipSecond = (w2 & QUAD_FLIPA_FLAG) != 0;
  f.hasB = (f.o1b != f.o2b);
  f.isSingleB = (f.o3b == f.o2b);
  f.flipSecondB = (w2 & QUAD_FLIPB_FLAG) != 0;
  f.user = (w3 >> QUAD_LEAF_USER_SHIFT) & QUAD_LEAF_USER_MASK;
  return f;
}

// Decode a leaf from its body pointer: skip word W0 sits at [-1] (4th uint32 of the 16-byte node
// header), W1/W2/W3 follow at [0]/[1]/[2].
__forceinline QuadLeafFields decodeQuadLeafFieldsAt(const uint8_t *data, uint32_t dataOffset)
{
  const uint32_t *p = (const uint32_t *)(data + dataOffset);
  return decodeQuadLeafFields(p[-1], p[0], p[1], p[2]);
}

// Sub-triangle count (1..4) a leaf emits, from the same present/single sentinels expandQuadLeafTris
// uses so the count and the expansion cannot drift: quad A is 1 or 2 tris, quad B (when present) adds
// 1 or 2. Lets a caller apply a partition window before paying the per-tri index expand.
__forceinline uint32_t quadLeafTriCount(const QuadLeafFields &f)
{
  return (f.isSingle ? 1u : 2u) + (f.hasB ? (f.isSingleB ? 1u : 2u) : 0u);
}

// Expand a decoded leaf into its 1..4 sub-triangle index triples, calling cb(i0, i1, i2) per triangle in
// canonical order (quad A tri 1, [tri 2], then quad B tri 1, [tri 2]). `base` is quad A's apex vertex
// index in the caller's index space (quad B's apex is base + f.deltaB). Each quad's 2nd triangle is the
// strip (v1,v2,v3) with flipSecond swapping the first two corners; a single quad (isSingle/isSingleB)
// emits only its 1st triangle. The one place for the W1/W2/W3 winding rules, shared by MOC, collision
// face-walking and getNodeFaceVertsByRef (the GPU/SoA hot paths keep their own specialized decoders).
template <class Cb>
__forceinline void expandQuadLeafTris(const QuadLeafFields &f, uint32_t base, const Cb &cb)
{
  cb(base, base + (uint32_t)f.o1, base + (uint32_t)f.o2);
  if (!f.isSingle)
    cb(base + (uint32_t)(f.flipSecond ? f.o2 : f.o1), base + (uint32_t)(f.flipSecond ? f.o1 : f.o2), base + (uint32_t)f.o3);
  if (f.hasB)
  {
    const uint32_t baseB = base + f.deltaB;
    cb(baseB, baseB + (uint32_t)f.o1b, baseB + (uint32_t)f.o2b);
    if (!f.isSingleB)
      cb(baseB + (uint32_t)(f.flipSecondB ? f.o2b : f.o1b), baseB + (uint32_t)(f.flipSecondB ? f.o1b : f.o2b),
        baseB + (uint32_t)f.o3b);
  }
}

// Every sub-triangle vertex index of a decoded leaf must land inside [0, vert_count) -- the one
// bound authority for format-bearing validators (converter root bodies, stream deserializers).
__forceinline bool validateQuadLeafVertexIndices(const QuadLeafFields &f, uint32_t apex_idx, uint32_t vert_count)
{
  bool ok = true;
  expandQuadLeafTris(f, apex_idx,
    [&](uint32_t a, uint32_t b, uint32_t c) { ok = ok && a < vert_count && b < vert_count && c < vert_count; });
  return ok;
}

// Corner order of a quad's 2nd (strip) triangle from already-loaded verts: (v1,v2,v3), or
// (v2,v1,v3) when flip - the register-space form of expandQuadLeafTris' index rule.
static __forceinline void swblas_secondTriCorners(bool flip, vec3f v1, vec3f v2, vec3f v3, vec3f &a, vec3f &b, vec3f &c)
{
  a = flip ? v2 : v1;
  b = flip ? v1 : v2;
  c = v3;
}

// ============================================================================
// SoA 4-wide leaf vertex fetch + corner permutes (shared by leaf-testing traversals)
// ============================================================================

// Unpack 4 verts at (baseByteOfs + o*VertStride) into SoA: xs/ys/zs each = (v0,v1,v2,v3) per axis.
// VertStride 8 -> packed vert21 (21 bits per axis, box space); else raw float3 (transpose).
template <int VertStride>
static __forceinline void swblas_unpack4SoA(const uint8_t *data, int baseByteOfs, int o1, int o2, int o3, vec4f &xs, vec4f &ys,
  vec4f &zs)
{
  if constexpr (VertStride == 8)
  {
    // vert21 packed uint64 = X[0:20] | Y[21:41] | Z[42:62]
    // As 2x uint32 LE: c1 = X[0:20] | Y_low_11[21:31];  c2 = Y_high_10[0:9] | Z[10:30] | unused[31]
    const int *p0 = (const int *)(data + baseByteOfs);
    const int *p1 = (const int *)(data + baseByteOfs + o1 * VertStride);
    const int *p2 = (const int *)(data + baseByteOfs + o2 * VertStride);
    const int *p3 = (const int *)(data + baseByteOfs + o3 * VertStride);
    vec4i c1 = v_make_vec4i(p0[0], p1[0], p2[0], p3[0]);
    vec4i c2 = v_make_vec4i(p0[1], p1[1], p2[1], p3[1]);
    vec4i mask21 = v_splatsi(0x1FFFFF);
    vec4i mask10 = v_splatsi(0x3FF);
    vec4i xsi = v_andi(c1, mask21);
    vec4i ysi = v_ori(v_srli(c1, 21), v_slli(v_andi(c2, mask10), 11));
    vec4i zsi = v_andi(v_srli(c2, 10), mask21);
    vec4f inv32 = v_splats(1.f / 32.f);
    xs = v_mul(v_cvt_vec4f(xsi), inv32);
    ys = v_mul(v_cvt_vec4f(ysi), inv32);
    zs = v_mul(v_cvt_vec4f(zsi), inv32);
  }
  else
  {
    // Raw float3 verts (12 B each). Load each as (x,y,z,_) then SIMD-transpose to SoA.
    // v_ldu_p3 reads 4 floats (the 4th is harmless as long as the BLAS buffer has any trailing
    // bytes -- true for our packed vert array which is followed by the BVH tree).
    vec4f v0 = v_ldu_p3((const float *)(data + baseByteOfs));
    vec4f v1 = v_ldu_p3((const float *)(data + baseByteOfs + o1 * VertStride));
    vec4f v2 = v_ldu_p3((const float *)(data + baseByteOfs + o2 * VertStride));
    vec4f v3 = v_ldu_p3((const float *)(data + baseByteOfs + o3 * VertStride));
    v_mat44_transpose(v0, v1, v2, v3); // AoS rows -> SoA columns: v0=xs, v1=ys, v2=zs, v3=ws(unused)
    xs = v0;
    ys = v1;
    zs = v2;
  }
}

// One quad's two triangles as corner SoA in lanes 0,1 (lanes 2,3 are scratch, discarded by the
// v_perm_xyab combine). xs/ys/zs = the quad's 4 verts per axis. triA=(v0,v1,v2); triB is the strip
// (v1,v2,v3) or, when flip, (v2,v1,v3). For shadows the winding is irrelevant (any-hit), so a caller
// that never needs the normal/bary may pass flip=false unconditionally to drop the branch.
static __forceinline void swblas_quadCorners2(bool flip, vec4f xs, vec4f ys, vec4f zs, vec4f &c0x, vec4f &c0y, vec4f &c0z, vec4f &c1x,
  vec4f &c1y, vec4f &c1z, vec4f &c2x, vec4f &c2y, vec4f &c2z)
{
  if (!flip) // triA=(v0,v1,v2) triB=(v1,v2,v3)
  {
    c0x = v_perm_xyab(xs, xs);
    c1x = v_perm_yzab(xs, xs);
    c2x = v_perm_zwcd(xs, xs);
    c0y = v_perm_xyab(ys, ys);
    c1y = v_perm_yzab(ys, ys);
    c2y = v_perm_zwcd(ys, ys);
    c0z = v_perm_xyab(zs, zs);
    c1z = v_perm_yzab(zs, zs);
    c2z = v_perm_zwcd(zs, zs);
  }
  else // triA=(v0,v1,v2) triB=(v2,v1,v3)
  {
    c0x = v_perm_xzac(xs, xs);
    c1x = v_perm_yybb(xs, xs);
    c2x = v_perm_zwcd(xs, xs);
    c0y = v_perm_xzac(ys, ys);
    c1y = v_perm_yybb(ys, ys);
    c2y = v_perm_zwcd(ys, ys);
    c0z = v_perm_xzac(zs, zs);
    c1z = v_perm_yybb(zs, zs);
    c2z = v_perm_zwcd(zs, zs);
  }
}

// SoA leaf processor for a FULL leaf body (W1 W2 W3 at body_ofs, W0 passed in): decode both quads,
// run one 4-wide ray-tri across all 4 sub-triangles. Lanes 0,1 = quad A tris; lanes 2,3 = quad B
// tris (degenerate when no quad B or quad single). Layout-independent: both the stackless and the
// SoA4 tree store full leaf bodies in exactly this shape, only W0's location differs.
template <bool CullCCW, int VertStride = 8>
static __forceinline bool swblas_rayLeaf_SoA(RayData &r, int dataOffset, uint32_t skip)
{
  const uint8_t *data = r.data;
  const QuadLeafFields f = decodeQuadLeafFields(skip, ((const uint *)(data + dataOffset))[0], ((const uint *)(data + dataOffset))[1],
    ((const uint *)(data + dataOffset))[2]);
  const int baseA = dataOffset + (int)f.relBaseBytes;
  // Decode flip unconditionally: the returned barycentrics (r.bCoord) are taken against this vertex
  // order, and the canonical sub-tri reconstruction (decodeTri/getTri) always applies flip, so the SoA
  // path must too -- else a two-sided closest hit would report u,v for the unflipped 2nd-tri order.
  vec4f xsA, ysA, zsA;
  swblas_unpack4SoA<VertStride>(data, baseA, f.o1, f.o2, f.o3, xsA, ysA, zsA);

  vec4f xsB, ysB, zsB;
  bool flipB = false;
  if (f.hasB)
  {
    flipB = f.flipSecondB; // always decoded (bary consistency, as for quad A above)
    swblas_unpack4SoA<VertStride>(data, baseA + (int)f.deltaB * VertStride, f.o1b, f.o2b, f.o3b, xsB, ysB, zsB);
  }
  else
    xsB = ysB = zsB = v_zero(); // lanes 2,3 -> zero-area tris, rejected

  vec4f a0x, a0y, a0z, a1x, a1y, a1z, a2x, a2y, a2z;
  vec4f b0x, b0y, b0z, b1x, b1y, b1z, b2x, b2y, b2z;
  swblas_quadCorners2(f.flipSecond, xsA, ysA, zsA, a0x, a0y, a0z, a1x, a1y, a1z, a2x, a2y, a2z);
  swblas_quadCorners2(flipB, xsB, ysB, zsB, b0x, b0y, b0z, b1x, b1y, b1z, b2x, b2y, b2z);
  vec4f c0x = v_perm_xyab(a0x, b0x), c0y = v_perm_xyab(a0y, b0y), c0z = v_perm_xyab(a0z, b0z);
  vec4f c1x = v_perm_xyab(a1x, b1x), c1y = v_perm_xyab(a1y, b1y), c1z = v_perm_xyab(a1z, b1z);
  vec4f c2x = v_perm_xyab(a2x, b2x), c2y = v_perm_xyab(a2y, b2y), c2z = v_perm_xyab(a2z, b2z);

  vec4f us, vs;
  vec4f ts = rayTriangle4_SoA<CullCCW>(r.rayOrigin, r.rayDir, c0x, c0y, c0z, c1x, c1y, c1z, c2x, c2y, c2z, us, vs);
  // Branchless best-lane pick: min-reduce the 4 t's, then the lowest lane bitwise-equal to the min
  // wins (same first-lane tie-break as a scalar < scan; misses carry FLT_MAX-like t, never NaN --
  // rayTriangle4_SoA sel's them). A min at or past r.t rejects the leaf, which also makes any
  // pre-masking of farther lanes against r.t unnecessary.
  vec4f m2 = v_min(ts, v_perm_zwxy(ts));
  vec4f tMin = v_min(m2, v_perm_yzwx(m2)); // the min in all four lanes
  if (v_extract_x(tMin) >= r.t)
    return false;
  const int bestLane = (int)__bsf_unsafe((unsigned)v_truemask(v_cmp_eq(ts, tMin)));
  alignas(16) float uArr[4], vArr[4];
  v_st(uArr, us);
  v_st(vArr, vs);
  r.t = v_extract_x(tMin);
  r.bCoord.x = uArr[bestLane];
  r.bCoord.y = vArr[bestLane];
  r.bestSubTri = (int8_t)bestLane;
  return true;
}

// ============================================================================
// Ordered ("nearest-first") traversal key helpers, layout-independent
// ============================================================================

// A child's (entry distance, byte offset) packed into one uint64: distance float bits high, offset
// low. near_t is clamped >= 0 (RayIntersectsBoxNearT) and the IEEE-754 bit pattern of a non-negative
// float is monotonic in value, so plain uint64 order == entry-distance order (the offset only breaks
// ties, which cannot affect the hit found).
__forceinline uint64_t swblas_packChildKey(float near_t, int ofs)
{
  uint32_t tb;
  memcpy(&tb, &near_t, sizeof(tb));
  return ((uint64_t)tb << 32) | (uint32_t)ofs;
}
__forceinline float swblas_childKeyDist(uint64_t key)
{
  uint32_t tb = (uint32_t)(key >> 32);
  float f;
  memcpy(&f, &tb, sizeof(f));
  return f;
}
__forceinline int swblas_childKeyOfs(uint64_t key) { return (int)(uint32_t)key; }

// Branchless compare-exchange leaving a <= b: `a < b` lowers to cmp+sbb, the xor-swap needs no cmov.
__forceinline void swblas_cax64(uint64_t &a, uint64_t &b)
{
  const uint64_t d = (a ^ b) & (0ull - (uint64_t)(a < b));
  const uint64_t mn = b ^ d, mx = a ^ d;
  a = mn;
  b = mx;
}

// Flat, fully-unrolled branchless ascending sort of exactly 4 keys: the optimal 5-comparator network
// (3 stages), no loop. UINT64_MAX padding for absent/missed lanes sinks to the top.
__forceinline void swblas_sort4net(uint64_t &a, uint64_t &b, uint64_t &c, uint64_t &d)
{
  swblas_cax64(a, b);
  swblas_cax64(c, d);
  swblas_cax64(a, c);
  swblas_cax64(b, d);
  swblas_cax64(b, c);
}
