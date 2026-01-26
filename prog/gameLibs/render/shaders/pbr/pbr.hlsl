#include "BRDF.hlsl"
#include "microShadow.hlsl"

#ifndef __PBR_COMMON__
#define __PBR_COMMON__
  //from http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr_v2.pdf (original version had bug in code!)
  float computeSpecOcclusion ( float saturated_NdotV, float AO, float ggx_alpha)
    { 
      return saturate(pow( saturated_NdotV + AO, exp2(-16.0*ggx_alpha- 1.0)) - 1 + AO);
    }
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
  
  //add toon BRDF

  half RoughnessToToonRange(half Roughness, half NoH)
  {
    // Offset roughness to ensure even low roughness has some visibility
    half R = Roughness + 0.3;
    
    // Calculate the width of the smoothstep transition (softness of the edge)
    half w = 0.05 * R + 0.01;
    
    // Calculate the size/threshold of the highlight
    half R2 = (R * R) * w;
    
    // Map NoH to the toon ramp
    // As NoH approaches 1.0 (center of highlight), we step from 0 to 1
    return smoothstep(1.0 - w - R2, 1.0 - w + R2, NoH);
  }

  half3 toonBRDF(half NoV, half NoL, half3 baseDiffuseColor, half ggx_alpha, half linearRoughness, half3 specularColor, half specularStrength, half3 lightDir, half3 view, half3 normal)
  {
     // 1. Calculate Half Vector (Essential for Specular)
    half3 H = normalize(view + lightDir);
    half NoH = saturate(dot(normal, H));

    
    // 2. Stylized Diffuse (Cell Shading Ramp)
    half diffThreshold = 0.5; 
    half diffSmoothness = 0.05;
    half toonRamp = smoothstep(diffThreshold - diffSmoothness, diffThreshold + diffSmoothness, NoL);
    half3 toonDiffuse = diffuseLambert(baseDiffuseColor) * (toonRamp * 0.9 + 0.1); 

    #if SPECULAR_DISABLED
        return toonDiffuse;
    #else
       // --- SPECULAR ---
        // Use the custom function to determine the highlight shape
        half toonShape = RoughnessToToonRange(linearRoughness, NoH);

        // Apply Fresnel (NoV) to Stylize the Strength
        // Highlights become stronger/brighter at glancing angles
        half fresnelTerm = BRDF_fresnel(specularColor, NoV).r; // Using .r for scalar strength or standard fresnel approx
        // Alternatively simple fresnel: half fresnelTerm = pow(1.0 - NoV, 4.0);
        
        // Combine: Color * Shape * Strength * (Optional Fresnel Boost)
        // We clamp the fresnel influence so it doesn't disappear entirely at face-on angles
        half3 finalSpecular = specularColor * toonShape * specularStrength * max(0.5, fresnelTerm * 2.0);

        return toonDiffuse + finalSpecular;
    #endif
  }
#endif