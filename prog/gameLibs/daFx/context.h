#pragma once

#include "common.h"
#include "binds.h"
#include "systems.h"
#include "buffers.h"
#include "shaders.h"
#include "culling.h"
#include "instances.h"
#include "globalData.h"
#include "commandQueue.h"
#include "frameBoundaryBufferManager.h"
#include <generic/dag_relocatableFixedVector.h>
#include <util/dag_threadPool.h>
#include <util/dag_generationReferencedData.h>
#include <osApiWrappers/dag_spinlock.h>

namespace dafx
{

struct AsyncPrepareJob final : public cpujobs::IJob
{
  ContextId cid;
  float dt = 0.f;
  bool gpu_fetch = false;
  void doJob() override;
};

struct AsyncCpuComputeJob final : public cpujobs::IJob
{
  ContextId cid;
  bool emission = false;
  int depth = 0;
  int threadId = 0;
  void doJob() override;
};

struct AsyncStartNextComputeBatchJob final : public cpujobs::IJob
{
  ContextId cid;
  int depth;
  bool emission;
  cpujobs::IJob *prepare(ContextId cid_, int depth_, bool emi);
  void doJob() override;
};

struct AsyncCpuCullJob final : public cpujobs::IJob
{
  ContextId cid;
  int start = 0;
  int count = 0;
  void doJob() override;
};

struct AsyncStats
{
  volatile int cpuElemProcessed;
  volatile int gpuElemProcessed;
  volatile int cpuDispatchCalls;
  volatile int gpuDispatchCalls;
};

using WorkersByDepth = eastl::array<eastl::vector<int>, Config::max_system_depth>;
struct Context
{
  Config cfg;
  Stats stats;
  AsyncStats asyncStats;
  uint32_t debugFlags;

  Binds binds;
  Shaders shaders;
  GlobalData globalData;
  GenerationReferencedData<RefDataId, RefData> refDatas;

  Culling culling;
  StagingRing staging;
  eastl::array<RenderBuffer, Config::max_render_tags> renderBuffers;
  uint32_t beforeRenderUpdatedTags;

  GpuBufferPool gpuBufferPool;
  CpuBufferPool cpuBufferPool;

  eastl::vector<GpuResourcePtr> renderDispatchBuffers;
  eastl::vector<GpuResourcePtr> computeDispatchBuffers;
  int currentRenderDispatchBuffer = 0;
  int rndSeed = 0;

  Systems systems;
  Instances instances;
  CullingStates cullingStates;

  WorkersByDepth cpuEmissionWorkers;
  WorkersByDepth gpuEmissionWorkers;

  WorkersByDepth cpuSimulationWorkers;
  WorkersByDepth gpuSimulationWorkers;

  eastl::vector<int> allRenderWorkers;
  eastl::array<eastl::vector<int>, Config::max_render_tags> cpuRenderWorkers;

  eastl::vector<int> gpuTransferWorkers;
  eastl::vector<int> allEmissionWorkers;

  eastl::vector<eastl::vector<int>> workersByThreads;

  CommandQueue commandQueue;
  CommandQueue commandQueueNext;

  AsyncPrepareJob asyncPrepareJob;
  AsyncStartNextComputeBatchJob startNextComputeBatchJob;
  dag::RelocatableFixedVector<AsyncCpuComputeJob, 8> asyncCpuComputeJobs;
  dag::RelocatableFixedVector<AsyncCpuCullJob, 8> asyncCpuCullJobs;

  volatile int asyncCounter = -1;

  BaseTexture *customDepth = nullptr;
  unsigned int currentFrame = 0;
  int renderBufferMaxUsage = 0;
  bool updateInProgress = false;

  int systemDataVarId = -1;
  int renderCallsVarId = -1;
  int computeDispatchVarId = -1;
  int updateGpuCullingVarId = -1;
  bool supportsNoOverwrite = true;

  FrameBoundaryBufferManager frameBoundaryBufferManager;

  os_spinlock_t queueLock;

  Context() = default;
  ~Context() = default;
  Context(Context &&) = default;
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  Context &operator=(Context &&) = delete;
};

extern GenerationReferencedData<ContextId, Context, uint8_t, 0> g_ctx_list;
#define GET_CTX()                                                  \
  Context *ctxp = g_ctx_list.get(cid);                             \
  G_ASSERTF(ctxp, "dafx: GET_CTX failed, rid: %d", (uint32_t)cid); \
  if (!ctxp)                                                       \
    return;                                                        \
  Context &ctx = *ctxp;
#define GET_CTX_RET(v)                                                           \
  Context *ctxp = g_ctx_list.get(cid);                                           \
  G_ASSERTF_RETURN(ctxp, v, "dafx: GET_CTX_RET failed, rid: %d", (uint32_t)cid); \
  Context &ctx = *ctxp;
} // namespace dafx

DAG_DECLARE_RELOCATABLE(dafx::AsyncPrepareJob);
DAG_DECLARE_RELOCATABLE(dafx::AsyncCpuComputeJob);
DAG_DECLARE_RELOCATABLE(dafx::AsyncStartNextComputeBatchJob);
DAG_DECLARE_RELOCATABLE(dafx::AsyncCpuCullJob);
