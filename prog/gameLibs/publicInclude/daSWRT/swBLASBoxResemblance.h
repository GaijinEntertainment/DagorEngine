//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_bounds3.h>

// Heuristics that estimate how well a built BLAS is approximated by its bounding box
// from a shadow-silhouette point of view. Each function returns a score in [0, 1]:
//   1.0 -> mesh occupies its AABB so well that the box is a perfect shadow proxy
//   0.0 -> mesh is essentially empty inside its AABB
//
// The score is the area-weighted union of leaf-bbox silhouettes projected onto the three
// cardinal planes (XY, XZ, YZ). It is a strict upper bound on the true silhouette
// coverage (leaf boxes inflate around individual triangles), so a high score is necessary
// but not sufficient for "this BLAS is box-like" -- meshes like rocks routinely score
// 0.8+ here while their actual silhouette is ~0.3. For an unbiased estimate use the
// Monte-Carlo functions below.
//
// To map this conservative estimate closer to the true silhouette empirically, the
// typical post-processing is `out = powf(out, 1.5f)` (fitted against MC-yaw on the testGI
// scene). RMSE remains ~0.12 due to irreducible noise -- callers needing better than that
// should use MC instead.
namespace daSWRT
{

// Box encoding used by the BLAS tree's node bboxes.
enum class BlasBoxEncoding : uint8_t
{
  // UINT16 in [0, 65535] box space (RenderSWRT's legacy/CreateGeomVtex layout, and the
  // testMocBLAS-style packed-21-bit layout). For this encoding the per-axis pixel scale
  // is res/65535 and `world_box.bmin` is ignored (only the size is used for area
  // weighting in the reduction).
  Quantized16,
  // FP16 in [-1, 1] AABB-normalized space (RenderSWRT::buildBLAS / BuiltBLAS layout --
  // bvhReencode.cpp emits `fp16 = uint16/32767.5 - 1`, matching the GPU
  // `world_to_blas_encoded` convention). `world_box` is only used for area weighting in
  // this encoding; the pixel mapping is space-invariant (pixel = (val + 1) * res * 0.5).
  Fp16,
};

// Walks the BVH, treats every leaf as a solid box, rasterizes those boxes onto three
// cardinal projection planes (XY, XZ, YZ) into `resolution` x `resolution` bitmaps.
// Per-axis fill ratios are reduced by an AREA-WEIGHTED MEAN: weight for axis `a` is the
// world-space AABB face area perpendicular to `a` (e.g. a thin fence's 10cm side gets
// ~1/40th the weight of its 4x2.67m face, so a low fill on the side barely affects the
// score).
//
// `world_box` is the model AABB in world space. For Fp16 encoding both bmin and size are
// used; for Quantized16 only the size matters (bmin is ignored).
//
// Fast. No triangle decode, no rays. Conservative (upper bound on true silhouette).
float computeBlasBoxResemblanceVoxel(const uint8_t *blas_data, int tree_start, int tree_bytes, bbox3f world_box,
  BlasBoxEncoding encoding, int resolution = 96);

// =============================================================================
// Monte-Carlo CPU ray-trace estimators.
//
// IMPORTANT format constraint: these functions go through BLASTraverse<>::rayBLAS, which
// reads packed-21-bit vertices (8 bytes each) at the start of the BLAS buffer with the
// BVH tree after them. That layout is produced by build_bvh::writeQuadBVH2 with the
// testMocBLAS-style packing -- it is NOT the layout that RenderSWRT::buildBLAS emits
// (FP16-encoded tree + FP32 verts). So these MC functions only work with the offline
// packed-21bit BLAS used by build-time tools (e.g. testMocBLAS), not with a runtime
// BuiltBLAS. Use the voxel function above for the BuiltBLAS path.
// =============================================================================

// 2a) Monte-Carlo with isotropic directions. Generates `direction_samples` random
//     directions on the sphere; for each, shoots `rays_per_direction` rays through
//     the BLAS's AABB and counts how many also hit the BLAS via the existing CPU
//     shadow trace (rayBLAS). Rays that miss the AABB are excluded from both numerator
//     and denominator, so the result is independent of the chosen sampling box.
//
//     Resemblance = blasHits / aabbHits (over all sampled rays).
float computeBlasBoxResemblanceMCAny(const uint8_t *blas_data, int tree_start, int tree_bytes, int direction_samples = 64,
  int rays_per_direction = 256, uint32_t rng_seed = 0x9E3779B9u);

// 2b) Sun-direction resemblance, integrated over rotations of the asset about its
//     vertical axis. Models the common case: the sun is fixed in world space, but
//     placed instances of a single asset (houses, props, etc.) are randomly yawed.
//
//     For each of `rotation_samples` evenly-spaced yaw angles, the sun direction is
//     rotated around `vertical_axis_blas_space` (Rodrigues), and `rays_per_rotation`
//     rays are traced. Per-rotation ratios are computed independently and reduced via
//     min -> the returned resemblance is the WORST-CASE yaw, not the average. A bad
//     yaw produces visible shadow artifacts even if other yaws look fine, so the
//     conservative choice is what drives `dim_as_box_dist`.
//
//     Inputs are in the BLAS's normalized box space. Callers with world-space vectors
//     should pre-multiply componentwise by `65535 / (worldBoxMax - worldBoxMin)` and
//     re-normalize (this is the same anisotropic mapping used everywhere else in this
//     header, and it correctly handles non-cube AABBs).
//
//     `vertical_axis_blas_space` defaults to +Y; pass another axis if the asset's "up"
//     differs. If sun direction is parallel to the vertical axis, all rotations
//     collapse to one direction (degenerate but harmless).
float computeBlasBoxResemblanceMCDir(const uint8_t *blas_data, int tree_start, int tree_bytes, vec3f sun_dir_blas_space,
  vec3f vertical_axis_blas_space, int rotation_samples = 32, int rays_per_rotation = 512, uint32_t rng_seed = 0x9E3779B9u);

} // namespace daSWRT
