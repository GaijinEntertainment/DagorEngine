//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint2.h>

#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include <EASTL/optional.h>

class HudPrimitives;

enum class PerfDisplayMode
{
  OFF = 0,
  FPS,
  COMPACT,
  FULL
};

class FrameTimeMetricsAggregator
{
  eastl::fixed_string<char, 64> fpsText;
  eastl::string latencyText;
  // This ensures that all instances refresh in sync
  int lastPeriod = 0;
  int lastLatencyFrame = 0;
  int lastTimeMsec = 0;
  int lastFrameNo = 0;
  int textVersion = 0;
  float lastAverageFrameTime = 0.f;
  float lastAverageFps = 0.f;
  float lastMinFrameTime = 0.f;
  float lastMaxFrameTime = 0.f;
  float minFrameTime = 0.f;
  float maxFrameTime = 0.f;
  float lastAverageLatency = 0.f;
  float lastAverageLatencyA = 0.f;
  float lastAverageLatencyR = 0.f;
  PerfDisplayMode displayMode = PerfDisplayMode::OFF;
  bool achtung = false;
  eastl::optional<bool> isPixCapturerLoaded;
  eastl::optional<IPoint2> renderingResolution;

public:
  void update(float current_time_msec, uint32_t frame_no, float dt, PerfDisplayMode display_mode);
  const auto &getFpsInfoString() const { return fpsText; }
  const auto &getLatencyInfoString() const { return latencyText; }
  float getLastMinFrameTime() const { return lastMinFrameTime; }
  float getLastMaxFrameTime() const { return lastMaxFrameTime; }
  float getLastAverageFps() const { return lastAverageFps; }
  float getLastAverageFrameTime() const { return lastAverageFrameTime; }
  int getTextVersion() const { return textVersion; }
  float getLastAverageLatency() const { return lastAverageLatency; }
  float getLastAverageActLatency() const { return lastAverageLatencyA; }
  float getLastAverageRenderLatency() const { return lastAverageLatencyR; }
  PerfDisplayMode getDisplayMode() const { return displayMode; }
  uint32_t getColorForFpsInfo() const;
  void setRenderingResolution(const eastl::optional<IPoint2> &resolution) { renderingResolution = resolution; }
};

enum QualityColors
{
  EPIC = 0xffb080ffu,
  GOOD = 0xff00ff00u,
  OKAY = 0xffffa500u, // OKAY is worse then OK
  POOR = 0xffff0000u
};

const uint32_t SYSTEM_TEXT_BORDER_X = 4;
