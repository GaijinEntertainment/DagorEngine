//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_DynamicShaderHelper.h>
#include <3d/dag_sbufferIDHolder.h>
#include <3d/dag_resPtr.h>
#include <EASTL/array.h>
#include <render/gpuWatchMs.h>

class GpuBenchmark final
{
public:
  GpuBenchmark();
  ~GpuBenchmark();

  void update(float dt);
  void render(Texture *target_tex, Texture *depth_tex);
  void resetTimings();

  uint64_t getLastGpuTime() const { return lastGpuTime; }
  uint64_t getAvgGpuTime() const { return totalGpuTime / max<uint64_t>(numGpuFrames, 1); }

private:
  float rotAngle = 0;
  uint64_t lastGpuTime = 0;
  uint64_t totalGpuTime = 0;
  uint64_t numGpuFrames = 0;
  uint64_t numWarmGpuFrames = 0;

  DynamicShaderHelper benchShader;
  SbufferIDHolder vb;
  UniqueTexHolder randomTex;
  UniqueTex benchmarkDepthTex;

  int gpu_benchmark_hmapVarId;

  constexpr static int GPU_TIMESTAMP_LATENCY = 5;
  eastl::array<GPUWatchMs, GPU_TIMESTAMP_LATENCY> timings;
  size_t timingIdx = 0;
};
