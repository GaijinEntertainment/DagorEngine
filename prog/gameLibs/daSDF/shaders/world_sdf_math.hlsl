uint world_sdf_num_cascades() {return world_sdf_res.w;}
float2 get_decode_world_sdf_distance(uint clip)
{
  return float2(2,-1)*(sample_world_sdf_lt(clip).w*MAX_WORLD_SDF_VOXELS_BAND);
}
float world_sdf_voxel_size(uint clip) {return sample_world_sdf_lt(clip).w;}
float decode_world_sdf_distance(uint clip, float a)
{
  float2 dec = get_decode_world_sdf_distance(clip);
  return a*dec.x + dec.y;
}
float3 world_pos_to_world_sdf_tc(uint clip, float3 worldPos)
{
  return worldPos.xzy*sample_world_sdf_to_tc_add(clip).xyz;
  //float invVoxelSize = world_sdf_to_tc_add[clip].w;
  //return (worldPos.xzy/float3(world_sdf_res.xyz))*invVoxelSize + 0.5;
}
float3 world_sdf_clip_tc_to_atlas_tc_decoded(float2 decode, float3 tc)
{
  float ftcZ = frac(tc.z);
  return float3(tc.xy, ftcZ*decode.x + decode.y);
}
float2 world_sdf_clip_tc_to_atlas_tc_decoder(uint clip)
{
  return float2(world_sdf_to_atlas_decode.x, clip*world_sdf_to_atlas_decode.y + world_sdf_to_atlas_decode.z);
}
float3 world_sdf_clip_tc_to_atlas_tc(uint clip, float3 tc)
{
  return world_sdf_clip_tc_to_atlas_tc_decoded(world_sdf_clip_tc_to_atlas_tc_decoder(clip), tc);
}
bool world_sdf_is_inside_clip(uint i, float3 worldPos, float voxelAround = 0)
{
  float3 b0 = sample_world_sdf_lt(i).xyz + voxelAround*sample_world_sdf_lt(i).w,
          b1 = sample_world_sdf_lt(i).xyz + sample_world_sdf_lt(i).w*float3(world_sdf_res.xyx-voxelAround);
  return all(and(worldPos > b0, worldPos < b1));
}