#ifndef FOG_COMMON_INCLUDED
#define FOG_COMMON_INCLUDED 1



float depth_to_volume_pos(float v)
{
  float invRange = volfog_froxel_range_params.y;
  return depth_to_volume_pos(v, invRange);
}
float volume_pos_to_depth(float v)
{
  float range = volfog_froxel_range_params.x;
  return volume_pos_to_depth(v, range);
}

// TODO: instead of this, a use a header for fog vs GPU obj and define HEIGHTMAP_LOD
float get_ground_height(float3 world_pos)
{
  return get_ground_height_lod(world_pos, HEIGHTMAP_LOD);
}


float3 getDepthAboveTcAndVignette(float3 world_pos)
{
  float2 tc = saturate(world_to_depth_ao.xy * world_pos.xz + world_to_depth_ao.zw);
  float2 vignette = saturate(abs(tc * 2 - 1) * 10 - 9);
  float vignette_effect = saturate(dot(vignette, vignette));
  return float3(tc - depth_ao_heights.zw, vignette_effect);
}


float4 height_fog_node(HeightFogNode node, float3 world_pos, float screenTc_z, float3 wind_dir, float effect)
{
  float groundHt = world_pos.y - get_ground_height(world_pos);
  float2 coeffs;
  BRANCH if (!calc_height_fog_coeffs(node, world_pos, groundHt, coeffs))
    return 0;

  float windTime = global_time_phase;
  float3 perlin_pos = (world_pos - wind_dir * (windTime * node.wind_strength)) * node.world_noise_freq;
  float raw_perlin_noise = get_smooth_noise3d(perlin_pos);
  return calc_height_fog_base(node, world_pos, groundHt, raw_perlin_noise, effect, coeffs);
}

HeightFogNode make_height_fog_node(float3 color,
  float3 world_noise_freq, float wind_strength,
  float constant_density, float perlin_density,
  float height_fog_scale, float height_fog_layer_max,
  float ground_fog_density, float ground_fog_offset,
  float perlin_threshold)
{
  HeightFogNode hfn = (HeightFogNode)0;
  hfn.color = color,
  hfn.world_noise_freq = world_noise_freq,
  hfn.wind_strength = wind_strength,
  hfn.constant_density = constant_density,
  hfn.perlin_density = perlin_density,
  hfn.height_fog_scale = height_fog_scale,
  hfn.height_fog_layer_max = height_fog_layer_max,
  hfn.ground_fog_density = ground_fog_density,
  hfn.ground_fog_offset = ground_fog_offset;

  hfn.perlin_one_over_one_minus_threshold = 1 / (1 - perlin_threshold);
  hfn.perlin_scaled_threshold = - perlin_threshold * hfn.perlin_one_over_one_minus_threshold;
  return hfn;
}


#endif