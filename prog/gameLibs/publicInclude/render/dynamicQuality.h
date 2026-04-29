//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <render/gpuWatchMs.h>
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <math/integer/dag_IPoint2.h>
namespace ecs
{
class EntityManager;
}
#include <daECS/core/event.h>
#include <util/dag_simpleString.h>
#include <EASTL/array.h>

class DynamicQuality
{
  struct QualityRange
  {
    // desc part
    IPoint2 frameTimeMs;
    uint32_t frameCountThreshold;
    String name;
  };

  dag::Vector<QualityRange> ranges;
  uint32_t currentRange = 0;
  uint32_t defaultRange = 0;
  int32_t balance;

  constexpr static int GPU_TIMESTAMP_LATENCY = 5;
  eastl::array<GPUWatchMs, GPU_TIMESTAMP_LATENCY> timings;
  size_t timingIdx = 0;

  void processTimingRecord(ecs::EntityManager &mgr, uint64_t gpu_time_ms);
  void broadcastEvent(ecs::EntityManager &mgr);
  bool allowTracking();

public:
  DynamicQuality(ecs::EntityManager &mgr, const DataBlock *cfg);
  ~DynamicQuality();
  void reset(ecs::EntityManager &mgr, const DataBlock *cfg);

  void onFrameStart(ecs::EntityManager &mgr);
  void onFrameEnd();

#if DAGOR_DBGLEVEL > 0
  uint64_t lastGpuTime = 0;
  size_t timingLatency = 0;
  dag::Vector<int32_t> rangeBarsX;
  dag::Vector<int32_t> rangeBarsY;
  void debugUI();
#endif
};

struct DynamicQualityChangeEvent : public ecs::Event
{
  SimpleString qualityRange;
  ECS_BROADCAST_EVENT_DECL(DynamicQualityChangeEvent)
  DynamicQualityChangeEvent(const String &qr) : ECS_EVENT_CONSTRUCTOR(DynamicQualityChangeEvent), qualityRange(qr) {}
};
