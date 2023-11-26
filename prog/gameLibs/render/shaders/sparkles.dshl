float needs_sparkles = 0;
float sparkles_size = 1;
float sparkles_density = 1;
int sparkles_enable;
interval sparkles_enable: off < 1, on;

macro INIT_SPARKLES_BASE(code)
  if (sparkles_enable==on)
  {
    (code) { sparkles_enable__size__density@f3 = (needs_sparkles, sparkles_size, sparkles_density, 0); }
  }
endmacro

macro INIT_SPARKLES()
  INIT_SPARKLES_BASE(ps)
endmacro

macro USE_SPARKLES_BASE(code)
hlsl(code) {
#include "noise/Value2D.hlsl"

  ##if sparkles_enable==on
  void computeSnowSparkle(float3 world_pos, float3 world_view_pos, float max_alpha, inout float3 world_normal, inout float smoothness)
  {
    if (max_alpha <= 0.0)
      return;
    const int LEVELS_COUNT = 2000;
    const float APPEARS_SPEED = 0.5;
    const float DENSITY = 20.0;
    int MUTUAL_LEVELS = 800 * sparkles_enable__size__density.z;
    float SPARKLES_DEFAULT_SIZE = 0.00012 * sparkles_enable__size__density.y;

    float3 pointToEye = world_view_pos - world_pos;
    float z = length(pointToEye);
    float2 noises = float2(noise_Value2D(floor(world_pos.xz * DENSITY)), noise_Value2D(floor(world_pos.zx * DENSITY)));
    uint pointSparkleLevel = noises.x * LEVELS_COUNT;
    pointSparkleLevel = pointSparkleLevel % (LEVELS_COUNT - MUTUAL_LEVELS);
    float activeLevel = ((dot(sin(world_view_pos * APPEARS_SPEED), float3(1, 1, 1)) + 3) / 6) * LEVELS_COUNT;
    if (activeLevel > LEVELS_COUNT - MUTUAL_LEVELS)
      activeLevel -= LEVELS_COUNT - MUTUAL_LEVELS;
    float alpha = saturate((1 - abs(pointSparkleLevel - activeLevel + 0.5) / MUTUAL_LEVELS));
    float sparkleSize = SPARKLES_DEFAULT_SIZE * z * DENSITY;

    float3 noiseBias = float3(noises.x, noises.x * noises.y, noises.y);
    float3 pos = frac(world_pos * DENSITY) + noiseBias * 2 - 1;
    alpha = saturate(pow2(alpha) / max(z / 5, 1)) * 5;
    float3 newNormal = lerp(world_normal, normalize(normalize(pointToEye) - from_sun_direction.xyz), saturate(alpha));
    alpha *= pow2(1 - saturate(dot(pos, pos) / sparkleSize));
    alpha = min(alpha, max_alpha);

    float3 v = pointToEye / z;
    float dotvn = dot(v, newNormal);
    float3 ma = v - dotvn * newNormal;
    float3 P_x_proj = dot(pos, ma) * ma;
    pos += (abs(dotvn) - 1.0) * P_x_proj / dot(ma, ma);

    if (dot(-from_sun_direction.xyz, world_normal) < 0.2)
      alpha = 0;

    if (alpha > 0)
    {
      world_normal = newNormal;
      smoothness += alpha;
    }
  }
  ##else
  void computeSnowSparkle(float3 world_pos, float3 world_view_pos, float max_alpha, inout float3 world_normal, inout float smoothness){}
  ##endif
}
endmacro

macro USE_SPARKLES()
  USE_SPARKLES_BASE(ps)
endmacro