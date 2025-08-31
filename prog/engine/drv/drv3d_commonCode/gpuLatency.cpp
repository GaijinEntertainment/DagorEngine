// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/gpuLatency.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_commands.h>

static GpuLatency *instance = nullptr;

GpuLatency *GpuLatency::create(GpuVendor vendor)
{
  G_ASSERT(!instance);

  d3d::driver_command(Drv3dCommand::CREATE_GPU_LATENCY, (void *)&vendor, (void *)&instance);
  return instance;
}

void GpuLatency::teardown()
{
  if (instance)
  {
    delete instance;
    instance = nullptr;
  }
}

GpuLatency *GpuLatency::getInstance() { return instance; }
