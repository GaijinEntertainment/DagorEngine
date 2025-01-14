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

  eastl::array<volatile int, Config::max_simulation_lods> cpuElemProcessedByLods;
  eastl::array<volatile int, Config::max_simulation_lods> gpuElemProcessedByLods;
};

using WorkersByDepth = eastl::array<eastl::vector<int>, Config::max_system_depth>;
struct Context
{
  Config cfg;
  Stats stats;
  AsyncStats asyncStats;
  dag::Vector<SystemUsageStat> systemUsageStats;
  uint32_t debugFlags;

  Binds binds;
  Shaders shaders;
  GlobalData globalData;
  GenerationReferencedData<RefDataId, RefData> refDatas;
  eastl::fixed_vector<SamplerHandles, 4, true> samplersCache;

  Culling culling;
  StagingRing staging;
  eastl::array<RenderBuffer, Config::max_render_tags> renderBuffers;
  uint32_t beforeRenderUpdatedTags;

  VDECL instancingVdecl;
  serial_buffer::SerialBufferCounter serialBuf;
  GpuBufferPool gpuBufferPool;
  CpuBufferPool cpuBufferPool;

  eastl::vector<UniqueBuf> multidrawBufers;
  eastl::vector<GpuResourcePtr> renderDispatchBuffers;
  eastl::vector<GpuResourcePtr> computeDispatchBuffers;
  int currentRenderDispatchBuffer = 0;
  int currentMutltidrawBuffer = 0;
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
  dag::RelocatableFixedVector<AsyncCpuComputeJob, 10> asyncCpuComputeJobs;
  dag::RelocatableFixedVector<AsyncCpuCullJob, 10> asyncCpuCullJobs;

  dag::Vector<bool> activeQueryContainer;

  volatile int asyncCounter = -1;

  unsigned int currentFrame = 0;
  int renderBufferMaxUsage = 0;
  bool updateInProgress = false;
  bool simulationIsPaused = false;
  bool app_was_inactive = false;

  int systemDataVarId = -1;
  int renderCallsVarId = -1;
  int computeDispatchVarId = -1;
  int updateGpuCullingVarId = -1;
  bool supportsNoOverwrite = true;
  volatile int maxTextureSlotsAllocated = 0;

  FrameBoundaryBufferManager frameBoundaryBufferManager;

  os_spinlock_t queueLock;

  Context() = default;
  ~Context() = default;
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
