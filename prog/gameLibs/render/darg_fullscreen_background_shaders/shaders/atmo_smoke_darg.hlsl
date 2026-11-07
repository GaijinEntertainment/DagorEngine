static const float3 SMOKE_TINT = float3(0.6, 0.57, 0.54);

#include "atmo_common.hlsl"

float4 darg_ps(float2 uv)
{
  float aspect = darg_resolution.x / darg_resolution.y;
  float2 p = float2(uv.x * aspect, uv.y) / knob(darg_params1.z) + seedShift();
  float3 tint = any(darg_params0.rgb) ? darg_params0.rgb : SMOKE_TINT;
  float t = darg_time * knob(darg_params1.y);
  float wind = darg_params3.x != 0.0 ? darg_params3.x : 0.006;

  float patch = smoothstep(0.35, 0.8, vnoise(p * 1.1 + float2(-t * wind * 0.5, t * 0.012) + 9.0));
  if (patch == 0.0)
    return float4(0, 0, 0, 0);

  float2 q = p * float2(1.5, 1.0) + float2(-t * wind, t * 0.025);
  // warp only offsets the domain of n, fine octaves are not visible there
  float2 warp = float2(fbmOct(q * 0.9 + float2(0.0, t * 0.01), 3), fbmOct(q * 0.9 + float2(5.2, 1.3 + t * 0.013), 3));
  float n = fbm(q * 1.7 + warp * 1.6 + float2(0.0, t * 0.02));
  float wisp = 1.0 - abs(2.0 * n - 1.0);
  wisp *= wisp;
  wisp *= wisp;

  float rise = smoothstep(-0.1, 0.9, uv.y);
  float a = saturate(wisp * patch * rise * 0.42 * knob(darg_params1.x) * knob(darg_params0.a)) * layerFade(uv.x);

  float3 col = tint * (0.85 + 0.3 * n);
  return float4(col * a, a);
}
