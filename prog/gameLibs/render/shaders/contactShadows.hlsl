#ifndef CONTACT_SHADOWS_INCLUDED
#define CONTACT_SHADOWS_INCLUDED 1

#ifndef COMPARE_TOLERANCE_SCALE
  #define COMPARE_TOLERANCE_SCALE(a) 1
#endif

half contactShadowRayCastWithScale(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, float stepOffset,
  float4x4 projTm,
  float linearDepth,
  float4x4 viewProjTmNoOfs,
  out float2 hitUV, float2 vignetteEff, float4 viewScaleOfs)
{
  float4 rayStartClip = mul( float4( cameraToPoint, 1 ), viewProjTmNoOfs );
  float4 rayDirClip = mul( float4( rayDirection * rayLength, 0 ), viewProjTmNoOfs );
  float4 rayEndClip = rayStartClip + rayDirClip;

  float3 rayStartScreen = rayStartClip.xyz / rayStartClip.w;
  float3 rayEndScreen = rayEndClip.xyz / rayEndClip.w;

  float3 rayStepScreen = rayEndScreen - rayStartScreen;

  // The rayStepUVz.xy doesn't need to be offseted, only scaled. Offsetting breaks it as steps will get larger as it gets further from the origin.
  float3 rayStartUVz = float3( rayStartScreen.xy * float2(0.5, -0.5) + 0.5, rayStartScreen.z );
  float3 rayStepUVz = float3( rayStepScreen.xy * float2(0.5, -0.5), rayStepScreen.z );
  rayStartUVz.xy = rayStartUVz.xy * viewScaleOfs.xy + viewScaleOfs.zw;
  rayStepUVz.xy = rayStepUVz.xy * viewScaleOfs.xy;

  //float4 rayDepthClip = rayStartClip + mul( float4( 0, 0, rayLength, 0 ), projTm );
  //float rayDepthScreenZ = rayDepthClip.z / rayDepthClip.w;
  float rayStartClipZ = rayStartClip.z;//can be calculated from linearDepth, as it is just
  float4 rayDepthClip = mul( float4( 0, 0, rayLength, 0 ), projTm );
  float rayDepthScreenZ = (rayStartClipZ + rayDepthClip.z) / (rayDepthClip.w + linearDepth);

  const float stepLen = 1.0 / numSteps;

  const float compareTolerance = abs( rayDepthScreenZ - rayStartScreen.z ) * stepLen*2 * COMPARE_TOLERANCE_SCALE(linearDepth);

  #if USE_LINEAR_THRESHOLD
    rayStartUVz.z = linearDepth;
    rayStepUVz.z = linearize_z(rayEndScreen.z, zn_zfar.zw)-linearDepth;
    float zThickness = 0.1;
    float TraceBias = 0.03;
  #endif

  float sampleT = stepOffset * stepLen + stepLen;

  float hitT = -1;

  for( int i = 0; i < numSteps; i++ )
  {
    float3 sampleUVz = rayStartUVz + rayStepUVz * sampleT;
    #if USE_LINEAR_THRESHOLD
      float sampleDepth = linearize_z(tex2Dlod( depth_gbuf, float4(sampleUVz.xy, 0,0) ).r, zn_zfar.zw);
      float depthDiff = sampleUVz.z - sampleDepth - 0.02 * linearDepth * TraceBias;
      bool hasHit = (depthDiff > 0.0 && depthDiff < zThickness);
    #else
      float sampleDepth = tex2Dlod( depth_gbuf, float4(sampleUVz.xy, 0,0) ).r;
      float depthDiff = sampleUVz.z - sampleDepth;
      bool hasHit = abs( depthDiff + compareTolerance ) < compareTolerance;
    #endif


    #if EARLY_EXIT_CONTACT_SHADOWS
    if (hasHit)
    {
      hitT = sampleT;
      break;
    }
    #else
      hitT = (hasHit && hitT < 0.0) ? sampleT : hitT;
    #endif

    sampleT += stepLen;
  }

  float shadow = hitT > 0.0 ? saturate(1-pow4(hitT)) : 0.0;
  hitUV = rayStartUVz.xy + rayStepUVz.xy * hitT;

  // Off screen masking
  float2 Vignette = max(vignetteEff.x * abs(hitUV*2-1) - vignetteEff.y, 0.0);
  shadow *= saturate( 1.0 - dot( Vignette, Vignette ) );

  return 1 - shadow;
}

half contactShadowRayCast(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, float stepOffset,
  float4x4 projTm,
  float linearDepth,
  float4x4 viewProjTmNoOfs,
  out float2 hitUV, float2 vignetteEff)
{
  return contactShadowRayCastWithScale(depth_gbuf, depth_gbuf_samplerstate, cameraToPoint, rayDirection,
                                       rayLength, numSteps, stepOffset, projTm, linearDepth,
                                       viewProjTmNoOfs, hitUV, vignetteEff, float4(1,1,0,0));
}

#endif