#ifndef USE_SPARKLES_3D_NOISE
  #error USE_SPARKLES_3D_NOISE needs to be set to either 0 or 1
#endif

void computeSnowSparkle_impl(float3 world_pos, float3 world_view_pos, float max_alpha, float size, float density, float3 sparkles_albedo, float sparkles_reflectance,
  inout float3 world_normal, inout float3 albedo, inout float smoothness, inout float reflectance, float sparkles_smoothness = 1)
{
  if (max_alpha <= 0.0)
    return;
  const int LEVELS_COUNT = 2000;
  const float APPEARS_SPEED = 0.5;
  const float DENSITY = 20.0;
  int MUTUAL_LEVELS = 800 * density;
  float SPARKLES_DEFAULT_SIZE = 0.00012 * size;

  float3 pointToEye = world_view_pos - world_pos;
  float z = length(pointToEye);
  #if USE_SPARKLES_3D_NOISE
    float2 noises = float2(noise_Value3D(floor(world_pos.xyz * DENSITY)), noise_Value3D(floor(world_pos.yzx * DENSITY)));
  #else
    float2 noises = float2(noise_Value2D(floor(world_pos.xz * DENSITY)), noise_Value2D(floor(world_pos.zx * DENSITY)));
  #endif
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
    smoothness += alpha * sparkles_smoothness;
    albedo = lerp(albedo, sparkles_albedo, alpha);
    reflectance = lerp(reflectance, sparkles_reflectance, alpha);
  }
}