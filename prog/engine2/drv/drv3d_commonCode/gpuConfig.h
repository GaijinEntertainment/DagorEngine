#pragma once

#include <3d/dag_gpuConfig.h>

struct GpuDriverConfig : GpuUserConfig
{
  GpuDriverConfig();

  bool forceFullscreenToWindowed;
  bool flushBeforeSurvey;
};

const GpuDriverConfig &get_gpu_driver_cfg();