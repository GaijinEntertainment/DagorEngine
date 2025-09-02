// rnd_u - random, (R -> [0..1])
DAFX_INLINE
float2 generate_gaussian_noise(float mu, float sigma, float2 rnd_u)
{
  float epsilon = 0.000001;
  float two_pi = 2.0 * PI;

  float u1, u2;
  u1 = rnd_u.x;
  u2 = rnd_u.y;
  u1 = max(u1, epsilon);

  float z0 = sqrt(-2.0 * log(u1)) * cos(two_pi * u2);
  float z1 = sqrt(-2.0 * log(u1)) * sin(two_pi * u2);
  return float2(z0 * sigma + mu, z1 * sigma + mu);
}

// bias - (R -> [0, N])
DAFX_INLINE
float calc_biased_random(float random_value, float bias)
{
  return pow(random_value, 1.0 - bias);
}

DAFX_INLINE
float LinearDistribution_gen(LinearDistribution ld, rnd_seed_ref seed)
{
  return lerp(ld.start, ld.end, dafx_frnd(seed));
}

DAFX_INLINE
float LinearDistribution_lerp(LinearDistribution ld, float k)
{
  return lerp(ld.start, ld.end, k);
}

DAFX_INLINE
float3 CubeDistribution_gen(CubeDistribution cd, rnd_seed_ref seed)
{
  float3 rvec = dafx_srnd_vec3(seed);
  return cd.center + cd.size * 0.5 * rvec;
}

DAFX_INLINE
float3 gen_spherical_dir(float azimuth_min, float azimuth_max, float zenith_min, float zenith_max, rnd_seed_ref seed)
{
  float phiSin, phiCos;
  sincos((azimuth_min + (azimuth_max - azimuth_min) * dafx_frnd(seed)) * 2 * PI - PI, phiSin, phiCos);
  float thetaSin, thetaCos;
  sincos((zenith_min + (zenith_max - zenith_min) * dafx_frnd(seed)) * PI, thetaSin, thetaCos);
  return float3(thetaSin * phiCos, thetaCos, thetaSin * phiSin);
}

DAFX_INLINE
float3 SphereDistribution_gen(SphereDistribution sd, rnd_seed_ref seed)
{
  return sd.center + sd.radius * gen_spherical_dir(0, 1, 0, 1, seed);
}

DAFX_INLINE
float3 SectorDistribution_gen_dir(SectorDistribution sd, rnd_seed_ref seed)
{
  return gen_spherical_dir(0, sd.azimutMax, sd.inclinationMin, sd.inclinationMax, seed);
}

DAFX_INLINE
float3 SectorDistribution_gen_dir_from(SectorDistribution sd, float3 dir, rnd_seed_ref seed)
{
  return sd.center + (sd.radiusMin + (sd.radiusMax - sd.radiusMin) * dafx_frnd(seed)) * dir * sd.scale;
}

DAFX_INLINE
float3 SectorDistribution_gen(SectorDistribution sd, rnd_seed_ref seed)
{
  return SectorDistribution_gen_dir_from(sd, SectorDistribution_gen_dir(sd, seed), seed);
}

DAFX_INLINE
float3 VectorDistribution_gen(VectorDistribution vd, rnd_seed_ref seed)
{
  G_UNREFERENCED( vd );
  G_UNREFERENCED( seed );
  return float3(0, 0, 0);
}

DAFX_INLINE
float3 SafeNormalize(float3 V)
{
  return V / sqrt(max(dot(V, V), 0.01f));
}