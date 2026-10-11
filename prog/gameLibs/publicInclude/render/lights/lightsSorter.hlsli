#ifndef LIGHTS_SORTER_INCLUDED
#define LIGHTS_SORTER_INCLUDED 1

#include "renderLightsConsts.hlsli"

#if MAX_SCENE_OMNI_LIGHTS > MAX_SCENE_SPOT_LIGHTS
#define MAX_SCENE_SORT_LIGHTS MAX_SCENE_OMNI_LIGHTS
#else
#define MAX_SCENE_SORT_LIGHTS MAX_SCENE_SPOT_LIGHTS
#endif

#define LIGHTS_SORTER_BASIC_GROUP_SIZE 16
#define LIGHTS_SORTER_MEDIUM_GROUP_SIZE 64
#define LIGHTS_SORTER_HIGH_GROUP_SIZE 256

#define SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE 12
#define SORT_LIGHTS_DISPATCH_ARGS_BASIC_OFFSET (0 * SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE)
#define SORT_LIGHTS_DISPATCH_ARGS_MEDIUM_OFFSET (1 * SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE)
#define SORT_LIGHTS_DISPATCH_ARGS_SCALAR_HIGH_OFFSET (2 * SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE)
#define SORT_LIGHTS_DISPATCH_ARGS_BATCHED_HIGH_OFFSET (3 * SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE)
#define SORT_LIGHTS_DISPATCH_ARGS_WAVE_OFFSET (4 * SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE)

#define SORT_LIGHTS_DISPATCH_ARGS_FINALIZE_OFFSET (5 * SORT_LIGHTS_DISPATCH_ARGS_RECORD_SIZE)
#define SORT_LIGHTS_DISPATCH_ARGS_RECORDS_COUNT 6

// per tier, the index of the first group that belongs to spot lights (omni groups are dispatched first)
#define LIGHTS_SORTER_BASIC_STAGE_DATA_INDEX 0
#define LIGHTS_SORTER_MEDIUM_STAGE_DATA_INDEX 1
#define LIGHTS_SORTER_SCALAR_HIGH_STAGE_DATA_INDEX 2
#define LIGHTS_SORTER_BATCHED_HIGH_STAGE_DATA_INDEX 3
#define LIGHTS_SORTER_WAVE_STAGE_DATA_INDEX 4
#define LIGHTS_SORTER_STAGE_DATA_RECORDS_COUNT 5

#ifdef __cplusplus
  #define LIGHTS_SORTER_UINT uint32_t
  #define LIGHTS_SORTER_FLOAT_TO_HALF(v) static_cast<uint32_t>(float_to_half(v))
#else
  #define LIGHTS_SORTER_UINT uint
  #define LIGHTS_SORTER_FLOAT_TO_HALF(v) f32tof16(v)
#endif

inline float get_relative_sort_distance(float light_to_camera_squared_distance, float partition_lights_inverse_zfar)
{
  return sqrt(light_to_camera_squared_distance) * partition_lights_inverse_zfar;
}

inline LIGHTS_SORTER_UINT encode_sort_data(LIGHTS_SORTER_UINT scene_index, float dist_over_zfar)
{
  return scene_index | (LIGHTS_SORTER_FLOAT_TO_HALF(dist_over_zfar) << 16);
}

#ifndef __cplusplus

inline LIGHTS_SORTER_UINT decode_sort_data_scene_index(LIGHTS_SORTER_UINT data) { return data & 0xFFFFu; }

inline uint next_power_of_two(uint x)
{
  return x <= 1 ? 1u : 1u << (firstbithigh(x - 1) + 1);
}

inline LIGHTS_SORTER_UINT get_default_sort_data()
{
  return encode_sort_data(0xFFFF, asfloat(0x7F7FFFFF));
}

#endif

#undef LIGHTS_SORTER_FLOAT_TO_HALF
#undef LIGHTS_SORTER_UINT

#endif
