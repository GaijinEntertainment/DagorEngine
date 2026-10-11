#ifndef BVH_SKINNED_INSTANCE_DATA_INCLDUED
#define BVH_SKINNED_INSTANCE_DATA_INCLDUED 1

struct BvhSkinnedInstanceData
{
  float4x4 inv_wtm;
  uint target_offset;
  uint source_offset;
  int start_vertex;
  int vertex_stride;
  int vertex_count;
  int processed_vertex_stride;
  int position_offset;
  int skin_indices_offset;
  int skin_weights_offset;
  int color_offset;
  int normal_offset;
  int texcoord_offset;
  int texcoord_size;
  uint instance_offset;
  uint source_slot;
  uint pos_format_half;
  float4 cloth_wind__noise_time_scale;
  uint cloth_noise_combined_tex_slot;
  float cloth_wind__noise_amp;
  float cloth_wind__ambient_influence;
  uint morph_atlas_tex_slot;
  // WT dynmodel path: bound pack transform and the two dwords that the
  // non-batched path passes as immediate consts (node chunk offset|size, instance chunk offset).
  float4 pos_mul;
  float4 pos_ofs;
  uint node_data_dword;
  uint instance_data_dword;
  uint morph_uv_offset;
  uint morph_uv_size;
  // constraint: the Vulkan StructuredBuffer stride of this type needs a size that is a multiple of 16
  uint cor_offset;
  uint padding0, padding1, padding2;
};

#endif
