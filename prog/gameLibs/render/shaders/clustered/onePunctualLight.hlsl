  float3 point2light = pos_and_radius.xyz-worldPos.xyz;
  float distSqFromLight = dot(point2light,point2light);
  float ggx_alpha = max(1e-4, pow2(gbuffer.linearRoughness));

#if LAMBERT_LIGHT
  float radius2 = pow2(pos_and_radius.w);
  #if DYNAMIC_LIGHTS_EARLY_EXIT
  bool shouldExit = distSqFromLight >= radius2;
  #if WAVE_INTRINSICS
    shouldExit = (bool)WaveReadFirstLane(WaveAllBitAnd(uint(shouldExit)));
  #endif
  BRANCH
  if (shouldExit)
    EXIT_STATEMENT;
  #endif
  float invSqrRad = rcp(radius2);
  float attenuation = getDistanceAtt( distSqFromLight, invSqrRad );

  float3 lightDir = point2light*rsqrt(0.0000001+distSqFromLight);
  float NoL = saturate(dot(gbuffer.normal, lightDir));
  half shadowTerm = attenuation;//no shadows
  shadowTerm *= calc_micro_shadow(NoL, ao);

  #if OMNI_SHADOWS
    shadowTerm *= getOmniShadow(shadowTcToAtlas, pos_and_radius, worldPos, NoL, screenpos);
  #endif

  #if !DYNAMIC_LIGHTS_SPECULAR
  half3 lightBRDF = diffuseLambert(gbuffer.diffuseColor)*(NoL*shadowTerm)*color_and_attenuation.xyz;
  #else
  half3 diffuse = diffuseLambert(gbuffer.diffuseColor);

  float3 H = normalize(view + lightDir);
  float NoH = saturate( dot(gbuffer.normal, H) );
  float VoH = saturate( dot(view, H) );
  float D = BRDF_distribution( ggx_alpha, NoH )*dynamicLightsSpecularStrength;
  float G = NoL > 0 ? BRDF_geometricVisibility( ggx_alpha, NoV, NoL, VoH ) : 0;
  float3 F = BRDF_fresnel( specularColor, VoH );
  half3 result = (diffuse + (D*G)*F)*NoL;
  #if DYNAMIC_LIGHTS_SSS
  if (isSubSurfaceShader(gbuffer.material))
    result += (foliageSSS(NoL, view, lightDir)*gbuffer.ao) * gbuffer.translucencyColor;//can make gbuffer.ao*gbuffer.translucencyColor only once for all lights
  #endif
  half3 lightBRDF = result * shadowTerm * color_and_attenuation.xyz;
  #endif
#else
  float NoL = dot(gbuffer.normal, point2light);
  float invSqrRad = rcp(pow2(pos_and_radius.w));
  float attenuation = getDistanceAtt( distSqFromLight, invSqrRad );
  float rcpDistFromLight = rsqrt(0.0000001+distSqFromLight);
  NoL *= rcpDistFromLight;
  attenuation *= calc_micro_shadow(NoL, ao);

  #if DYNAMIC_LIGHTS_EARLY_EXIT
  bool shouldExit = min(attenuation, NoL) <= 0;
  #if WAVE_INTRINSICS
    shouldExit = (bool)WaveReadFirstLane(WaveAllBitAnd(uint(shouldExit)));
  #endif
  BRANCH
  if (shouldExit)
    EXIT_STATEMENT;
  #endif

  #if OMNI_SHADOWS
    attenuation *= getOmniShadow(shadowTcToAtlas, pos_and_radius, worldPos, NoL, screenpos);
  #endif

  float3 lightDir = point2light*rcpDistFromLight;
  half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, dynamicLightsSpecularStrength, lightDir, view, gbuffer.normal, gbuffer.translucencyColor, gbuffer.translucency);
  #if !DYNAMIC_LIGHTS_EARLY_EXIT
  result = NoL>0 ? result : 0;
  #endif
  #if DYNAMIC_LIGHTS_SSS
  if (isSubSurfaceShader(gbuffer.material))
    result += (foliageSSS(NoL, view, lightDir)*gbuffer.ao) * gbuffer.translucencyColor;//can make gbuffer.ao*gbuffer.translucencyColor only once for all lights
  #endif
  half3 lightBRDF = result*attenuation*color_and_attenuation.xyz;
#endif
