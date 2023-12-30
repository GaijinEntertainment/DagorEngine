#ifndef __BRDF_COMMON__
#define __BRDF_COMMON__

#include <diffuse_brdf.hlsl>
#include <specular_brdf.hlsl>
#include <envi_brdf.hlsl>

// Diffuse model
#define DIFFUSE_LAMBERT 0
#define DIFFUSE_OREN_NAYAR 1
#define DIFFUSE_BURLEY 2
#define DIFFUSE_BURLEY_FIXED 3
#define DIFFUSE_CHAN 4

#ifndef BRDF_DIFFUSE
#define BRDF_DIFFUSE DIFFUSE_BURLEY_FIXED//DIFFUSE_OREN_NAYAR//DIFFUSE_BURLEY//DIFFUSE_OREN_NAYAR//DIFFUSE_BURLEY//DIFFUSE_LAMBERT//
#endif

// Microfacet distribution function

#define SPEC_D_BLINN 0
#define SPEC_D_BECKMANN 1
#define SPEC_D_GGX 2

#ifndef BRDF_SPEC_D
#define BRDF_SPEC_D SPEC_D_GGX
#endif

// Geometric attenuation or shadowing
#define SPEC_G_IMPLICIT 0
#define SPEC_G_NEUMANN 1
#define SPEC_G_KELEMEN 2
#define SPEC_G_SHLICK 3
#define SPEC_G_SMITH_GGX 4
#define SPEC_G_SMITH_CORRELATED 5
#define SPEC_G_SMITH_CORRELATED_APPROX 6

#ifndef BRDF_SPEC_G
#define BRDF_SPEC_G SPEC_G_SMITH_CORRELATED
#endif

// Fresnel
#define SPEC_F_NONE 0
#define SPEC_F_SHLICK 1
#define SPEC_F_FRESNEL 2
#ifndef BRDF_SPEC_F
#define BRDF_SPEC_F SPEC_F_SHLICK
#endif

// Sheen
#ifndef SHEEN_SPECULAR
#define SHEEN_SPECULAR 0
#endif

float3 BRDF_diffuse( float3 diffuseColor, float linearRoughness, float NoV, float NoL, float VoH )
{
#if   BRDF_DIFFUSE == DIFFUSE_LAMBERT
  return diffuseLambert( diffuseColor );
#elif BRDF_DIFFUSE == DIFFUSE_OREN_NAYAR
  return diffuseOrenNayar( diffuseColor, linearRoughness, NoV, NoL, VoH );
#elif BRDF_DIFFUSE == DIFFUSE_BURLEY
  return diffuseBurley( diffuseColor, linearRoughness, NoV, NoL, VoH );
#elif BRDF_DIFFUSE == DIFFUSE_BURLEY_FIXED
  return diffuseBurleyFixed( diffuseColor, linearRoughness, NoV, NoL, VoH );
#elif BRDF_DIFFUSE == DIFFUSE_CHAN
  //#error call different BRDF_diffuse, with NoH
  return 0;
#endif
}

float3 BRDF_diffuse( float3 diffuseColor, float linearRoughness, float NoV, float NoL, float VoH, float NoH )
{
#if   BRDF_DIFFUSE == DIFFUSE_LAMBERT
  return diffuseLambert( diffuseColor );
#elif BRDF_DIFFUSE == DIFFUSE_OREN_NAYAR
  return diffuseOrenNayar( diffuseColor, linearRoughness, NoV, NoL, VoH );
#elif BRDF_DIFFUSE == DIFFUSE_BURLEY
  return diffuseBurley( diffuseColor, linearRoughness, NoV, NoL, VoH );
#elif BRDF_DIFFUSE == DIFFUSE_BURLEY_FIXED
  return diffuseBurleyFixed( diffuseColor, linearRoughness, NoV, NoL, VoH );
#elif BRDF_DIFFUSE == DIFFUSE_CHAN
  return diffuseChan( diffuseColor, linearRoughness*linearRoughness, NoV, NoL, VoH, NoH);
#endif
}

float BRDF_distribution( float ggx_alpha, float NoH )
{
#if   BRDF_SPEC_D == SPEC_D_BLINN
  return distributionBlinn( ggx_alpha, NoH );
#elif BRDF_SPEC_D == SPEC_D_BECKMANN
  return distributionBeckmann( ggx_alpha, NoH );
#elif BRDF_SPEC_D == SPEC_D_GGX
  return distributionGGX( ggx_alpha, NoH );
#endif
}

// Vis = G / (4*NoL*NoV)
float BRDF_geometricVisibility( float ggx_alpha, float NoV, float NoL, float VoH )
{
#if   BRDF_SPEC_G == SPEC_G_IMPLICIT
  return geometryImplicit();
#elif BRDF_SPEC_G == SPEC_G_NEUMANN
  return geometryNeumann( NoV, NoL );
#elif BRDF_SPEC_G == SPEC_G_KELEMEN
  return geometryKelemen( VoH );
#elif BRDF_SPEC_G == SPEC_G_SHLICK
  return geometrySchlick( ggx_alpha, NoV, NoL );
#elif BRDF_SPEC_G == SPEC_G_SMITH_GGX
  return geometrySmith( ggx_alpha, NoV, NoL );
#elif BRDF_SPEC_G == SPEC_G_SMITH_CORRELATED
  return geometrySmithCorrelated( ggx_alpha, NoV, NoL );
#elif BRDF_SPEC_G == SPEC_G_SMITH_CORRELATED_APPROX
  return geometrySmithCorrelatedApprox( ggx_alpha, NoV, NoL );
#endif
}

float3 BRDF_fresnel( float3 specularColor, float VoH )
{
#if   BRDF_SPEC_F == 0
  return fresnelNone( specularColor );
#elif BRDF_SPEC_F == 1
  return fresnelSchlick( specularColor, VoH );
#elif BRDF_SPEC_F == 2
  return fresnelFresnel( specularColor, VoH );
#endif
}

float3 BRDF_specular(float ggx_alpha, float NoV, float NoL, float VoH, float NoH, half sheenStrength, half3 sheenColor)
{
  float D = BRDF_distribution( ggx_alpha, NoH );
  float G = BRDF_geometricVisibility( ggx_alpha, NoV, NoL, VoH );
  float3 result = D*G;
  #if SHEEN_SPECULAR
    if (sheenStrength > 0)
    {
      float clothD = distributionCloth( ggx_alpha, NoH );
      float clothG = geometryCloth( NoV, NoL );
      result = lerp(result,sheenColor*(clothD*clothG), sheenStrength);
    }
  #endif
  return result;
}

half foliageSSSBackDiffuseAmount(half NoL)
{
  half LdotNBack = -NoL;
  // Energy conserving wrap
  half diffuseWrap = .6h;
  half backDiffuse = saturate(LdotNBack * (diffuseWrap * diffuseWrap) + (1.h - diffuseWrap) * diffuseWrap);//division by PI omitted intentionally, lightColor is divided by Pi
  return backDiffuse;
}

half foliageSSS(half NoL, half3 view, half3 lightDir)
{
  half EdotL = saturate(dot(view, -lightDir));
  half PowEdotL = pow4h(EdotL);
  //float exponent = 8;
  //PowEdotL = PowEdotL * (exponent + 1) / (2.0f * PIh);// Modified phong energy conservation.
  half exponent = 8.h;
  PowEdotL = PowEdotL * (exponent + 1.h)/(2.0h * PIh);

  // Energy conserving wrap
  half viewDependenceAmount = .5h;
  half backShading = lerp(foliageSSSBackDiffuseAmount(NoL), PowEdotL, viewDependenceAmount);
  return backShading;
}

half foliageSSSfast(half NoL)
{
  //leave only backlight component
  half viewDependenceAmount = .5h;
  return foliageSSSBackDiffuseAmount(NoL) * viewDependenceAmount;
}
#endif
