// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_bitset.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <dag/dag_vector.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <osApiWrappers/dag_spinlock.h>
#include <generic/dag_objectPool.h>

#include "driver.h"
#include "container_mutex_wrapper.h"

#include "resource_manager/raytrace_acceleration_structure.h"
#include "resource_manager/buffer_components.h"


namespace drv3d_dx12
{

namespace resource_manager
{

#if D3D_HAS_RAY_TRACING

static constexpr uint32_t RAYTRACE_HEAP_ALIGNMENT = 65536;
static constexpr uint32_t RAYTRACE_AS_ALIGNMENT = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
static_assert(0 == (RAYTRACE_AS_ALIGNMENT & (RAYTRACE_AS_ALIGNMENT - 1)), "AS_ALIGNMENT must be power of 2");
static constexpr uint32_t RAYTRACE_HEAP_SIZE = 1024 * 1024 / 2;
static_assert(RAYTRACE_HEAP_SIZE % RAYTRACE_HEAP_ALIGNMENT == 0);

struct RaytraceAccelerationStructureHeap : resource_manager::BasicBuffer
{
  static constexpr uint32_t SLOTS = RAYTRACE_HEAP_SIZE / RAYTRACE_AS_ALIGNMENT;
  Bitset<SLOTS> freeSlots;
  uint16_t takenSlotCount = 0;

  RaytraceAccelerationStructureHeap() { freeSlots.set(); }
};

class RaytraceAccelerationStructureObjectProvider : public BufferHeap
{
  using BaseType = BufferHeap;

protected:
  OSSpinlock rtasSpinlock;

private:
  ska::flat_hash_map<size_t, dag::Vector<RaytraceAccelerationStructureHeap>> heapBuckets DAG_TS_GUARDED_BY(rtasSpinlock);

protected:
  ObjectPool<RaytraceAccelerationStructure> accelStructPool DAG_TS_GUARDED_BY(rtasSpinlock);
  uint64_t memoryUsed DAG_TS_GUARDED_BY(rtasSpinlock) = 0;

private:
  RaytraceAccelerationStructureHeap allocAccelStructHeap(DXGIAdapter *adapter, Device &device, uint32_t size)
    DAG_TS_REQUIRES(rtasSpinlock);
  void freeAccelStructHeap(RaytraceAccelerationStructureHeap &&heap) DAG_TS_REQUIRES(rtasSpinlock);

  RaytraceAccelerationStructure *allocAccelStruct(DXGIAdapter *adapter, Device &device, uint32_t size);
  void freeAccelStruct(RaytraceAccelerationStructure *accelStruct);

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<RaytraceAccelerationStructure *> deletedBottomAccelerationStructure;
    dag::Vector<RaytraceAccelerationStructure *> deletedTopAccelerationStructure;
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    {
      for (auto as : data.deletedTopAccelerationStructure)
      {
        freeBufferSRVDescriptor(as->descriptor);
        recordRaytraceTopStructureFreed(as->requestedSize);
        freeAccelStruct(as);
      }
      for (auto as : data.deletedBottomAccelerationStructure)
      {
        G_ASSERT(as->descriptor == D3D12_CPU_DESCRIPTOR_HANDLE{});
        recordRaytraceBottomStructureFreed(as->requestedSize);
        freeAccelStruct(as);
      }
    }
    data.deletedTopAccelerationStructure.clear();
    data.deletedBottomAccelerationStructure.clear();

    BaseType::completeFrameExecution(info, data);
  }

  // Assume no concurrent access on shutdown
  void shutdown() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
  {
    BaseType::shutdown();
    heapBuckets.clear();
    memoryUsed = 0;
  }

  // Assume no concurrent access on recovery
  void preRecovery() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
  {
    BaseType::preRecovery();
    heapBuckets.clear();
    memoryUsed = 0;
  }

public:
  uint64_t getRaytraceAccelerationStructuresGpuMemoryUsage()
  {
    OSSpinlockScopedLock lock{rtasSpinlock};
    return memoryUsed;
  }

  RaytraceAccelerationStructure *newRaytraceTopAccelerationStructure(DXGIAdapter *adapter, Device &device, uint64_t size);

  RaytraceAccelerationStructure *newRaytraceBottomAccelerationStructure(DXGIAdapter *adapter, Device &device, uint64_t size);

  void deleteRaytraceTopAccelerationStructureOnFrameCompletion(RaytraceAccelerationStructure *ras)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [=](auto &data) { data.deletedTopAccelerationStructure.push_back(ras); });
  }
  void deleteRaytraceBottomAccelerationStructureOnFrameCompletion(RaytraceAccelerationStructure *ras)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [=](auto &data) { data.deletedBottomAccelerationStructure.push_back(ras); });
  }
};
#else
using RaytraceAccelerationStructureObjectProvider = BufferHeap;
#endif

} // namespace resource_manager
} // namespace drv3d_dx12
