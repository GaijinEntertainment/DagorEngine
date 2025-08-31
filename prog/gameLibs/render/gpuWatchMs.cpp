// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/gpuWatchMs.h>
#include <debug/dag_debug.h>
#include <drv/3d/dag_commands.h>

uint64_t GPUWatchMs::gpuFreq = 0;

void GPUWatchMs::init_freq()
{
  if (gpuFreq)
    return;

  // first query can be 0, retry for some if it is
  const int RETRY_COUNT = 10;
  for (int i = 0; i < RETRY_COUNT; ++i)
  {
    d3d::driver_command(Drv3dCommand::TIMESTAMPFREQ, &gpuFreq);
    if (gpuFreq)
    {
      debug("GPUWatchMs: GPU ts freq %llu", gpuFreq);
      return;
    }
  }
  logwarn("GPUWatchMs: GPU timings unavailable");
}

bool GPUWatchMs::available() { return gpuFreq != 0; }

GPUWatchMs::~GPUWatchMs()
{
  d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &startQuery);
  d3d::driver_command(Drv3dCommand::RELEASE_QUERY, &endQuery);
}

void GPUWatchMs::start() { d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &startQuery); }

void GPUWatchMs::stop() { d3d::driver_command(Drv3dCommand::TIMESTAMPISSUE, &endQuery); }

bool GPUWatchMs::read(uint64_t &ov, uint64_t units)
{
  if (!endQuery || !startQuery)
    return false;

  int64_t tEnd, tStart;
  if (!d3d::driver_command(Drv3dCommand::TIMESTAMPGET, endQuery, &tEnd))
    return false;
  if (!d3d::driver_command(Drv3dCommand::TIMESTAMPGET, startQuery, &tStart))
    return false;

  // 1000 - convert from 1/s to 1/ms
  ov = (tEnd - tStart) / max<uint64_t>(gpuFreq / max<uint64_t>(units, 1), 1);

  return true;
}