#include <pcg_hash.hlsl>

float4 rand4(float2 cell, float salt)
{
  return float4(pcg4d_hash_32bit(uint4(uint2(int2(cell)), uint(salt), 0x9e3779b9u))) * (1.0 / 4294967296.0);
}

float hash12(float2 p)
{
  uint2 q = uint2(int2(p)) * uint2(1597334673u, 3812015801u);
  uint n = (q.x ^ q.y) * 1597334673u;
  return float(n) * (1.0 / 4294967296.0);
}

float vnoise(float2 p)
{
  float2 i = floor(p);
  float2 f = p - i;
  f = f * f * (3.0 - 2.0 * f);
  return lerp(lerp(hash12(i), hash12(i + float2(1, 0)), f.x), lerp(hash12(i + float2(0, 1)), hash12(i + float2(1, 1)), f.x), f.y);
}

// octaves must be a literal at the call site so the loop unrolls
float fbmOct(float2 p, int octaves)
{
  float v = 0.0;
  float a = 0.5;
  [unroll]
  for (int i = 0; i < octaves; i++)
  {
    v += a * vnoise(p);
    p = mul(float2x2(1.6, -1.2, 1.2, 1.6), p) + 17.1;
    a *= 0.5;
  }
  return v / (1.0 - exp2(-float(octaves))); // weights sum to 1 - 2^-octaves
}

float fbm(float2 p) { return fbmOct(p, 5); }

float2 flow(float2 p, float t, float scale, float rate_a, float rate_b)
{
  return float2(vnoise(p * scale + float2(0.0, t * rate_a)), vnoise(p * scale + float2(5.2, 1.3 - t * rate_b))) - 0.5;
}

float knob(float v)
{
  return v == 0.0 ? 1.0 : max(v, 0.0);
}

float2 seedShift()
{
  return darg_params3.w * float2(1.37, 0.71);
}

float layerFade(float x)
{
  float2 f = darg_params2.xy;
  return darg_opacity * (f.y > f.x ? lerp(1.0, darg_params2.z, smoothstep(f.x, f.y, x)) : 1.0);
}
