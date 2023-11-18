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
  typedef Timeline<RenderWork, CpuReplaySync, MAX_RETIREMENT_QUEUE_ITEMS> CpuReplay;
  typedef Timeline<FrameInfo, GpuExecuteSync, FRAME_FRAME_BACKLOG_LENGTH> GpuExecute;

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

} // namespace drv3d_vulkan
