#ifndef BVH_TREE_INSTANCE_DATA_INCLDUED
#define BVH_TREE_INSTANCE_DATA_INCLDUED 1

struct BvhTreeInstanceData
{
  float4x4 wtm;
  float4x4 packed_itm;
  float4 wind_per_level_angle_rot_max;
  float4 wind_channel_strength;
  float wind_noise_speed_base;
  float wind_noise_speed_level_mul;
  float wind_angle_rot_base;
  float wind_angle_rot_level_mul;
  float wind_parent_contrib;
  float wind_motion_damp_base;
  float wind_motion_damp_level_mul;
  float AnimWindScale;
  uint apply_tree_wind;
  uint target_offset;
  uint start_vertex;
  uint vertex_stride;
  uint vertex_count;
  uint processed_vertex_stride;
  uint position_offset;
  uint color_offset;
  uint normal_offset;
  uint texcoord_offset;
  uint texcoord_size;
  uint indirect_texcoord_offset;
  uint perInstanceRenderAdditionalData;
  uint pad0;
  uint pad1;
  uint pad2;
};

#endif
