  half lightAngleScale = lightColor.a;
  half lightAngleOffset = lightDirection.a;

  half geomAttenuation; half3 dirFromLight, point2light;//point2light - not normalized
  spot_light_params(worldPos.xyz, lightPosRadius, lightDirection.xyz, lightAngleScale, lightAngleOffset, geomAttenuation, dirFromLight, point2light);

  const float inFrontOfIlluminatingPlane = saturate(sign(dot(-point2light, lightDirection.xyz) - illuminatingPlaneOffset));
  geomAttenuation *= inFrontOfIlluminatingPlane;

  half NoL = dot(half3(gbuffer.normal), dirFromLight);
  half attenuation = calc_micro_shadow(NoL, ao)*geomAttenuation;
  half ggx_alpha = max(1e-4, gbuffer.linearRoughness*gbuffer.linearRoughness);

  #if DYNAMIC_LIGHTS_EARLY_EXIT
    #if DYNAMIC_LIGHTS_SSS
      bool shouldExit = attenuation <= 0 || (!isSubSurfaceShader(gbuffer.material) && NoL<=0);
    #else
      attenuation = saturate(attenuation*NoL);
      bool shouldExit = attenuation == 0;
    #endif
    #if WAVE_INTRINSICS
      shouldExit = (bool)WaveReadFirstLane(WaveAllBitAnd(uint(shouldExit)));
    #endif
    BRANCH
    if (shouldExit)
      EXIT_STATEMENT;
  #else
    #if !DYNAMIC_LIGHTS_SSS
      attenuation = saturate(attenuation*NoL);
    #endif
  #endif

  half spotShadow = 1;
  #if SPOT_SHADOWS || defined(SPOT_CONTACT_SHADOWS_CALC)
  float zbias = shadowZBias + shadowSlopeZBias / (abs(NoL)+0.1);
  float4x4 spotLightTm = getSpotLightTm(spot_light_index);
  float4 lightShadowTC = mul(spotLightTm, float4(worldPos.xyz+(point2light+dirFromLight)*zbias, 1));
  #endif

  #if !RT_DYNAMIC_LIGHTS && (SPOT_SHADOWS || defined(SPOT_CONTACT_SHADOWS_CALC))
    if (lightShadowTC.w > 1e-6)
    {
      lightShadowTC.xyz /= lightShadowTC.w;
      #if SPOT_SHADOWS
        #ifdef SIMPLE_PCF_SHADOW
          spotShadow = 1-dynamic_shadow_sample(lightShadowTC.xy, lightShadowTC.z);
        #else
          #ifndef shadow_frame
          float shadow_frame = 0;
          #endif
          spotShadow = 1-dynamic_shadow_sample_8tap(screenpos, lightShadowTC.xy, lightShadowTC.z, 1.5*shadowAtlasTexel.x*(0.75+saturate(0.3*length(point2light))), shadow_frame);
        #endif
      #endif
      #ifdef SPOT_CONTACT_SHADOWS_CALC
        SPOT_CONTACT_SHADOWS_CALC
      #endif
    }
    attenuation *= spotShadow;
  #endif

  #if DYNAMIC_LIGHTS_SSS
    NoL = saturate(NoL);

    half3 lightBRDF = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, dynamicLightsSpecularStrength, dirFromLight, view, gbuffer.normal, gbuffer.translucencyColor, gbuffer.sheen);

    #if USE_SSSS && SPOT_SHADOWS
      BRANCH if (gbuffer.material == SHADING_SUBSURFACE)
      {
        SpotlightShadowDescriptor spotlightDesc = spot_lights_ssss_shadow_desc[spot_light_index];
        BRANCH if (lightShadowTC.w > 1e-6 && spotlightDesc.hasDynamic)
        {
          float4 ssssShadowTC = mul(spotLightTm, float4(ssssWorldPos, 1));
          ssssShadowTC /= ssssShadowTC.w;
          ShadowDescriptor desc;
          desc.decodeDepth = spotlightDesc.decodeDepth;
          desc.meterToUvAtZfar = spotlightDesc.meterToUvAtZfar;
          desc.uvMinMax = spotlightDesc.uvMinMax;
          desc.shadowRange = lightPosRadius.w;
          float ssssTransmittance = ssss_get_transmittance_factor(
            gbuffer.translucency, tc, dynamic_light_shadows_smp, dynamic_light_shadows_size, ssssShadowTC.xyz, desc);
          result += gbuffer.diffuseColor * lightColor.rgb * ssss_get_profiled_transmittance(gbuffer.normal, dirFromLight, ssssTransmittance) * geomAttenuation;
        }
      }
      else
    #endif
    {
      BRANCH if (isSubSurfaceShader(gbuffer.material))
        lightBRDF += (foliageSSS(NoL, view, dirFromLight)*ao) * gbuffer.translucencyColor;//can make ao*gbuffer.translucencyColor only once for all lights
    }
  #else
    #if SHEEN_SPECULAR
      half3 lightBRDF = standardBRDF_NO_NOL( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, dynamicLightsSpecularStrength, dirFromLight, view, gbuffer.normal, gbuffer.translucencyColor, gbuffer.sheen);
    #else
      half3 lightBRDF = standardBRDF_NO_NOL( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, dynamicLightsSpecularStrength, dirFromLight, view, gbuffer.normal);
    #endif
  #endif
    attenuation = applyPhotometryIntensity(-dirFromLight, lightDirection.xyz, texId_scale.x,
                                            texId_scale.y, attenuation);
    lightBRDF *= attenuation * lightColor.xyz;
  #if WAVE_INTRINSICS || !DYNAMIC_LIGHTS_EARLY_EXIT
    FLATTEN
    if (attenuation <= 0)
      lightBRDF = 0;
  #endif
