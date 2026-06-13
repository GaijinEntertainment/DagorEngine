// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/exposureCompute.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_driverDesc.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>

ExposureBuffer ExposureCompute::makeInitialExposure(float exposure, float inv_exposure, float prev_exposure_ratio,
  float prev_inv_exposure, float inv_smoothed_luminance, float inv_auto_luminance)
{
  const float range = ADAPTATION_INITIAL_MAX_LOG - ADAPTATION_INITIAL_MIN_LOG;
  return ExposureBuffer{exposure, inv_exposure, prev_exposure_ratio, prev_inv_exposure, ADAPTATION_INITIAL_MIN_LOG,
    ADAPTATION_INITIAL_MAX_LOG, range, 1.0f / range, inv_smoothed_luminance, inv_auto_luminance};
}

ExposureCompute::ExposureCompute()
{
  adaptExposureCS.reset(new_compute_shader("AdaptExposureCS"));
  if (!adaptExposureCS)
    return;

  g_Exposure = dag::buffers::create_ua_sr_structured(4, EXPOSURE_BUF_SIZE, "Exposure");
  uploadInitialExposure();

  generateHistogramCenterWeightedFromSourceCS.reset(new_compute_shader("GenerateHistogramCenterWeightedFromSourceCS"));
  accumulate_hist_cs.reset(new_compute_shader("accumulate_hist_cs"));

  static constexpr int exposureNormalizationFlags = TEXCF_RTARGET | TEXCF_UNORDERED | TEXFMT_R32F;
  exposureNormalizationFactor = dag::create_tex(NULL, 1, 1, exposureNormalizationFlags, 1, "exposure_normalization_factor");
}

bool ExposureCompute::afterDeviceReset()
{
  const bool ok = writeExposure(1);
  resetReadback();
  return ok;
}

void ExposureCompute::resetReadback()
{
  if (exposureReadback)
    exposureReadback->reset();
  latestReadbackFrameCopied = 0;
  if (histogramReadback)
    histogramReadback->reset();
  latestHistogramFrameCopied = 0;
}

void ExposureCompute::uploadInitialExposure() { writeExposure(1); }

void ExposureCompute::clearNormalizationFactor()
{
  // Run only once after sheduleClear
  if (!isClearNeeded)
    return;

  isClearNeeded = false;
  float ones[4] = {1, 1, 1, 1};
  d3d::clear_rwtexf(exposureNormalizationFactor.getTex2D(), ones, 0, 0);
}

bool ExposureCompute::writeExposure(float exposure)
{
  // this is not correct, we can't know for sure that prev frame Exposure was same (i.e. initExposure[3] = 1./exposure is not correct)
  // todo: correct way is continue call shader (different one, which will write buffer[3] = prevBuffer[1], and write everything else)
  alignas(16) ExposureBuffer initExposure = makeInitialExposure(exposure, 1.0f / exposure, 1, 1.0f / exposure, 1.f, 1.f);

  const bool ok = g_Exposure.getBuf()->updateData(0, data_size(initExposure), initExposure.data(), VBLOCK_WRITEONLY);
  if (!ok)
    debug("Adaptation: can't lock buffer for exposure");
  return ok;
}

bool ExposureCompute::seedExposure(float exposure, float inv_exposure, float prev_exposure_ratio)
{
  alignas(16) ExposureBuffer initExposure = makeInitialExposure(exposure, inv_exposure, prev_exposure_ratio, 0.0f, 1.f, 1.f);

  const bool ok = g_Exposure.getBuf()->updateData(0, data_size(initExposure), initExposure.data(), 0);
  if (!ok)
    logerr("Adaptation: can't lock buffer for exposure");
  return ok;
}

// Generate an HDR histogram.
void ExposureCompute::genHistogram(const AdaptationSettings &settings, bool use_albedo_luma)
{
  ShaderGlobal::set_int(vars.use_albedo_luma_adaptationVarId, use_albedo_luma ? 1 : 0);
  ShaderGlobal::set_int(vars.adaptation__includesHeroCockpitVarId, settings.includesHeroCockpit);
  ShaderGlobal::set_float(vars.adaptation__minSamplesVarId, settings.minSamples);

  // legacy center weighted function params for backward compatibility
  ShaderGlobal::set_float(vars.adaptation_center_weighted_paramVarId, settings.centerWeight);
  ShaderGlobal::set_float(vars.adaptation_cw_samples_countVarId, DispatchInfo::pixelsCount);

  const float R = settings.fnNonLinearDistributionRadius;
  const float r = settings.fnUniformDistributionRadius;
  const float s = settings.fnUniformSamplesPercentage * 0.01f;
  const float k = s > 0.0f ? r / sqrt(s) : 0.0f;
  const float p = settings.fnNonLinearDistributionCurvinesPow;
  const Point2 fma = s > 0.0f ? Point2{R, -1.0f * (R - k * sqrt(s)) / pow(1.0f - s, p)} : Point2{R, 0.0f};

  ShaderGlobal::set_float(vars.adaptation_samples_countVarId, DispatchInfo::pixelsCount);
  ShaderGlobal::set_float(vars.adaptation_uniform_samples_countVarId, (float)DispatchInfo::pixelsCount * s);
  ShaderGlobal::set_float(vars.adaptation_uniform_func_tanVarId, k);
  ShaderGlobal::set_float4(vars.adaptation_nonlinear_func_fmaVarId, Point4{fma.x, fma.y, 0.0f, 0.0f});
  ShaderGlobal::set_float(vars.adaptation_nonlinear_func_powVarId, p);

  ShaderGlobal::set_buffer(vars.ExposureInVarId, g_Exposure);
  generateHistogramCenterWeightedFromSourceCS->dispatch(DispatchInfo::groupsCount, 1, 1);
}

void ExposureCompute::accumulateHistogram() { accumulate_hist_cs->dispatch(1, 1, 1); }

void ExposureCompute::adaptExposure(const AdaptationSettings &settings, bool instant)
{
  if (settings.useLuminanceTrimming && settings.luminanceLowRange >= settings.luminanceHighRange)
    LOGERR_ONCE("adaptation: luminanceLowRange=%@ should be lower than luminanceHighRange=%@", settings.luminanceLowRange,
      settings.luminanceHighRange);

  ShaderGlobal::set_float(vars.adaptation_use_luminance_trimmingVarId, settings.useLuminanceTrimming ? 1.0f : 0.0f);
  ShaderGlobal::set_float4(vars.EyeAdaptationParams_0VarId, settings.luminanceLowRange, settings.luminanceHighRange,
    settings.minExposure, settings.maxExposure);
  ShaderGlobal::set_float4(vars.EyeAdaptationParams_1VarId, settings.autoExposureScale, accumulatedTime, settings.adaptDownSpeed,
    settings.adaptUpSpeed);
  ShaderGlobal::set_float4(vars.EyeAdaptationParams_2VarId, 1, 0, float(DispatchInfo::pixelsCount), settings.fadeMul);
  ShaderGlobal::set_float4(vars.EyeAdaptationParams_3VarId, settings.brightnessPerceptionLinear, settings.brightnessPerceptionPower,
    0., instant ? 1.f : 0.f);
  d3d::set_rwtex(STAGE_CS, 1, exposureNormalizationFactor.getTex2D(), 0, 0);
  ShaderGlobal::set_buffer(vars.ExposureOutVarId, g_Exposure);
  adaptExposureCS->dispatch(1, 1, 1);
  d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
  d3d::resource_barrier({exposureNormalizationFactor.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  accumulatedTime = 0;
}

bool ExposureCompute::updateReadback()
{
  if (!exposureReadback)
  {
    exposureReadback.reset(new RingCPUBufferLock());
    exposureReadback->init(4, EXPOSURE_BUF_SIZE, 4, "exposureReadBack", SBCF_UA_STRUCTURED_READBACK, 0, false);
  }

  bool fresh = false;
  int stride;
  uint32_t latestReadbackFrameRead;
  // WORKAROUND: some AMD cards in dx11 have issue with locking - the event has already happend, but GPU is still working with the
  // readback buffer
  static const bool getTopMost = !(d3d::get_driver_code().is(d3d::dx11) && d3d::get_driver_desc().info.vendor == GpuVendor::AMD);
  if (float *data = (float *)exposureReadback->lock(stride, latestReadbackFrameRead, getTopMost))
  {
    memcpy(lastExposureBuffer.data(), data, sizeof(lastExposureBuffer));

    if (DAGOR_UNLIKELY(isDumpExposureBuffer))
    {
      for (uint32_t i = 0; i < EXPOSURE_BUF_SIZE; ++i)
        debug("Exposure[%d]: %f", i, data[i]);
      isDumpExposureBuffer = false;
    }

    exposureReadback->unlock();
    fresh = true;
  }

  Sbuffer *newTarget = (Sbuffer *)exposureReadback->getNewTarget(latestReadbackFrameCopied);
  if (newTarget)
  {
    g_Exposure->copyTo(newTarget);
    exposureReadback->startCPUCopy();
  }
  return fresh;
}

void ExposureCompute::updateHistogramReadback(Sbuffer *histogram)
{
  if (!histogram)
    return;

  if (!histogramReadback)
  {
    histogramReadback.reset(new RingCPUBufferLock());
    histogramReadback->init(sizeof(uint32_t), ADAPTATION_HISTOGRAM_BINS, 2, "adaptationHistogramReadback", SBCF_UA_STRUCTURED_READBACK,
      0, false);
  }

  int stride;
  uint32_t frameRead;
  if (const auto *data = reinterpret_cast<uint32_t *>(histogramReadback->lock(stride, frameRead, true)))
  {
    memcpy(lastHistogram.data(), data, sizeof(lastHistogram));
    histogramReadback->unlock();
  }

  Sbuffer *newTarget = static_cast<Sbuffer *>(histogramReadback->getNewTarget(latestHistogramFrameCopied));
  if (newTarget)
  {
    histogram->copyTo(newTarget);
    histogramReadback->startCPUCopy();
  }
}
