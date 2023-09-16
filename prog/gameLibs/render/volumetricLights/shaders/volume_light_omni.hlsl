
  #if defined(CHECK_OMNI_LIGHT_MASK)
    if ( !check_omni_light(omni_light_index))
      continue;
  #endif
  RenderOmniLight ol = omni_lights_cb[omni_light_index];
  float4 pos_and_radius = ol.posRadius;
  float4 color_and_specular = getFinalColor(ol, worldPos);
  float3 point2light = pos_and_radius.xyz-worldPos.xyz;
  float distSqFromLight = dot(point2light,point2light);
  float radius2 = pow2(pos_and_radius.w);
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  BRANCH
  if (distSqFromLight >= radius2)
    continue;
  #endif
  float invSqrRad = rcp(radius2);
  float attenuation = getDistanceAtt( distSqFromLight, invSqrRad );

#if OMNI_SHADOWS
  attenuation *= getOmniShadow(getOmniLightShadowData(omni_light_index), pos_and_radius, worldPos, 1, float2(0, 0));
#endif

  dynamicLighting += (ambPhase*attenuation*lightsEffect)*color_and_specular.xyz;//ambient phase. shlick phase is heavier and will cause to much temporal difference
