#ifndef DIFFUSE_BRDF_HLSL
#define DIFFUSE_BRDF_HLSL 1

float3 diffuseLambert( float3 diffuseColor )
{
  return diffuseColor;//division by PI omitted intentionally, lightColor is divided by Pi
}

//linearRoughness - perceptual linear roughness
float3 diffuseBurley( float3 diffuseColor, float linearRoughness, float NoV, float NoL, float VoH )
{
  float FD90 = 0.5 + 2 * VoH * VoH * linearRoughness;
  float FdV = 1 + (FD90 - 1) * pow5(1 - NoV);
  float FdL = 1 + (FD90 - 1) * pow5(1 - NoL);
  return diffuseColor * ( FdV * FdL );//division by PI omitted intentionally, lightColor is divided by Pi
}

//from moving FrostBite to PBR, energy conservative burley diffuse
float diffuseBurleyFixedFresnel( float linearRoughness, float NoV, float NoL, float VoH )
{
  float energyBias = 0.5;
  float energyFactor = (0.45f * linearRoughness - 0.475f) * linearRoughness + 1.0f;
  float FD90 = energyBias + 2 * VoH * VoH * linearRoughness;
  float FdV = 1 + (FD90 - 1) * pow5(1 - NoV);
  float FdL = 1 + (FD90 - 1) * pow5(1 - NoL);
  return ( FdV * FdL * energyFactor);//division by PI omitted intentionally, lightColor is divided by Pi
}

float3 diffuseBurleyFixed( float3 diffuseColor, float linearRoughness, float NoV, float NoL, float VoH )
{
  return diffuseColor * diffuseBurleyFixedFresnel( linearRoughness, NoV, NoL, VoH);//division by PI omitted intentionally, lightColor is divided by Pi
}

float diffuseChanFresnel( float linear_roughness, float NoV, float NoL, float VoH, float NoH, float retroReflectivityWeight=1)
{
  // Since we use CoD fitting, need to invert/convert our roughness parametrization to
  // their glossiness parametrization 'g'
  float ggx_alpha = linear_roughness*linear_roughness;
  float a2 = ggx_alpha*ggx_alpha;
  float g = saturate( (1.0 / 18.0) * log2( 2 / a2 - 1 ) );

  float f0 = VoH + pow5(1 - VoH);
  // This factor tunes the enery boost at grazing angle for low roughness material.
  // It creates energy ring at grazing angle for smooth dieletric material
  // The higher the value 'rimScale' is, the lower the energy boost will be
  // The default value from Chan is 0.75.
  // Setting the value to 1.0 get rid off this energy boost.
  const float rimScale = 0.75f;
  float f1 = (1.0 - rimScale * pow5(1.0 - NoL)) * (1.0 - rimScale * pow5(1.0 - NoV));

  // Rough (f0) to smooth f1 response interpolation
  float fd = lerp( f0, f1, saturate( 2.2 * g - 0.5 ) );

  // Retro reflectivity contribution.
  float fb = ( (34.5 * g - 59 ) * g + 24.5 ) * VoH * exp2( -max( 73.2 * g - 21.2, 8.9 ) * sqrt( NoH ) );
  // allow to fade our rough retro reflectivity
  fb *= retroReflectivityWeight;
  return ( fd + fb );
}

float3 diffuseChan( float3 diffuseColor, float linear_roughness, float NoV, float NoL, float VoH, float NoH, float retroReflectivityWeight=1)
{
  return diffuseColor * diffuseChanFresnel(linear_roughness, NoV, NoL, VoH, NoH, retroReflectivityWeight);
}

float3 diffuseOrenNayar( float3 diffuseColor, float linear_roughness, float NoV, float NoL, float VoH )
{
  float VoL = 2 * VoH - 1;
  float m = linear_roughness * linear_roughness;
  float m2 = m * m;
  float C1 = 1 - 0.5 * m2 *rcp(m2 + 0.33);
  float Cosri = VoL - NoV * NoL;

  float C2 = 0.45 * m2 * rcp(m2 + 0.09) * ( Cosri >= 0 ? Cosri * rcp(max(NoL, NoV)) : 0 );
  return diffuseColor * (C1 + C2);
  //float C2 = 0.45 * m2 * rcp(m2 + 0.09) * Cosri * ( Cosri >= 0 ? min( 1, NoL / NoV ) : NoL );
  //return diffuseColor * ( NoL * C1 + C2 );//division by PI omitted intentionally, lightColor is divided by Pi
}

// N is the normal direction
// V is the view vector
// NdotV is the cosine angle between the view vector and the normal
float3 getDiffuseDominantDir ( float3 N, float3 V, float saturated_NdotV, float ggx_alpha)
{
  float a = 1.02341f * ggx_alpha - 1.51174f;
  float b = -0.511705f * ggx_alpha + 0.755868f;
  float lerpFactor = saturate (( saturated_NdotV* a + b) * ggx_alpha);
  // The result is not normalized as we fetch in a cubemap
  return lerp (N, V, lerpFactor );
}


#endif