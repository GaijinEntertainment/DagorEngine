#ifndef OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
  #define OMNI_DYNAMIC_LIGHTS_EARLY_EXIT DYNAMIC_LIGHTS_EARLY_EXIT
  #define UNDEF_OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
#endif
  half3 point2light = half3(pos_and_radius.xyz-worldPos.xyz);
  half distSqFromLight = dot(point2light,point2light);
  half ggx_alpha = max(1e-4h, pow2(gbuffer.linearRoughness));
  half radius = half(pos_and_radius.w);

#if LAMBERT_LIGHT
  half radius2 = pow2(radius);
  #if OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
  bool shouldExit = distSqFromLight >= radius2;
  #if WAVE_INTRINSICS
    shouldExit = (bool)WaveReadFirstLane(WaveAllBitAnd(uint(shouldExit)));
  #endif
  BRANCH
  if (shouldExit)
    EXIT_STATEMENT;
  #endif
  half invSqrRad = rcp(radius2);
  half attenuation = getDistanceAtt( distSqFromLight, invSqrRad );

  half3 lightDir = point2light*rsqrt(0.0000001h+distSqFromLight);
  half NoL = saturate(dot(half3(gbuffer.normal), lightDir));
  half shadowTerm = attenuation;//no shadows
  shadowTerm *= calc_micro_shadow(NoL, ao);

  #if OMNI_SHADOWS
    shadowTerm *= getOmniShadow(shadowTcToAtlas, pos_and_radius, worldPos, NoL, screenpos);
  #endif

  #if !DYNAMIC_LIGHTS_SPECULAR
  half3 lightBRDF = diffuseLambert(gbuffer.diffuseColor)*(NoL*shadowTerm)*color_and_attenuation.xyz;
  #else
  half3 diffuse = diffuseLambert(gbuffer.diffuseColor);

  half3 H = normalize(view + lightDir);
  half NoH = saturate( dot(half3(gbuffer.normal), H) );
  half VoH = saturate( dot(view, H) );
  half D = BRDF_distribution( ggx_alpha, NoH )*dynamicLightsSpecularStrength;
  half G = NoL > 0 ? BRDF_geometricVisibility( ggx_alpha, NoV, NoL, VoH ) : 0;
  half3 F = BRDF_fresnel( specularColor, VoH );
  half3 result = (diffuse + (D*G)*F)*NoL;
  #if DYNAMIC_LIGHTS_SSS
  if (isSubSurfaceShader(gbuffer.material))
    result += (foliageSSS(NoL, view, lightDir)*gbuffer.ao) * gbuffer.translucencyColor;//can make gbuffer.ao*gbuffer.translucencyColor only once for all lights
  #endif
  half3 lightBRDF = result * shadowTerm * color_and_attenuation.xyz;
  #endif
#else
  half NoL = dot(half3(gbuffer.normal), point2light);
  half invSqrRad = rcp(pow2(radius));
  half attenuation = getDistanceAtt( distSqFromLight, invSqrRad );
  half rcpDistFromLight = rsqrt(0.0000001h+distSqFromLight);
  NoL *= rcpDistFromLight;
  attenuation *= calc_micro_shadow(NoL, ao);

  #if OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
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

  half3 lightDir = point2light*rcpDistFromLight;
  #if SHEEN_SPECULAR
    half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, dynamicLightsSpecularStrength, lightDir, view, half3(gbuffer.normal), gbuffer.translucencyColor, gbuffer.sheen);
  #else
    half3 result = standardBRDF( NoV, NoL, gbuffer.diffuseColor, ggx_alpha, gbuffer.linearRoughness, specularColor, dynamicLightsSpecularStrength, lightDir, view, half3(gbuffer.normal));
  #endif

  #if !DYNAMIC_LIGHTS_EARLY_EXIT
  result = NoL>0 ? result : 0;
  #endif
  #if DYNAMIC_LIGHTS_SSS
  if (isSubSurfaceShader(gbuffer.material))
    result += (foliageSSS(NoL, view, lightDir)*gbuffer.ao) * gbuffer.translucencyColor;//can make gbuffer.ao*gbuffer.translucencyColor only once for all lights
  #endif
  half3 lightBRDF = result*attenuation*color_and_attenuation.xyz;
#endif

#ifdef UNDEF_OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
  #undef OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
  #undef UNDEF_OMNI_DYNAMIC_LIGHTS_EARLY_EXIT
#endif
