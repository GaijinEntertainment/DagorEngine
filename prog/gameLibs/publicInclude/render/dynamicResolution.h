//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_tex3d.h>
#include <3d/dag_texMgr.h>
#include <util/dag_stdint.h>
#include <EASTL/vector.h>

class DynamicResolution
{
public:
  DynamicResolution(int target_width, int target_height);
  ~DynamicResolution();

  void applySettings();

  void beginFrame();
  void endFrame();
  void getCurrentResolution(int &w, int &h) const
  {
    w = currentResolutionWidth;
    h = currentResolutionHeight;
  }
  int getTargetFrameRate() { return targetFrameRate; }

  void debugImguiWindow();

private:
  void adjustResolution();

  struct TimestampQueries
  {
    void *beginQuery = nullptr;
    void *endQuery = nullptr;
  };

  enum GpuFrameState
  {
    GPU_FRAME_STARTED,
    GPU_FRAME_FINISHED
  };
  GpuFrameState gpuFrameState = GPU_FRAME_FINISHED;

  bool autoAdjust = true;
  float maxResolutionScale = 1.0;
  float minResolutionScale = 0.5;
  float resolutionScaleStep;

  double gpuMsPerTick;

  unsigned frameIdx = 0;

  int targetFrameRate;
  float targetMsPerFrame;
  float currentMsPerFrame = 0;
  float maxThresholdToChange = 0.95;
  float minThresholdToChange = 0.85;
  int framesAboveThreshold = 0;
  int framesUnderThreshold = 0;

  int targetResolutionWidth, targetResolutionHeight;
  int currentResolutionWidth, currentResolutionHeight;

  float resolutionScale = 1.0;


  eastl::vector<TimestampQueries> timestamps;
};
