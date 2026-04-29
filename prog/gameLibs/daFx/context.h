// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
#include <3d/dag_serialIntBuffer.h>
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/fixed_vector.h>
#include <util/dag_threadPool.h>
#include <util/dag_generationReferencedData.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_events.h>

namespace dafx
{
struct SamplerHandles
{
  float mipBias = invalidMipBias;

  d3d::SamplerHandle samplerBilinear = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle samplerAniso = d3d::INVALID_SAMPLER_HANDLE;

  SamplerHandles() = default;
  SamplerHandles(float in_mipBias);
  ~SamplerHandles();
  SamplerHandles(const SamplerHandles &other) = delete;
  SamplerHandles(SamplerHandles &&other) noexcept;
  SamplerHandles &operator=(const SamplerHandles &other) = delete;
  SamplerHandles &operator=(SamplerHandles &&other) noexcept;

private:
  inline static constexpr float invalidMipBias = 1000.f;
  void close();
};

struct AsyncPrepareJob final : public cpujobs::IJob
{
  ContextId cid;
  float dt = 0.f;
  bool gpu_fetch = false;
  bool clear_debug_lods = false;
  const char *getJobName(bool &) const override { return "dafx_prepare"; }
  void doJob() override;
};

struct AsyncCpuComputeJob final : public cpujobs::IJob
{
  ContextId cid;
  bool emission = false;
  int depth = 0;
  int threadId = 0;
  const char *getJobName(bool &) const override { return "dafx_cpu_compute_tasks"; }
  void doJob() override;
};

struct AsyncStartNextComputeBatchJob final : public cpujobs::IJob
{
  ContextId cid;
  int depth;
  bool emission;
  cpujobs::IJob *prepare(ContextId cid_, int depth_, bool emi);
  const char *getJobName(bool &) const override { return "AsyncStartNextComputeBatchJob"; }
  void doJob() override;
};

struct AsyncCpuCullJob final : public cpujobs::IJob
{
  ContextId cid;
  int start = 0;
  int count = 0;
  const char *getJobName(bool &) const override { return "AsyncCpuCullJob"; }
  void doJob() override;
};

struct AsyncCpuDefragJob final : public cpujobs::IJob
{
  ContextId cid;
  const char *getJobName(bool &) const override { return "AsyncCpuDefragJob"; }
  void doJob() override;
};

struct AsyncStats
{
  volatile int cpuElemProcessed;
  volatile int gpuElemProcessed;
  volatile int cpuDispatchCalls;
  volatile int gpuDispatchCalls;
  volatile int updateEmitters;

  eastl::array<volatile int, Config::max_simulation_lods> cpuElemProcessedByLods;
  eastl::array<volatile int, Config::max_simulation_lods> gpuElemProcessedByLods;
};

using WorkersByDepth = eastl::array<eastl::vector<int>, Config::max_system_depth>;

struct Workers
{
  WorkersByDepth cpuEmissionWorkers{};
  WorkersByDepth gpuEmissionWorkers{};
  WorkersByDepth cpuSimulationWorkers{};
  WorkersByDepth gpuSimulationWorkers{};
  eastl::vector<int> allRenderWorkers{};
  eastl::array<eastl::vector<int>, Config::max_render_tags> cpuRenderWorkers{};
  eastl::vector<int> gpuTransferWorkers{};
  eastl::vector<int> allEmissionWorkers{};
  eastl::vector<int> cullingCpuWorkers{};
  eastl::vector<int> cullingGpuWorkers{};

  void clear()
  {
    for (int i = 0; i < Config::max_system_depth; ++i)
    {
      cpuEmissionWorkers[i].clear();
      gpuEmissionWorkers[i].clear();
      cpuSimulationWorkers[i].clear();
      gpuSimulationWorkers[i].clear();
    }
    allRenderWorkers.clear();
    for (int i = 0; i < Config::max_render_tags; ++i)
      cpuRenderWorkers[i].clear();
    gpuTransferWorkers.clear();
    allEmissionWorkers.clear();
    cullingCpuWorkers.clear();
    cullingGpuWorkers.clear();
  }
};

struct WorkerStats
{
  eastl::array<int, Config::max_simulation_lods> cpuElemTotalSimLods{};
  eastl::array<int, Config::max_simulation_lods> gpuElemTotalSimLods{};
  int cpuEmissionWorkers = 0;
  int cpuSimulationWorkers = 0;
  int allRenderWorkers = 0;
  int activeInstances = 0;
  int renderInstances = 0;
  int genVisibilityLod = 0;

  // @TODO: are these supposed to be reported as well? Were not in orig code.
  // int gpuEmissionWorkers;
  // int gpuSimulationWorkers;

  void clear()
  {
#if DAFX_STAT
    memset(this, 0, sizeof(*this));
#endif
  }
};

struct Context
{
  Config cfg;
  Config pendingCfg;
  Stats stats;
  AsyncStats asyncStats;
  dag::Vector<SystemUsageStat> systemUsageStats;
  uint32_t debugFlags = 0;

  Binds binds;
  Shaders shaders;
  GlobalData globalData;
  uint32_t lastUploadedGlobalDataFrame = 0;
  GenerationReferencedData<RefDataId, RefData> refDatas;
  eastl::fixed_vector<SamplerHandles, 4, true> samplersCache;

  Culling culling;
  StagingRing staging;
  eastl::array<RenderBuffer, Config::max_render_tags> renderBuffers;
  uint32_t beforeRenderUpdatedTags = 0;

  VDECL instancingVdecl;
  serial_buffer::SerialBufferCounter serialBuf;
  GpuBufferPool gpuBufferPool;
  CpuBufferPool cpuBufferPool;

  static constexpr int multiDrawBufferRingSize = 3;
  eastl::vector<UniqueBuf> multidrawBufers[multiDrawBufferRingSize];
  eastl::vector<RenderDispatchBuffer> renderDispatchBuffers;
  eastl::vector<GpuResourcePtr> computeDispatchBuffers;
  int currentRenderDispatchBuffer = 0;
  int currentMutltidrawBuffer = 0;
  int currentMutltidrawBufferRingId = 0;
  int rndSeed = 0;

  Systems systems;
  Instances instances;
  CullingStates cullingStates;

  eastl::vector<Workers> workersPrepByThreads;
  eastl::vector<WorkerStats> workersPrepStatsByThreads;
  Workers warmupWorkers{};
  WorkerStats warmupWorkerStats{};

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
  dag::RelocatableFixedVector<AsyncCpuComputeJob, 10> asyncCpuComputeJobs;
  dag::RelocatableFixedVector<AsyncCpuCullJob, 10> asyncCpuCullJobs;

  AsyncCpuDefragJob asyncCpuDefragJob;
  int nextPageToDefrag = 0;

  dag::Vector<bool> activeQueryContainer;
  SimLodGenParams simLodGenParams;

  volatile int asyncCounter = -1;
  os_event_t allAsyncCpuJobsDoneEvent;
  void asyncCpuJobsCompleted()
  {
    interlocked_release_store(asyncCounter, -1);
    os_event_set(&allAsyncCpuJobsDoneEvent);
  }

  unsigned int currentFrame = 0;
  int renderBufferMaxUsage = 0;
  bool updateInProgress = false;
  bool simulationIsPaused = false;
  bool app_was_inactive = false;
  bool use32BitIndex = false;

  int systemDataVarId = -1;
  int renderCallsVarId = -1;
  int renderCallsCountVarId = -1;
  int computeDispatchVarId = -1;
  int reducedRenderVarId = -1;
  bool supportsNoOverwrite = true;
  volatile int maxTextureSlotsAllocated = 0;

  FrameBoundaryBufferManager frameBoundaryBufferManager;

  os_spinlock_t queueLock;
  os_spinlock_t configLock;

  Context() { os_event_create(&allAsyncCpuJobsDoneEvent); } //-V730
  ~Context() { os_event_destroy(&allAsyncCpuJobsDoneEvent); }
  Context(Context &&) = default;
  Context(const Context &) = delete;
  Context &operator=(const Context &) = delete;
  Context &operator=(Context &&) = delete;

#if DAGOR_DBGLEVEL > 0
  volatile int systemsLockCounter = 0;
  volatile int instanceListLockCounter = 0;
  volatile int instanceTupleLockCounter = 0;
#endif
};

#if DAGOR_DBGLEVEL > 0
struct ScopedAccessGuard
{
  volatile int &cnt;

  ScopedAccessGuard(volatile int &counter) : cnt(counter) { interlocked_increment(cnt); }

  ~ScopedAccessGuard() { interlocked_decrement(cnt); }

  ScopedAccessGuard(ScopedAccessGuard &ctx) = delete;
  ScopedAccessGuard(const ScopedAccessGuard &ctx) = delete;
  ScopedAccessGuard &operator=(ScopedAccessGuard &) = delete;
  ScopedAccessGuard &operator=(const ScopedAccessGuard &) = delete;
};

#define SYS_LOCK_GUARD        ScopedAccessGuard DAG_CONCAT(syslockguard, __LINE__)(ctx.systemsLockCounter)
#define INST_LIST_LOCK_GUARD  ScopedAccessGuard DAG_CONCAT(instlockguard, __LINE__)(ctx.instanceListLockCounter)
#define INST_TUPLE_LOCK_GUARD ScopedAccessGuard DAG_CONCAT(instlockguard, __LINE__)(ctx.instanceTupleLockCounter)
#else
#define SYS_LOCK_GUARD
#define INST_LIST_LOCK_GUARD
#define INST_TUPLE_LOCK_GUARD
#endif

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
