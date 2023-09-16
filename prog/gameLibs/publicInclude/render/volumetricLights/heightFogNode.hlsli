

#ifndef HEIGHT_FOG_NODE_INCLUDED
#define HEIGHT_FOG_NODE_INCLUDED 1

struct HeightFogNode
{
  float3 color;//include absorbtion, i.e can be darker, but should not be brighter than 1
  float3 world_noise_freq;
  float wind_strength;
  float constant_density, perlin_density;
  float perlin_one_over_one_minus_threshold, perlin_scaled_threshold; // 1/(1-perlin_threshold), -perlin_threshold/(1-perlin_threshold)
  float height_fog_scale, height_fog_layer_max;
  float ground_fog_density, ground_fog_offset;
};

#endif