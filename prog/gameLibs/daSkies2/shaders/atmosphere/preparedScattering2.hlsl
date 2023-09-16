#ifndef SKIES_PREPARED_SCATTERING
#define SKIES_PREPARED_SCATTERING 1

vec4 getPreparedCombinedScattering(IN(PreparedScatteringTexture) preparedScattering, IN(vec2) prepared_tc, float layer)
{
  return sample_texture(preparedScattering, float3(prepared_tc.x, prepared_tc.y, layer));
}

float getInscatterLerpParam(vec2 viewdirXYNorm, vec2 sundirXYNorm)
{
  //remapped(0.5-0.5*dot(viewdirXYNorm, sundirXYNorm))
  float inscatterLerp = dot(viewdirXYNorm, sundirXYNorm);
  return inscatterLerp*(-0.5*((SKIES_OPTIMIZATION_NUM_LAYERS-1.)/(SKIES_OPTIMIZATION_NUM_LAYERS))) + 0.5;
}

vec4 getPreparedCombinedScatteringXYZ(IN(PreparedScatteringTexture) preparedScattering, float3 viewdir, float3 sundir, vec2 prepared_tc)
{
  return getPreparedCombinedScattering(preparedScattering, prepared_tc, getInscatterLerpParam(normalize(viewdir.xy), normalize(sundir.xy)));
}

DimensionlessSpectrum getPreparedTransmittanceFromUV(IN(TransmittanceTexture) preparedTransmittance, IN(vec2) prepared_tc)
{
  return DimensionlessSpectrumFromTexture(sample_texture(preparedTransmittance, prepared_tc));
}

#endif