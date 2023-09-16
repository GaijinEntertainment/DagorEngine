#include <render/gpuWatchMs.h>
#include <debug/dag_debug.h>
#include <3d/dag_drv3dCmd.h>

uint64_t GPUWatchMs::gpuFreq = 0;

void GPUWatchMs::init_freq()
{
  if (gpuFreq)
    return;

  // first query can be 0, retry for some if it is
  const int RETRY_COUNT = 10;
  for (int i = 0; i < RETRY_COUNT; ++i)
  {
    d3d::driver_command(D3V3D_COMMAND_TIMESTAMPFREQ, &gpuFreq, 0, 0);
    if (gpuFreq)
    {
      debug("dynamicQuality: GPU ts freq %u", gpuFreq);
      return;
    }
  }
  logwarn("dynamicQuality: GPU timings unavailable");
}

bool GPUWatchMs::available() { return gpuFreq != 0; }

GPUWatchMs::~GPUWatchMs()
{
  d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &startQuery, 0, 0);
  d3d::driver_command(DRV3D_COMMAND_RELEASE_QUERY, &endQuery, 0, 0);
}

void GPUWatchMs::start() { d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &startQuery, 0, 0); }

void GPUWatchMs::stop() { d3d::driver_command(D3V3D_COMMAND_TIMESTAMPISSUE, &endQuery, 0, 0); }

bool GPUWatchMs::read(uint64_t &ov, uint64_t units)
{
  if (!endQuery || !startQuery)
    return false;

  int64_t tEnd, tStart;
  if (!d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, endQuery, &tEnd, 0))
    return false;
  if (!d3d::driver_command(D3V3D_COMMAND_TIMESTAMPGET, startQuery, &tStart, 0))
    return false;

  // 1000 - convert from 1/s to 1/ms
  ov = (tEnd - tStart) / max<uint64_t>(gpuFreq / max<uint64_t>(units, 1), 1);

  return true;
}