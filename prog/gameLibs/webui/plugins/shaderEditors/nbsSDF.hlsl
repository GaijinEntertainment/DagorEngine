#define world_sdf_to_atlas_decode world_sdf_to_atlas_decode__gradient_offset.xyz

float4 sample_world_sdf_lt(uint clip)
{
  return world_sdf_lt[clip];
}
float4 sample_world_sdf_to_tc_add(uint clip)
{
  return world_sdf_to_tc_add[clip];
}