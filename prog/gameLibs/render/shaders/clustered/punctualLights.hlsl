#include <clustered/punctualLightsMath.hlsl>

half3 perform_point_light(float3 worldPos, half3 view, half NoV, ProcessedGbuffer gbuffer, half3 specularColor, half dynamicLightsSpecularStrength, half ao, float4 pos_and_radius, half4 color_and_attenuation, half4 shadowTcToAtlas, half2 screenpos)
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

half3 perform_light_brdf(half3 view, half NoV, ProcessedGbuffer gbuffer, half3 specularColor, half specularStrength, half ao, half3 light_color, half NoL, half attenuation, half3 dirFromLight)
{
  #if DYNAMIC_LIGHTS_SSS
    BRANCH
    if (attenuation <= 0 || (!isSubSurfaceShader(gbuffer.material) && NoL<=0) )
      return 0;

    half ggx_alpha = max(1e-4h, gbuffer.linearRoughness*gbuffer.linearRoughness);
    #if SHEEN_SPECULAR
      half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, specularStrength, dirFromLight, view, half3(gbuffer.normal), gbuffer.translucencyColor, gbuffer.sheen);
    #else
      half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, specularStrength, dirFromLight, view, half3(gbuffer.normal));
    #endif


    if (isSubSurfaceShader(gbuffer.material))
      result += (foliageSSS(NoL, view, dirFromLight)*gbuffer.ao) * gbuffer.translucencyColor;//can make gbuffer.ao*gbuffer.translucencyColor only once for all lights
  #else
    attenuation = saturate(attenuation*NoL);
    BRANCH
    if (attenuation <= 0)
      return 0;
    half ggx_alpha = max(1e-4h, gbuffer.linearRoughness*gbuffer.linearRoughness);
    #if SHEEN_SPECULAR
      half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, specularStrength, dirFromLight, view, half3(gbuffer.normal), gbuffer.translucencyColor, gbuffer.sheen);
    #else
      half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, specularStrength, dirFromLight, view, half3(gbuffer.normal));
    #endif

  #endif
  return result*attenuation*light_color.xyz;
}

half3 perform_spot_light(float3 worldPos, half3 view, half NoV, ProcessedGbuffer gbuffer, half3 specularColor, half specularStrength, half ao, float4 pos_and_radius, half4 color_and_attenuation, half3 light_direction, half lightAngleScale, half lightAngleOffset)
{
  half geomAttenuation; half3 dirFromLight, point2light;
  spot_light_params(worldPos, pos_and_radius, light_direction, lightAngleScale, lightAngleOffset, geomAttenuation, dirFromLight, point2light);
  half NoL = dot(half3(gbuffer.normal), dirFromLight);
  half attenuation = calc_micro_shadow(NoL, ao)*geomAttenuation;
  //attenuation *= projector();
  //attenuation *= shadow();
  return perform_light_brdf(view, NoV, gbuffer, specularColor, specularStrength, ao, color_and_attenuation.xyz, NoL, attenuation, dirFromLight);
}

half3 perform_sphere_light(float3 worldPos, half3 view, half NoV, ProcessedGbuffer gbuffer, half3 specularColor, half specularStrength, half ao, float4 pos_and_radius, half4 color_and_attenuation)
{
  half3 point2light = half3(pos_and_radius.xyz-worldPos.xyz);
  half distSqFromLight = dot(point2light,point2light);
  half radius = half(pos_and_radius.w);
  half radius2 = pow2(radius);
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  BRANCH
  if (distSqFromLight >= radius2)
    return 0;
  #endif
  half3 lightDir = point2light*rsqrt(0.0000001h+distSqFromLight);

  half invSqrRad = rcp(radius2);
  half attenuation = illuminanceSphereAttenuation (half3(gbuffer.normal), lightDir, color_and_attenuation.a, distSqFromLight);
  attenuation *= smoothDistanceAtt ( distSqFromLight, invSqrRad );

  half shadowTerm = attenuation;//no shadows
  half3 diffuse = diffuseLambert(gbuffer.diffuseColor);
  half ggx_alpha = max(1e-4h, gbuffer.linearRoughness*gbuffer.linearRoughness);
  #if !DYNAMIC_LIGHTS_SPECULAR
    //return 0;
    return diffuse*(shadowTerm)*color_and_attenuation.xyz;
  #else
    half4 specularLightVec = SphereAreaLightIntersection( half3(gbuffer.normal), view, lightDir, ggx_alpha, color_and_attenuation.a);
    shadowTerm *= lerp(calc_micro_shadow(abs(dot(half3(gbuffer.normal.xyz), specularLightVec.xyz)), ao), 1, saturate(pow2(color_and_attenuation.a)/distSqFromLight-1));
    //half3 AreaLightGGX(half3 N, half3 V, half3 L, half ggxRoughness, half3 SpecCol, half lightSize)
    half3 H = normalize(view + specularLightVec.xyz);
    half NoH = saturate( dot(half3(gbuffer.normal), H) );
    half VoH = saturate( dot(view, H) );
    half sNoL = saturate(dot(specularLightVec.xyz, half3(gbuffer.normal)));
    half D = BRDF_distribution( ggx_alpha, NoH )*specularStrength*specularLightVec.w;
    half G = sNoL > 0 ? BRDF_geometricVisibility( ggx_alpha, NoV, sNoL, VoH ) : 0;
    half3 F = BRDF_fresnel( specularColor, VoH );
    return (diffuse + (D*G)*F)*(shadowTerm)*color_and_attenuation.xyz;
  #endif
}
