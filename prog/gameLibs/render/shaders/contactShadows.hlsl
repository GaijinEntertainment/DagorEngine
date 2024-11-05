#ifndef CONTACT_SHADOWS_INCLUDED
#define CONTACT_SHADOWS_INCLUDED 1

#ifndef COMPARE_TOLERANCE_SCALE
  #define COMPARE_TOLERANCE_SCALE(a) 1
#endif

half contactShadowRayCastWithScale(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, half stepOffset,
  half4x4 projTm,
  half linearDepth,
  float4x4 viewProjTmNoOfs,
  out half2 hitUV, half2 vignetteEff, half4 viewScaleOfs)
{
  float4 rayStartClip = mul( float4( cameraToPoint, 1 ), viewProjTmNoOfs );
  float4 rayDirClip = mul( float4( rayDirection * rayLength, 0 ), viewProjTmNoOfs );
  float4 rayEndClip = rayStartClip + rayDirClip;

  half3 rayStartScreen = half3(rayStartClip.xyz / rayStartClip.w);
  half3 rayEndScreen = half3(rayEndClip.xyz / rayEndClip.w);

  half3 rayStepScreen = rayEndScreen - rayStartScreen;

  // The rayStepUVz.xy doesn't need to be offseted, only scaled. Offsetting breaks it as steps will get larger as it gets further from the origin.
  half3 rayStartUVz = half3( rayStartScreen.xy * half2(0.5, -0.5) + 0.5h, rayStartScreen.z );
  half3 rayStepUVz = half3( rayStepScreen.xy * half2(0.5, -0.5), rayStepScreen.z );
  rayStartUVz.xy = rayStartUVz.xy * viewScaleOfs.xy + viewScaleOfs.zw;
  rayStepUVz.xy = rayStepUVz.xy * viewScaleOfs.xy;

  //half4 rayDepthClip = rayStartClip + mul( half4( 0, 0, rayLength, 0 ), projTm );
  //half rayDepthScreenZ = rayDepthClip.z / rayDepthClip.w;
  half rayStartClipZ = half(rayStartClip.z);//can be calculated from linearDepth, as it is just
  half4 rayDepthClip = mul( half4( 0, 0, rayLength, 0 ), projTm );
  half rayDepthScreenZ = (rayStartClipZ + rayDepthClip.z) / (rayDepthClip.w + linearDepth);

  const half stepLen = 1.0h / half(numSteps);

  const half compareTolerance = abs( rayDepthScreenZ - rayStartScreen.z ) * stepLen*2 * COMPARE_TOLERANCE_SCALE(linearDepth);

  #if USE_LINEAR_THRESHOLD
    rayStartUVz.z = linearDepth;
    rayStepUVz.z = linearize_z(rayEndScreen.z, zn_zfar.zw)-linearDepth;
    half zThickness = 0.1;
    half TraceBias = 0.03;
  #endif

  half sampleT = stepOffset * stepLen + stepLen;

  half hitT = -1;

  for( int i = 0; i < numSteps; i++ )
  {
    half3 sampleUVz = rayStartUVz + rayStepUVz * sampleT;
    #if USE_LINEAR_THRESHOLD
      #if FSR_DISTORTION
        half sampleDepth = linearize_z(tex2Dlod(depth_gbuf, half4(linearToDistortedTc(sampleUVz.xy), 0, 0)).r, zn_zfar.zw);
      #else
        half sampleDepth = linearize_z(tex2Dlod(depth_gbuf, half4(sampleUVz.xy, 0, 0)).r, zn_zfar.zw);
      #endif
      half depthDiff = sampleUVz.z - sampleDepth - 0.02 * linearDepth * TraceBias;
      bool hasHit = (depthDiff > 0.0 && depthDiff < zThickness);
    #else
      #if FSR_DISTORTION
        half sampleDepth = half(tex2Dlod(depth_gbuf, half4(linearToDistortedTc(sampleUVz.xy), 0, 0)).r);
      #else
        half sampleDepth = half(tex2Dlod(depth_gbuf, half4(sampleUVz.xy, 0, 0)).r);
      #endif
      half depthDiff = sampleUVz.z - sampleDepth;
      bool hasHit = abs( depthDiff + compareTolerance ) < compareTolerance;
    #endif


    #if EARLY_EXIT_CONTACT_SHADOWS
    if (hasHit)
    {
      hitT = sampleT;
      break;
    }
    #else
      hitT = (hasHit && hitT < 0.0h) ? sampleT : hitT;
    #endif

    sampleT += stepLen;
  }

  half shadow = hitT > 0.0h ? saturate(1-pow2(hitT)) : 0.0h;
  hitUV = rayStartUVz.xy + rayStepUVz.xy * hitT;
  //hard off screen masking seem to be good enough and less frustrating
  FLATTEN
  if (any(or(hitUV <= 0, hitUV >= 1)))
    shadow = 0;

  // soft off screen masking
  //half2 Vignette = max(vignetteEff.x * abs(hitUV*2-1) - vignetteEff.y, 0.0h);
  //shadow *= saturate( 1.0h - dot( Vignette, Vignette ) );

  return 1 - shadow;
}

half contactShadowRayCast(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, half stepOffset,
  half4x4 projTm,
  half linearDepth,
  float4x4 viewProjTmNoOfs,
  out half2 hitUV, half2 vignetteEff)
{
  return contactShadowRayCastWithScale(depth_gbuf, depth_gbuf_samplerstate, cameraToPoint, rayDirection,
                                       rayLength, numSteps, stepOffset, projTm, linearDepth,
                                       viewProjTmNoOfs, hitUV, vignetteEff, half4(1,1,0,0));
}

#endif