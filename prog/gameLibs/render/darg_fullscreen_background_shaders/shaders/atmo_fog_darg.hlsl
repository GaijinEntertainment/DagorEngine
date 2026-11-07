static const float3 FOG_TINT = float3(0.74, 0.77, 0.8);

#include "atmo_common.hlsl"

float4 darg_ps(float2 uv)
{
  float aspect = darg_resolution.x / darg_resolution.y;
  float2 p = float2(uv.x * aspect, uv.y) / knob(darg_params1.z) + seedShift();
  float3 tint = any(darg_params0.rgb) ? darg_params0.rgb : FOG_TINT;
  float t = darg_time * knob(darg_params1.y);
  float drift = darg_params3.x != 0.0 ? darg_params3.x : 0.012;
  float top = darg_params3.y != 0.0 ? min(darg_params3.y, 0.85) : 0.45;

  if (uv.y < top - 0.2)
    return float4(0, 0, 0, 0);

  // warp only offsets the domain of far and near, fine octaves are not visible there
  float2 warp = float2(fbmOct(p * 0.7 + float2(t * 0.01, 0.0), 3), fbmOct(p * 0.7 + float2(3.3, 8.1 - t * 0.008), 3));
  float far = fbm(p * float2(1.2, 2.8) + warp * 0.8 - float2(t * drift, 0.0));
  float near = fbm(p * float2(0.7, 1.6) + warp * 0.6 - float2(t * drift * 2.2, 0.0) + 31.7);

  float farBand = smoothstep(top - 0.2, top + 0.25, uv.y);
  float nearBand = smoothstep(top + 0.1, 1.0, uv.y);
  float breath = 0.8 + 0.4 * vnoise(float2(t * 0.04, 2.5));
  float fog = saturate((far - 0.4) * 2.2) * farBand * 0.55 + saturate((near - 0.38) * 2.0) * nearBand * 0.7 + 0.08 * farBand;
  float a = saturate(fog * breath * 0.5 * knob(darg_params1.x) * knob(darg_params0.a)) * layerFade(uv.x);

  float3 col = tint * (0.92 + 0.16 * near);
  return float4(col * a, a);
}
