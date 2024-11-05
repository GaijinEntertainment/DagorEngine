#ifndef ATMOSPHERE_FUNCTIONS_HLSLI_INCLUDED
#define ATMOSPHERE_FUNCTIONS_HLSLI_INCLUDED 1
//only parametrization and lut transmittance is used from Eric Bruneton code.
//we can reimplement it ofc
/**
 * Copyright (c) 2017 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Precomputed Atmospheric Scattering
 * Copyright (c) 2008 INRIA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

INLINE Number GetTextureCoordFromUnitRange(Number x, int texture_size)
{
  return 0.5 / Number(texture_size) + x * (1.0 - 1.0 / Number(texture_size));
}

INLINE Number GetUnitRangeFromTextureCoord(Number u, int texture_size)
{
  return saturate(u / (1.0 - 1.0 / Number(texture_size)) - 0.5 / (Number(texture_size) - 1.0));//we saturate just in case, due to numeric instabilities
}

INLINE vec2 GetTransmittanceTextureUvFromRMu(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu)
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  // Distance to top atmosphere_p boundary for a horizontal ray at ground level.
  Length H = sqrt(atmosphere_p.top_radius * atmosphere_p.top_radius -
      atmosphere_p.bottom_radius * atmosphere_p.bottom_radius);
  // Distance to the horizon.
  Length rho =
      SafeSqrt(r * r - atmosphere_p.bottom_radius * atmosphere_p.bottom_radius);
  // Distance to the top atmosphere_p boundary for the ray (r,mu), and its minimum
  // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
  Length d = DistanceToTopAtmosphereBoundary(atmosphere_p, r, mu);
  Length d_min = atmosphere_p.top_radius - r;
  Length d_max = rho + H;
  Number x_mu = (d - d_min) / (d_max - d_min);
  Number x_r = rho / H;
  return vec2(GetTextureCoordFromUnitRange(x_mu, TRANSMITTANCE_TEXTURE_WIDTH),
              GetTextureCoordFromUnitRange(x_r, TRANSMITTANCE_TEXTURE_HEIGHT));
}

/*
<p>and the inverse mapping follows immediately:
*/

INLINE void GetRMuFromTransmittanceTextureUv(IN(AtmosphereParameters) atmosphere_p,
    IN(vec2) uv, OUT(Length) r, OUT(Number) mu)
{
  assert(uv.x >= 0.0 && uv.x <= 1.0);
  assert(uv.y >= 0.0 && uv.y <= 1.0);
  Number x_mu = GetUnitRangeFromTextureCoord(uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
  Number x_r = GetUnitRangeFromTextureCoord(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);
  // Distance to top atmosphere_p boundary for a horizontal ray at ground level.
  Length H = sqrt(atmosphere_p.top_radius * atmosphere_p.top_radius -
      atmosphere_p.bottom_radius * atmosphere_p.bottom_radius);
  // Distance to the horizon, from which we can compute r:
  Length rho = H * x_r;
  // due to float precision, sqrt(0 + x*x) can produce value lower than x, so we need to acknowledge that
  r = rho > 0 ? sqrt(rho * rho + atmosphere_p.bottom_radius * atmosphere_p.bottom_radius) : atmosphere_p.bottom_radius;
  // Distance to the top atmosphere_p boundary for the ray (r,mu), and its minimum
  // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon) -
  // from which we can recover mu:
  Length d_min = atmosphere_p.top_radius - r;
  Length d_max = rho + H;
  Length d = d_min + x_mu * (d_max - d_min);
  mu = d == 0.0 * meter ? Number(1.0) : (H * H - rho * rho - d * d) / (2.0 * r * d);
  mu = ClampCosine(mu);
}

/*
<p>It is now easy to define a fragment shader function to precompute a texel of
the transmittance texture:
*/

DimensionlessSpectrum GetTransmittanceToTopAtmosphereBoundary(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    Length r, Number mu)
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  vec2 uv = GetTransmittanceTextureUvFromRMu(atmosphere_p, r, mu);
  return DimensionlessSpectrumFromTexture(sample_texture(transmittance_texture, uv));
}

DimensionlessSpectrum GetTransmittance(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    Length r, Number mu, Length d, bool ray_r_mu_intersects_ground)
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  assert(d >= 0.0 * meter);

  Length r_d = ClampRadius(atmosphere_p, sqrt(d * d + 2.0 * r * mu * d + r * r));
  Number mu_d = ClampCosine((r * mu + d) / r_d);

  if (ray_r_mu_intersects_ground) {
    return saturate(
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere_p, transmittance_texture, r_d, -mu_d) /
        max(float3(1e-10, 1e-10, 1e-10), GetTransmittanceToTopAtmosphereBoundary(
            atmosphere_p, transmittance_texture, r, -mu)));
  } else {
    return saturate(
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere_p, transmittance_texture, r, mu) /
        max(float3(1e-10, 1e-10, 1e-10), GetTransmittanceToTopAtmosphereBoundary(
            atmosphere_p, transmittance_texture, r_d, mu_d)));
  }
}

DimensionlessSpectrum GetTransmittanceToSun(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    Length r, Number mu_s)
{
  Number sin_theta_h = atmosphere_p.bottom_radius / r;
  Number cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
  return GetTransmittanceToTopAtmosphereBoundary(
          atmosphere_p, transmittance_texture, r, mu_s) *
      smoothstep(-sin_theta_h * atmosphere_p.sun_angular_radius / rad,
                 sin_theta_h * atmosphere_p.sun_angular_radius / rad,
                 mu_s - cos_theta_h);
}

Length DistanceToNearestAtmosphereBoundary(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu, bool ray_r_mu_intersects_ground)
{
  if (ray_r_mu_intersects_ground) {
    return DistanceToBottomAtmosphereBoundary(atmosphere_p, r, mu);
  } else {
    return DistanceToTopAtmosphereBoundary(atmosphere_p, r, mu);
  }
}

INLINE InverseSolidAngle RayleighPhaseFunction(Number nu)
{
  InverseSolidAngle k = 3.0 / (16.0 * PI * sr);
  return k * (1.0 + nu * nu);
}

float2 GetMiePhaseConsts(Number g);

#if 0
INLINE float MiePhaseFunctionDivideByRayleigh(Number g, Number nu)
{
  return MiePhaseFunctionDivideByRayleighOptimized(float4(GetMiePhaseConsts(g)*pow(0.5,-1./1.5), GetMiePhaseConsts(-0.25*g)*pow(0.5,-1./1.5)), nu);
  //float k = 2.0 * (1.0 - g * g) / (2.0 + g * g);//this is constantant
  //float phase = 1.0 + g * g - 2.0 * g * nu;//this is nother two consts (1.0 + g * g) + (-2.0 * g)*nu, each of which can be pre-dvided by pow(k,-1.5)
  //return k / (phase*sqrt(phase));//pow(1.5)
}

//only for reference
//https://arxiv.org/pdf/1812.00799.pdf
//ON SAMPLING OF SCATTERING PHASE FUNCTIONS
//CORNETTE SHANKS PHASE
INLINE InverseSolidAngle MiePhaseCornetteShanksFunction(Number g, Number nu)
{
  InverseSolidAngle k = 3.0 / (8.0 * PI * sr) * (1.0 - g * g) / (2.0 + g * g);
  float phase = 1.0 + g * g - 2.0 * g * nu;
  return k * (1.0 + nu * nu) / (phase*sqrt(phase));//pow(1.5
}
//full Drain phase function
INLINE InverseSolidAngle MiePhaseDrainFunction(Number g, Number nu)
{
  float alpha = 1.0;
  InverseSolidAngle k = 3.0 / (4.0 * PI * sr) * (1.0 - g * g) / (3 + alpha*(1.0 + 2.0*g * g));
  float phase = 1.0 + g * g - 2.0 * g * nu;
  return k * (1.0 + alpha*nu * nu) / (phase*sqrt(phase));//pow(1.5
}
//http://www.csroc.org.tw/journal/JOC25-3/JOC25-3-2.pdf
//Zhang function
INLINE InverseSolidAngle MiePhaseZhangFunction(Number g, Number nu)
{
  InverseSolidAngle k = 3.0 / (8.0 * PI * sr) * (1.0 - g * g) / (2.0 + g * g);
  float phase = 1.0 + g * g - 2.0 * g * nu;
  return k * (1.0 + nu * nu) / (phase*sqrt(phase)) + g*nu*1.0/(4.0*PI * sr);
}

#endif
INLINE float SafePow(Number a, Number b) {return a != 0 ? pow(a, b) : 0;}
INLINE float2 GetMiePhaseConsts(Number g, Number weight)
{
  return float2(1.0 + g * g, - 2.0 * g)*SafePow(weight*2.0 * (1.0 - g * g) / (2.0 + g * g), 1./-1.5);
}

INLINE float2 GetMiePhaseConsts(Number g) { return GetMiePhaseConsts(g, 1); }

INLINE float MiePhaseFunctionDivideByRayleighOptimized(float4 mie_phase_consts, Number nu)
{
  float forward_phase  = mie_phase_consts.x + mie_phase_consts.y*nu;//this is another two consts (1.0 + g * g) + (-2.0 * g)*nu, each of which can be pre-dvided by pow(k,-1.5)
  float backward_phase = mie_phase_consts.z + mie_phase_consts.w*nu;//this is another two consts (1.0 + g * g) + (-2.0 * g)*nu, each of which can be pre-dvided by pow(k,-1.5)
  return pow(forward_phase, -1.5) + pow(backward_phase, -1.5);//it is faster than rcp(phase*sqrt(phase)) or phase*phase*rsqrt(phase) on Xb1+
}


INLINE RadianceSpectrum GetPhasedRadianceOptimized(float4 mie_phase_consts,
  IrradianceSpectrum scattering,
  IrradianceSpectrum single_mie_scattering,
  Number nu)
{
  return (scattering + single_mie_scattering *
      MiePhaseFunctionDivideByRayleighOptimized(mie_phase_consts, nu)) * RayleighPhaseFunction(nu);
}

INLINE RadianceSpectrum GetPhasedRadiance(IN(AtmosphereParameters) atmosphere_p,
  IrradianceSpectrum scattering,
  IrradianceSpectrum single_mie_scattering,
  Number nu)
{
  return GetPhasedRadianceOptimized(atmosphere_p.mie_phase_consts, scattering, single_mie_scattering, nu);
}

IrradianceSpectrum GetIrradiance(
    IN(AtmosphereParameters) atmosphere_p,
    IN(IrradianceTexture) irradiance_texture,
    Length r, Number mu_s);

vec2 GetIrradianceTextureUvFromRMuS(IN(AtmosphereParameters) atmosphere_p,
    Length r, Number mu_s)
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  assert(mu_s >= -1.0 && mu_s <= 1.0);
  Number x_r = (r - atmosphere_p.bottom_radius) /
      (atmosphere_p.top_radius - atmosphere_p.bottom_radius);
  Number x_mu_s = mu_s * 0.5 + 0.5;
  return vec2(GetTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH),
              GetTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
}

/*
<p>The inverse mapping follows immediately:
*/

void GetRMuSFromIrradianceTextureUv(IN(AtmosphereParameters) atmosphere_p,
    IN(vec2) uv, OUT(Length) r, OUT(Number) mu_s)
{
  assert(uv.x >= 0.0 && uv.x <= 1.0);
  assert(uv.y >= 0.0 && uv.y <= 1.0);
  Number x_mu_s = GetUnitRangeFromTextureCoord(uv.x, IRRADIANCE_TEXTURE_WIDTH);
  Number x_r = GetUnitRangeFromTextureCoord(uv.y, IRRADIANCE_TEXTURE_HEIGHT);
  r = atmosphere_p.bottom_radius +
      x_r * (atmosphere_p.top_radius - atmosphere_p.bottom_radius);
  mu_s = ClampCosine(2.0 * x_mu_s - 1.0);
}
/*
<h4 id="irradiance_lookup">Lookup</h4>

<p>Thanks to these precomputed textures, we can now get the ground irradiance
with a single texture lookup:
*/

IrradianceSpectrum GetIrradiance(
    IN(AtmosphereParameters) atmosphere_p,
    IN(IrradianceTexture) irradiance_texture,
    Length r, Number mu_s)
{
  vec2 uv = GetIrradianceTextureUvFromRMuS(atmosphere_p, r, mu_s);
  return IrradianceSpectrumFromTexture(sample_texture(irradiance_texture, uv));
}

//rendering part
DimensionlessSpectrum GetExtrapolatedSingleMieScatteringCoef(
    DimensionlessSpectrum extrapolateMieCoef, IN(vec4) scattering)
{
  // Algebraically this can never be negative, but rounding errors can produce
  // that effect for sufficiently short view rays.
  return (scattering.w / max(scattering.x, 1e-4f)) * extrapolateMieCoef;//betaRMie = (betaR.x / betaR);
}

IrradianceSpectrum GetExtrapolatedSingleMieScattering(
    DimensionlessSpectrum extrapolateMieCoef, IN(vec4) scattering)
{
  return IrradianceSpectrumFromTexture(scattering) * GetExtrapolatedSingleMieScatteringCoef(extrapolateMieCoef, scattering);
}
DimensionlessSpectrum GetExtrapolatedSingleMieScatteringCoefConst(
    IN(AtmosphereParameters) atmosphere_p)
{
  return (atmosphere_p.rayleigh_scattering.x / atmosphere_p.mie_scattering.x) *
    (atmosphere_p.mie_scattering / atmosphere_p.rayleigh_scattering);//constant
}

#ifdef COMBINED_SCATTERING_TEXTURES
IrradianceSpectrum GetExtrapolatedSingleMieScattering(
    IN(AtmosphereParameters) atmosphere_p, IN(vec4) scattering)
{
  return GetExtrapolatedSingleMieScattering(GetExtrapolatedSingleMieScatteringCoefConst(atmosphere_p), scattering);
}

#endif

/*
<p>We can then retrieve all the scattering components (Rayleigh + multiple
scattering on one side, and single Mie scattering on the other side) with the
following function, based on
<a href="#single_scattering_lookup"><code>GetScattering</code></a> (we duplicate
some code here, instead of using two calls to <code>GetScattering</code>, to
make sure that the texture coordinates computation is shared between the lookups
in <code>scattering_texture</code> and
<code>single_mie_scattering_texture</code>):
*/

#define SKIES_PREPARED_SHORT_PART (112./128.)
#define SKIES_PREPARED_SHORT_PART_SQ (SKIES_PREPARED_SHORT_PART*SKIES_PREPARED_SHORT_PART)
float scattering_DistToTc_m(float d, float2 dist_to_prepared_tc) {return sqrt(saturate(d*dist_to_prepared_tc.x + dist_to_prepared_tc.y));}
float scattering_DistToTc_Km(float d, float2 dist_to_prepared_tc) {return scattering_DistToTc_m(d*1000, dist_to_prepared_tc);}//fixme:
float scattering_DistToTc_Long(float d, float4 dist_to_prepared_tc)
{
  float shortDistTc = saturate(d*dist_to_prepared_tc.x + dist_to_prepared_tc.y);
  float longDistTc = saturate(d*dist_to_prepared_tc.z + dist_to_prepared_tc.w);
  return sqrt(shortDistTc < SKIES_PREPARED_SHORT_PART_SQ ? shortDistTc : longDistTc);
}
float scattering_TcToDist_Km(float tcX, float4 dist_to_prepared_tc)
{
  float tc = (tcX*tcX);
  return 0.001*(tc < SKIES_PREPARED_SHORT_PART_SQ ? (tc-dist_to_prepared_tc.y)/dist_to_prepared_tc.x : (tc-dist_to_prepared_tc.w)/dist_to_prepared_tc.z);
}

#define SKIES_LOWER_SPACE_PART 0.8
float scattering_viewZtoTc(float viewZ)
{
  //return acos(viewZ)*(-1./PI) + 1;
  return SKIES_LOWER_SPACE_PART + (viewZ < 0 ? SKIES_LOWER_SPACE_PART : (1-SKIES_LOWER_SPACE_PART))*viewZ;
}
float scattering_tcToViewZ(float tcY)
{
  //return cos(-PI*(tcY-1));
  float2 maddP = tcY<SKIES_LOWER_SPACE_PART ? float2((1./SKIES_LOWER_SPACE_PART), -1) : float2((1/(1-SKIES_LOWER_SPACE_PART)), - (1/(1-SKIES_LOWER_SPACE_PART)-1));
  return tcY*maddP.x + maddP.y;
}

vec2 short_get_prepared_scattering_tc(float mu, float dist, float2 short_dist_to_tc)
{
  return float2(scattering_DistToTc_m(dist, short_dist_to_tc), scattering_viewZtoTc(mu));
}

vec2 long_get_prepared_scattering_tc(float mu, float dist, float4 long_dist_to_tc)
{
  return float2(scattering_DistToTc_Long(dist, long_dist_to_tc), scattering_viewZtoTc(mu));
}

DimensionlessSpectrum GetPreparedTransmittanceUV(
    IN(TransmittanceTexture) prepared_transmittance_texture,
    vec2 uv)
{
  return DimensionlessSpectrumFromTexture(sample_texture(prepared_transmittance_texture, uv));
}

void ComputeMultipleScatteringApprox(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    int SampleCount,
    Length r, IN(Direction) viewDir, IN(Direction) sunDir,
    OUT(IrradianceSpectrum) L, OUT(DimensionlessSpectrum) MultiScatAs1)
{
  Number mu = viewDir.z, nu = dot(viewDir, sunDir), mu_s = sunDir.z;
  // Compute next intersection with atmosphere or ground
  bool rayIntersectGround = RayIntersectsGround(atmosphere_p, r, mu);
  Length tMax = DistanceToNearestAtmosphereBoundary(atmosphere_p, r, mu, rayIntersectGround);
  Length dx = tMax/SampleCount;

  // Ray march the atmosphere to integrate optical depth
  L = IrradianceSpectrum(0.0f,0.0f,0.0f);
  MultiScatAs1 = DimensionlessSpectrum(0,0,0);
  DimensionlessSpectrum throughput = DimensionlessSpectrum(1.0,1.0,1.0);
  Length d = 0.0f;
  for (int i = 0; i <= SampleCount; ++i)
  {
    Length dt = (i == SampleCount || i == 0) ? 0.5*dx : dx;
    d += dt;
    Length r_d = ClampRadius(atmosphere_p, sqrt(d * d + 2.0 * r * mu * d + r * r));
    Number mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);

    DimensionlessSpectrum scattering, extinction;
    SampleMedium(atmosphere_p, r_d-atmosphere_p.bottom_radius, Position(0,0,0), scattering, extinction);
    extinction = max(extinction, scattering);//to be sure we are energy conservative

    DimensionlessSpectrum sampleTransmittance = exp(-extinction * dt);
    DimensionlessSpectrum transmittanceToSun = GetTransmittanceToSun(atmosphere_p, transmittance_texture, r_d, mu_s_d);

    float3 S = transmittanceToSun * scattering;
    // See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
    float3 Sint = (S - S * sampleTransmittance) / extinction;    // integrate along the current step segment
    L += throughput * Sint;                                                     // accumulate and also take into account the transmittance from previous steps

    // When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge.
    // Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
    // The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
    // However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.
    float3 MS = scattering;
    float3 MSint = (MS - MS * sampleTransmittance) / extinction;
    MultiScatAs1 += throughput * MSint;

    throughput = throughput*sampleTransmittance;
  }
  // Phase functions
  const float uniformPhase = 1.0 / (4.0 * PI);
  L*=uniformPhase;

  if (rayIntersectGround)
  {
    // Account for bounced light off the earth
    Length r_d = atmosphere_p.bottom_radius;
    Length d = tMax;
    Number mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);
    float3 transmittanceToSun = GetTransmittanceToSun(
            atmosphere_p, transmittance_texture, r_d, mu_s_d);

    vec3 ground_normal = normalize(float3(0,0,r) + viewDir * d);
    const float NdotL = saturate(dot(ground_normal, sunDir));
    L += transmittanceToSun * throughput * NdotL * atmosphere_p.ground_albedo / PI;
  }
}

#if SHADOWMAP_ENABLED
// we artificially increase shadow for god rays from clouds
INLINE float finalShadowFromShadowTerm(float s){return s*s;}
#if SHADER_COMPILER_FP16_ENABLED
INLINE half finalShadowFromShadowTerm(half s){return s*s;}
#endif
#endif

INLINE IrradianceSpectrum ComputeScatteringForIrradiance(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    int SampleCount,
    Length r, IN(Direction) viewDir, IN(Direction) sunDir)
{
  Number mu = viewDir.z, nu = dot(viewDir, sunDir), mu_s = sunDir.z;

  // Compute next intersection with atmosphere or ground
  Length tMax = DistanceToNearestAtmosphereBoundary(atmosphere_p, r, mu, RayIntersectsGround(atmosphere_p, r, mu));
  Length dx = tMax/SampleCount;

  // Ray march the atmosphere to integrate optical depth
  IrradianceSpectrum L = IrradianceSpectrum(0.0f,0.0f,0.0f);
  float3 throughput = DimensionlessSpectrum(1.0,1.0,1.0);
  Number RayleighPhaseValue = RayleighPhaseFunction(nu);
  Number MiePhaseValue = RayleighPhaseValue*MiePhaseFunctionDivideByRayleighOptimized(atmosphere_p.mie_phase_consts, nu);
  Length d = 0.0f;
  for (int i = 0; i <= SampleCount; ++i)
  {
    Length dt = (i == SampleCount || i == 0) ? 0.5*dx : dx;
    d += dt;
    Length r_d = ClampRadius(atmosphere_p, sqrt(d * d + 2.0 * r * mu * d + r * r));
    Number mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);

    MediumSampleRGB medium = SampleMediumFull(atmosphere_p, r_d-atmosphere_p.bottom_radius, Position(0,0,0));

    const float3 SampleOpticalDepth = medium.extinction * dt;
    const float3 sampleTransmittance = exp(-SampleOpticalDepth);

    float3 transmittanceToSun = GetTransmittanceToSun( atmosphere_p, transmittance_texture, r_d, mu_s_d);

    #if SHADOWMAP_ENABLED
      // First evaluate opaque shadow
      float shadow = getShadow(float3(0,0,0), d, r_d, mu_s_d);
      transmittanceToSun *= finalShadowFromShadowTerm(shadow);
    #endif
    float3 PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;

    float3 multiScatteredLuminance = float3(0,0,0);
    //todo: add multiple scattering as well?
    //float3 multiScatteredLuminance = GetMultipleScattering(atmosphere_p, multiple_scattering_approx, r_d, mu_s_d);
    float3 S = (transmittanceToSun * PhaseTimesScattering + multiScatteredLuminance * medium.scattering);

    float3 Sint = (S - S * sampleTransmittance) / medium.extinction;    // integrate along the current step segment
    L += throughput * Sint;                                                     // accumulate and also take into account the transmittance from previous steps
    throughput = throughput*sampleTransmittance;
  }
  return L;
}

IrradianceSpectrum ComputeIndirectIrradianceSingle(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    Length r, Number mu_s,
    int SAMPLE_COUNT,
    int RAY_SAMPLE_COUNT)
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  assert(mu_s >= -1.0 && mu_s <= 1.0);

  const Angle dphi = pi / Number(SAMPLE_COUNT);
  const Angle dtheta = pi / Number(SAMPLE_COUNT);

  IrradianceSpectrum result =
      IrradianceSpectrum(0.0 * watt_per_square_meter_per_nm,0.0 * watt_per_square_meter_per_nm,0.0 * watt_per_square_meter_per_nm);
  vec3 omega_s = vec3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
  for (int j = 0; j < SAMPLE_COUNT / 2; ++j) {
    Angle theta = (Number(j) + 0.5) * dtheta;
    for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) {
      Angle phi = (Number(i) + 0.5) * dphi;
      vec3 omega =
          vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
      SolidAngle domega = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

      result += ComputeScatteringForIrradiance(atmosphere_p, transmittance_texture, RAY_SAMPLE_COUNT, r, omega, omega_s) *
        omega.z * domega * atmosphere_p.solar_irradiance;
    }
  }
  return result;
}


IrradianceSpectrum GetMultipleScattering(IN(AtmosphereParameters) atmosphere_p, IN(MultipleScatteringTexture) multiple_scattering_approx,
  Length r, Number mu)
{
  float MultiScatteringLUTRes = SKIES_MULTIPLE_SCATTERING_APPROX;
  float2 uv = saturate(float2(mu*0.5f + 0.5f, (r - atmosphere_p.bottom_radius) / (atmosphere_p.top_radius - atmosphere_p.bottom_radius)));
  uv = float2(GetTextureCoordFromUnitRange(uv.x, MultiScatteringLUTRes), GetTextureCoordFromUnitRange(uv.y, MultiScatteringLUTRes));

  return IrradianceSpectrumFromTexture(sample_texture(multiple_scattering_approx, uv));
}

struct SingleScatteringResult
{
  IrradianceSpectrum L;                       // Scattered light (luminance)
  IrradianceSpectrum ray,mie,ms;
  DimensionlessSpectrum Transmittance;           // Transmittance in [0,1] (unitless)
};

INLINE SingleScatteringResult IntegrateScatteredLuminanceMS(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    IN(MultipleScatteringTexture) multiple_scattering_approx,
    IN(Position) worldPos, IN(Direction) worldDir, Number shadow_offset,//only for shadows
    float sampleCountIni, bool variableSampleCount, float2 rayMarchMinMaxSPP,
    Length r, Number mu, Number nu, Number mu_s,
    bool ray_intersects_ground,//RayIntersectsGround(atmosphere_p, r, mu)
    float tMaxMax = 9000000.0f)
{
  SingleScatteringResult result;
  result.L = result.ray = result.mie = result.ms = IrradianceSpectrum(0,0,0);
  float start_distance = 0;
  #if ORIGIN_CAN_BE_IN_SPACE
  result.Transmittance = DimensionlessSpectrum(1,1,1);
  //just skip distance befroe atmosphere.
  //it is totally fine to render atmosphere from space without this defines on.
  //it is just produces better quality in such case
  if (r > atmosphere_p.top_radius)
  {
    Length rmu = r*mu;
    Length distance_to_top_atmosphere_boundary = -rmu -
      sqrt(rmu * rmu - r * r + atmosphere_p.top_radius * atmosphere_p.top_radius);
    if (distance_to_top_atmosphere_boundary > 0.0 * meter) {
      start_distance = distance_to_top_atmosphere_boundary;
    } else {
      // If the view ray does not intersect the atmosphere_p, simply return 0.
      return result;
    }
  }
  #endif

  // Compute next intersection with atmosphere or ground
  float tMax = DistanceToNearestAtmosphereBoundary(atmosphere_p, r, mu, ray_intersects_ground) - start_distance;
  tMax = min(tMax, tMaxMax);

  // Sample count
  float sampleCount = sampleCountIni;
  float sampleCountFloor = sampleCountIni-1;
  float tMaxFloor = tMax;
  if (variableSampleCount)
  {
    sampleCount = lerp(rayMarchMinMaxSPP.x, rayMarchMinMaxSPP.y, saturate(tMax*0.01));
    sampleCountFloor = floor(sampleCount);
    tMaxFloor = tMax * sampleCountFloor / sampleCount;  // rescale tMax to map to the last entire step segment.
  }
  // Phase functions
  //const Number uniformPhase = 1.0 / (4.0 * PI);
  Number RayleighPhaseValue = RayleighPhaseFunction(nu);
  Number MiePhaseValue = RayleighPhaseValue*MiePhaseFunctionDivideByRayleighOptimized(atmosphere_p.mie_phase_consts, nu);

  // Ray march the atmosphere to integrate optical depth
  IrradianceSpectrum L = IrradianceSpectrum(0.0f,0.0f,0.0f);
  DimensionlessSpectrum throughput = DimensionlessSpectrum(1.0,1.0,1.0);
  float t = 0.0f;
  float tPrev = 0.0;
  const float sampleSegmentT = 0.5f;
  float invSampleCountFloor = 1.0f/sampleCountFloor;
  Position curWorldPos = worldPos;
  for (float s = 0.0f, e = sampleCount*invSampleCountFloor; s < e; s += invSampleCountFloor)
  {
    float dt;
    if (variableSampleCount)
    {
      float t0 = s * s;
      float t1 = s + invSampleCountFloor;
      // Non linear distribution of sample within the range.
      t1 = t1 * t1;
      // Make t0 and t1 world space distances.
      t0 = tMaxFloor * t0;
      t1 = t1 > 1.0f ? tMax : tMaxFloor * t1;
      t = t0 + (t1 - t0)*sampleSegmentT;
      dt = t1 - t0;
    }
    else
    {
      float newT = tMax * saturate(s + sampleSegmentT*invSampleCountFloor);
      dt = newT - t;
      t = newT;
    }
    Length d = t + start_distance;
    Length r_d = ClampRadius(atmosphere_p, SafeSqrt(d * d + 2.0 * r * mu * d + r * r));
    Number mu_s_d = ClampCosine((r * mu_s + d * nu) / r_d);

    G_UNUSED(curWorldPos);G_UNUSED(worldDir);G_UNUSED(shadow_offset);
#if SHADOWMAP_ENABLED
    // First evaluate opaque shadow
    Position curWorldPos = worldPos + (d + shadow_offset*dt) * worldDir;
    float shadow = getShadow(curWorldPos, d, r_d, mu_s_d);
#endif
    MediumSampleRGB medium = SampleMediumFull(atmosphere_p, r_d-atmosphere_p.bottom_radius, curWorldPos);

    const float3 sampleOpticalDepth = medium.extinction * dt;
    const float3 sampleTransmittance = exp(-sampleOpticalDepth);

    float3 transmittanceToSun = GetTransmittanceToSun( atmosphere_p, transmittance_texture, r_d, mu_s_d);


    G_UNUSED(worldPos);G_UNUSED(worldDir);G_UNUSED(shadow_offset);
#if SHADOWMAP_ENABLED
    // First evaluate opaque shadow
    transmittanceToSun *= finalShadowFromShadowTerm(shadow);
#endif
    float3 PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;

    // Dual scattering for multi scattering

    float3 multiScatteredLuminance = GetMultipleScattering(atmosphere_p, multiple_scattering_approx, r_d, mu_s_d);

    #if ORIGIN_CAN_BE_IN_SPACE && SHADOWMAP_ENABLED
    multiScatteredLuminance *= shadow;
    #endif

    float3 S = (transmittanceToSun * PhaseTimesScattering + multiScatteredLuminance * medium.scattering);

    // When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge.
    // Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
    // The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
    // However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.

    // See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
    float3 Sint = (S - S * sampleTransmittance) / medium.extinction;    // integrate along the current step segment
    L += throughput * Sint;                                                     // accumulate and also take into account the transmittance from previous steps

    float3 rayS = medium.scatteringRay*transmittanceToSun;
    float3 raySint = (rayS - rayS * sampleTransmittance) / medium.extinction;
    result.ray += throughput * raySint;

    float3 mieS = medium.scatteringMie*transmittanceToSun;
    float3 mieSint = (mieS - mieS * sampleTransmittance) / medium.extinction;
    result.mie += throughput * mieSint;

    float3 msS = multiScatteredLuminance * medium.scattering;
    float3 msSint = (msS - msS * sampleTransmittance) / medium.extinction;
    result.ms += throughput * msSint;

    throughput = throughput*sampleTransmittance;
    tPrev = t;
  }

  result.L = L;
  result.Transmittance = throughput;
  return result;
}

IrradianceSpectrum ComputeIndirectIrradianceMS(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    IN(MultipleScatteringTexture) ms_texture,
    Length r, Number mu_s)
{
  assert(r >= atmosphere_p.bottom_radius && r <= atmosphere_p.top_radius);
  assert(mu_s >= -1.0 && mu_s <= 1.0);

  const int SAMPLE_COUNT = 8;
  const Angle dphi = pi / Number(SAMPLE_COUNT);
  const Angle dtheta = pi / Number(SAMPLE_COUNT);

  IrradianceSpectrum result =
      IrradianceSpectrum(0.0 * watt_per_square_meter_per_nm,0.0 * watt_per_square_meter_per_nm,0.0 * watt_per_square_meter_per_nm);
  vec3 omega_s = vec3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
  for (int j = 0; j < SAMPLE_COUNT / 2; ++j) {
    Angle theta = (Number(j) + 0.5) * dtheta;
    for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) {
      Angle phi = (Number(i) + 0.5) * dphi;
      vec3 omega =
          vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
      SolidAngle domega = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

      Number nu = dot(omega, omega_s);
      SingleScatteringResult ss = IntegrateScatteredLuminanceMS(
          atmosphere_p,
          transmittance_texture,
          ms_texture,
          float3(0,0,0),float3(0,0,0),0,//shadows
          32, true, float2(16,16),
          r, omega.z, nu, mu_s,
          false);
      result += ss.L * omega.z * domega * atmosphere_p.solar_irradiance;
    }
  }
  return result;
}


IrradianceSpectrum ComputeIndirectIrradianceTextureMS(
    IN(AtmosphereParameters) atmosphere_p,
    IN(TransmittanceTexture) transmittance_texture,
    IN(MultipleScatteringTexture) ms_texture,
    IN(vec2) frag_coord)
{
  Length r;
  Number mu_s;
  GetRMuSFromIrradianceTextureUv(
      atmosphere_p, frag_coord / vec2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), r, mu_s);
  //return ComputeIndirectIrradianceSingle(atmosphere_p, transmittance_texture, r, mu_s, 32, 32);
  return ComputeIndirectIrradianceMS(atmosphere_p,
      transmittance_texture, ms_texture, r, mu_s);
}

#endif