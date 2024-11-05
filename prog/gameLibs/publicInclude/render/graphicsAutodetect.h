//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <3d/dag_texMgr.h>
#include <render/gpuBenchmark.h>
#include <util/dag_simpleString.h>
#include <EASTL/vector.h>

class DataBlock;

class GraphicsAutodetect
{
public:
  GraphicsAutodetect(const DataBlock &blk, float gpu_time_avg_min);
  ~GraphicsAutodetect();

  void render(Texture *target_tex, Texture *depth_tex);
  void update(float dt);

  void startGpuBenchmark();
  void stopGpuBenchmark();
  bool isGpuBenchmarkRunning() const { return gpuBenchmarkStarted; }
  float getGpuTimeAvg() const { return gpuTimeAvg; }
  float getGpuTimeAvgMin() const { return gpuTimeAvgMin; }
  float getGpuBenchmarkDuration() const { return gpuBenchmarkDuration; }
  float getVSyncRefreshRate() const;
  bool isUltraLowMem() const;
  const SimpleString &getPresetForMaxQuality() const;
  const SimpleString &getPresetFor60Fps() const;
  const SimpleString &getPresetForMaxFPS() const;

private:
  struct Preset
  {
    SimpleString name;
    float timeMs30;
    float timeMs60;
    float timeMs120;
    float timeMs240;
  };
  eastl::vector<Preset> presets;
  SimpleString presetDefault;
  SimpleString presetUltralow;
  bool ultraLowMem;

  eastl::unique_ptr<GpuBenchmark> gpuBenchmark;
  bool gpuBenchmarkStarted = false;
  float gpuBenchmarkTime = 0;
  float gpuTimeAvg = 0;
  float gpuTimeAvgMin = 0;
  float gpuBenchmarkDuration = 0.0f;
};
