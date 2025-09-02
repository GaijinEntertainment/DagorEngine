#define SSRT_SAMPLE_BATCH_SIZE 4

struct SSRTRay
{
  float3 startScreen;
  float3 stepScreen;

  float compareTolerance;
  float worldDistance;
};

float2 rayBoxIntersect(float2 boxExt, float2 localOrigin, float2 localRay)
{
  float2 firstPlaneIntersect    = (boxExt - localOrigin) / localRay;
  float2 secondPlaneIntersect = (-boxExt - localOrigin) / localRay;
  float2 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
  float furthestDistance = min(furthestPlane.x, furthestPlane.y);
  float2 closestPlane = min(firstPlaneIntersect, secondPlaneIntersect);
  float closestDistance = max(closestPlane.x, closestPlane.y);
  return float2(closestDistance, furthestDistance);
}

float scaleToScreenEdge(float2 startScreen, float2 stepScreen)
{
  //fixme: optimize me (we know we are inside)
  float2 ab = abs(stepScreen);
  if (all(ab < 1e-4))
    return 1;
  //float l = length(stepScreen);
  float2 r = rayBoxIntersect(0.999, startScreen, stepScreen);
  return r.y;
}

SSRTRay init_screen_space_ray(
  float3 camPos,
  float3 worldDir,
  float rayDist,
  float slopeToleranceScale,
  float4x4 globtm_no_ofs,
  float4x4 projtm)
{

  float3 camEndPos = camPos + worldDir * rayDist;

  float4 rayStartNDC = mul(float4(camPos, 1.0), globtm_no_ofs);
  float4 rayEndNDC = mul(float4(camEndPos, 1.0), globtm_no_ofs);

  float3 startScreen = rayStartNDC.xyz * rcp(rayStartNDC.w);
  float3 endScreen = rayEndNDC.xyz * rcp(rayEndNDC.w);

  float4 rayDepthClip = rayStartNDC + mul(float4(0, 0, rayDist, 0), projtm);
  float rayDepthScreenZ = rayDepthClip.z / rayDepthClip.w;

  SSRTRay ray;
  ray.startScreen = startScreen;
  ray.stepScreen = endScreen - startScreen;

  float toScreenScale = min(1.0, scaleToScreenEdge(startScreen.xy, ray.stepScreen.xy));

  ray.stepScreen *= toScreenScale;
  ray.worldDistance = rayDist*toScreenScale;
  ray.compareTolerance = max(abs(ray.stepScreen.z), (startScreen.z - rayDepthScreenZ) * slopeToleranceScale);
  //ray.compareTolerance = abs(ray.stepScreen.z);

  return ray;
}

#define SCREEN_SPACE_RAY_MISS 0
#define SCREEN_SPACE_RAY_HIT 1
#define SCREEN_SPACE_RAY_HIT_UNCERTAIN 2
struct ScreenSpaceRayHit
{
  float3 hitUVz;
  uint hitType;
};

ScreenSpaceRayHit cast_screenspace_hzb_ray(
    Texture2D hyper_z, SamplerState hyper_z_samplerstate,
    float mip,
    SSRTRay ray,
    uint numSteps4, float backStepOffset,
    // hzb_uv_scale.xy = 2*hzb_size / source_size == 1 if even, hzb_uv_scale.zw = 1./hzb_uv_scale.xy
    float4 hzb_uv_scale)
{
  ScreenSpaceRayHit ret;
  const float3 startScreen = ray.startScreen;
  const float3 stepScreen = ray.stepScreen;

  float3 rayStartUVz = float3( (startScreen.xy * float2( 0.5, -0.5 ) + 0.5) * hzb_uv_scale.xy, startScreen.z );
  float3 rayStepUVz  = float3(  stepScreen.xy  * float2( 0.5, -0.5 )      * hzb_uv_scale.xy, stepScreen.z );
  uint totalSamples = numSteps4*SSRT_SAMPLE_BATCH_SIZE;

  const float invSamples = 1.0 / totalSamples;
  float compareTolerance = ray.compareTolerance * invSamples;

  float lastDepthDiff = 0;

  //backStepOffset = View.GeneralPurposeTweak;

  rayStepUVz *= invSamples;
  float3 rayUVz = rayStartUVz + rayStepUVz * backStepOffset;
  uint i;

  float batchAtIntersect;
  if (0) {
    //this branch is slower on Scarlett (20%( and on 4090 (10%). Although it was supposed to be faster (less divirgent)
    float4 samplesDepthDiff;
    bool4 samplesHit;
    bool4 samplesDisOcclusion;

    LOOP
    for (i = 0; i < totalSamples; i += SSRT_SAMPLE_BATCH_SIZE)
    {
      samplesDepthDiff.x = tex2Dlod(hyper_z, float4(rayUVz.xy + (float(i + 1)) * rayStepUVz.xy, 0, mip)).r;
      samplesDepthDiff.y = tex2Dlod(hyper_z, float4(rayUVz.xy + (float(i + 2)) * rayStepUVz.xy, 0, mip)).r;

      samplesDepthDiff.z = tex2Dlod(hyper_z, float4(rayUVz.xy + (float(i + 3)) * rayStepUVz.xy, 0, mip)).r;
      samplesDepthDiff.w = tex2Dlod(hyper_z, float4(rayUVz.xy + (float(i + 4)) * rayStepUVz.xy, 0, mip)).r;

      samplesDepthDiff = rayUVz.zzzz + float4(i + 1, i + 2, i + 3, i + 4) * rayStepUVz.z - samplesDepthDiff;

      samplesHit = abs(samplesDepthDiff + compareTolerance) < compareTolerance;

      samplesDisOcclusion = (samplesDepthDiff + compareTolerance) < -compareTolerance;
      BRANCH
      if (any(samplesDisOcclusion | samplesHit))
        break;

      lastDepthDiff = samplesDepthDiff.w;
    }
    samplesDisOcclusion.y = samplesDisOcclusion.y|samplesDisOcclusion.x;
    samplesDisOcclusion.z = samplesDisOcclusion.z|samplesDisOcclusion.y;
    samplesDisOcclusion.w = samplesDisOcclusion.w|samplesDisOcclusion.z;
    samplesHit = samplesHit & !samplesDisOcclusion;

    // Compute the output coordinates.
    BRANCH
    if (any(samplesHit))
    {
      float depthDiff0 = samplesDepthDiff[2];
      float depthDiff1 = samplesDepthDiff[3];
      float batchAt = 3;

      FLATTEN
      if (samplesHit[2])
      {
        depthDiff0 = samplesDepthDiff[1];
        depthDiff1 = samplesDepthDiff[2];
        batchAt = 2;
      }
      FLATTEN
      if (samplesHit[1])
      {
        depthDiff0 = samplesDepthDiff[0];
        depthDiff1 = samplesDepthDiff[1];
        batchAt = 1;
      }
      FLATTEN
      if (samplesHit[0])
      {
        depthDiff0 = lastDepthDiff;
        depthDiff1 = samplesDepthDiff[0];
        batchAt = 0;
      }

      batchAtIntersect = batchAt + saturate(depthDiff0 / (depthDiff0 - depthDiff1));
      ret.hitType = SCREEN_SPACE_RAY_HIT;
      //ret.hitType = isDisOccluded ? SCREEN_SPACE_RAY_HIT_UNCERTAIN : SCREEN_SPACE_RAY_HIT;
    }
    else
    {
      batchAtIntersect = 0;//more conservative
      //more correct
      //batchAtIntersect = samplesDisOcclusion.y ? 1 : samplesDisOcclusion.z ? 2 : samplesDisOcclusion.w ? 3 : 0;
      ret.hitType = samplesDisOcclusion.w ? SCREEN_SPACE_RAY_HIT_UNCERTAIN : SCREEN_SPACE_RAY_MISS;
    }
  }
  else
  {
    float samplesDepthDiff, samplesDisOcclusion;
    bool samplesHit;
    rayUVz += rayStepUVz;
    LOOP
    for (i = 0; i < totalSamples; i ++)
    {
      samplesDepthDiff = tex2Dlod(hyper_z, float4(rayUVz.xy, 0, mip)).r;

      samplesDepthDiff = rayUVz.z - samplesDepthDiff;

      samplesHit = abs(samplesDepthDiff + compareTolerance) < compareTolerance;

      samplesDisOcclusion = (samplesDepthDiff + compareTolerance) < -compareTolerance;
      BRANCH
      if (samplesDisOcclusion || samplesHit)
        break;

      lastDepthDiff = samplesDepthDiff;
      rayUVz += rayStepUVz;
    }
    samplesHit = samplesHit && !samplesDisOcclusion;

    // Compute the output coordinates.
    FLATTEN
    if (samplesHit)
    {
      batchAtIntersect = saturate(lastDepthDiff / (lastDepthDiff - samplesDepthDiff));
      ret.hitType = SCREEN_SPACE_RAY_HIT;
      //ret.hitType = isDisOccluded ? SCREEN_SPACE_RAY_HIT_UNCERTAIN : SCREEN_SPACE_RAY_HIT;
    }
    else
    {
      batchAtIntersect = 0;//more conservative
      //more correct
      //batchAtIntersect = samplesDisOcclusion.y ? 1 : samplesDisOcclusion.z ? 2 : samplesDisOcclusion.w ? 3 : 0;
      ret.hitType = samplesDisOcclusion ? SCREEN_SPACE_RAY_HIT_UNCERTAIN : SCREEN_SPACE_RAY_MISS;
    }
    ret.hitUVz = rayStartUVz + rayStepUVz * max(batchAtIntersect + float(i) + backStepOffset, 0);
    ret.hitUVz.xy *= hzb_uv_scale.zw;
    return ret;
  }
  ret.hitUVz = rayStartUVz + rayStepUVz * max(batchAtIntersect + float(i) + backStepOffset, 0);
  ret.hitUVz.xy *= hzb_uv_scale.zw;
  return ret;
}

