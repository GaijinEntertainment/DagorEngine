//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric_limits.h>

namespace lowlatency
{
enum class LatencyMarkerType
{
  SIMULATION_START,
  SIMULATION_END,
  RENDERSUBMIT_START,
  RENDERSUBMIT_END,
  PRESENT_START,
  PRESENT_END,
  TRIGGER_FLASH
};

struct Statistics
{
  float min = eastl::numeric_limits<float>::infinity();
  float max = -eastl::numeric_limits<float>::infinity();
  float avg = 0;
  void apply(float value)
  {
    min = eastl::min(min, value);
    max = eastl::max(max, value);
    avg += value;
  }
  void finalize(uint32_t count) // count is not stored inside the struct
  {
    if (count > 0)
      avg /= float(count);
  }
};

struct LatencyData
{
  // times are measured in ms
  // time points are relative to simStartTime
  uint32_t frameCount = 0;
  uint32_t latestFrameId = 0;

  // These are currently measured by only the NVIDIA low latency sdk
  // but they can be implemented without the sdk
  // implementation should be placed in the set_marker function
  Statistics gameLatency;
  Statistics inputSampleTime;
  Statistics simEndTime;
  Statistics renderSubmitStartTime;
  Statistics renderSubmitEndTime;
  Statistics presentStartTime;
  Statistics presentEndTime;

  // these are only available with NVIDIA low latency sdk
  Statistics gameToRenderLatency;
  Statistics renderLatency;
  Statistics driverStartTime;
  Statistics driverEndTime;
  Statistics osRenderQueueStartTime;
  Statistics osRenderQueueEndTime;
  Statistics gpuRenderStartTime;
  Statistics gpuRenderEndTime;
};

using LatencyModeFlag = uint8_t;

enum LatencyMode : LatencyModeFlag
{
  LATENCY_MODE_OFF = 0,
  LATENCY_MODE_ON = 1, // present block + sleep before sampling input

  // NVIDIA low latency sdk
  LATENCY_MODE_NV_BOOST = 2, // ON + gpu clock control
};
} // namespace lowlatency
