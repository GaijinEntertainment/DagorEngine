#include "timelines.h"

namespace drv3d_vulkan
{

template <>
TimelineManager::CpuReplay &TimelineManager::get()
{
  return cpuReplay;
}

template <>
TimelineManager::GpuExecute &TimelineManager::get()
{
  return gpuExecute;
}

template <>
PipelineCompileTimeline &TimelineManager::get()
{
  return pipelineCompileTimeline;
}

void TimelineManager::shutdown()
{
  cpuReplay.shutdown();
  gpuExecute.shutdown();
  pipelineCompileTimeline.shutdown();
}

void TimelineManager::init()
{
  cpuReplay.init();
  gpuExecute.init();
  pipelineCompileTimeline.init();
}

} // namespace drv3d_vulkan