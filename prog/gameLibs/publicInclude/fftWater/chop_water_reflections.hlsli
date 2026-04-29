#ifndef CHOP_WATER_REFLECTIONS_HLSL
#define CHOP_WATER_REFLECTIONS_HLSL

// LSB-packed alpha:
// a8 == 0     : hole / no-data
// a8 != 0     : valid
// shadow      : a8 & 7   (3 bits, 0..7)
// depthQ      : a8 >> 3  (5 bits, 0..31)
// depth01     : depthQ / 31

#define REFL_SHADOW_BITS  3
#define REFL_SHADOW_MAX   7u
#define REFL_SHADOW_MAX_INV 0.1428571
#define REFL_DEPTH_BITS   5
#define REFL_DEPTH_MAX    31u

struct ReflAlphaDecoded
{
  uint a8;
  uint shadow;   // 0..7  (0 = full shadow, 7 = fully lit)
  uint depthQ;   // 0..31
  float depth01; // depthQ / 31
  uint hole;
};

uint ReflAlphaToA8(float a)
{
  return (uint)round(saturate(a) * 255.0);
}

bool PackedReflIsHole(float a)
{
  return a < 0.0019607843;
}

ReflAlphaDecoded DecodeReflAlpha(float a)
{
  ReflAlphaDecoded d;
  d.a8 = ReflAlphaToA8(a);
  d.hole = (d.a8 == 0u);
  d.shadow = d.a8 & REFL_SHADOW_MAX;
  d.depthQ = d.a8 >> REFL_SHADOW_BITS;
  d.depth01 = (float)d.depthQ * (1.0 / (float)REFL_DEPTH_MAX);
  return d;
}

uint PackReflAlphaA8(uint depthQ, uint shadow)
{
  if (depthQ == 0u && shadow == 0u)
    shadow = 1u;
  return (depthQ << REFL_SHADOW_BITS) | (shadow & REFL_SHADOW_MAX);
}

float PackReflAlphaFloat(uint depthQ, uint shadow)
{
  return (float)PackReflAlphaA8(depthQ, shadow) * (1.0 / 255.0);
}

float GetReflectionHitDistMax(float worldDist)
{
  float maxNear = 100.0;
  float maxFar = 3500.0;
  float W0 = 50.0;
  //float W1 = 5000.0;
  //float t = (log2(max(worldDist, W0)) - log2(W0)) / (log2(W1) - log2(W0));
  float t = (log2(max(worldDist, W0)) - 5.643856) * 0.150515; // 1 / 6.643856
  t = saturate(t);
  t = t*t;
  return lerp(maxNear, maxFar, t);
}

#endif // CHOP_WATER_REFLECTIONS_HLSL
