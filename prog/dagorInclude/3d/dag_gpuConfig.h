//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driverCode.h>
#include <drv/3d/dag_consts.h>
#include <util/dag_string.h>
#include <EASTL/vector.h>

class DataBlock;

struct GpuVideoSettings
{
  DriverCode drvCode;
  bool disableNvTweaks = false;
  bool disableAtiTweaks = false;
  bool ignoreOutdatedDriver = false;
  bool configCompatibilityMode = false;
  bool allowDx10Fallback = false;
  bool adjustVideoSettings = false;
  bool vulkanNVnativePresent = false;
  eastl::vector<String> oldHardwareList;
  int lowVideoMemMb = 0;
  int ultraLowVideoMemMb = 0;
  int lowSystemMemAtMb = 0;
  int ultralowSystemMemAtMb = 0;
};

struct GpuUserConfig
{
  GpuVendor primaryVendor = GpuVendor::UNKNOWN;
  uint32_t deviceId = 0;
  bool outdatedDriver = false;
  bool fallbackToCompatibilty = false;
  union
  {
    bool integrated;
    bool usedSlowIntegrated = false;
  };
  bool usedSlowIntegratedSwitchableGpu = false;
  bool gradientWorkaroud = false;
  bool disableSbuffers = false;
  bool forceDx10 = false;
  bool hardwareDx10 = false;
  bool oldHardware = false;
  uint32_t driverVersion[4] = {0, 0, 0, 0};

  bool lowMem = false;
  bool ultraLowMem = false;
  uint32_t videoMemMb = 0;
  int freePhysMemMb = 0;
  int64_t freeVirtualMemMb = 0;
  int64_t totalVirtualMemMb = 0;
};

void d3d_read_gpu_video_settings(const DataBlock &blk, GpuVideoSettings &out_video);
const GpuUserConfig &d3d_get_gpu_cfg();

void d3d_apply_gpu_settings(const GpuVideoSettings &video);
void d3d_apply_gpu_settings(const DataBlock &blk);
