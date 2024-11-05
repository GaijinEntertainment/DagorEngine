// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "timeline.h"
#include "driver.h"
#include "render_work.h"
#include "frame_info.h"
#include "pipeline/compiler.h"

namespace drv3d_vulkan
{

class TimelineManager
{

  struct CpuReplaySync : public TimelineSyncPartLockFree,
                         public TimelineSyncPartSingleWriterSingleReader,
                         public TimelineSyncPartEventWaitable
  {};

  struct GpuExecuteSync : public TimelineSyncPartLockFree, public TimelineSyncPartNonConcurrent, public TimelineSyncPartNonWaitable
  {};

public:
  typedef Timeline<RenderWork, CpuReplaySync, REPLAY_TIMELINE_HISTORY_SIZE> CpuReplay;
  typedef Timeline<FrameInfo, GpuExecuteSync, GPU_TIMELINE_HISTORY_SIZE> GpuExecute;

private:
  CpuReplay cpuReplay;
  GpuExecute gpuExecute;
  PipelineCompileTimeline pipelineCompileTimeline;

  TimelineManager(const TimelineManager &) = delete;

public:
  template <typename T>
  T &get();

  void shutdown();
  void init();

  TimelineManager() = default;
};

// wrappers to make them accesible via forward declaration
class GpuExecuteTimelineSpan : public TimelineSpan<TimelineManager::GpuExecute>
{
public:
  template <typename Manager>
  GpuExecuteTimelineSpan(Manager &tl_man) : TimelineSpan<TimelineManager::GpuExecute>(tl_man)
  {}
};

class CpuReplayTimelineSpan : public TimelineSpan<TimelineManager::CpuReplay>
{
public:
  template <typename Manager>
  CpuReplayTimelineSpan(Manager &tl_man) : TimelineSpan<TimelineManager::CpuReplay>(tl_man)
  {}
};

} // namespace drv3d_vulkan
