float sample_world_sdf_value_atlas_tc(float3 tc)
{
  return tex3Dlod(world_sdf_clipmap, float4(tc, 0)).x;
}
float sample_world_sdf_value_clip_tc(uint clip, float3 tc)
{
  return sample_world_sdf_value_atlas_tc(world_sdf_clip_tc_to_atlas_tc(clip, tc));
}

float sample_world_sdf_value(uint clip, float3 worldPos)
{
  return sample_world_sdf_value_clip_tc(clip, world_pos_to_world_sdf_tc(clip, worldPos));
}

float compute_simple_sdf_occlusion(float3 worldPos, float3 worldNorm, float range)
{
  float r = range * 0.5;
  float3 pos = worldPos + worldNorm * r;

  const uint lastClip = world_sdf_num_cascades() - 1;
  uint clip = 0;

  while (clip <= lastClip)
  {
    if (clip == lastClip || (range <= world_sdf_voxel_size(clip) && world_sdf_is_inside_clip(clip, pos, 1)))
    {
      float v = sample_world_sdf_value(clip, pos);
      if (v < 1)
        return saturate(decode_world_sdf_distance(clip, v) / r - 0.25);
    }
    clip++;
  }

  return 1;
}