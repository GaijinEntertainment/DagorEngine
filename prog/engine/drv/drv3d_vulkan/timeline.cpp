#include "timeline.h"

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

void TimelineManager::shutdown()
{
  cpuReplay.shutdown();
  gpuExecute.shutdown();
}

void TimelineManager::init()
{
  cpuReplay.init();
  gpuExecute.init();
}

} // namespace drv3d_vulkan