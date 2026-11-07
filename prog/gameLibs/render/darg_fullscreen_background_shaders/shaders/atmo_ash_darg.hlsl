static const float3 ASH_TINT = float3(0.45, 0.43, 0.41);

#include "atmo_common.hlsl"

float4 flakeLayer(float2 p, float t, float cells, float fall, float radius, float density, float salt, float3 tint)
{
  float2 q = p * cells - float2(0.0, t * fall * cells);
  float2 cell = floor(q);
  float4 r = rand4(cell, salt);
  float lifeT = t * (0.05 + 0.06 * r.x) + r.y;
  float life = frac(lifeT);
  float4 c = rand4(cell, salt + 8.0 * floor(lifeT));

  float phase = c.z * 6.2832;
  float swayRate = 0.4 + 0.8 * c.x;
  float2 center = float2(0.5 + (c.x - 0.5) * 0.36 + 0.12 * sin(t * swayRate + phase), 0.35 + 0.3 * c.y);
  float2 d = q - cell - center;
  float rc = radius * cells;
  // cutoff: the gaussian is at most exp(-12) beyond it
  if (dot(d, d) > 12.0 * rc * rc)
    return float4(0, 0, 0, 0);

  float spin = t * (0.5 + 1.5 * c.y) + phase;
  float face = abs(cos(spin));
  float s, co;
  sincos(r.z * 6.2832 + t * (r.w - 0.5), s, co);
  float2 e = float2(co * d.x - s * d.y, (s * d.x + co * d.y) / (0.2 + 0.8 * face));
  float m = exp(-dot(e, e) / (rc * rc));

  float fade = smoothstep(0.0, 0.15, life) * (1.0 - smoothstep(0.6, 1.0, life)) * step(c.w, density);
  float a = m * fade;
  float face2 = face * face;
  float face4 = face2 * face2;
  float glint = a * face4 * face4 * face4;
  return float4(tint * a + glint * 0.3, a);
}

float4 darg_ps(float2 uv)
{
  float aspect = darg_resolution.x / darg_resolution.y;
  float2 p = float2(uv.x * aspect, uv.y) + seedShift();
  float3 tint = any(darg_params0.rgb) ? darg_params0.rgb : ASH_TINT;
  float density = knob(darg_params1.x);
  float t = darg_time * knob(darg_params1.y);
  float size = knob(darg_params1.z) / 1080.0;
  float minRadius = 0.7 / darg_resolution.y;
  float wind = darg_params3.x != 0.0 ? darg_params3.x : 0.008;

  float2 wp = p + flow(p, t, 1.2, 0.05, 0.04) * 0.25;
  float4 acc = flakeLayer(wp - float2(t * wind * 0.6, 0.0), t, 14.0, 0.025, max(2.0 * size, minRadius), 0.45 * density, 1.0, tint) * 0.7;
  acc += flakeLayer(wp - float2(t * wind, 0.0), t, 7.0, 0.04, max(3.2 * size, minRadius), 0.35 * density, 2.0, tint) * 0.9;
  acc += flakeLayer(wp - float2(t * wind * 1.8, 0.0), t, 3.0, 0.07, max(9.0 * size, minRadius), 0.12 * density, 3.0, tint) * 0.45;

  return saturate(acc) * knob(darg_params0.a) * layerFade(uv.x);
}
