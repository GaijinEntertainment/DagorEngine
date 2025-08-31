#ifndef FOG_COMMON_INCLUDED
#define FOG_COMMON_INCLUDED 1

#define prev_volfog_shadow_samplerstate  land_heightmap_tex_samplerstate
#define volfog_mask_tex_samplerstate  land_heightmap_tex_samplerstate


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

float sample_volfog_mask(float2 world_xz)
{
  float2 tc = world_xz * world_to_volfog_mask.xy + world_to_volfog_mask.zw;
  return tex2Dlod(volfog_mask_tex, float4(tc, 0, 0)).x;
}

float2 getPrevTc(float3 world_pos, out float3 prev_clipSpace)
{
  // TODO: very ugly and subpotimal
  float4x4 prev_globtm_psf;
  prev_globtm_psf._m00_m10_m20_m30 = prev_globtm_psf_0;
  prev_globtm_psf._m01_m11_m21_m31 = prev_globtm_psf_1;
  prev_globtm_psf._m02_m12_m22_m32 = prev_globtm_psf_2;
  prev_globtm_psf._m03_m13_m23_m33 = prev_globtm_psf_3;
  float4 lastClipSpacePos = mul(float4(world_pos, 1), prev_globtm_psf);
  prev_clipSpace = lastClipSpacePos.w < 0.0001 ? float3(2,2,2) : lastClipSpacePos.xyz/lastClipSpacePos.w;
  return prev_clipSpace.xy*float2(0.5,-0.5) + float2(0.5,0.5);
}
float2 getPrevTc(float3 world_pos)
{
  float3 prevClip;
  return getPrevTc(world_pos, prevClip);
}

// TODO: deprecate it (remove from levels first)
int4 getBiomeIndices(float3 world_pos)
{
  return getBiomeData(world_pos).indices;
}

// TODO: deprecate it (remove from levels first)
float getBiomeInfluence(float3 world_pos, int index)
{
  return calcBiomeInfluence(getBiomeData(world_pos), index);
}

#endif