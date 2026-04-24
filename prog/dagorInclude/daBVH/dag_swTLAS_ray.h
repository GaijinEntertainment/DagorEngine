//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daBVH/dag_swTLAS.h>
#include <daBVH/dag_swBLAS_ray.h>
#include <math/dag_hlsl_floatx.h>
#include <util/dag_bitwise_cast.h>

static constexpr int TLAS_NODE_SIZE = 16;

// ============================================================================
// Templated TLAS traversal: UseHalves plus BLAS class
// ============================================================================

template <bool UseHalves, class BLAS>
struct TLASTraverse
{
  static inline void decodeNode(const uint8_t *data, int offset, vec3f &bmin, vec3f &bmax, uint32_t &skip)
  {
    vec4i e12 = v_ldui((const int *)(data + offset));
    if constexpr (UseHalves)
    {
      bmin = v_half_to_float(v_andi(e12, v_splatsi(0xFFFF)));
      bmax = v_half_to_float(v_srli(e12, 16));
    }
    else
    {
      bmin = v_cvt_vec4f(v_andi(e12, v_splatsi(0xFFFF)));
      bmax = v_cvt_vec4f(v_srli(e12, 16));
    }
    skip = v_extract_wi(e12);
  }

  static inline const build_bvh::TLASLeaf &decodeLeaf(const uint8_t *data, uint32_t offset)
  {
    return *(const build_bvh::TLASLeaf *)(data + (offset & 0x7FFFFFFF));
  }

  static inline bool decodeLeafTransform(const build_bvh::TLASLeaf &leaf, vec3f &origin, vec3f &scale, mat33f &invTm)
  {
    origin = v_ldu((const float *)leaf.origin);
    scale = v_ldu((const float *)leaf.scale);
    // Negative scale.x signals a reflected transform (det < 0); clear sign bit
    const int reflected = bitwise_cast<uint32_t>(leaf.scale[0]) >> 31;
    scale = v_and(scale, v_cast_vec4f(v_make_vec4i(0x7FFFFFFF, -1, -1, -1)));
    invTm.col0 = v_ldu(leaf.invTm);
    invTm.col1 = v_ldu(leaf.invTm + 3);
    // Identity: col0=(1,0,0), col1=(0,1,0)
    vec4f diff0 = v_sub(invTm.col0, V_C_UNIT_1000);
    vec4f diff1 = v_sub(invTm.col1, V_C_UNIT_0100);
    if (v_check_xyz_all_false(v_or(diff0, diff1)))
    {
      invTm.col2 = reflected ? v_neg(V_C_UNIT_0010) : V_C_UNIT_0010;
      return !reflected;
    }
    invTm.col2 = v_cross3(invTm.col0, invTm.col1);
    if (reflected)
      invTm.col2 = v_neg(invTm.col2);
    return false;
  }

  // 32767.5 factor is baked into leaf origin/scale during TLAS serialization for !UseHalves,
  // so these functions are unconditional.

  static inline vec3f worldToBlas(vec3f worldPos, vec3f origin, vec3f scale, const mat33f &invTm)
  {
    return v_mul(v_mat33_mul_vec3(invTm, v_sub(worldPos, origin)), scale);
  }

  static inline vec3f worldToBlas(vec3f worldPos, vec3f origin, vec3f scale) { return v_mul(v_sub(worldPos, origin), scale); }

  static inline vec3f worldDirToBlas(vec3f worldDir, vec3f scale, const mat33f &invTm)
  {
    return v_mul(v_mat33_mul_vec3(invTm, worldDir), scale);
  }

  static inline vec3f worldDirToBlas(vec3f worldDir, vec3f scale) { return v_mul(worldDir, scale); }

  static inline float blasEncodedYToWorld(float blasY, vec3f origin, vec3f scale)
  {
    return blasY / v_extract_y(scale) + v_extract_y(origin);
  }

  // ---- Generic TLAS tree walk ----
  // NodeTest: (vec3f bmin, vec3f bmax) -> bool
  // LeafCb:   (const build_bvh::TLASLeaf &leaf) -> bool (true = early-out)
  template <class NodeTest, class LeafCb>
  static inline bool iterateFiltered(const uint8_t *tlasData, int startOffset, const NodeTest &nodeTest, const LeafCb &leafCb)
  {
    int dataOffset = startOffset;
    for (;;)
    {
      vec3f bmin, bmax;
      uint32_t offsetToNextNode;
      decodeNode(tlasData, dataOffset, bmin, bmax, offsetToNextNode);
      if (!offsetToNextNode)
        break;
      const bool isLeaf = offsetToNextNode & 0x80000000;
      bool collision = nodeTest(bmin, bmax);
      if (!isLeaf)
      {
        dataOffset += collision ? TLAS_NODE_SIZE : offsetToNextNode;
      }
      else
      {
        if (collision)
          if (leafCb(decodeLeaf(tlasData, offsetToNextNode)))
            return true;
        dataOffset += TLAS_NODE_SIZE;
      }
    }
    return false;
  }

  // ---- TLAS query functions ----

  static inline int firstOffsetToCheckXZ(const uint8_t *tlasData, vec3f check_bmin, vec3f check_bmax, int startOffset = 0)
  {
    int dataOffset = startOffset;
    for (;;)
    {
      vec3f bmin, bmax;
      uint32_t offsetToNextNode;
      decodeNode(tlasData, dataOffset, bmin, bmax, offsetToNextNode);
      if (!offsetToNextNode)
        break;
      const bool isLeaf = offsetToNextNode & 0x80000000;
      if (!v_check_xz_all_true(v_and(v_cmp_ge(check_bmax, bmin), v_cmp_ge(bmax, check_bmin))))
      {
        dataOffset += isLeaf ? TLAS_NODE_SIZE : offsetToNextNode;
      }
      else
      {
        if (!isLeaf && v_check_xz_all_true(v_and(v_cmp_ge(check_bmin, bmin), v_cmp_ge(bmax, check_bmax))))
          dataOffset += TLAS_NODE_SIZE;
        else
          return dataOffset;
      }
    }
    return -1;
  }

  template <class LeafCb>
  static inline void forEachLeafXZ(const uint8_t *tlasData, vec3f check_bmin, vec3f check_bmax, int startOffset, const LeafCb &cb)
  {
    iterateFiltered(
      tlasData, startOffset,
      [check_bmin, check_bmax](vec3f bmin, vec3f bmax) {
        return v_check_xz_all_true(v_and(v_cmp_ge(check_bmax, bmin), v_cmp_ge(bmax, check_bmin)));
      },
      [&](const build_bvh::TLASLeaf &leaf) {
        cb(leaf);
        return false;
      });
  }

  template <class HitCb = BestHitCb>
  static inline bool rayTLAS(RayData &r, const uint8_t *tlasData, const uint8_t *blasData, vec3f tlasScale, vec3f tlasOfs,
    const HitCb &cb = HitCb())
  {
    vec3f tlasRayOrigin, tlasRayDirInv;
    if constexpr (!UseHalves)
    {
      tlasRayOrigin = v_madd(r.rayOrigin, tlasScale, tlasOfs);
      vec3f tlasRayDir = v_mul(r.rayDir, tlasScale);
      tlasRayDirInv = v_rcp(v_sel(v_splats(1e-32f), tlasRayDir, v_cmp_gt(v_abs(tlasRayDir), v_splats(1e-32f))));
    }
    else
    {
      tlasRayOrigin = r.rayOrigin;
      tlasRayDirInv = r.rayDirInv;
    }
    vec3f rayOriginScaled = v_neg(v_mul(tlasRayOrigin, tlasRayDirInv));
    bool found_hit = false;
    typename BLAS::RayDataType localR;
    localR.data = blasData;

    iterateFiltered(
      tlasData, 0,
      [tlasRayDirInv, rayOriginScaled, &r](vec3f bmin, vec3f bmax) {
        return RayIntersectsBoxT0T1(v_madd(bmin, tlasRayDirInv, rayOriginScaled), v_madd(bmax, tlasRayDirInv, rayOriginScaled), r.t);
      },
      [&](const build_bvh::TLASLeaf &leaf) {
        vec3f origin, scale;
        mat33f invTm;
        bool isIdentity = decodeLeafTransform(leaf, origin, scale, invTm);

        localR.rayOrigin = isIdentity ? worldToBlas(r.rayOrigin, origin, scale) : worldToBlas(r.rayOrigin, origin, scale, invTm);
        localR.rayDir = isIdentity ? worldDirToBlas(r.rayDir, scale) : worldDirToBlas(r.rayDir, scale, invTm);
        localR.t = r.t;
        localR.bestTriOffset = 0;
        localR.bCoord = Point2(0, 0);
        localR.calc();

        if (BLAS::rayBLASOOL(localR, leaf.blasStart, leaf.blasSize))
        {
          r.t = localR.t;
          r.bCoord = localR.bCoord;
          r.bestTriOffset = localR.bestTriOffset;
          found_hit = true;
          return cb(r, localR.bestTriOffset);
        }
        return false;
      });
    return found_hit;
  }

  static inline bool distTLAS(DistData &d, const uint8_t *tlasData, const uint8_t *blasData, vec3f tlasScale, vec3f tlasOfs)
  {
    vec3f tlasPos;
    float tlasDist2Scale = 1.f;
    if constexpr (!UseHalves)
    {
      tlasPos = v_madd(d.pos, tlasScale, tlasOfs);
      alignas(16) float tsF[4];
      v_st(tsF, tlasScale);
      float maxTS = max(tsF[0], max(tsF[1], tsF[2]));
      tlasDist2Scale = maxTS * maxTS;
    }
    else
    {
      tlasPos = d.pos;
    }
    DistData localD;
    localD.data = blasData;

    iterateFiltered(
      tlasData, 0,
      [tlasPos, &d, tlasDist2Scale](vec3f bmin, vec3f bmax) {
        return v_extract_x(v_distance_sq_to_bbox_x(bmin, bmax, tlasPos)) < d.bestDist2 * tlasDist2Scale;
      },
      [&](const build_bvh::TLASLeaf &leaf) {
        vec3f origin, scale;
        mat33f invTm;
        bool isIdentity = decodeLeafTransform(leaf, origin, scale, invTm);

        localD.pos = isIdentity ? worldToBlas(d.pos, origin, scale) : worldToBlas(d.pos, origin, scale, invTm);
        alignas(16) float scaleF[4];
        v_st(scaleF, scale);
        float maxScale = max(scaleF[0], max(scaleF[1], scaleF[2]));
        localD.bestDist2 = d.bestDist2 * maxScale * maxScale;
        localD.bestTriOffset = -1;

        if (BLAS::distBLAS(localD, leaf.blasStart, leaf.blasSize))
        {
          d.bestDist2 = localD.bestDist2 / (maxScale * maxScale);
          d.bestTriOffset = localD.bestTriOffset;
        }
        return false;
      });
    return d.bestTriOffset >= 0;
  }

  template <class HitCb>
  static inline void rayTLASXZInf(RayData &r, const uint8_t *tlasData, const uint8_t *blasData, vec3f tlasScale, vec3f tlasOfs,
    const HitCb &cb, int startOffset = 0)
  {
    vec3f tlasRayOrigin;
    if constexpr (!UseHalves)
      tlasRayOrigin = v_madd(r.rayOrigin, tlasScale, tlasOfs);
    else
      tlasRayOrigin = r.rayOrigin;
    typename BLAS::RayDataType localR;
    localR.data = blasData;

    iterateFiltered(
      tlasData, startOffset,
      [tlasRayOrigin](vec3f bmin, vec3f bmax) {
        return v_check_xz_all_true(v_and(v_cmp_ge(tlasRayOrigin, bmin), v_cmp_ge(bmax, tlasRayOrigin)));
      },
      [&](const build_bvh::TLASLeaf &leaf) {
        vec3f origin, scale;
        mat33f invTm;
        bool isIdentity = decodeLeafTransform(leaf, origin, scale, invTm);

        localR.rayOrigin = isIdentity ? worldToBlas(r.rayOrigin, origin, scale) : worldToBlas(r.rayOrigin, origin, scale, invTm);

        BLAS::rayBLASXZInf(localR, leaf.blasStart, leaf.blasSize, [&](typename BLAS::RayDataType &lr, int dataOfs) {
          r.t = blasEncodedYToWorld(lr.t, origin, scale);
          r.bCoord = lr.bCoord;
          r.bestTriOffset = lr.bestTriOffset;
          cb(r, dataOfs);
          return false;
        });
        return false;
      });
  }
};
