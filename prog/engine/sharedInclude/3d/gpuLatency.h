// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_latencyTypes.h>
#include <generic/dag_span.h>

enum class GpuVendor : uint8_t;

struct GpuLatency
{
  enum class Mode
  {
    Off,
    On,
    OnPlusBoost
  };

  virtual ~GpuLatency() = default;

  virtual dag::ConstSpan<Mode> getAvailableModes() const = 0;

  virtual void startFrame(uint32_t frame_id) = 0;
  virtual void setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) = 0;
  virtual void setOptions(Mode mode, uint32_t frame_limit_us) = 0;
  virtual void sleep(uint32_t frame_id) = 0;
  virtual lowlatency::LatencyData getStatisticsSince(uint32_t frame_id, uint32_t max_count = 0) = 0;

  virtual void *getHandle() const = 0;
  virtual bool isEnabled() const = 0;

  virtual GpuVendor getVendor() const = 0;
  virtual bool isVsyncAllowed() const = 0;

  static GpuLatency *create(GpuVendor vendor);
  static void teardown();
  static GpuLatency *getInstance();
};
