#ifndef CLOUDS_RAIN_MAP_INCLUDED
#define CLOUDS_RAIN_MAP_INCLUDED 1

float2 getRainMapUv_impl(float3 pos)
{
  float2 offset = clouds_rain_map_offset_scale.xy;
  float scale = clouds_rain_map_offset_scale.z;
  return pos.xz * scale + offset;
}

float getRainCoverage_impl(float3 pos, float density_lim)
{

  if (clouds_rain_map_valid)
  {
    float2 heightDensity = tex2Dlod(clouds_rain_map_tex, float4(getRainMapUv_impl(pos), 0, 0)).xy;
    return (heightDensity.y > density_lim && pos.y <= clouds_rain_map_max_height - heightDensity.x) ? heightDensity.y - density_lim : 0.f;
  }
  return 0.f;
}

float getAccurateCloudStart_impl(float3 pos)
{
  if (clouds_rain_map_valid)
    return clouds_rain_map_max_height - tex2Dlod(clouds_rain_map_tex, float4(getRainMapUv_impl(pos), 0, 0)).x;

  return nbs_clouds_start_altitude2_meters;
}

#endif