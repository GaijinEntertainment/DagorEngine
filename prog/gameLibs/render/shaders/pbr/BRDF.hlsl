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

half3 BRDF_diffuse( half3 diffuseColor, half linearRoughness, half NoV, half NoL, half VoH )
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

half3 BRDF_diffuse( half3 diffuseColor, half linearRoughness, half NoV, half NoL, half VoH, half NoH )
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

half BRDF_distribution( half ggx_alpha, half NoH )
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
half BRDF_geometricVisibility( half ggx_alpha, half NoV, half NoL, half VoH )
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

half3 BRDF_fresnel( half3 specularColor, half VoH )
{
#if   BRDF_SPEC_F == 0
  return fresnelNone( specularColor );
#elif BRDF_SPEC_F == 1
  return fresnelSchlick( specularColor, VoH );
#elif BRDF_SPEC_F == 2
  return fresnelFresnel( specularColor, VoH );
#endif
}

half3 BRDF_specular(half ggx_alpha, half NoV, half NoL, half VoH, half NoH, half sheenStrength, half3 sheenColor)
{
  half D = BRDF_distribution( ggx_alpha, NoH );
  half G = BRDF_geometricVisibility( ggx_alpha, NoV, NoL, VoH );
  half3 result = D*G;
  // limit result to avoid infinite values
  result = min( result, 10000.h);
  #if SHEEN_SPECULAR
    if (sheenStrength > 0)
    {
      half clothD = distributionCloth( ggx_alpha, NoH );
      half clothG = geometryCloth( NoV, NoL );
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
  half PowEdotL = pow4(EdotL);
  //half exponent = 8;
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
