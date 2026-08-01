//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#ifndef FRUSTUM_CLUSTERS_HLSLI_INCLUDED
#define FRUSTUM_CLUSTERS_HLSLI_INCLUDED 1

#ifndef __cplusplus
  #define FRUSTUM_CLUSTERS_INLINE
  #define FRUSTUM_CLUSTERS_LOG2F log2
  #define FRUSTUM_CLUSTERS_UINT uint
  #define FRUSTUM_CLUSTERS_FLOORF floor
  #define FRUSTUM_CLUSTERS_CEILF ceil
  #define FRUSTUM_CLUSTERS_EXP2F(v) exp2(v)

  uint frustum_clusters_get_cluster_index(uint3 gridCoord)
  {
    return gridCoord.x+gridCoord.y*CLUSTERS_W + gridCoord.z*CLUSTERS_W*CLUSTERS_H;
  };

  uint frustum_clusters_get_cluster_index(half2 tc, uint sliceId)
  {
    uint2 gridCoord = min(uint2(saturate(tc)*half2(CLUSTERS_W, CLUSTERS_H)), uint2(CLUSTERS_W-1, CLUSTERS_H-1));
    return frustum_clusters_get_cluster_index(uint3(gridCoord, sliceId));
  };

#else
  #define FRUSTUM_CLUSTERS_INLINE static inline
  #define FRUSTUM_CLUSTERS_LOG2F log2f
  #define FRUSTUM_CLUSTERS_UINT uint32_t
  #define FRUSTUM_CLUSTERS_FLOORF floorf
  #define FRUSTUM_CLUSTERS_CEILF ceilf
  #define FRUSTUM_CLUSTERS_EXP2F(v) powf(2.0f, v)
#endif

FRUSTUM_CLUSTERS_INLINE float frustum_clusters_safe_log2f(float v) { return v > 1e-5f ? FRUSTUM_CLUSTERS_LOG2F(v) : -1000000.0f; };

FRUSTUM_CLUSTERS_INLINE FRUSTUM_CLUSTERS_UINT frustum_clusters_get_slice_at_depth(float d, float dSliceScale, float dSliceBias)
{
  return (FRUSTUM_CLUSTERS_UINT)max(0, (int)FRUSTUM_CLUSTERS_FLOORF(frustum_clusters_safe_log2f(d) * dSliceScale + dSliceBias));
};

FRUSTUM_CLUSTERS_INLINE FRUSTUM_CLUSTERS_UINT frustum_clusters_get_max_slice_at_depth(float d, float dSliceScale, float dSliceBias)
{
  return (FRUSTUM_CLUSTERS_UINT)max(0, (int)FRUSTUM_CLUSTERS_CEILF(frustum_clusters_safe_log2f(d) * dSliceScale + dSliceBias));
};

FRUSTUM_CLUSTERS_INLINE float frustum_clusters_get_depth_at_slice(FRUSTUM_CLUSTERS_UINT slice, float dSliceScale, float dSliceBias)
{
  return FRUSTUM_CLUSTERS_EXP2F(((float)slice) / dSliceScale - dSliceBias / dSliceScale);
};

#ifndef __cplusplus
  uint frustum_clusters_get_cluster_index(float2 tc, float depth, float slice_scale, float slice_bias)
  {
    uint sliceId = min(frustum_clusters_get_slice_at_depth(depth, slice_scale, slice_bias), CLUSTERS_D);
    return frustum_clusters_get_cluster_index(tc, sliceId);
  };

  uint get_cluster_grid_omni_address(uint cluster_index, uint words_per_omni)
  {
    return cluster_index * words_per_omni;
  };

  // the flat bit grid packs omni words for all clusters first, spot words after them
  uint get_cluster_grid_spot_address(uint cluster_index, uint words_per_spot, uint words_per_omni)
  {
    return cluster_index * words_per_spot + (CLUSTERS_D + 1) * CLUSTERS_H * CLUSTERS_W * words_per_omni;
  };
#endif

#undef FRUSTUM_CLUSTERS_LOG2F
#undef FRUSTUM_CLUSTERS_INLINE
#undef FRUSTUM_CLUSTERS_UINT
#undef FRUSTUM_CLUSTERS_FLOORF
#undef FRUSTUM_CLUSTERS_CEILF
#undef FRUSTUM_CLUSTERS_EXP2F
#endif
