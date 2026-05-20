// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBLASBoxResemblance.h>
#include <vecmath/dag_vecMath.h> // must precede dag_swBLAS_ray.h -- that header uses v_* helpers without including vecmath itself
#include <daBVH/dag_swBLAS_ray.h>
#include <math.h>
#include <float.h>

namespace daSWRT
{

//
// Monte-Carlo box-resemblance: shoot rays through the BLAS's AABB, count how many also
// hit the BLAS via the existing CPU shadow trace. Two flavors share the same kernel:
//   - 3a) isotropic over the direction sphere (general resemblance)
//   - 3b) fixed light direction (sun-specific resemblance)
//
// Build-time / offline only. BLASTraverse<>::rayBLAS reads packed-21-bit vertices at the
// start of the BLAS buffer (testMocBLAS-style layout, produced by build_bvh::writeQuadBVH2
// with `useHalves=false`). It does NOT support the FP16 BLAS layout that
// RenderSWRT::buildBLAS emits at runtime, and intentionally so -- the MC functions are
// meant for offline analysis (e.g. dim_as_box_dist autodetection), not runtime use.

namespace
{

// xorshift32 -- cheap, deterministic, good enough for stratified-by-count sampling.
struct Rng
{
  uint32_t s;
  explicit Rng(uint32_t seed) : s(seed ? seed : 0xA5A5A5A5u) {}
  uint32_t next()
  {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  // [0, 1) with 24-bit mantissa precision -- enough for both direction sampling and
  // 2D origin offsets at any sane resolution.
  float nextF() { return float(next() >> 8) * (1.f / float(1 << 24)); }
};

struct AnyHitCb
{
  bool operator()(RayData &r, int dataOffset) const
  {
    // Same convention BestHitCb uses: rayBLAS returns (bestTriOffset > 0). dataOffset
    // is always >= startOffset + 16 here (advanced past the BVH node header), so 0
    // remains an unambiguous "no hit" sentinel.
    r.bestTriOffset = dataOffset;
    return true; // any-hit: stop traversal at first triangle hit
  }
};

// Uniform sample on the unit sphere (Archimedes / Marsaglia).
static __forceinline vec3f randomUnitDir(Rng &rng)
{
  const float z = rng.nextF() * 2.f - 1.f;
  const float phi = rng.nextF() * 2.f * float(M_PI);
  const float r = sqrtf(1.f - z * z);
  return v_make_vec4f(r * cosf(phi), r * sinf(phi), z, 0.f);
}

// Build an orthonormal basis (u, v) given a unit direction `d`. Robust against axis
// alignment by switching the fallback up vector when `d` is near vertical.
static __forceinline void buildBasisFor(vec3f d, vec3f &u, vec3f &v)
{
  const vec3f up = (fabsf(v_extract_y(d)) < 0.9f) ? v_make_vec4f(0, 1, 0, 0) : v_make_vec4f(1, 0, 0, 0);
  u = v_norm3(v_cross3(up, d));
  v = v_cross3(d, u);
}

// Trace `rays_to_cast` shadow rays through direction `d`. Origins are sampled uniformly
// inside the 2D AABB of the BLAS-box's silhouette on the plane perpendicular to `d`,
// back-shifted along `-d` so each origin lies outside the box.
//
//   aabb_hits  -- rays that actually pierce the BLAS's [0, 65535]^3 root box
//   blas_hits  -- subset of those that also hit BLAS geometry
//
// Rays falling outside the silhouette miss the AABB and are excluded from both totals,
// keeping the ratio independent of the rectangular sampling window we picked.
static void traceDir(const uint8_t *blas_data, int tree_start, int tree_bytes, vec3f d, int rays_to_cast, Rng &rng, int &aabb_hits,
  int &blas_hits)
{
  vec3f u, v;
  buildBasisFor(d, u, v);

  const vec3f bmin = v_zero();
  const vec3f bmax = v_splats(65535.f);
  const vec3f boxCenter = v_splats(32767.5f);

  // Projection of [0, 65535]^3 onto axis `a`: half-extent = 0.5 * 65535 * sum_k |a.k|.
  // dot3(abs(a), splat(32767.5)) collapses that to a single MAD.
  const vec3f halfExt = v_splats(32767.5f);
  const float uHalf = v_extract_x(v_dot3_x(v_abs(u), halfExt));
  const float vHalf = v_extract_x(v_dot3_x(v_abs(v), halfExt));

  // Back-shift by the box diagonal (~113.5k) plus margin, so origins start outside the AABB.
  const float backDist = 65535.f * 2.f;
  const vec3f originBase = v_sub(boxCenter, v_mul(d, v_splats(backDist)));

  const vec3f dirInv = v_rcp_safe(d, V_C_MAX_VAL); // matches RayData::calc() convention

  for (int k = 0; k < rays_to_cast; k++)
  {
    const float ru = (rng.nextF() * 2.f - 1.f) * uHalf;
    const float rv = (rng.nextF() * 2.f - 1.f) * vHalf;
    const vec3f origin = v_add(originBase, v_add(v_mul(u, v_splats(ru)), v_mul(v, v_splats(rv))));

    // Ray-AABB test (infinite ray). Mirrors the BLAS root box; if this misses, the BLAS
    // can't be hit either, so we exclude the ray from both numerator and denominator.
    const vec3f t0 = v_mul(v_sub(bmin, origin), dirInv);
    const vec3f t1 = v_mul(v_sub(bmax, origin), dirInv);
    if (!RayIntersectsBoxInf(t0, t1))
      continue;
    aabb_hits++;

    RayData rd;
    rd.rayOrigin = origin;
    rd.rayDir = d;
    rd.rayDirInv = dirInv;
    rd.data = blas_data;
    rd.t = 1e9f; // shadow ray: unbounded along ray; any-hit cb breaks on first triangle
    if (BLASTraverse<>::rayBLAS(rd, tree_start, tree_bytes, AnyHitCb{}))
      blas_hits++;
  }
}

} // namespace

float computeBlasBoxResemblanceMCAny(const uint8_t *blas_data, int tree_start, int tree_bytes, int direction_samples,
  int rays_per_direction, uint32_t rng_seed)
{
  if (!blas_data || tree_bytes <= 0 || direction_samples <= 0 || rays_per_direction <= 0)
    return 0.f;

  Rng rng(rng_seed);
  int aabbHits = 0, blasHits = 0;
  for (int i = 0; i < direction_samples; i++)
  {
    const vec3f d = randomUnitDir(rng);
    traceDir(blas_data, tree_start, tree_bytes, d, rays_per_direction, rng, aabbHits, blasHits);
  }
  return (aabbHits > 0) ? float(blasHits) / float(aabbHits) : 0.f;
}

// Rodrigues rotation: v rotated around unit axis k by angle whose cos/sin are precomputed.
//   v_rot = v*c + (k x v)*s + k*(k . v)*(1 - c)
static __forceinline vec3f rotateAroundAxis(vec3f v, vec3f k, float c, float s)
{
  const vec3f kxv = v_cross3(k, v);
  const vec3f kdv = v_splat_x(v_dot3_x(k, v));
  const vec3f cV = v_splats(c);
  const vec3f sV = v_splats(s);
  const vec3f oneMinusC = v_splats(1.f - c);
  return v_add(v_add(v_mul(v, cV), v_mul(kxv, sV)), v_mul(k, v_mul(kdv, oneMinusC)));
}

float computeBlasBoxResemblanceMCDir(const uint8_t *blas_data, int tree_start, int tree_bytes, vec3f sun_dir_blas_space,
  vec3f vertical_axis_blas_space, int rotation_samples, int rays_per_rotation, uint32_t rng_seed)
{
  if (!blas_data || tree_bytes <= 0 || rotation_samples <= 0 || rays_per_rotation <= 0)
    return 0.f;

  // Normalize defensively; callers transforming from world-space may pass unnormalized vectors.
  const vec3f sun = v_norm3_safe(sun_dir_blas_space, v_make_vec4f(0, 1, 0, 0));
  const vec3f up = v_norm3_safe(vertical_axis_blas_space, v_make_vec4f(0, 1, 0, 0));

  // Evenly-spaced yaw angles with a single jittered phase offset; the offset avoids
  // axis-aligned bias when rotation_samples is small (e.g. 4 -> samples land on the
  // cardinal yaws without jitter, which is a worst case for stratification).
  Rng rng(rng_seed);
  const float phaseJitter = rng.nextF();
  const float dTheta = 2.f * float(M_PI) / float(rotation_samples);

  // Worst-yaw reduction: a placed instance gets one specific yaw at runtime, and a single
  // bad yaw is a visible artifact even if the average looks fine. So we report the min
  // per-rotation ratio rather than the hit-aggregate (averaged) one. Rotations whose
  // sampling rectangle happens to miss the AABB entirely (aabb_hits == 0) are skipped so
  // a degenerate yaw doesn't fake a zero score.
  float minR = 1.f;
  bool any = false;
  for (int i = 0; i < rotation_samples; i++)
  {
    const float theta = (float(i) + phaseJitter) * dTheta;
    const vec3f d = rotateAroundAxis(sun, up, cosf(theta), sinf(theta));
    int aabbHits = 0, blasHits = 0;
    traceDir(blas_data, tree_start, tree_bytes, d, rays_per_rotation, rng, aabbHits, blasHits);
    if (aabbHits <= 0)
      continue;
    const float r = float(blasHits) / float(aabbHits);
    if (r < minR)
      minR = r;
    any = true;
  }
  return any ? minR : 0.f;
}

} // namespace daSWRT
