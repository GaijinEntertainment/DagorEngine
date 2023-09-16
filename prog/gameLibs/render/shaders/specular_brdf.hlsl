#ifndef SPECULAR_BRDF_HLSL
#define SPECULAR_BRDF_HLSL 1

// Microfacet specular = D*G*F / (4*NoL*NoV) = D*Vis*F
// Vis = G / (4*NoL*NoV)

#ifndef INV_MIN_IOR
#define INV_MIN_IOR 50.0h
#endif

// [Blinn 1977, "Models of light reflection for computer synthesized pictures"]
float distributionBlinn( float ggx_alpha, float NoH )
{
  float m2 = ggx_alpha * ggx_alpha;
  float n2 = 1 / m2 - 1;
  return (n2+1) * clampedPow( NoH, n2*2 );//division by PI omitted intentionally, lightColor is divided by Pi
}

// [Beckmann 1963, "The scattering of electromagnetic waves from rough surfaces"]
float distributionBeckmann( float ggx_alpha, float NoH )
{
  float m2 = ggx_alpha * ggx_alpha;
  float NoH2 = NoH * NoH;
  return exp( (NoH2 - 1) / (m2 * NoH2) ) / (m2 * NoH2 * NoH2 );//division by PI omitted intentionally, lightColor is divided by Pi
}

// GGX / Trowbridge-Reitz
float distributionGGX( float ggx_alpha, float NoH )
{
  float alpha2 = ggx_alpha * ggx_alpha;
  float d = ( NoH * alpha2 - NoH ) * NoH + 1;	// 2 mad
  return alpha2 / max(1e-8, d*d );//division by PI omitted intentionally, lightColor is divided by Pi
}

// Anisotropic GGX, Disney
float distributionGGXaniso( float ggx_alphaX, float ggx_alphaY, float NoH, float3 H, float3 X, float3 Y )
{
  float XoH = dot( X, H );
  float YoH = dot( Y, H );
  float d = XoH*XoH / pow2(ggx_alphaX) + YoH*YoH / pow2(ggx_alphaY) + NoH*NoH;
  return 1 / max(1e-8, ggx_alphaX*ggx_alphaY * d*d );//division by PI omitted intentionally, lightColor is divided by Pi
}

//Ashikhmin
//https://knarkowicz.wordpress.com/2018/01/04/cloth-shading/
float distributionCloth(float ggx_alpha, float NoH)
{
  float r2    = ggx_alpha * ggx_alpha;
  float cos2h = NoH * NoH;
  float sin2h = 1. - cos2h;
  float sin4h = sin2h * sin2h;
  return (sin4h + 4. * exp(-cos2h / (sin2h * r2))) / ((1. + 4. * r2) * sin4h);// division by Pi is ommited
}

float geometryImplicit()
{
  return 0.25;
}

float geometryNeumann( float NoV, float NoL )
{
  return rcp( 4 * max( NoL, NoV ) +1e-5);
}

float geometryCookTorrance(float NoH, float NoL, float NoV, float VoH, float ggx_alpha)
{
  float NdotL_clamped= max(NoL, 0.0);
  float NdotV_clamped= max(NoV, 0.0);
  float NoH_dov_VdotH = 2*NoH / VoH;
  return min( min( NoH_dov_VdotH * NdotV_clamped, NoH_dov_VdotH * NdotL_clamped), 1.0);
}

// [Kelemen 2001, "A microfacet based coupled specular-matte brdf model with importance sampling"]
float geometryKelemen( float VoH )
{
  return rcp( 4 * VoH * VoH );
}

float geometrySchlick( float ggx_alpha, float NoV, float NoL )
{
  float k = pow2( ggx_alpha ) * 0.5;
  float geometrySchlickV = NoV * (1 - k) + k;
  float geometrySchlickL = NoL * (1 - k) + k;
  return 0.25 / ( geometrySchlickV * geometrySchlickL );
}

// Smith term for GGX
float geometrySmith( float ggx_alpha, float NoV, float NoL )
{
  float a2 = pow2( ggx_alpha );

  float geometrySmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2 );
  float geometrySmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2 );
  return rcp( geometrySmithV * geometrySmithL );
}

// Appoximation of correlated Smith term for GGX/Heitz
float geometrySmithCorrelated( float ggx_alpha, float NoV, float NoL )
{
  float a2 = pow2( ggx_alpha );
  float geometrySmithV = NoL * sqrt (NoV*(NoV - NoV * a2) + a2 );
  float geometrySmithL = NoV * sqrt (NoL*(NoL - NoL * a2) + a2 );
  //float a = ggx_alpha;
  //float geometrySmithV = NoL * ( NoV * ( 1 - a ) + a );
  //float geometrySmithL = NoV * ( NoL * ( 1 - a ) + a );
  return (0.5 * rcp( geometrySmithV + geometrySmithL ));
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float geometrySmithCorrelatedApprox( float ggx_alpha, float NoV, float NoL )
{
  float geometrySmithV = NoL * ( NoV * ( 1 - ggx_alpha ) + ggx_alpha );
  float geometrySmithL = NoV * ( NoL * ( 1 - ggx_alpha ) + ggx_alpha );
  return 0.5 * rcp( geometrySmithV + geometrySmithL );
}

//Ashikhmin
//https://knarkowicz.wordpress.com/2018/01/04/cloth-shading/
float geometryCloth(float NoV, float NoL)
{
  return 1. / (4. * (NoL+ NoV - NoL * NoV));
}

float3 fresnelNone( float3 specularColor )
{
  return specularColor;
}

float3 fresnelSchlick( float3 specularColor, float VoH )
{
  float Fc = pow5( 1 - VoH );
  
  // Anything less than 2% is physically impossible and is instead considered to be shadowing
  return saturate( INV_MIN_IOR * specularColor.g ) * Fc + (1 - Fc) * specularColor;
  
}

float3 fresnelFresnel( float3 specularColor, float VoH )
{
  float3 SpecularColorSqrt = sqrt( clamp( float3(0, 0, 0), float3(0.99, 0.99, 0.99), specularColor ) );
  float3 n = ( 1 + SpecularColorSqrt ) / ( 1 - SpecularColorSqrt );
  float3 g = sqrt( n*n + VoH*VoH - 1 );
  return 0.5 * pow2_vec3( (g - VoH) / (g + VoH) ) * ( 1 + pow2_vec3( ((g+VoH)*VoH - 1) / ((g-VoH)*VoH + 1) ) );
}

float3 getRoughReflectionVec(float3 R, float3 normal, float ggx_alpha)//alpha = pow2(linear_roughness)
{
  return lerp( normal, R, (1 - ggx_alpha) * ( sqrt(1 - ggx_alpha) + ggx_alpha ) );
}

#endif
