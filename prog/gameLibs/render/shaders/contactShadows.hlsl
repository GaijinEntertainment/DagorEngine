#ifndef CONTACT_SHADOWS_INCLUDED
#define CONTACT_SHADOWS_INCLUDED 1

#ifndef COMPARE_TOLERANCE_SCALE
  #define COMPARE_TOLERANCE_SCALE(a) 1
#endif

//returns hit (t) if hitT < 1, >= 1 on miss
//screenHtAspectAndMinLen.x is target height/width, and screenHtAspectAndMinLen.y is distance in CS space (i.e. 2.0 is full width)
// this prevents super small rays (in pixels) in case of fixed rayLength, this should be only used for infinite ray (sun)
float contactShadowRayCastWithScaleHitT(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, float stepOffset,
  float4x4 projTm,
  float linearDepth,
  float4x4 viewProjTmNoOfs,
  out float2 hitUV, float4 viewScaleOfs, float2 screenHtAspectAndMinLen = float2(1080. / 1920., 0.0))
{
  float4 rayStartClip = mul( float4( cameraToPoint, 1 ), viewProjTmNoOfs );
  float4 rayDirClip = mul( float4( rayDirection * rayLength, 0 ), viewProjTmNoOfs );
  float4 rayEndClip = rayStartClip + rayDirClip;

  float3 rayStartScreen = float3(rayStartClip.xyz / rayStartClip.w);
  float3 rayEndScreen = float3(rayEndClip.xyz / rayEndClip.w);

  float3 rayStepScreen = rayEndScreen - rayStartScreen;
  if (screenHtAspectAndMinLen.y > 0)
  {
    float rayScreenLen = length(rayStepScreen.xy*float2(screenHtAspectAndMinLen.x, 1));
    float screenScaledRayLen = clamp(screenHtAspectAndMinLen.y/max(1e-6,rayScreenLen), 1., (linearDepth/rayLength)*0.99);
    rayEndClip = rayStartClip + rayDirClip * screenScaledRayLen;
    rayLength *= screenScaledRayLen;
    rayStepScreen = float3(rayEndClip.xyz / rayEndClip.w) - rayStartScreen;
  }

  // The rayStepUVz.xy doesn't need to be offseted, only scaled. Offsetting breaks it as steps will get larger as it gets further from the origin.
  float3 rayStartUVz = float3( rayStartScreen.xy * float2(0.5, -0.5) + 0.5h, rayStartScreen.z );
  float3 rayStepUVz = float3( rayStepScreen.xy * float2(0.5, -0.5), rayStepScreen.z );
  rayStartUVz.xy = rayStartUVz.xy * viewScaleOfs.xy + viewScaleOfs.zw;
  rayStepUVz.xy = rayStepUVz.xy * viewScaleOfs.xy;

  //float4 rayDepthClip = rayStartClip + mul( float4( 0, 0, rayLength, 0 ), projTm );
  //float rayDepthScreenZ = rayDepthClip.z / rayDepthClip.w;
  float rayStartClipZ = float(rayStartClip.z);//can be calculated from linearDepth, as it is just
  float4 rayDepthClip = float4(mul( float4( 0, 0, rayLength, 0 ), projTm ));
  float rayDepthScreenZ = (rayStartClipZ + rayDepthClip.z) / (rayDepthClip.w + linearDepth);

  const float stepLen = float(1.0h / float(numSteps));

  const float compareTolerance = abs( rayDepthScreenZ - rayStartScreen.z ) * stepLen * COMPARE_TOLERANCE_SCALE(linearDepth);

  float sampleT = stepOffset * stepLen + stepLen;

  float hitT = 1.h;

  for( int i = 0; i < numSteps; i++ )
  {
    float3 sampleUVz = rayStartUVz + rayStepUVz * sampleT;
    #if FSR_DISTORTION
      float sampleDepth = float(tex2Dlod(depth_gbuf, float4(linearToDistortedTc(sampleUVz.xy), 0, 0)).r);
    #else
      float sampleDepth = float(tex2Dlod(depth_gbuf, float4(sampleUVz.xy, 0, 0)).r);
    #endif
    float depthDiff = sampleUVz.z - sampleDepth;
    bool hasHit = abs( depthDiff + compareTolerance ) < compareTolerance;


    #if EARLY_EXIT_CONTACT_SHADOWS
    if (hasHit)
    {
      hitUV = sampleUVz.xy;
      return any(or(hitUV < 0, hitUV > 1)) ? 1.h : sampleT;
    }
    #else
      hitT = hasHit ? min(hitT, sampleT) : hitT;
    #endif

    sampleT += stepLen;
  }

  #if EARLY_EXIT_CONTACT_SHADOWS
  return 1.h;
  #else
  hitUV = rayStartUVz.xy + rayStepUVz.xy * hitT;
  return float(any(or(hitUV < 0, hitUV>1)) ? 1.h : hitT);
  #endif
}

// based on TineGlades ray marching invention https://www.youtube.com/watch?v=jusWW2pPnA0
// https://gist.github.com/h3r2tic/3675a9b76a40a90a25b972a9b0df0a0b
// this makes contact shadows almost acne free, even with numFilteredSteps = 1, and completely free with numFilteredSteps == 2,3
// the cost is neglible, less than +3% on Xbox One
// with EARLY_EXIT_CONTACT_SHADOWS  difference is much higher (~+10%), as EARLY_EXIT_CONTACT_SHADOWS are faster in general

float contactShadowFilteredRayCastWithScaleHitT(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate, SamplerState depth_gbuf_linear_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numFilteredSteps, int numUnfilteredSteps, float stepOffset,
  float4x4 projTm,
  float linearDepth,
  float4x4 viewProjTmNoOfs,
  out float2 hitUV, float4 viewScaleOfs, float2 screenHtAspectAndMinLen = float2(1080./1920., 0.0))
{
  float4 rayStartClip = mul( float4( cameraToPoint, 1 ), viewProjTmNoOfs );
  float4 rayDirClip = mul( float4( rayDirection * rayLength, 0 ), viewProjTmNoOfs );
  float4 rayEndClip = rayStartClip + rayDirClip;

  float3 rayStartScreen = float3(rayStartClip.xyz / rayStartClip.w);
  float3 rayEndScreen = float3(rayEndClip.xyz / rayEndClip.w);

  float3 rayStepScreen = rayEndScreen - rayStartScreen;
  if (screenHtAspectAndMinLen.y > 0)
  {
    float rayScreenLen = length(rayStepScreen.xy*float2(screenHtAspectAndMinLen.x, 1));
    float screenScaledRayLen = clamp(screenHtAspectAndMinLen.y/max(1e-6,rayScreenLen), 1., (linearDepth/rayLength)*0.99);
    rayEndClip = rayStartClip + rayDirClip * screenScaledRayLen;
    rayLength *= screenScaledRayLen;
    rayStepScreen = float3(rayEndClip.xyz / rayEndClip.w) - rayStartScreen;
  }

  // The rayStepUVz.xy doesn't need to be offseted, only scaled. Offsetting breaks it as steps will get larger as it gets further from the origin.
  float3 rayStartUVz = float3( rayStartScreen.xy * float2(0.5, -0.5) + 0.5h, rayStartScreen.z );
  float3 rayStepUVz = float3( rayStepScreen.xy * float2(0.5, -0.5), rayStepScreen.z );
  rayStartUVz.xy = rayStartUVz.xy * viewScaleOfs.xy + viewScaleOfs.zw;
  rayStepUVz.xy = rayStepUVz.xy * viewScaleOfs.xy;

  //float4 rayDepthClip = rayStartClip + mul( float4( 0, 0, rayLength, 0 ), projTm );
  //float rayDepthScreenZ = rayDepthClip.z / rayDepthClip.w;
  float rayStartClipZ = float(rayStartClip.z);//can be calculated from linearDepth, as it is just
  float4 rayDepthClip = float4(mul( float4( 0, 0, rayLength, 0 ), projTm ));
  float rayDepthScreenZ = (rayStartClipZ + rayDepthClip.z) / (rayDepthClip.w + linearDepth);
  int numSteps = numUnfilteredSteps + numFilteredSteps;
  const float stepLen = float(1.0h / float(numSteps));

  float compareTolerance = float(abs( rayDepthScreenZ - rayStartScreen.z ) * (2.h*stepLen) * COMPARE_TOLERANCE_SCALE(linearDepth));

  float sampleT = stepOffset * stepLen + stepLen;

  float hitT = 1.h;
  int i = 0;

  #define BRANCHED_VERSION 0 // branched version is same speed on 4090, but better check om 4080
  #if !BRANCHED_VERSION
  for (; i < numFilteredSteps; i++ )
  {
    float3 sampleUVz = rayStartUVz + rayStepUVz * sampleT;
    #if FSR_DISTORTION
    float2 uv = linearToDistortedTc(sampleUVz.xy);
    #else
    float2 uv = sampleUVz.xy;
    #endif
    const float unfiltered_raw_depth = float(depth_gbuf.SampleLevel(depth_gbuf_samplerstate, uv, 0).x);
    float linear_raw_depth = float(depth_gbuf.SampleLevel(depth_gbuf_linear_samplerstate, uv, 0).x);
    bool hasHit = min(unfiltered_raw_depth, linear_raw_depth) > sampleUVz.z && max(unfiltered_raw_depth, linear_raw_depth) < sampleUVz.z + compareTolerance;
    #if EARLY_EXIT_CONTACT_SHADOWS
    if (hasHit)
    {
      hitUV = sampleUVz.xy;
      return any(or(hitUV < 0, hitUV > 1)) ? 1.h : sampleT;
    }
    #else
      hitT = hasHit ? min(hitT, sampleT) : hitT;
    #endif
    sampleT += stepLen;
  }
  compareTolerance *= 0.5;
  #endif

  for (; i < numSteps; i++ )
  {
    float3 sampleUVz = rayStartUVz + rayStepUVz * sampleT;
    #if FSR_DISTORTION
    float2 uv = linearToDistortedTc(sampleUVz.xy);
    #else
    float2 uv = sampleUVz.xy;
    #endif
    float sampleDepth = float(depth_gbuf.SampleLevel(depth_gbuf_samplerstate, uv, 0).x);
    float depthDiff = sampleUVz.z - sampleDepth;

    #if BRANCHED_VERSION
    float minDepth = sampleDepth, maxDepth = sampleDepth;
    BRANCH
    if (i < numFilteredSteps)
    {
      float linear_raw_depth = depth_gbuf.SampleLevel(depth_gbuf_linear_samplerstate, uv, 0).x;
      minDepth = min(minDepth, linear_raw_depth);
      maxDepth = min(maxDepth, linear_raw_depth);
    }
    bool hasHit = minDepth > sampleUVz.z && maxDepth < sampleUVz.z + compareTolerance;
    #else
    bool hasHit = abs( depthDiff + compareTolerance ) < compareTolerance;
    #endif

    #if EARLY_EXIT_CONTACT_SHADOWS
    if (hasHit)
    {
      hitUV = sampleUVz.xy;
      return any(or(hitUV < 0, hitUV > 1)) ? 1.h : sampleT;
    }
    #else
      hitT = hasHit ? min(hitT, sampleT) : hitT;
    #endif

    sampleT += stepLen;
  }

  #if EARLY_EXIT_CONTACT_SHADOWS
  return 1.h;
  #else
  hitUV = rayStartUVz.xy + rayStepUVz.xy * hitT;
  return float(any(or(hitUV < 0, hitUV>1)) ? 1.h : hitT);
  #endif
}

float contactShadowRayCastWithScale(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, float stepOffset,
  float4x4 projTm,
  float linearDepth,
  float4x4 viewProjTmNoOfs,
  out float2 hitUV, float4 viewScaleOfs)
{
  float hitT = contactShadowRayCastWithScaleHitT(depth_gbuf, depth_gbuf_samplerstate, cameraToPoint, rayDirection, rayLength,
    numSteps, stepOffset, projTm, linearDepth, viewProjTmNoOfs, hitUV, viewScaleOfs);
  return float(1.h - saturate(1.h - pow2(hitT)));
}

float contactShadowRayCast(
  Texture2D depth_gbuf, SamplerState depth_gbuf_samplerstate,//native (hyper) depth
  float3 cameraToPoint, float3 rayDirection, float rayLength,
  int numSteps, float stepOffset,
  float4x4 projTm,
  float linearDepth,
  float4x4 viewProjTmNoOfs,
  out float2 hitUV)
{
  return contactShadowRayCastWithScale(depth_gbuf, depth_gbuf_samplerstate, cameraToPoint, rayDirection,
                                       rayLength, numSteps, stepOffset, projTm, linearDepth,
                                       viewProjTmNoOfs, hitUV, float4(1,1,0,0));
}

#endif