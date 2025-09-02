//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include "frustumClipRegion.h"
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include "clustered_constants.hlsli"


class Occlusion;
struct FrustumClusters
{
  static inline float getDepthAtSlice(uint32_t slice, float depthSliceScale, float depthSliceBias)
  {
    // slice = log2f(depth) * depthSliceScale + depthSliceBias
    return powf(2.0f, slice / depthSliceScale - depthSliceBias / depthSliceScale);
    // return (powf( 2.0f, mExponentK * uSlice ) + minDist);
  }
  //-----------------------------------------------------------------------------------
  static inline float safe_log2f(float v) { return v > 1e-5f ? log2f(v) : -1000000.f; }
  static inline uint32_t getSliceAtDepth(float depth, float depthSliceScale, float depthSliceBias)
  {
    return max(int(floorf(safe_log2f(depth) * depthSliceScale + depthSliceBias)), int(0));
    // return uint32_t(floorf( log2f( max( depth - minDist, 1.f ) ) * mInvExponentK ) );
  }
  static inline uint32_t getMaxSliceAtDepth(float depth, float depthSliceScale, float depthSliceBias)
  {
    return max(int(ceilf(safe_log2f(depth) * depthSliceScale + depthSliceBias)), int(0));
    // return uint32_t(floorf( log2f( max( depth - minDist, 1.f ) ) * mInvExponentK ) );
  }
  static inline uint32_t getVecMaxSliceAtDepth(vec4f depth, vec4f depthSliceScaleBias, vec4f &maxRowSlice)
  {
    vec4f maxSlices = v_ceil(v_add(v_mul(v_log2_est_p3(depth), v_splat_x(depthSliceScaleBias)), v_splat_y(depthSliceScaleBias)));
    maxSlices = v_max(maxSlices, v_zero());
    maxSlices = v_min(maxSlices, v_splats(255.f));
    maxRowSlice = v_max(maxSlices, maxRowSlice);
    vec4i maxSlicesi = v_cvt_ceili(maxSlices);
#if _TARGET_SIMD_SSE
    vec4i result12 = _mm_packs_epi32(maxSlicesi, _mm_setzero_si128());
    return v_extract_xi(_mm_packus_epi16(result12, _mm_setzero_si128()));
#else
    DECL_ALIGN16(int, ret[4]);
    v_sti(ret, maxSlicesi);
    return ret[0] | (ret[1] << 8) | (ret[2] << 16) | (ret[3] << 24);
#endif
  }

  carray<float, CLUSTERS_D + 1> sliceDists;                                                  //-V730_NOINIT
  carray<Point3_vec4, CLUSTERS_W + 1> x_planes;                                              //-V730_NOINIT
  carray<Point3_vec4, CLUSTERS_H + 1> y_planes;                                              //-V730_NOINIT
  carray<Point2, CLUSTERS_W + 1> x_planes2;                                                  //-V730_NOINIT
  carray<Point2, CLUSTERS_H + 1> y_planes2;                                                  //-V730_NOINIT
  carray<Point3_vec4, (CLUSTERS_W + 1) * (CLUSTERS_H + 1) * (CLUSTERS_D + 1)> frustumPoints; //-V730_NOINIT
  carray<uint8_t, CLUSTERS_H> sliceNoRowMax;                                                 //-V730_NOINIT
  carray<uint8_t, CLUSTERS_W * CLUSTERS_H> maxSlicesNo;                                      //-V730_NOINIT

#define HAS_FROXEL_SPHERES 0
#if HAS_FROXEL_SPHERES
  carray<Point3_vec4, (CLUSTERS_W) * (CLUSTERS_H) * (CLUSTERS_D)> frustumSpheres; //-V730_NOINIT
#endif

  struct ItemRect3D
  {
    FrustumScreenRect rect; //-V730_NOINIT
    uint8_t zmin, zmax;     //-V730_NOINIT
    uint16_t itemId;        //-V730_NOINIT
    ItemRect3D() {}
    ItemRect3D(const FrustumScreenRect &r, uint8_t zmn, uint8_t zmx, uint16_t l) : rect(r), zmin(zmn), zmax(zmx), itemId(l) {}
  };

  void prepareFrustum(mat44f_cref view_, mat44f_cref proj_, float zn, float minDist, float maxDist, Occlusion *occlusion);

  typedef uint32_t MaskType;
  struct ClusterGridItemMasks
  {
    static constexpr int MAX_ITEM_COUNT = 256;                             // can be replaced with dynamic
    carray<MaskType, MAX_ITEM_COUNT * CLUSTERS_H * CLUSTERS_D> sliceMasks; //-V730_NOINIT
    carray<uint16_t, MAX_ITEM_COUNT> sliceMasksStart;                      //-V730_NOINIT

    StaticTab<ItemRect3D, MAX_ITEM_COUNT> rects3d;

    uint32_t itemsListCount = 0;
    void reset()
    {
      itemsListCount = 0;
      rects3d.clear();
    }
  };

  uint32_t getSpheresClipSpaceRects(const vec4f *pos_radius, int aligned_stride, int count,
    StaticTab<ItemRect3D, ClusterGridItemMasks::MAX_ITEM_COUNT> &rects3d,
    StaticTab<vec4f, ClusterGridItemMasks::MAX_ITEM_COUNT> &spheresViewSpace);


  uint32_t fillItemsSpheresGrid(ClusterGridItemMasks &items,
    const StaticTab<vec4f, ClusterGridItemMasks::MAX_ITEM_COUNT> &lightsViewSpace, uint32_t *result_mask, uint32_t word_count);


  uint32_t fillItemsSpheres(const vec4f *pos_radius, int aligned_stride, int count, ClusterGridItemMasks &items, uint32_t *result_mask,
    uint32_t word_count);


  uint32_t cullFrustum(ClusterGridItemMasks &items, int i, mat44f_cref plane03_XYZW, mat44f_cref plane47_XYZW, uint32_t *result_mask,
    uint32_t word_count);
  uint32_t cullSpot(ClusterGridItemMasks &items, int i, vec4f pos_radius, vec4f dir_angle, uint32_t *result_mask, uint32_t word_count);


  uint32_t cullSpots(const vec4f *pos_radius, int pos_aligned_stride, const vec4f *dir_angle, int dir_aligned_stride,
    ClusterGridItemMasks &items, uint32_t *result_mask, uint32_t word_count);


  float depthSliceScale = 1, depthSliceBias = 0, minSliceDist = 0, maxSliceDist = 1, znear = 0.01;

  mat44f view, proj; //-V730_NOINIT
};
