#include "BRDF.hlsl"
#include "microShadow.hlsl"

#ifndef __PBR_COMMON__
#define __PBR_COMMON__
  //from http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr_v2.pdf (original version had bug in code!)
  float computeSpecOcclusion ( float saturated_NdotV, float AO, float ggx_alpha)
    { return saturate(pow( saturated_NdotV + AO, exp2(-16.0*ggx_alpha- 1.0)) - 1 + AO);}
  //ggx_alpha = linearRoughness*linearRoughness
  half3 standardBRDF_NO_NOL( float NoV, float NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, float3 lightDir, float3 view, half3 normal )
  {
    #if SPECULAR_DISABLED && BRDF_DIFFUSE == DIFFUSE_LAMBERT
      return diffuseLambert( baseDiffuseColor );
    #else
    float3 H = normalize(view + lightDir);
    float NoH = saturate( dot(normal, H) );
    float VoH = saturate( dot(view, H) );
    half3 diffuse = BRDF_diffuse( baseDiffuseColor, linearRoughness, NoV, NoL, VoH, NoH );
    #if !SPECULAR_DISABLED
    // Generalized microfacet specular
    float D = BRDF_distribution( ggx_alpha, NoH )*specularStrength;
    float G = BRDF_geometricVisibility( ggx_alpha, NoV, NoL, VoH );
    //float G = geometryCookTorrance(NoH, NoL, NoV, VoH, ggx_alpha);
    float3 F = BRDF_fresnel( specularColor, VoH );
    //return (diffuse + D*G*F)*NoL;
    //float maxSpecular = 256.0;
    //return diffuse*NoL + min(D*G*NoL, maxSpecular)*F;
    return (diffuse + D*G*F);

    #else

    return diffuse;
    
    #endif

    #endif
  }
  half3 standardBRDF( float NoV, float NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, float3 lightDir, float3 view, half3 normal )
  {
    return standardBRDF_NO_NOL( NoV, NoL, baseDiffuseColor, ggx_alpha, linearRoughness, specularColor, specularStrength, lightDir, view, normal ) * NoL;
  }
#endif