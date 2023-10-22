#pragma once
#include "common.h"
#include "buffers.h"
#include "shaders.h"
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_eventQueryHolder.h>
#include <util/dag_generationReferencedData.h>

namespace dafx
{
struct Context;

struct CullingState
{
  bool isProxy = false;
  uint32_t renderTagsMask = 0;
  eastl::vector<uint32_t> renderTags;
  eastl::array<SortingType, Config::max_render_tags> sortings;
  eastl::array<eastl::vector<int>, Config::max_render_tags> workers;
  eastl::array<int, Config::max_render_tags> remapTags;
  eastl::array<int, Config::max_render_tags> vrsRemapTags;
  eastl::array<int, Config::max_render_tags> shadingRates;
};
using CullingStates = GenerationReferencedData<CullingId, CullingState>;

struct CullingGpuFeedback
{
  int allocCount = 0;
  bool queryIssued = false;
  bool readbackIssued = false;
  GpuResourcePtr gpuRes;
  EventQueryHolder eventQuery;
  eastl::vector<int> frameWorkers; // workers for that specific frame (might be 1..5 frames old)
};

struct Culling
{
  int cpuCullIdx = 0;
  int gpuCullIdx = 0;
  int cpuResSize = 0;
  int gpuFeedbackIdx = 0;
  int gpuFetchedIdx = -1;
  CpuResourcePtr cpuRes;
  GpuComputeShaderPtr discardShader;
  eastl::vector<int> cpuWorkers;
  eastl::vector<int> gpuWorkers;
  eastl::array<CullingGpuFeedback, Config::max_culling_feedbacks> gpuFeedbacks;
};

bool init_culling(Context &ctx);
void reset_culling(Context &ctx, bool clear_cpu);
void prepare_cpu_culling(Context &ctx, bool exec_clear);
bool prepare_gpu_culling(Context &ctx, bool exec_clear);
void issue_culling_feedback(Context &ctx);
void fetch_culling(Context &ctx);
void process_cpu_culling(Context &ctx, int start, int count);
void process_gpu_culling(Context &ctx);
} // namespace dafx