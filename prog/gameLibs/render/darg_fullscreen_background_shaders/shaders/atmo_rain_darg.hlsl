static const float3 RAIN_TINT = float3(0.74, 0.79, 0.86);

#include "atmo_common.hlsl"

float streakLayer(float2 p, float t, float2 cellSize, float speed, float width, float density, float salt)
{
  float2 q = float2(p.x / cellSize.x, (p.y - t * speed) / cellSize.y);
  q.y += hash12(float2(floor(q.x), salt));
  float2 cell = floor(q);
  float2 f = q - cell;
  float4 r = rand4(cell, salt);

  float len = 0.35 + 0.25 * r.z;
  float along = (f.y - r.y * (1.0 - len)) / len;
  float body = smoothstep(0.0, 0.7, along) * (1.0 - smoothstep(0.92, 1.0, along));
  float dx = (f.x - 0.25 - 0.5 * r.x) * cellSize.x;
  return exp(-dx * dx / (width * width)) * body * (0.4 + 0.6 * r.w) * step(frac(r.w * 7.31), density);
}

float lightning(float t, out float x)
{
  float slot = floor(t / 11.0);
  float4 r = rand4(float2(slot, 3.0), 91.0);
  x = r.w;
  float lt = t - slot * 11.0 - r.y * 6.0;
  float f = exp(-max(lt, 0.0) * 10.0) * step(0.0, lt);
  f += 0.7 * exp(-max(lt - 0.22, 0.0) * 8.0) * step(0.22, lt);
  f += 0.4 * exp(-max(lt - 0.5, 0.0) * 12.0) * step(0.5, lt);
  return f * step(r.x, 0.3) * (0.6 + 0.4 * r.z);
}

float4 darg_ps(float2 uv)
{
  float aspect = darg_resolution.x / darg_resolution.y;
  float2 p = float2(uv.x * aspect, uv.y) + seedShift();
  float3 tint = any(darg_params0.rgb) ? darg_params0.rgb : RAIN_TINT;
  float density = knob(darg_params1.x);
  float t = darg_time * knob(darg_params1.y);
  float size = knob(darg_params1.z) / 1080.0;
  float minWidth = 0.6 / darg_resolution.y;
  float slant = darg_params3.x != 0.0 ? darg_params3.x : 0.12;

  float2 sp = float2(p.x - p.y * slant, p.y);
  float rain = streakLayer(sp, t, float2(0.006, 0.09), 0.7, max(0.6 * size, minWidth), 0.35 * density, 1.0) * 0.35;
  rain += streakLayer(sp + 3.7, t, float2(0.012, 0.18), 1.1, max(0.9 * size, minWidth), 0.25 * density, 2.0) * 0.5;
  rain += streakLayer(sp + 7.3, t, float2(0.03, 0.4), 1.8, max(2.2 * size, minWidth), 0.12 * density, 3.0) * 0.25;
  rain *= 0.35 + 0.65 * smoothstep(0.3, 0.8, vnoise(float2(p.x * 1.1 - t * 0.18, p.y * 0.5 + t * 0.03)));

  float flashX;
  float flash = lightning(t, flashX) * knob(darg_params1.w);
  rain *= 1.0 + 4.0 * flash;
  float sky = pow(1.0 - uv.y, 1.5) * (1.0 - 0.5 * abs(uv.x - flashX));

  float k = knob(darg_params0.a) * layerFade(uv.x);
  float3 col = tint * rain * 0.27 + float3(0.85, 0.9, 1.0) * flash * sky * 0.07;
  return float4(col * k, saturate(rain * 0.05) * k);
}
