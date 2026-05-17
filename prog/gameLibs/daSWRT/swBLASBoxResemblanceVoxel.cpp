// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBLASBoxResemblance.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_tab.h>
#include <memory/dag_framemem.h>
#include <util/dag_globDef.h>
#include <math/dag_mathBase.h> // clamp
#include <string.h>

//
// Voxelized box-resemblance: walk BVH leaves, rasterize each leaf bbox onto the three
// cardinal projection planes, compute fill ratio. No triangles touched, no rays.
namespace daSWRT
{

float computeBlasBoxResemblanceVoxel(const uint8_t *blas_data, int tree_start, int tree_bytes, bbox3f world_box,
  BlasBoxEncoding encoding, int resolution)
{
  if (!blas_data || tree_bytes <= 0)
    return 0.f;
  const int res = clamp(resolution, 4, 1024);
  const int planePx = res * res;

  // Three planes packed back-to-back. Byte-per-pixel keeps the writer branchless.
  Tab<uint8_t> bmp(framemem_ptr());
  bmp.assign(planePx * 3, uint8_t(0));
  uint8_t *plane[3] = {bmp.data(), bmp.data() + planePx, bmp.data() + 2 * planePx};

  // Per-axis offset/scale mapping leaf bbox values into [0, res] pixel space:
  //   pixel = (val - ofs) * scale
  // Quantized16: values are uint16 in [0, 65535]; pixel = val * (res / 65535).
  // Fp16:        values are fp16 in [-1, 1] (AABB-normalized -- bvhReencode.cpp packs
  //              uint16/32767.5 - 1.0, see also GPU world_to_blas_encoded which divides by
  //              halfExtent after centering). World bbox only feeds the area weighting.
  vec4f ofsV, scaleV;
  if (encoding == BlasBoxEncoding::Quantized16)
  {
    ofsV = v_zero();
    scaleV = v_splats(float(res) / 65535.f);
  }
  else // Fp16
  {
    ofsV = v_splats(-1.f);
    scaleV = v_splats(float(res) * 0.5f);
  }

  // Projection axes:
  //   plane 0 (drop X): u = y, v = z
  //   plane 1 (drop Y): u = x, v = z
  //   plane 2 (drop Z): u = x, v = y
  static const int axU[3] = {1, 0, 0};
  static const int axV[3] = {2, 2, 1};

  constexpr int NODE_SZ = 16;  // = BVH_BLAS_NODE_SIZE
  constexpr int LEAF_TAIL = 4; // leaf is node (16B) + 4B vertex offset, but we never read vertices

  int dataOffset = tree_start;
  const int endOffset = tree_start + tree_bytes;
  while (dataOffset < endOffset)
  {
    // Node layout (matches BLASTraverse::decodeRaw): four uint32s.
    //   u32[0] = bmin.x (low16) | bmax.x (high16)   [bits depend on encoding]
    //   u32[1] = bmin.y (low16) | bmax.y (high16)
    //   u32[2] = bmin.z (low16) | bmax.z (high16)
    //   u32[3] = skip | flags (bit31 = isLeaf)
    const uint32_t skipWord = *(const uint32_t *)(blas_data + dataOffset + 12);
    const bool isLeaf = (skipWord & QUAD_LEAF_FLAG) != 0;
    const int nodeOffset = dataOffset;
    dataOffset += NODE_SZ;
    if (!isLeaf)
      continue; // intermediate nodes are encoded inline in the linear stream; we just walk past them
    dataOffset += LEAF_TAIL;

    // Decode the four lanes at once. Lane w is the skip word's bytes reinterpreted; we
    // ignore it since we only read xyz.
    const vec4i raw = v_ldui((const int *)(blas_data + nodeOffset));
    const vec4i mins16 = v_andi(raw, v_splatsi(0xFFFF));
    const vec4i maxs16 = v_srli(raw, 16);
    vec4f bmin, bmax;
    if (encoding == BlasBoxEncoding::Quantized16)
    {
      bmin = v_cvt_vec4f(mins16);
      bmax = v_cvt_vec4f(maxs16);
    }
    else // Fp16
    {
      bmin = v_half_to_float(mins16);
      bmax = v_half_to_float(maxs16);
    }

    // Map to pixel space and stash into scalar arrays for the rasterizer.
    alignas(16) float bminA[4], bmaxA[4];
    v_st(bminA, v_mul(v_sub(bmin, ofsV), scaleV));
    v_st(bmaxA, v_mul(v_sub(bmax, ofsV), scaleV));

    for (int a = 0; a < 3; a++)
    {
      int u0 = (int)bminA[axU[a]];
      int v0 = (int)bminA[axV[a]];
      int u1 = (int)bmaxA[axU[a]] + 1; // inclusive max -> exclusive end
      int v1 = (int)bmaxA[axV[a]] + 1;
      u0 = max(0, u0);
      v0 = max(0, v0);
      u1 = min(res, u1);
      v1 = min(res, v1);
      if (u0 >= u1 || v0 >= v1)
        continue;

      uint8_t *p = plane[a];
      const int runLen = u1 - u0;
      for (int v = v0; v < v1; v++)
        memset(p + v * res + u0, 1, runLen);
    }
  }

  // Reduction is an AREA-WEIGHTED mean of per-axis fills. Weight for axis `a` is the
  // world-space area of the AABB face perpendicular to `a`:
  //   axis 0 (drop X, project to YZ): area = sizeY * sizeZ
  //   axis 1 (drop Y, project to XZ): area = sizeX * sizeZ
  //   axis 2 (drop Z, project to XY): area = sizeX * sizeY
  //
  // Equivalent to (1 - missArea / totalArea) where missArea is the per-axis "AABB silhouette
  // not covered by the mesh" summed across axes -- the right physical quantity for "how much
  // false shadow does the box approximation produce". A pure min reduction treats a thin
  // fence's 10cm side as equally important as its 4x2.67m face, which gives a misleadingly
  // low score: from any sun direction other than near-grazing along the thin axis, the small
  // face contributes negligibly to the actual shadow.
  alignas(16) float s[4];
  v_st(s, v_sub(world_box.bmax, world_box.bmin));
  float area[3] = {s[1] * s[2], s[0] * s[2], s[0] * s[1]};
  float totalArea = area[0] + area[1] + area[2];
  if (!(totalArea > 0.f)) // includes NaN and zero -- caller did not supply box size; uniform weights
  {
    area[0] = area[1] = area[2] = 1.f;
    totalArea = 3.f;
  }

  const float invPx = 1.f / float(planePx);
  float weighted = 0.f;
  for (int a = 0; a < 3; a++)
  {
    int filled = 0;
    const uint8_t *p = plane[a];
    for (int i = 0; i < planePx; i++)
      filled += p[i];
    weighted += float(filled) * invPx * area[a];
  }
  return weighted / totalArea;
}

} // namespace daSWRT
