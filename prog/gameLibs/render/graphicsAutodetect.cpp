#include <render/graphicsAutodetect.h>
#include "ioSys/dag_dataBlock.h"
#include "gui/dag_visConsole.h"
#include "3d/dag_drv3dCmd.h"
#include <3d/dag_gpuConfig.h>

const float GPU_BENCHMARK_DURATION = 10.0f;


GraphicsAutodetect::GraphicsAutodetect(const DataBlock &blk, float gpu_time_avg_min) : gpuTimeAvgMin(gpu_time_avg_min)
{
  gpuBenchmarkDuration = blk.getReal("gpuBenchmarkDuration", GPU_BENCHMARK_DURATION);

  const DataBlock *presetsBlk = blk.getBlockByNameEx("presets");
  presets.resize(presetsBlk->blockCount());
  for (int presetNo = 0; presetNo < presetsBlk->blockCount(); ++presetNo)
  {
    const DataBlock *presetBlk = presetsBlk->getBlock(presetNo);
    presets[presetNo].name = presetBlk->getBlockName();
    presets[presetNo].timeMs30 = presetBlk->getReal("timeMs30", 0.0f);
    presets[presetNo].timeMs60 = presetBlk->getReal("timeMs60", 0.0f);
    presets[presetNo].timeMs120 = presetBlk->getReal("timeMs120", 0.0f);
    presets[presetNo].timeMs240 = presetBlk->getReal("timeMs240", 0.0f);
  }
  presetDefault = blk.getStr("presetDefault", "high");
  presetUltralow = blk.getStr("presetUltralow", "ultralow");

  ultraLowMem = d3d_get_gpu_cfg().ultraLowMem;

  gpuBenchmark.reset(new GpuBenchmark());
}

GraphicsAutodetect::~GraphicsAutodetect() { stopGpuBenchmark(); }

void GraphicsAutodetect::render(Texture *target_tex, Texture *depth_tex)
{
  if (!gpuBenchmarkStarted)
    return;

  G_ASSERT(gpuBenchmark);
  gpuBenchmark->render(target_tex, depth_tex);
}

void GraphicsAutodetect::update(float dt)
{
  if (!gpuBenchmarkStarted)
    return;
  if (gpuBenchmarkTime > GPU_BENCHMARK_DURATION)
  {
    stopGpuBenchmark();
    String result = String(0,
      "gpuTimeAvg - %0.3f\n"
      "gpuTimeAvgMin - %0.3f\n"
      "max quality preset - %s\n"
      "60 fps preset - %s\n"
      "max fps preset(%0.3f hz) - %s",
      gpuTimeAvg, gpuTimeAvgMin, getPresetForMaxQuality().str(), getPresetFor60Fps().str(), getVSyncRefreshRate(),
      getPresetForMaxFPS().str());
#if DAGOR_DBGLEVEL > 0
    console::print_d(result);
#else
    debug("%s", result.c_str());
#endif
    return;
  }

  gpuBenchmarkTime += dt;

  G_ASSERT(gpuBenchmark);
  gpuBenchmark->update(dt);
}

void GraphicsAutodetect::startGpuBenchmark()
{
  gpuBenchmarkStarted = true;
  gpuBenchmarkTime = 0;
  gpuTimeAvg = 0;
  G_ASSERT(gpuBenchmark);
  gpuBenchmark->resetTimings();
}

void GraphicsAutodetect::stopGpuBenchmark()
{
  if (!gpuBenchmarkStarted)
    return;

  gpuBenchmarkStarted = false;

  G_ASSERT(gpuBenchmark);
  gpuTimeAvg = gpuBenchmark->getAvgGpuTime() * 0.001f;
  gpuTimeAvgMin = gpuTimeAvgMin > 0 ? min(gpuTimeAvgMin, gpuTimeAvg) : gpuTimeAvg;
}

float GraphicsAutodetect::getVSyncRefreshRate() const
{
  double refreshRate = 0;
  d3d::driver_command(DRV3D_COMMAND_GET_VSYNC_REFRESH_RATE, &refreshRate, NULL, NULL);
  return refreshRate;
}

bool GraphicsAutodetect::isUltraLowMem() const { return ultraLowMem; }

const SimpleString &GraphicsAutodetect::getPresetForMaxQuality() const
{
  if (gpuTimeAvgMin == 0)
    return presetDefault;
  if (ultraLowMem)
    return presetUltralow;

  for (int presetNo = presets.size() - 1; presetNo >= 0; --presetNo)
  {
    if (presets[presetNo].timeMs30 > gpuTimeAvgMin)
      return presets[presetNo].name;
  }

  return presets.size() > 0 ? presets[0].name : presetDefault;
}

const SimpleString &GraphicsAutodetect::getPresetFor60Fps() const
{
  if (gpuTimeAvgMin == 0)
    return presetDefault;
  if (ultraLowMem)
    return presetUltralow;

  for (int presetNo = presets.size() - 1; presetNo >= 0; --presetNo)
  {
    if (presets[presetNo].timeMs60 > gpuTimeAvgMin)
      return presets[presetNo].name;
  }

  return presets.size() > 0 ? presets[0].name : presetDefault;
}

const SimpleString &GraphicsAutodetect::getPresetForMaxFPS() const
{
  if (gpuTimeAvgMin == 0)
    return presetDefault;
  if (ultraLowMem)
    return presetUltralow;

  int refreshRate = static_cast<int>(floor(getVSyncRefreshRate()));
  if (refreshRate > 0 && refreshRate <= 60)
    return getPresetFor60Fps();

  for (int presetNo = presets.size() - 1; presetNo >= 0; --presetNo)
  {
    if (refreshRate > 0 && refreshRate <= 120 && presets[presetNo].timeMs120 > gpuTimeAvgMin)
      return presets[presetNo].name;
    else if (presets[presetNo].timeMs240 > gpuTimeAvgMin)
      return presets[presetNo].name;
  }

  return presets.size() > 0 ? presets[0].name : presetDefault;
}