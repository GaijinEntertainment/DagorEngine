#ifndef DIFFUSE_BRDF_HLSL
#define DIFFUSE_BRDF_HLSL 1

half3 diffuseLambert( half3 diffuseColor )
{
  return diffuseColor;//division by PI omitted intentionally, lightColor is divided by Pi
}

//linearRoughness - perceptual linear roughness
half3 diffuseBurley( half3 diffuseColor, half linearRoughness, half NoV, half NoL, half VoH )
{
  half FD90 = 0.5h + 2 * VoH * VoH * linearRoughness;
  half FdV = 1 + (FD90 - 1) * pow5(1 - NoV);
  half FdL = 1 + (FD90 - 1) * pow5(1 - NoL);
  return diffuseColor * ( FdV * FdL );//division by PI omitted intentionally, lightColor is divided by Pi
}

//from moving FrostBite to PBR, energy conservative burley diffuse
half diffuseBurleyFixedFresnel( half linearRoughness, half NoV, half NoL, half VoH )
{
  half energyBias = 0.5h;
  half energyFactor = (0.45h * linearRoughness - 0.475h) * linearRoughness + 1;
  half FD90 = energyBias + 2 * VoH * VoH * linearRoughness;
  half FdV = 1 + (FD90 - 1) * pow5(1 - NoV);
  half FdL = 1 + (FD90 - 1) * pow5(1 - NoL);
  return ( FdV * FdL * energyFactor);//division by PI omitted intentionally, lightColor is divided by Pi
}

half3 diffuseBurleyFixed( half3 diffuseColor, half linearRoughness, half NoV, half NoL, half VoH )
{
  return diffuseColor * diffuseBurleyFixedFresnel( linearRoughness, NoV, NoL, VoH);//division by PI omitted intentionally, lightColor is divided by Pi
}

half diffuseChanFresnel( half linear_roughness, half NoV, half NoL, half VoH, half NoH, half retroReflectivityWeight=1)
{
  // Since we use CoD fitting, need to invert/convert our roughness parametrization to
  // their glossiness parametrization 'g'
  half ggx_alpha = linear_roughness*linear_roughness;
  half a2 = ggx_alpha*ggx_alpha;
  half g = saturate( (1.0h / 18.0h) * log2( 2 / a2 - 1 ) );

  half f0 = VoH + pow5(1 - VoH);
  // This factor tunes the enery boost at grazing angle for low roughness material.
  // It creates energy ring at grazing angle for smooth dieletric material
  // The higher the value 'rimScale' is, the lower the energy boost will be
  // The default value from Chan is 0.75.
  // Setting the value to 1.0 get rid off this energy boost.
  const half rimScale = 0.75h;
  half f1 = (1.0h - rimScale * pow5(1.0h - NoL)) * (1.0h - rimScale * pow5(1.0h - NoV));

  // Rough (f0) to smooth f1 response interpolation
  half fd = lerp( f0, f1, saturate( 2.2h * g - 0.5h ) );

  // Retro reflectivity contribution.
  half fb = ( (34.5h * g - 59 ) * g + 24.5h ) * VoH * exp2( -max( 73.2h * g - 21.2h, 8.9h ) * sqrt( NoH ) );
  // allow to fade our rough retro reflectivity
  fb *= retroReflectivityWeight;
  return ( fd + fb );
}

half3 diffuseChan( half3 diffuseColor, half linear_roughness, half NoV, half NoL, half VoH, half NoH, half retroReflectivityWeight=1)
{
  return diffuseColor * diffuseChanFresnel(linear_roughness, NoV, NoL, VoH, NoH, retroReflectivityWeight);
}

half3 diffuseOrenNayar( half3 diffuseColor, half linear_roughness, half NoV, half NoL, half VoH )
{
  half VoL = 2 * VoH - 1;
  half m = linear_roughness * linear_roughness;
  half m2 = m * m;
  half C1 = 1 - 0.5h * m2 *rcp(m2 + 0.33h);
  half Cosri = VoL - NoV * NoL;

  half C2 = 0.45h * m2 * rcp(m2 + 0.09h) * ( Cosri >= 0 ? Cosri * rcp(max(NoL, NoV)) : 0 );
  return diffuseColor * (C1 + C2);
  //half C2 = 0.45 * m2 * rcp(m2 + 0.09) * Cosri * ( Cosri >= 0 ? min( 1, NoL / NoV ) : NoL );
  //return diffuseColor * ( NoL * C1 + C2 );//division by PI omitted intentionally, lightColor is divided by Pi
}

// N is the normal direction
// V is the view vector
// NdotV is the cosine angle between the view vector and the normal
half3 getDiffuseDominantDir ( half3 N, half3 V, half saturated_NdotV, half ggx_alpha)
{
  half a = 1.02341h * ggx_alpha - 1.51174h;
  half b = -0.511705h * ggx_alpha + 0.755868h;
  half lerpFactor = saturate (( saturated_NdotV* a + b) * ggx_alpha);
  // The result is not normalized as we fetch in a cubemap
  return lerp (N, V, lerpFactor );
}


#endif