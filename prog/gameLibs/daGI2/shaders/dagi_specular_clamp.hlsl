#ifndef DAGI_SPECULAR_CLAMP_HLSL
#define DAGI_SPECULAR_CLAMP_HLSL 1

// Requires luminance() from shader_global.dshl.

// Max plausible ambient-specular brightness given the diffuse irradiance,
// in multiples of it ("Volumetric Global Illumination at Treyarch",
// J.T.Hooker, SIGGRAPH 2016; constants are from the talk).
float dagi_max_specular_scale(float linear_roughness, float NoV, float min_scale, float num_probe_mips)
{
  float mip = linear_roughness*num_probe_mips;
  float v = max3(min_scale,
                 9.13681*exp2(6.85741 - 2.*mip)*NoV,
                 9.70809*exp2(7.085 - mip - 0.403181*mip*mip)*NoV);
  return min(v, 32.);
}
// Treyarch harmonic soft-min. Attenuates well below the ceiling too (0.5x
// already at spec == ceiling) - the price paid where specular can leak
// through thin walls (sparse radiance grid) or has no local GI data at all
// (envi/indoor probe fallback).
float3 dagi_clamp_untrusted_specular(float3 spec, float3 ambient, float max_scale)
{
  float3 m = ambient*max_scale;
  return spec*(m/(m + spec + 0.001));
}
// Screen-confirmed probe specular does not leak; its one defect is that
// probe radiance has no preintegrated roughness mips, so a bright source a
// wide lobe should have averaged away stays unfiltered in the sample and
// rough surfaces read implausibly bright (the "moist" look). Bound it by
// the local irradiance: identity below 1.268*irradiance (the r->1 limit of
// the Treyarch model, plausible at any roughness), then harmonic
// compression of only the excess, saturating at the max_scale ceiling.
// Legitimate specular pays nothing, unlike the soft-min above.
float3 dagi_bound_probe_specular(float3 spec, float3 ambient, float max_scale)
{
  float lumA = max((float)luminance(ambient), 1e-4);
  float lumS = max((float)luminance(spec), 1e-6);
  float m = max_scale*lumA;
  float t = min(1.26816*lumA, 0.5*m);
  float over = max(lumS - t, 0.), range = m - t;
  return spec*((min(lumS, t) + over*range/(over + range))/lumS);
}

#endif
