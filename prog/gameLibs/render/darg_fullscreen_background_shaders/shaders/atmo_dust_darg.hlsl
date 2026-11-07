static const float3 DUST_TINT = float3(1.0, 0.9, 0.74);

#include "atmo_common.hlsl"

float shaftMask(float2 p, float2 shift, float t, float2 dir, float aspect)
{
  float s = dot(p + shift, float2(dir.y, -dir.x));
  float a = dot(p, dir) - min(0.0, aspect * dir.x) - min(0.0, dir.y);
  float n = vnoise(float2(s * 3.5 + t * 0.02, t * 0.035)) * 0.65 + vnoise(float2(s * 9.0 - t * 0.03, 4.0 + t * 0.06)) * 0.35;
  return smoothstep(0.42, 0.82, n) * exp(-a * 0.6);
}

float moteLayer(float2 p, float t, float cells, float radius, float density, float salt, bool bokeh)
{
  float2 q = p * cells + float2(t * 0.006, -t * 0.003) * cells;
  float2 cell = floor(q);
  float4 r = rand4(cell, salt);
  float lifeT = t * (0.03 + 0.05 * r.x) + r.y;
  float life = frac(lifeT);
  float4 c = rand4(cell, salt + 8.0 * floor(lifeT));

  float2 wander = float2(sin(t * (0.21 + 0.3 * c.x) + c.z * 6.2832), sin(t * (0.17 + 0.25 * c.y) + c.z * 4.1));
  float2 center = 0.5 + (c.xy - 0.5) * 0.36 + 0.12 * wander;
  float dist = length(q - cell - center);
  float rc = radius * cells;
  float m = bokeh ? (1.0 - smoothstep(0.8 * rc, rc, dist)) * (0.75 + 0.25 * smoothstep(0.4 * rc, rc, dist)) : exp(-dist * dist / (rc * rc));

  float tw = 0.5 + 0.5 * sin(t * (0.8 + 1.7 * r.z) + r.w * 6.2832);
  float tw2 = tw * tw;
  float twinkle = 0.3 + 0.7 * tw2 * tw2 * tw2;
  float fade = smoothstep(0.0, 0.2, life) * (1.0 - smoothstep(0.6, 1.0, life)) * step(c.w, density);
  return m * fade * (bokeh ? 1.0 : twinkle);
}

float4 darg_ps(float2 uv)
{
  float aspect = darg_resolution.x / darg_resolution.y;
  float2 p = float2(uv.x * aspect, uv.y);
  float3 tint = any(darg_params0.rgb) ? darg_params0.rgb : DUST_TINT;
  float density = knob(darg_params1.x);
  float t = darg_time * knob(darg_params1.y);
  float size = knob(darg_params1.z) / 1080.0;
  float minRadius = 0.7 / darg_resolution.y;
  float2 dir = normalize(any(darg_params3.xy) ? darg_params3.xy : float2(0.45, 1.0));
  float2 shift = seedShift();

  float shaft = shaftMask(p, shift, t, dir, aspect) * knob(darg_params1.w);
  float lit = 0.45 + 3.0 * shaft;
  p += shift;
  float motes = moteLayer(p, t, 20.0, max(1.2 * size, minRadius), 0.35 * density, 1.0, false) * 0.8;
  motes += moteLayer(p, t, 10.0, max(1.8 * size, minRadius), 0.25 * density, 2.0, false) * 1.1;
  float bokeh = moteLayer(p, t * 1.5, 3.5, max(16.0 * size, minRadius), 0.1 * density, 3.0, true) * 0.08;

  float3 col = tint * (motes * lit + bokeh * (0.5 + 2.0 * shaft) + shaft * 0.07);
  return float4(col * knob(darg_params0.a) * layerFade(uv.x), 0.0);
}
