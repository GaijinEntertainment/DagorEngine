//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>
#include <math/dag_Point2.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>

class ComputeShaderElement;
class RingCPUBufferLock;
class Sbuffer;

inline constexpr uint32_t EXPOSURE_BUF_SIZE = 16;
using ExposureBuffer = eastl::array<float, EXPOSURE_BUF_SIZE>;

inline constexpr uint32_t ADAPTATION_HISTOGRAM_BINS = 256;
using AdaptationHistogram = eastl::array<uint32_t, ADAPTATION_HISTOGRAM_BINS>;

inline constexpr float ADAPTATION_LOG_RANGE = 15.0f;
inline constexpr float ADAPTATION_INITIAL_MIN_LOG = -12.0f;
inline constexpr float ADAPTATION_INITIAL_MAX_LOG = ADAPTATION_INITIAL_MIN_LOG + ADAPTATION_LOG_RANGE;


struct AdaptationSettings
{
  float minExposure = 0.25, maxExposure = 5, autoExposureScale = 1.5, adaptDownSpeed = 8, adaptUpSpeed = 1, fixedExposure = -1;
  float brightnessPerceptionLinear = 0.9, brightnessPerceptionPower = 0.9;
  float fadeMul = 1.0;
  float centerWeight = 0.7;

  float luminanceLowRange = 0, luminanceHighRange = 0.01f;
  bool useLuminanceTrimming = false, includesHeroCockpit = true, focusOnAim = false;
  Point2 focusAimScale = {0.5, 0.5};
  float minSamples = 9.5;

  float fnUniformSamplesPercentage = 30.0f;
  float fnUniformDistributionRadius = 0.2;
  float fnNonLinearDistributionRadius = 1.0;
  float fnNonLinearDistributionCurvinesPow = 0.3;

  bool isFixedExposure() const { return fixedExposure > 0; }
};

class ExposureCompute
{
public:
  struct DispatchInfo
  {
    static constexpr int groupsCount = 32;
    static constexpr int threadsPerGroup = 64;
    static constexpr int pixelsCount = threadsPerGroup * groupsCount;
  };

  ExposureCompute();

  bool isValid() const { return bool(adaptExposureCS); }

  void uploadInitialExposure();
  bool writeExposure(float exposure);
  bool seedExposure(float exposure, float inv_exposure, float prev_exposure_ratio);

  static ExposureBuffer makeInitialExposure(float exposure, float inv_exposure, float prev_exposure_ratio, float prev_inv_exposure,
    float inv_smoothed_luminance, float inv_auto_luminance);

  void genHistogram(const AdaptationSettings &settings, bool use_albedo_luma);
  void accumulateHistogram();
  void adaptExposure(const AdaptationSettings &settings, bool instant);

  // Reads the latest exposure buffer back to the CPU
  // Returns true when fresh data became available this call
  bool updateReadback();

  void updateHistogramReadback(Sbuffer *histogram);
  const AdaptationHistogram &getLastHistogram() const { return lastHistogram; }

  void updateTime(float dt) { accumulatedTime += dt; }
  void sheduleClear() { isClearNeeded = true; }
  void clearNormalizationFactor();
  void resetReadback();
  bool afterDeviceReset();
  void dumpExposureBuffer() { isDumpExposureBuffer = true; }

  D3DRESID getExposureBufferId() { return g_Exposure.getBufId(); }
  ManagedTexView getNormalizationFactor() { return ManagedTexView(exposureNormalizationFactor); }
  const ExposureBuffer &getLastExposureBuffer() const { return lastExposureBuffer; }
  bool hasReadback() const { return bool(exposureReadback); }

private:
  UniqueTex exposureNormalizationFactor;
  UniqueBuf g_Exposure;
  eastl::unique_ptr<ComputeShaderElement> generateHistogramCenterWeightedFromSourceCS;
  eastl::unique_ptr<ComputeShaderElement> adaptExposureCS, accumulate_hist_cs;
  eastl::unique_ptr<RingCPUBufferLock> exposureReadback;
  eastl::unique_ptr<RingCPUBufferLock> histogramReadback;
  float accumulatedTime = 0;
  uint32_t latestReadbackFrameCopied = 0;
  uint32_t latestHistogramFrameCopied = 0;
  bool isDumpExposureBuffer = false;
  bool isClearNeeded = false;
  ExposureBuffer lastExposureBuffer = {};
  AdaptationHistogram lastHistogram = {};

  struct ShaderVars
  {
    ShaderVariableInfo ExposureInVarId = ShaderVariableInfo("ExposureIn");
    ShaderVariableInfo ExposureOutVarId = ShaderVariableInfo("ExposureOut");
    ShaderVariableInfo adaptation_cw_samples_countVarId = ShaderVariableInfo("adaptation_cw_samples_count", true);
    ShaderVariableInfo adaptation_use_luminance_trimmingVarId = ShaderVariableInfo("adaptation_use_luminance_trimming");
    ShaderVariableInfo adaptation__includesHeroCockpitVarId = ShaderVariableInfo("adaptation__includesHeroCockpit", true);
    ShaderVariableInfo adaptation__minSamplesVarId = ShaderVariableInfo("adaptation__minSamples");
    ShaderVariableInfo EyeAdaptationParams_0VarId = ShaderVariableInfo("EyeAdaptationParams_0");
    ShaderVariableInfo EyeAdaptationParams_1VarId = ShaderVariableInfo("EyeAdaptationParams_1");
    ShaderVariableInfo EyeAdaptationParams_2VarId = ShaderVariableInfo("EyeAdaptationParams_2");
    ShaderVariableInfo EyeAdaptationParams_3VarId = ShaderVariableInfo("EyeAdaptationParams_3");

    ShaderVariableInfo adaptation_center_weighted_paramVarId = ShaderVariableInfo("adaptation_center_weighted_param", true);
    ShaderVariableInfo adaptation_samples_countVarId = ShaderVariableInfo("adaptation_samples_count", true);
    ShaderVariableInfo adaptation_uniform_samples_countVarId = ShaderVariableInfo("adaptation_uniform_samples_count", true);
    ShaderVariableInfo adaptation_uniform_func_tanVarId = ShaderVariableInfo("adaptation_uniform_func_tan", true);
    ShaderVariableInfo adaptation_nonlinear_func_fmaVarId = ShaderVariableInfo("adaptation_nonlinear_func_fma", true);
    ShaderVariableInfo adaptation_nonlinear_func_powVarId = ShaderVariableInfo("adaptation_nonlinear_func_pow", true);

    ShaderVariableInfo use_albedo_luma_adaptationVarId = ShaderVariableInfo("use_albedo_luma_adaptation", true);
  } vars;
};
