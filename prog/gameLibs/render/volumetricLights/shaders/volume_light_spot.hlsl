
  #if defined(CHECK_SPOT_LIGHT_MASK)
    if ( !check_spot_light(spot_light_index))
      continue;
  #endif
  RenderSpotLight sl = spot_lights_cb[spot_light_index];
  float4 lightPosRadius = sl.lightPosRadius;
  float4 lightColor = sl.lightColorAngleScale;
  lightColor.w = abs(lightColor.w);
  float4 lightDirection = sl.lightDirectionAngleOffset;
  float2 texId_scale = sl.texId_scale.xy;

  float lightAngleScale = lightColor.a;
  float lightAngleOffset = lightDirection.a;
  half geomAttenuation; float3 dirFromLight, point2light;//point2light - not normalized
  spot_light_params(jitteredWorldPos.xyz, lightPosRadius, lightDirection.xyz, lightAngleScale, lightAngleOffset, geomAttenuation, dirFromLight, point2light);
  half attenuation = geomAttenuation;

  BRANCH
  if (attenuation <= 0)
    continue;

  half spotShadow = 1;
  #if SPOT_SHADOWS
    float4 lightShadowTC = mul(getSpotLightTm(spot_light_index), float4(jitteredWorldPos.xyz, 1));
    //float4 lightShadowTC = mul(getSpotLightTm(spot_light_index/3), float4(worldPos.xyz, 1));
    if (lightShadowTC.w > 1e-6)
    {
      lightShadowTC.xyz /= lightShadowTC.w;
      spotShadow = 1-dynamic_shadow_sample(lightShadowTC.xy, lightShadowTC.z);
    }
    attenuation *= spotShadow;
  #endif
  float phase = phaseSchlickTwoLobe(dot(dirFromLight, jitteredView), mieG0*1.5, mieG1*1.5);//shlick phase can result in temporal changes, but overall looking better
  float intensity = applyPhotometryIntensity(-dirFromLight, lightDirection.xyz, texId_scale.x,
                                             texId_scale.y, phase *attenuation *lightsEffect);
  dynamicLighting += intensity * lightColor.xyz;
