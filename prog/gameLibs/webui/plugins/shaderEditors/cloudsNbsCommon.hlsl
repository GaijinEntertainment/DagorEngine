#ifndef CLOUDS_NBS_COMMON_HLSL
#define CLOUDS_NBS_COMMON_HLSL

float nbs_remap(float v, float old_min, float old_max, float new_min, float new_max)
{
  return new_min + saturate((v - old_min) / (old_max - old_min)) * (new_max - new_min);
}

float nbs_remap_shape_coverage(float shape, float coverage)
{
  return (shape - 1 + coverage) / coverage;
}

float nbs_sample_weather(float2 world_xz, out float3 covarage__rain)
{
  float4 encoded_weather = tex2Dlod(clouds_weather_texture, float4(world_xz * inv_weather_size + 0.5, 0, 0));
  covarage__rain = encoded_weather.xyw;
  return encoded_weather.z; // cloud_type
}

float nbs_sample_cloud_shape(float3 world_pos, float scale)
{
  return tex3Dlod(gen_cloud_shape, float4((world_pos * scale).xzy, 0)).x;
}

float nbs_sample_cloud_detail(float3 world_pos, float scale)
{
  return tex3Dlod(gen_cloud_detail, float4((world_pos * scale).xzy, 0)).x;
}

float2 nbs_sample_curl_2d(float3 world_pos, float scale)
{
  float2 uv = world_pos.xz * scale + 0.5 * (scale * weather_size);
  float sliceCoord = world_pos.y * 0.000041 * 16;
  float fIndex = floor(sliceCoord);
  float2 c = lerp(
    tex2Dlod(clouds_curl_2d, float4(uv + fIndex * 0.125, 0, 0)).xy,
    tex2Dlod(clouds_curl_2d, float4(uv + fIndex * 0.125 + 0.125, 0, 0)).xy,
    frac(sliceCoord));
  return c;
}

float2 nbs_sample_types_lut(float height_fraction, float cloud_type)
{
  float type_tc = cloud_type * ((CLOUDS_TYPES_LUT - 1.) / CLOUDS_TYPES_LUT) + 0.5 / CLOUDS_TYPES_LUT;
  return tex2Dlod(clouds_types_lut, float4(height_fraction, type_tc, 0, 0)).xy; // .zw are erosion used in raymarch
}

#endif
