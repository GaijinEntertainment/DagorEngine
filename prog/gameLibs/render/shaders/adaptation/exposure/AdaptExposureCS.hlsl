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

void calc_weighted_sum_with_luminance_borders(const uint GI, float min_luminance, float max_luminance,
                                              out float weighted_sum, out float total_weight)
{
  const float lowTrimmingBinPos = ComputeHistogramPositionFromLuminance(min_luminance);
  const float highTrimminBinPos = ComputeHistogramPositionFromLuminance(max_luminance);

  const uint lowBinIdx = max(1u, (uint)(saturate(lowTrimmingBinPos) * HISTOGRAM_SIZE));
  const uint highBinIdx = min(HISTOGRAM_SIZE - 1u, (uint)(saturate(highTrimminBinPos) * HISTOGRAM_SIZE));

  const uint threadBinIdx = lowBinIdx + GI;
  const bool inTrimmingRange = threadBinIdx <= highBinIdx;

  const float cHist = inTrimmingRange ? Histogram[min(threadBinIdx, HISTOGRAM_SIZE - 1u)] : 0;
  const float logLuminanceH = inTrimmingRange
                            ? ComputeLogLuminanceFromHistogramPosition(threadBinIdx/(float)HISTOGRAM_SIZE)
                            : -1e12;

  float weightedSum = logLuminanceH * cHist;
  UNROLL
  for (uint i = 1; i < 256; i *= 2)
  {
      gs_Accum[GI] = weightedSum;
      GroupMemoryBarrierWithGroupSync();
      weightedSum += gs_Accum[(GI + i) % 256];
      GroupMemoryBarrierWithGroupSync();
  }

  float totalWeight = cHist;
  UNROLL
  for (uint j = 1; j < 256; j *= 2)
  {
      gs_Accum[GI] = totalWeight;
      GroupMemoryBarrierWithGroupSync();
      totalWeight += gs_Accum[(GI + j) % 256];
      GroupMemoryBarrierWithGroupSync();
  }

  weighted_sum = weightedSum;
  total_weight = totalWeight;
}

void calc_weighted_sum(const uint GI, float total_pixels, out float weighted_sum, out float total_weight)
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
  weighted_sum = weightedSum;
  total_weight = total_pixels - cHist;
}

[numthreads( 256, 1, 1 )]
void main( uint GI : SV_GroupIndex )
{
    float weightedSum;
    float totalWeight;
    BRANCH
    if (adaptation_use_luminance_trimming > 0.0f && EyeAdaptationParams[0].y > EyeAdaptationParams[0].x)
      calc_weighted_sum_with_luminance_borders(GI, EyeAdaptationParams[0].x, EyeAdaptationParams[0].y, weightedSum, totalWeight);
    else
      calc_weighted_sum(GI, EyeAdaptationParams[2].z, weightedSum, totalWeight);

    bool enoughSamples = totalWeight >= adaptation__minSamples;
    if (GI == 0 && enoughSamples)
    {
      const float exposureOffsetMultipler = EyeAdaptationParams[1].x;
      const float avgLogLuminance = totalWeight > 0
                                      ? weightedSum/totalWeight
                                      : -1e12;
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
      Exposure[10] = currentLuminance;

      static const float midGray = 0.18;
      const float normalizationFactor = midGray / (currentLuminance * (1 - midGray) * exposureScale);

#if !_HARDWARE_METAL  //due to compiler bug, Metal is unable to write to texture with if.
      exposureNormalizationFactor[uint2(0,0)] = isfinite(normalizationFactor) ? normalizationFactor : 1.f;
#else
      if (isfinite(normalizationFactor))
        exposureNormalizationFactor[uint2(0,0)] = normalizationFactor;
#endif
    }
}
