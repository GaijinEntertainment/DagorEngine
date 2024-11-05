#include <sh1.hlsl>
#include <waveUtils.hlsl>

#if DAGI_IRRADIANCE_ONE_WARP && WAVE_INTRINSICS
SH1 dagi_calc_irradiance_shared(uint tid, uint warpSize, float3 rayDir, float3 radiance)
{
  SH1 localSH1 = mul_sh1(sh_basis1(rayDir), radiance.xyz);
  float4 norm = sh_basis1(1)*sh1_l0_l1_coef().xyyy;
  localSH1.R *= norm;
  localSH1.G *= norm;
  localSH1.B *= norm;
  SH1 shLighting;
  shLighting.R.x = WaveAllSum_F32(localSH1.R.x);shLighting.R.y = WaveAllSum_F32(localSH1.R.y);shLighting.R.z = WaveAllSum_F32(localSH1.R.z);shLighting.R.w = WaveAllSum_F32(localSH1.R.w);
  shLighting.G.x = WaveAllSum_F32(localSH1.G.x);shLighting.G.y = WaveAllSum_F32(localSH1.G.y);shLighting.G.z = WaveAllSum_F32(localSH1.G.z);shLighting.G.w = WaveAllSum_F32(localSH1.G.w);
  shLighting.B.x = WaveAllSum_F32(localSH1.B.x);shLighting.B.y = WaveAllSum_F32(localSH1.B.y);shLighting.B.z = WaveAllSum_F32(localSH1.B.z);shLighting.B.w = WaveAllSum_F32(localSH1.B.w);
  return shLighting;
}
#else
groupshared uint shared_radiance_max;
groupshared int4 shared_sh1_r,shared_sh1_g, shared_sh1_b;
SH1 dagi_calc_irradiance_shared(uint tid, uint warpSize, float3 rayDir, float3 radiance)
{
  SH1 localSH1 = mul_sh1(sh_basis1(rayDir), radiance.xyz);
  float4 norm = sh_basis1(1)*sh1_l0_l1_coef().xyyy;
  localSH1.R *= norm;
  localSH1.G *= norm;
  localSH1.B *= norm;
  if (tid == 0)
  {
    shared_radiance_max = 0;
    shared_sh1_r = shared_sh1_g = shared_sh1_b = 0;
  }
  GroupMemoryBarrierWithGroupSync();
  float radianceMax = max3(radiance.x, radiance.y, radiance.z);
  InterlockedMax(shared_radiance_max, asuint(radianceMax));
  GroupMemoryBarrierWithGroupSync();
  radianceMax = asfloat(shared_radiance_max);
  const float MASK = 1<<23;//fixed point. Up to 16x16 radiance will work correctly with that
  float2 sh01Max = max(1e-9f, radianceMax)*(sh_basis1(1).xz*sh1_l0_l1_coef().xy*sh_basis1(1).xz/MASK);
  float2 sh01InvMax = rcp(sh01Max);

  int4 localR = clamp(int4(floor(localSH1.R.xyzw*sh01InvMax.xyyy + 0.5)), -MASK, MASK),
       localG = clamp(int4(floor(localSH1.G.xyzw*sh01InvMax.xyyy + 0.5)), -MASK, MASK),
       localB = clamp(int4(floor(localSH1.B.xyzw*sh01InvMax.xyyy + 0.5)), -MASK, MASK);
  InterlockedAdd(shared_sh1_r.x, localR.x); InterlockedAdd(shared_sh1_r.y, localR.y);InterlockedAdd(shared_sh1_r.z, localR.z);InterlockedAdd(shared_sh1_r.w, localR.w);
  InterlockedAdd(shared_sh1_g.x, localG.x); InterlockedAdd(shared_sh1_g.y, localG.y);InterlockedAdd(shared_sh1_g.z, localG.z);InterlockedAdd(shared_sh1_g.w, localG.w);
  InterlockedAdd(shared_sh1_b.x, localB.x); InterlockedAdd(shared_sh1_b.y, localB.y);InterlockedAdd(shared_sh1_b.z, localB.z);InterlockedAdd(shared_sh1_b.w, localB.w);
  GroupMemoryBarrierWithGroupSync();
  SH1 shLighting;
  shLighting.R = shared_sh1_r.xyzw*sh01Max.xyyy;
  shLighting.G = shared_sh1_g.xyzw*sh01Max.xyyy;
  shLighting.B = shared_sh1_b.xyzw*sh01Max.xyyy;
  return shLighting;
}
#endif