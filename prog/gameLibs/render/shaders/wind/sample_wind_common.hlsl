#ifndef SAMPLE_WIND_COMMON_INCLUDED
#define SAMPLE_WIND_COMMON_INCLUDED 1


float2 sample_ambient_wind_dir_base(float3 world_pos) {
  float2 tc = world_pos.xz*ambient_wind_map_scale__offset.xy + ambient_wind_map_scale__offset.zw;
  return (2*tex2Dlod(ambient_wind_tex, float4(tc, 0, 0)).xy - 1);
}

float3 sample_ambient_wind_dir(float3 world_pos) {
  float2 dir = sample_ambient_wind_dir_base(world_pos);
  return float3(dir.x, 0, dir.y);
}

float get_ambient_wind_speed()
{
  return ambient_wind__speed__current_time__previous_time.x;
}

#endif