//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//
// The group size is 16x16, but one group iterates over an entire 16-wide column of pixels (384 pixels tall)
// Assuming the total workspace is 640x384, there will be 40 thread groups computing the histogram in parallel.
// The histogram measures logarithmic luminance ranging from 2^-12 up to 2^4.  This should provide a nice window
// where the exposure would range from 2^-4 up to 2^4.

#include "PostEffectsRS.hlsli"
#include "ShaderUtility.hlsli"

RWTexture2D<float> exposureTex : register(u1);

#define HISTOGRAM_SIZE 256

float computeEV100FromAvgLuminance ( float avgLuminance )
{
  // We later use the middle gray at 12.7% in order to have
  // a middle gray at 18% with a sqrt (2) room for specular highlights
  // But here we deal with the spot meter measuring the middle gray
  // which is fixed at 12.5 for matching standard camera
  // constructor settings (i.e. calibration constant K = 12.5)
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  return log2 (avgLuminance * (100.0f / 12.5f));
}

float convertEV100ToMaxLuminance(float EV100)
{
  // Compute the maximum luminance possible with H_sbs sensitivity
  // maxLum = 78 / ( S * q ) * N^2 / t
  // = 78 / ( S * q ) * 2^ EV_100
  // = 78 / (100 * 0.65) * 2^ EV_100
  // = 1.2 * 2^ EV
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  return 1.2f * exp2(EV100);
}

groupshared float gs_Accum[HISTOGRAM_SIZE];

// HistogramPosition 0..1
float ComputeHistogramPositionFromLuminance(float Luminance)
{
  return (log2(Luminance) - Exposure[4]) / ADAPTATION_LOG_RANGE ;
}

float ComputeLogLuminanceFromHistogramPosition(float HistogramPosition)
{
  return HistogramPosition * ADAPTATION_LOG_RANGE + Exposure[4];
}

float ComputeEyeAdaptation(float OldExposure, float TargetExposure, float FrameTime)
{
  const float Diff = TargetExposure - OldExposure;
  const float EyeAdaptionSpeedUp = EyeAdaptationParams[1].z;
  const float EyeAdaptionSpeedDown = EyeAdaptationParams[1].w;
  const float AdaptionSpeed = (Diff > 0) ? EyeAdaptionSpeedUp : EyeAdaptionSpeedDown;
  const float Factor = 1.0f - exp2(-FrameTime * AdaptionSpeed);
  return OldExposure + Diff * Factor;
}

[numthreads( 256, 1, 1 )]
void main( uint GI : SV_GroupIndex )
{
    const float cHist = Histogram[GI];
    const float logLuminanceH = ComputeLogLuminanceFromHistogramPosition(GI/(float)HISTOGRAM_SIZE);
    float weightedSum = GI == 0 ? 0 : logLuminanceH * cHist;

    UNROLL
    for (uint i = 1; i < 256; i *= 2)
    {
        gs_Accum[GI] = weightedSum;                 // Write
        GroupMemoryBarrierWithGroupSync();          // Sync
        weightedSum += gs_Accum[(GI + i) % 256];    // Read
        GroupMemoryBarrierWithGroupSync();          // Sync
    }

    if (GI == 0)
    {
      const float totalPixels = EyeAdaptationParams[2].z;
      const float sum = totalPixels - cHist;
      const float exposureOffsetMultipler = EyeAdaptationParams[1].x;
      const float avgLogLuminance = sum > 0 ? weightedSum/sum : -1e12; // default to some very low value instead of NaN
      const float currentLuminance = exp2(avgLogLuminance);
      const float oldLuminance = Exposure[1]*exposureOffsetMultipler;
      const float frameTime = max(0, EyeAdaptationParams[1].y);

      // eye adaptation changes over time
      const float autoEV100 = computeEV100FromAvgLuminance(currentLuminance);
      float autoLuminance = convertEV100ToMaxLuminance(autoEV100);

      const float a = EyeAdaptationParams[3].x;
      const float b = EyeAdaptationParams[3].y;
      autoLuminance = lerp(1, pow(autoLuminance, b), a);//fit percepted brightness so that it is darker when everything dark and brighter when everything bright

      float smoothedLuminance = ComputeEyeAdaptation(oldLuminance, autoLuminance, frameTime);
      const bool instantAdaptation = EyeAdaptationParams[3].w > 0.;
      if (instantAdaptation)
        smoothedLuminance = autoLuminance;

      float exposureScale = exposureOffsetMultipler / max(0.0001f, smoothedLuminance);

      if (!instantAdaptation)
        exposureScale = clamp(exposureScale, EyeAdaptationParams[0].z, EyeAdaptationParams[0].w);

      FLATTEN
      if (!isfinite(exposureScale))//just for sanity!
        exposureScale = 1;

      const float fadeMultiplier = EyeAdaptationParams[2].w;
      exposureScale *= fadeMultiplier;

      const float previousInvExposure = Exposure[1];
      Exposure[0] = exposureScale;
      Exposure[1] = 1./exposureScale;
      Exposure[2] = exposureScale * previousInvExposure;
      Exposure[3] = previousInvExposure;

      float MinLog = Exposure[4];
      float MaxLog = Exposure[5];
      static const float RcpLogRange = 1./ADAPTATION_LOG_RANGE;

      const float biasToCenter = ComputeHistogramPositionFromLuminance(currentLuminance) - 0.5;
      if (abs(biasToCenter) > RcpLogRange)
      {
          // If at least one stop off from center, correct histogram range by one stop.
          MinLog += sign(biasToCenter);
          MaxLog = MinLog + ADAPTATION_LOG_RANGE;
      }

      Exposure[4] = MinLog;
      Exposure[5] = MaxLog;
      Exposure[6] = ADAPTATION_LOG_RANGE;
      Exposure[7] = RcpLogRange;
      Exposure[8] = 1.0 / max(0.0001f, smoothedLuminance); // Current exposure without extra factors.
      Exposure[9] = 1.0 / max(0.0001f, autoLuminance); // Target exposure wiithout extra factors.

#if HAS_PRE_EXPOSURE
      exposureTex[uint2(0, 0)] = 1;
#else
      exposureTex[uint2(0, 0)] = exposureScale;
#endif
    }
}
