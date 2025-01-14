#ifndef SPECULAR_BRDF_HLSL
#define SPECULAR_BRDF_HLSL 1

// Microfacet specular = D*G*F / (4*NoL*NoV) = D*Vis*F
// Vis = G / (4*NoL*NoV)

#ifndef INV_MIN_IOR
#define INV_MIN_IOR 50.0h
#endif

// [Blinn 1977, "Models of light reflection for computer synthesized pictures"]
half distributionBlinn( half ggx_alpha, half NoH )
{
  half m2 = ggx_alpha * ggx_alpha;
  half n2 = 1 / m2 - 1;
  return (n2+1) * clampedPow( NoH, n2*2 );//division by PI omitted intentionally, lightColor is divided by Pi
}

// [Beckmann 1963, "The scattering of electromagnetic waves from rough surfaces"]
half distributionBeckmann( half ggx_alpha, half NoH )
{
  half m2 = ggx_alpha * ggx_alpha;
  half NoH2 = NoH * NoH;
  return exp( (NoH2 - 1) / (m2 * NoH2) ) / (m2 * NoH2 * NoH2 );//division by PI omitted intentionally, lightColor is divided by Pi
}

// GGX / Trowbridge-Reitz
half distributionGGX( float ggx_alpha, float NoH )
{
  float alpha2 = ggx_alpha * ggx_alpha;
  float d = ( NoH * alpha2 - NoH ) * NoH + 1;	// 2 mad
  return half(alpha2 / max(1e-8, d * d)); //division by PI omitted intentionally, lightColor is divided by Pi
}

// Anisotropic GGX, Disney
half distributionGGXaniso( half ggx_alphaX, half ggx_alphaY, half NoH, half3 H, half3 X, half3 Y )
{
  half XoH = dot( X, H );
  half YoH = dot( Y, H );
  half d = XoH*XoH / pow2(ggx_alphaX) + YoH*YoH / pow2(ggx_alphaY) + NoH*NoH;
  return 1 / max(1e-8, ggx_alphaX*ggx_alphaY * d*d );//division by PI omitted intentionally, lightColor is divided by Pi
}

//Ashikhmin
//https://knarkowicz.wordpress.com/2018/01/04/cloth-shading/
half distributionCloth(half ggx_alpha, half NoH)
{
  half r2    = ggx_alpha * ggx_alpha;
  half cos2h = NoH * NoH;
  half sin2h = max(1e-9, 1. - cos2h);
  half sin4h = sin2h * sin2h;
  return (sin4h + 4. * exp(-cos2h / (sin2h * r2))) / ((1. + 4. * r2) * sin4h);// division by Pi is ommited
}

half geometryImplicit()
{
  return 0.25;
}

half geometryNeumann( half NoV, half NoL )
{
  return rcp( 4 * max( NoL, NoV ) +1e-5);
}

half geometryCookTorrance(half NoH, half NoL, half NoV, half VoH, half ggx_alpha)
{
  half NdotL_clamped= max(NoL, 0.0);
  half NdotV_clamped= max(NoV, 0.0);
  half NoH_dov_VdotH = 2*NoH / VoH;
  return min( min( NoH_dov_VdotH * NdotV_clamped, NoH_dov_VdotH * NdotL_clamped), 1.0);
}

// [Kelemen 2001, "A microfacet based coupled specular-matte brdf model with importance sampling"]
half geometryKelemen( half VoH )
{
  return rcp( 4 * VoH * VoH );
}

half geometrySchlick( half ggx_alpha, half NoV, half NoL )
{
  half k = pow2( ggx_alpha ) * 0.5;
  half geometrySchlickV = NoV * (1 - k) + k;
  half geometrySchlickL = NoL * (1 - k) + k;
  return 0.25 / ( geometrySchlickV * geometrySchlickL );
}

// Smith term for GGX
half geometrySmith( half ggx_alpha, half NoV, half NoL )
{
  half a2 = pow2( ggx_alpha );

  half geometrySmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2 );
  half geometrySmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2 );
  return rcp( geometrySmithV * geometrySmithL );
}

// Appoximation of correlated Smith term for GGX/Heitz
half geometrySmithCorrelated( half ggx_alpha, half NoV, half NoL )
{
  half a2 = pow2( ggx_alpha );
  half geometrySmithV = NoL * sqrt (NoV*(NoV - NoV * a2) + a2 );
  half geometrySmithL = NoV * sqrt (NoL*(NoL - NoL * a2) + a2 );
  //half a = ggx_alpha;
  //half geometrySmithV = NoL * ( NoV * ( 1 - a ) + a );
  //half geometrySmithL = NoV * ( NoL * ( 1 - a ) + a );
  return (0.5 * rcp( geometrySmithV + geometrySmithL ));
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
half geometrySmithCorrelatedApprox( half ggx_alpha, half NoV, half NoL )
{
  half geometrySmithV = NoL * ( NoV * ( 1 - ggx_alpha ) + ggx_alpha );
  half geometrySmithL = NoV * ( NoL * ( 1 - ggx_alpha ) + ggx_alpha );
  return 0.5 * rcp( geometrySmithV + geometrySmithL );
}

//Ashikhmin
//https://knarkowicz.wordpress.com/2018/01/04/cloth-shading/
half geometryCloth(half NoV, half NoL)
{
  return 1. / (4. * (NoL+ NoV - NoL * NoV));
}

half3 fresnelNone( half3 specularColor )
{
  return specularColor;
}

half3 fresnelSchlick( half3 specularColor, half VoH )
{
  half Fc = pow5( 1 - VoH );
  
  // Anything less than 2% is physically impossible and is instead considered to be shadowing
  return saturate( INV_MIN_IOR * specularColor.g ) * Fc + (1 - Fc) * specularColor;
  
}

half3 fresnelFresnel( half3 specularColor, half VoH )
{
  half3 SpecularColorSqrt = sqrt( clamp( half3(0, 0, 0), half3(0.99, 0.99, 0.99), specularColor ) );
  half3 n = ( 1 + SpecularColorSqrt ) / ( 1 - SpecularColorSqrt );
  half3 g = sqrt( n*n + VoH*VoH - 1 );
  return 0.5 * pow2_vec3( (g - VoH) / (g + VoH) ) * ( 1 + pow2_vec3( ((g+VoH)*VoH - 1) / ((g-VoH)*VoH + 1) ) );
}

half3 getRoughReflectionVec(half3 R, half3 normal, half ggx_alpha)//alpha = pow2(linear_roughness)
{
  return lerp( normal, R, (1.h - ggx_alpha) * ( sqrt(1.h - ggx_alpha) + ggx_alpha ) );
}

#endif
