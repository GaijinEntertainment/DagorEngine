#include "BRDF.hlsl"
#include "microShadow.hlsl"

#ifndef __PBR_COMMON__
#define __PBR_COMMON__
  //from http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr_v2.pdf (original version had bug in code!)
  float computeSpecOcclusion ( float saturated_NdotV, float AO, float ggx_alpha)
    { return saturate(pow( saturated_NdotV + AO, exp2(-16.0*ggx_alpha- 1.0)) - 1 + AO);}
  //ggx_alpha = linearRoughness*linearRoughness
  half3 standardBRDF_NO_NOL(half NoV, half NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, half3 lightDir, half3 view, half3 normal,  half3 sheenColor, half translucency)
  {
    #if SPECULAR_DISABLED && BRDF_DIFFUSE == DIFFUSE_LAMBERT
      return diffuseLambert( baseDiffuseColor );
    #else
      half3 H = normalize(view + lightDir);
      half NoH = saturate( dot(normal, H) );
      half VoH = saturate( dot(view, H) );
      half3 diffuse = (half3)BRDF_diffuse( baseDiffuseColor, linearRoughness, NoV, NoL, VoH );
      #if !SPECULAR_DISABLED
        half3 specular = BRDF_specular( ggx_alpha, NoV, NoL, VoH, NoH, translucency, sheenColor) * specularStrength;
        half3 F = BRDF_fresnel( specularColor, VoH );
        return diffuse + F*specular;
      #else
        return diffuse;
      #endif
    #endif
  }

  half3 standardBRDF(half NoV, half NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, half3 lightDir, half3 view, half3 normal, half3 sheenColor, half translucency)
  {
    return standardBRDF_NO_NOL(NoV, NoL, baseDiffuseColor, ggx_alpha, linearRoughness, specularColor, specularStrength, lightDir, view, normal, sheenColor, translucency) * NoL;
  }

  half3 standardBRDF_NO_NOL(half NoV, half NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, half3 lightDir, half3 view, half3 normal)
  {
    return standardBRDF_NO_NOL(NoV, NoL, baseDiffuseColor, ggx_alpha, linearRoughness, specularColor, specularStrength, lightDir, view, normal, 0, 0);
  }

  half3 standardBRDF(half NoV, half NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, half3 lightDir, half3 view, half3 normal)
  {
    return standardBRDF_NO_NOL(NoV, NoL, baseDiffuseColor, ggx_alpha, linearRoughness, specularColor, specularStrength, lightDir, view, normal, 0, 0) * NoL;
  }
#endif