// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/entityComponent.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_commands.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>

class ComputeShaderElement;
class RingCPUBufferLock;

static const uint32_t EXPOSURE_BUF_SIZE = 16;
using ExposureBuffer = eastl::array<float, EXPOSURE_BUF_SIZE>;

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

struct AdaptationManager
{
  AdaptationManager();

  bool isFixedExposure() const { return settings.isFixedExposure(); }
  bool getExposure(float &exp) const;
  void updateExposure();
  void setExposure(bool value);
  bool writeExposure(float exposure);
  AdaptationSettings getSettings() { return settings; }
  bool setSettings(const AdaptationSettings &value);
  void dumpExposureBuffer() { isDumpExposureBuffer = true; }
  void updateTime(float dt) { accumulatedTime += dt; }
  void genHistogramFromSource();
  void accumulateHistogram();
  void adaptExposure();
  void updateReadbackExposure();
  void afterDeviceReset();
  void clearNormalizationFactor();
  void sheduleClear() { isClearNeeded = true; }
  D3DRESID getExposureBufferId() { return g_Exposure.getBufId(); }
  void uploadInitialExposure();
  const ExposureBuffer &getLastExposureBuffer() const { return lastExposureBuffer; }

private:
  friend class AdaptationDebug;
  struct DispatchInfo
  {
    static constexpr int groupsCount = 32;
    static constexpr int threadsPerGroup = 64;
    static constexpr int pixelsCount = threadsPerGroup * groupsCount;
  };

private:
  UniqueTex exposureNormalizationFactor;
  UniqueBuf g_Exposure, g_NoExposureBuffer;
  eastl::unique_ptr<ComputeShaderElement> generateHistogramCenterWeightedFromSourceCS;
  eastl::unique_ptr<ComputeShaderElement> adaptExposureCS, accumulate_hist_cs;
  eastl::unique_ptr<RingCPUBufferLock> exposureReadback;
  dafg::NodeHandle registerExposureNodeHandle;
  AdaptationSettings settings;
  float accumulatedTime = 0;
  float lastFixedExposure = -1;
  uint32_t latestReadbackFrameCopied = 0, lastValidReadbackFrame = 0;
  bool exposureWritten = false;
  bool isDumpExposureBuffer = false;
  bool isClearNeeded = false;
  ExposureBuffer lastExposureBuffer = {};
};

ECS_DECLARE_BOXED_TYPE(AdaptationManager);