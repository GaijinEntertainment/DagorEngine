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
};

#endif
