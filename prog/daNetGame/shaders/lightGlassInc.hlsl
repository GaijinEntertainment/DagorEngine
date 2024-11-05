    half linearRoughness = linearSmoothnessToLinearRoughness(smoothness);
    half ggx_alpha = max(1e-4, linearRoughness*linearRoughness);
    float3 cameraToPoint = -input.pointToEye.xyz;
    float3 worldPos = world_view_pos-input.pointToEye.xyz;

    float dist2 = dot(input.pointToEye.xyz, input.pointToEye.xyz);
    float rdist = rsqrt(dist2);
    float dist = dist2*rdist;
    float3 view = input.pointToEye.xyz*rdist;

    float NdotV = dot(view, worldNormal);
    float NoV = abs(NdotV)+1e-5;
  //sun
    half3 lightDir = -from_sun_direction.xyz;
    float3 H = normalize(view + lightDir);
    float NoH = saturate( dot(worldNormal, H) );
    float VoH = saturate( dot(view, H) );
    float NoL = dot(lightDir, worldNormal);
    half shadow = 1;
    #if HAVE_SHADOWS
      shadow *= clouds_shadow(worldPos);
      shadow *= getStaticShadow(worldPos);
    #endif
    #if USE_CSM_SHADOWS
      shadow *= getCSMShadow(input.pointToEye.xyz);
    #endif
    float shadowedNoL = shadow*NoL;
    float G = BRDF_geometricVisibility( ggx_alpha, NoV, NoL, VoH );
    float D = BRDF_distribution( ggx_alpha, NoH );
    float F = BRDF_fresnel( glassSpecular, VoH ).g;
    half3 diffuse = diffuseColor.rgb;//lambert is good when it is smooth anyway!
    float3 diffuseTerm = diffuse*abs(shadowedNoL)*sun_color_0;
    //float3 diffuseTerm = diffuse*phaseSchlick(abs(NoL), 0.999)*shadow;//should be phase function of sime kind
    float3 specularTerm = shadowedNoL > 0 ? (D*G*F*shadowedNoL)*sun_color_0 : 0;
    float3 reflectionVec = 2 * NdotV * worldNormal - view;

    //use perfect reflection for simplicity
    float roughnessMip = ComputeReflectionCaptureMipFromRoughness(linearRoughness);
    float4 indoorRefl__localWeight = use_indoor_probes(worldPos.xyz, worldNormal, reflectionVec, roughnessMip, dist);
    float3 reflectedEnvi = sample_envi_probe(float4(reflectionVec, roughnessMip)).rgb * indoorRefl__localWeight.w + indoorRefl__localWeight.rgb;
    float enviBRDF = EnvBRDFApproxNonmetal(linearRoughness, NoV);
    #ifdef FIXED_ENVI_BRDF
      enviBRDF = FIXED_ENVI_BRDF;
    #endif
    half3 enviLight = GetSkySHDiffuse(worldNormal);
    half3 ambient = enviLight;
    #if GLASS_USE_GI
      #if IS_DYNAMIC_GLASS
        bool normalFiltering = false;
      #else
        bool normalFiltering = true;
      #endif

      float noise = 0;//interleavedGradientNoise( screenpos.xy );
      half3 giSpecular = reflectedEnvi;

      if (get_directional_irradiance_radiance(ambient, giSpecular, world_view_pos, screenUV, input.clipPos.w, worldPos.xyz, worldNormal, reflectionVec, view, dist, normalFiltering ? input.normal : 0, noise))
      {
        ambient *= skylight_gi_weight_atten;
        giSpecular *= skylight_gi_weight_atten;
        BRANCH
        if (linearRoughness < 0.5)
        {
          float4 indoorRefl__localWeight_blurred = use_indoor_probes(worldPos.xyz, worldNormal, reflectionVec, 4, dist);
          float3 reflectedEnviBlurred = sample_envi_probe(float4(reflectionVec, 4)).rgb * indoorRefl__localWeight_blurred.w + indoorRefl__localWeight_blurred.rgb;
          reflectedEnvi = lerp(giSpecular, reflectedEnvi*(giSpecular/max(1e-6, reflectedEnviBlurred)), 1 - linearRoughness*2);
        } else
          reflectedEnvi = giSpecular;
      }
      //alternatively, we always use Treyarch trick
      float mipForTrick = linearRoughness*NUM_PROBE_MIPS;
      float maximumSpecValue = max3( 1.26816, 9.13681 * exp2( 6.85741 - 2 * mipForTrick ) * NdotV, 9.70809 * exp2( 7.085 - mipForTrick - 0.403181 * pow2(mipForTrick)) * NdotV );
      maximumSpecValue = min(maximumSpecValue, 32);//to be removed with envi light probes in rooms
      float adjustedMaxSpec = luminance(ambient) * maximumSpecValue;
      float specLum = luminance( reflectedEnvi );
      reflectedEnvi = reflectedEnvi * (adjustedMaxSpec / max(1e-6,( adjustedMaxSpec + specLum )));
    #endif

    diffuseTerm += (1-enviBRDF)*diffuseColor.rgb*ambient;
    specularTerm += reflectedEnvi * enviBRDF;
  //  


  //half fresnel = fresnelSchlick(glassSpecular, NoV).g; or half fresnel = glassSpecular + (1.0f - glassSpecular) * pow5(saturate(1.0f - NoV));
  half fresnel = enviBRDF;
