#include <clustered/punctualLightsMath.hlsl>

half3 perform_point_light(float3 worldPos, float3 view, float NoV, ProcessedGbuffer gbuffer, half3 specularColor, half dynamicLightsSpecularStrength, half ao, float4 pos_and_radius, float4 color_and_attenuation, float4 shadowTcToAtlas, float2 screenpos)
{
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  #define EXIT_STATEMENT return 0
  #endif
  #include <clustered/onePunctualLight.hlsl>
  return lightBRDF;
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  #undef EXIT_STATEMENT
  #endif
}

half3 perform_light_brdf(float3 view, float NoV, ProcessedGbuffer gbuffer, half3 specularColor, half specularStrength, half ao, float3 light_color, float NoL, half attenuation, float3 dirFromLight)
{
  #if DYNAMIC_LIGHTS_SSS
    BRANCH
    if (attenuation <= 0 || (!isSubSurfaceShader(gbuffer.material) && NoL<=0) )
      return 0;

    float ggx_alpha = max(1e-4, gbuffer.linearRoughness*gbuffer.linearRoughness);
    half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, specularStrength, dirFromLight, view, gbuffer.normal);

    if (isSubSurfaceShader(gbuffer.material))
      result += (foliageSSS(NoL, view, dirFromLight)*gbuffer.ao) * gbuffer.translucencyColor;//can make gbuffer.ao*gbuffer.translucencyColor only once for all lights
  #else
    attenuation = saturate(attenuation*NoL);
    BRANCH
    if (attenuation <= 0)
      return 0;
    float ggx_alpha = max(1e-4, gbuffer.linearRoughness*gbuffer.linearRoughness);
    half3 result = standardBRDF_NO_NOL( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, specularStrength, dirFromLight, view, gbuffer.normal);
  #endif
  return result*attenuation*light_color.xyz;
}

half3 perform_spot_light(float3 worldPos, float3 view, float NoV, ProcessedGbuffer gbuffer, half3 specularColor, half specularStrength, half ao, float4 pos_and_radius, float4 color_and_attenuation, float3 light_direction, float lightAngleScale, float lightAngleOffset)
{
  half geomAttenuation; float3 dirFromLight, point2light;
  spot_light_params(worldPos, pos_and_radius, light_direction, lightAngleScale, lightAngleOffset, geomAttenuation, dirFromLight, point2light);
  float NoL = dot(gbuffer.normal, dirFromLight);
  half attenuation = calc_micro_shadow(NoL, ao)*geomAttenuation;
  //attenuation *= projector();
  //attenuation *= shadow();
  return perform_light_brdf(view, NoV, gbuffer, specularColor, specularStrength, ao, color_and_attenuation.xyz, NoL, attenuation, dirFromLight);
}

half3 perform_sphere_light(float3 worldPos, float3 view, float NoV, ProcessedGbuffer gbuffer, half3 specularColor, half specularStrength, half ao, float4 pos_and_radius, float4 color_and_attenuation)
{
  float3 point2light = pos_and_radius.xyz-worldPos.xyz;
  float distSqFromLight = dot(point2light,point2light);
  float radius2 = pow2(pos_and_radius.w);
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  BRANCH
  if (distSqFromLight >= radius2)
    return 0;
  #endif
  float3 lightDir = point2light*rsqrt(0.0000001+distSqFromLight);

  float invSqrRad = rcp(radius2);
  float attenuation = illuminanceSphereAttenuation (gbuffer.normal, lightDir, color_and_attenuation.a, distSqFromLight);
  attenuation *= smoothDistanceAtt ( distSqFromLight, invSqrRad );

  half shadowTerm = attenuation;//no shadows
  half3 diffuse = diffuseLambert(gbuffer.diffuseColor);
  float ggx_alpha = max(1e-4, gbuffer.linearRoughness*gbuffer.linearRoughness);
  #if !DYNAMIC_LIGHTS_SPECULAR
    //return 0;
    return diffuse*(shadowTerm)*color_and_attenuation.xyz;
  #else
    half4 specularLightVec = SphereAreaLightIntersection( gbuffer.normal, view, lightDir, ggx_alpha, color_and_attenuation.a);
    shadowTerm *= lerp(calc_micro_shadow(abs(dot(gbuffer.normal.xyz, specularLightVec.xyz)), ao), 1, saturate(pow2(color_and_attenuation.a)/distSqFromLight-1));
    //half3 AreaLightGGX(half3 N, half3 V, half3 L, half ggxRoughness, half3 SpecCol, half lightSize)
    float3 H = normalize(view + specularLightVec.xyz);
    float NoH = saturate( dot(gbuffer.normal, H) );
    float VoH = saturate( dot(view, H) );
    float sNoL = saturate(dot(specularLightVec.xyz, gbuffer.normal));
    float D = BRDF_distribution( ggx_alpha, NoH )*specularStrength*specularLightVec.w;
    float G = sNoL > 0 ? BRDF_geometricVisibility( ggx_alpha, NoV, sNoL, VoH ) : 0;
    float3 F = BRDF_fresnel( specularColor, VoH );
    return (diffuse + (D*G)*F)*(shadowTerm)*color_and_attenuation.xyz;
  #endif
}
