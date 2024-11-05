// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include "gpuConfig.h"
#include "drv_log_defs.h"

#if !_TARGET_APPLE // see displayM.mm for MacOS

#include <math/dag_mathUtils.h>

#if _TARGET_PC_WIN
#include <Windows.h>
#endif

float d3d::get_display_scale()
{
#if _TARGET_PC_WIN
  HDC hdc = GetDC(0);
  if (!hdc)
    return -1.f;

  int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
  ReleaseDC(0, hdc);
  float scale = dpi / 96.f;
  return scale > 1.f ? scale : 1.f;
#else
  return -1.f;
#endif
}

// Use dt fusion weighted by frame to frame difference.

void select_best_dt(float cpu_dt, float gpu_dt, float &best_dt, bool &use_gpu_dt)
{
  const float AVG_DT_SMOOTHNESS = 0.99f;
  const float GPU_MAX_DEVIATION = 0.2f;
  const float GPU_DT_MIN_FPS = 59.f;
  static float averageCpuDt = cpu_dt;
  static float averageGpuDt = gpu_dt;
  static float prevDt = cpu_dt;
  averageCpuDt = lerp(cpu_dt, averageCpuDt, AVG_DT_SMOOTHNESS);
  averageGpuDt = lerp(gpu_dt, averageGpuDt, AVG_DT_SMOOTHNESS);
  best_dt = cpu_dt;
  if (safediv(fabsf(averageGpuDt - averageCpuDt), averageCpuDt) < GPU_MAX_DEVIATION && gpu_dt < 1.f / GPU_DT_MIN_FPS)
  {
    float cpuWeight = 1.f / (FLT_EPSILON + sqr(cpu_dt - prevDt));
    float gpuWeight = 1.f / (FLT_EPSILON + sqr(gpu_dt - prevDt));
    best_dt = (cpuWeight * cpu_dt + gpuWeight * gpu_dt) / (cpuWeight + gpuWeight);
  }
  prevDt = best_dt;
  use_gpu_dt = true;
}

#endif

void d3d::update_window_mode()
{
  auto &videoBlk = *dgs_get_settings()->getBlockByNameEx("video");
  if (int idx = videoBlk.findParam("mode"); idx != -1)
  {
    auto modeStr = videoBlk.getStr(idx);
    if (!strcmp(modeStr, "fullscreenwindowed"))
    {
      dgs_set_window_mode(WindowMode::WINDOWED_FULLSCREEN);
      return;
    }
    if (!strcmp(modeStr, "windowed"))
    {
      dgs_set_window_mode(WindowMode::WINDOWED);
      return;
    }
    if (!strcmp(modeStr, "resizablewindowed"))
    {
      dgs_set_window_mode(WindowMode::WINDOWED_RESIZABLE);
      return;
    }
    if (!strcmp(modeStr, "noborderwindow"))
    {
      dgs_set_window_mode(WindowMode::WINDOWED_NO_BORDER);
      return;
    }
    if (strcmp(modeStr, "fullscreen") != 0)
    {
      D3D_ERROR("%s is invalid arg for config:video/mode:t", modeStr);
    }
  }

  if (get_gpu_driver_cfg().forceFullscreenToWindowed || videoBlk.getBool("forbidTrueFullscreen", false))
  {
    dgs_set_window_mode(WindowMode::WINDOWED_FULLSCREEN);
    return;
  }

  dgs_set_window_mode(WindowMode::FULLSCREEN_EXCLUSIVE);
}