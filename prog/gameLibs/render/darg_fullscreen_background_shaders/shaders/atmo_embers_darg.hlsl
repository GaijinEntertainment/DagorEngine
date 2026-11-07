static const float3 EMBER_TINT = float3(1.0, 0.42, 0.1);

#include "atmo_common.hlsl"

float3 emberLayer(float2 p, float t, float cells, float rise, float radius, float density, float salt, float3 tint)
{
  float2 q = p * cells + float2(0.0, t * rise * cells);
  float2 cell = floor(q);
  float4 r = rand4(cell, salt);
  float lifeT = t * (0.07 + 0.09 * r.x) + r.y;
  float life = frac(lifeT);
  float4 c = rand4(cell, salt + 8.0 * floor(lifeT));

  float phase = c.z * 6.2832;
  float swayRate = 0.5 + 1.1 * c.x;
  float2 center = float2(0.5 + (c.x - 0.5) * 0.36 + 0.12 * sin(t * swayRate + phase), 0.3 + 0.4 * c.y);
  float2 d = q - cell - center;
  float rc2 = radius * radius * cells * cells;
  // cutoff: 0.1 * halo, the widest term, is at most 0.0011 beyond it
  if (dot(d, d) > 100.0 * rc2)
    return float3(0, 0, 0);

  float2 dir = normalize(float2(0.12 * swayRate * cos(t * swayRate + phase), -rise * cells));
  float along = dot(d, dir);
  float across = dot(d, float2(-dir.y, dir.x));
  float core = exp(-(along * along * 0.16 + across * across) / rc2);
  float halo = exp(-(along * along * 0.45 + across * across) / (rc2 * 10.0));

  float fade = smoothstep(0.0, 0.1, life) * (1.0 - smoothstep(0.4, 1.0, life));
  float flicker = 0.65 + 0.35 * sin(t * (6.0 + 10.0 * c.y) + phase * 5.0);
  float3 hot = lerp(tint, float3(1.0, 0.93, 0.78), 0.6);
  float3 col = lerp(hot, tint * 0.75, saturate(life * 1.5));
  return col * (core + 0.1 * halo) * fade * flicker * step(c.w, density);
}

float firelight(float2 uv, float2 p, float t)
{
  float flick = vnoise(float2(t * 2.1, 0.5)) * 0.6 + vnoise(float2(t * 5.3, 7.5)) * 0.4;
  float spread = vnoise(float2(p.x * 1.5 + t * 0.07, t * 0.11));
  return uv.y * uv.y * uv.y * (0.4 + 0.6 * flick) * (0.5 + spread);
}

float4 darg_ps(float2 uv)
{
  float aspect = darg_resolution.x / darg_resolution.y;
  float2 p = float2(uv.x * aspect, uv.y) + seedShift();
  float3 tint = any(darg_params0.rgb) ? darg_params0.rgb : EMBER_TINT;
  float density = knob(darg_params1.x);
  float t = darg_time * knob(darg_params1.y);
  float size = knob(darg_params1.z) / 1080.0;
  float minRadius = 0.7 / darg_resolution.y;
  float wind = darg_params3.x != 0.0 ? darg_params3.x : 0.012;

  float2 wp = p + flow(p, t, 1.4, 0.06, 0.05) * 0.22;
  float3 col = emberLayer(wp - float2(t * wind * 0.6, 0.0), t, 16.0, 0.035, max(1.1 * size, minRadius), 0.25 * density, 1.0, tint) * 0.7;
  col += emberLayer(wp - float2(t * wind, 0.0), t, 9.0, 0.06, max(1.6 * size, minRadius), 0.25 * density, 2.0, tint) * 1.1;
  col += emberLayer(wp - float2(t * wind * 1.6, 0.0), t, 4.5, 0.1, max(3.5 * size, minRadius), 0.14 * density, 3.0, tint) * 0.55;
  col *= lerp(0.25, 1.0, smoothstep(0.0, 1.0, uv.y));
  col += tint * firelight(uv, p, t) * 0.08 * knob(darg_params1.w);

  return float4(col * knob(darg_params0.a) * layerFade(uv.x), 0.0);
}
