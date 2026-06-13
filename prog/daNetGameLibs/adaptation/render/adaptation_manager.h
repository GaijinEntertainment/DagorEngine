// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/entityComponent.h>
#include <3d/dag_resPtr.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <render/exposureCompute.h>

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
  void dumpExposureBuffer() { exposure.dumpExposureBuffer(); }
  void updateTime(float dt) { exposure.updateTime(dt); }
  void genHistogramFromSource(bool use_albedo_luma);
  void accumulateHistogram();
  void adaptExposure();
  void updateReadbackExposure();
  void afterDeviceReset();
  void clearNormalizationFactor();
  void sheduleClear() { exposure.sheduleClear(); }
  D3DRESID getExposureBufferId() { return exposure.getExposureBufferId(); }
  void uploadInitialExposure();
  const ExposureBuffer &getLastExposureBuffer() const { return exposure.getLastExposureBuffer(); }
  void updateHistogramReadback(Sbuffer *histogram) { exposure.updateHistogramReadback(histogram); }
  const AdaptationHistogram &getLastHistogram() const { return exposure.getLastHistogram(); }

  using DispatchInfo = ExposureCompute::DispatchInfo;

private:
  void fillNoExposureBuffer();

  ExposureCompute exposure;
  UniqueBuf g_NoExposureBuffer;
  dafg::NodeHandle registerExposureNodeHandle;
  AdaptationSettings settings;
  float lastFixedExposure = -1;
  bool exposureWritten = false;
};

ECS_DECLARE_BOXED_TYPE(AdaptationManager);
