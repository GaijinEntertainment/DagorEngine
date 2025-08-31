#ifndef GLASS_HAS_ENVI_PROBE_SAMPLING_FUNC
float3 glass_sample_envi_probe(float2 screenpos, float3 world_normal, float3 reflection_vec, float roughness_mip, float3 e2p)
{
  return sample_envi_probe(float4(reflection_vec, roughness_mip)).rgb;
}
#endif