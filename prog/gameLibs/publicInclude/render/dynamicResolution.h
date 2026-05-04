//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <util/dag_stdint.h>
#include <EASTL/vector.h>
#include <math/integer/dag_IPoint2.h>

class DynamicResolution
{
public:
  DynamicResolution(int target_width, int target_height);
  ~DynamicResolution();

  DynamicResolution(const DynamicResolution &) = delete;
  DynamicResolution(DynamicResolution &&) = default;
  DynamicResolution &operator=(const DynamicResolution &) = delete;
  DynamicResolution &operator=(DynamicResolution &&) = default;

  void applySettings();

  void beginFrame();
  void endFrame();
  void getCurrentResolution(int &w, int &h) const
  {
    w = currentResolutionWidth;
    h = currentResolutionHeight;
  }
  void getMaxPossibleResolution(int &w, int &h) const
  {
    w = targetResolutionWidth * maxResolutionScale;
    h = targetResolutionHeight * maxResolutionScale;
  }
  int getTargetFrameRate() { return targetFrameRate; }
  void setMinimumMsPerFrame(float ms) { minimumMsPerFrame = ms; }

  void debugImguiWindow();

  void setResolutionRange(const IPoint2 &min_dynamic_resolution, const IPoint2 &max_dynamic_resolution);

private:
  void trackCpuTime();
  void calculateAllowableTimeRange(float &lower_bound, float &upper_bound);
  void adjustResolution();

  struct TimestampQueries
  {
    void *beginQuery = nullptr;
    void *endQuery = nullptr;
    void *vsyncStallBeginQuery = nullptr;
    void *vsyncStallEndQuery = nullptr;
  };

  enum GpuFrameState
  {
    GPU_FRAME_STARTED,
    GPU_FRAME_FINISHED
  };
  GpuFrameState gpuFrameState = GPU_FRAME_FINISHED;

  bool autoAdjust = true;
  bool considerCPUbottleneck = false;
  float maxResolutionScale = 1.0;
  float minResolutionScale = 0.5;
  float resolutionScaleStep;

  float currentCpuMsPerFrame = 0;

  double gpuMsPerTick;

  unsigned frameIdx = 0;

  float targetFrameRate;
  float targetMsPerFrame;
  float minimumMsPerFrame = 0.f;
  float currentGpuMsPerFrame = 0;
  float maxThresholdToChange = 0.95;
  float minThresholdToChange = 0.85;
  int framesAboveThreshold = 0;
  int framesUnderThreshold = 0;

  int targetResolutionWidth, targetResolutionHeight;
  int currentResolutionWidth, currentResolutionHeight;

  float resolutionScale = 1.0;

  bool vsyncEnabled = false;

  eastl::vector<TimestampQueries> timestamps;
};
