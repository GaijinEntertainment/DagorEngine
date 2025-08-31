// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include "buffer_components.h"

#if D3D_HAS_RAY_TRACING

#include "buffer_components.h"
#include "raytrace_acceleration_structure.h"
#include <container_mutex_wrapper.h>
#include <driver.h>

#include <dag/dag_vector.h>
#include <generic/dag_bitset.h>
#include <generic/dag_objectPool.h>
#include <osApiWrappers/dag_spinlock.h>
#include <ska_hash_map/flat_hash_map2.hpp>


namespace drv3d_dx12::resource_manager
{
class RaytraceAccelerationStructurePoolProvider : public BufferHeap
{
  using BaseType = BufferHeap;

  using RayTraceAccelerationStructurePoolVector = dag::Vector<eastl::unique_ptr<RayTraceAccelerationStructurePool>>;
  ContainerMutexWrapper<RayTraceAccelerationStructurePoolVector, OSSpinlock> pools;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<RayTraceAccelerationStructurePool *> deletedRaytraceAccelerationStructurePools;
    dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> tlasDescriptors;
  };

  void shutdown()
  {
    BaseType::shutdown();
    pools.access()->clear();
  }

  void preRecovery()
  {
    BaseType::preRecovery();
    pools.access()->clear();
  }

  template <typename T>
  void iterateRayTraceAccelerationStructurePools(T iterator)
  {
    auto poolsAccess = pools.access();
    for (auto &pool : *poolsAccess)
    {
      iterator(const_cast<RayTraceAccelerationStructurePool &>(*pool));
    }
  }

  void removePool(RayTraceAccelerationStructurePool *pool)
  {
    recordRaytraceAccelerationStructurePoolFreed(pool->sizeInBytes);

    pool->subStructures.iterateAllocated([this](auto *as) {
      if (as->descriptor != D3D12_CPU_DESCRIPTOR_HANDLE{})
      {
        freeBufferSRVDescriptor(as->descriptor);
        as->descriptor = {};
      }
    });
    {
      auto poolAccess = pools.access();
      auto ref = eastl::find_if(poolAccess->begin(), poolAccess->end(), [pool](auto &p) { return pool == p.get(); });
      G_ASSERT(ref != poolAccess->end());
      if (ref != poolAccess->end())
      {
        // order is not important so we can do this a bit faster
        if (poolAccess->size() > 1)
        {
          eastl::swap(*ref, poolAccess->back());
          poolAccess->pop_back();
        }
        else
        {
          poolAccess->erase(ref);
        }
      }
      else
      {
        delete pool;
      }
    }
  }

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    freeBufferSRVDescriptors(data.tlasDescriptors);
    data.tlasDescriptors.clear();

    // first free all descriptors without deleting the pools, as we can do this without any locking of the pool set.
    // this also avoids possible issues with locking orders as freeBufferSRVDescriptor may take a data structure lock.
    for (auto pool : data.deletedRaytraceAccelerationStructurePools)
    {
      pool->subStructures.iterateAllocated([this](auto *as) {
        if (as->descriptor != D3D12_CPU_DESCRIPTOR_HANDLE{})
        {
          freeBufferSRVDescriptor(as->descriptor);
          as->descriptor = {};
        }
      });
      recordRaytraceAccelerationStructurePoolFreed(pool->sizeInBytes);
    }
    // now do the cleanup of the pool set under the access of the pool
    {
      auto poolAccess = pools.access();
      for (auto pool : data.deletedRaytraceAccelerationStructurePools)
      {
        auto ref = eastl::find_if(poolAccess->begin(), poolAccess->end(), [pool](auto &p) { return pool == p.get(); });
        G_ASSERT(ref != poolAccess->end());
        if (ref != poolAccess->end())
        {
          // order is not important so we can do this a bit faster
          if (poolAccess->size() > 1)
          {
            eastl::swap(*ref, poolAccess->back());
            poolAccess->pop_back();
          }
          else
          {
            poolAccess->erase(ref);
          }
        }
        else
        {
          delete pool;
        }
      }
    }
    data.deletedRaytraceAccelerationStructurePools.clear();

    BaseType::completeFrameExecution(info, data);
  }

public:
  // assumes input was validated
  ::raytrace::AccelerationStructurePool createAccelerationStructurePool(Device &device,
    const ::raytrace::AccelerationStructurePoolCreateInfo &info);
  RaytraceAccelerationStructure *createAccelerationStructure(Device &device, ::raytrace::AccelerationStructurePool pool,
    const ::raytrace::TopAccelerationStructurePlacementInfo &info);
  RaytraceAccelerationStructure *createAccelerationStructure(::raytrace::AccelerationStructurePool pool,
    const ::raytrace::BottomAccelerationStructurePlacementInfo &info);
  void destroyAccelerationStructurePoolOnFrameCompletion(::raytrace::AccelerationStructurePool pool)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) {
      data.deletedRaytraceAccelerationStructurePools.push_back(reinterpret_cast<RayTraceAccelerationStructurePool *>(pool));
    });
  }
  void destroyAccelerationStructureOnFrameCompletion(::raytrace::AccelerationStructurePool pool,
    ::raytrace::AnyAccelerationStructure structure)
  {
    auto asPool = reinterpret_cast<RayTraceAccelerationStructurePool *>(pool);
    RaytraceAccelerationStructure *rts = nullptr;
    if (structure.top)
    {
      rts = reinterpret_cast<RaytraceAccelerationStructure *>(structure.top);
      // only need to delay free of the descriptor as we may otherwise have a race of the content of the descriptor
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
        [=](auto &data) { data.tlasDescriptors.push_back(rts->descriptor); });
    }
    else if (structure.bottom)
    {
      rts = reinterpret_cast<RaytraceAccelerationStructure *>(structure.bottom);
    }

    if (rts)
    {
      asPool->subStructures.free(rts);
    }
  }
};

static constexpr uint32_t RAYTRACE_HEAP_ALIGNMENT = 65536;
static constexpr uint32_t RAYTRACE_AS_ALIGNMENT = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
static_assert(0 == (RAYTRACE_AS_ALIGNMENT & (RAYTRACE_AS_ALIGNMENT - 1)), "AS_ALIGNMENT must be power of 2");
static constexpr uint32_t RAYTRACE_HEAP_SIZE = 1024 * 1024 / 2;
static_assert(RAYTRACE_HEAP_SIZE % RAYTRACE_HEAP_ALIGNMENT == 0);

struct RaytraceAccelerationStructureHeap
{
  RayTraceAccelerationStructurePool *pool = nullptr;
  static constexpr uint32_t SLOTS = RAYTRACE_HEAP_SIZE / RAYTRACE_AS_ALIGNMENT;
  Bitset<SLOTS> freeSlots;
  uint16_t takenSlotCount = 0;

  RaytraceAccelerationStructureHeap() { freeSlots.set(); }
};

class RaytraceAccelerationStructureObjectProvider : public RaytraceAccelerationStructurePoolProvider
{
  using BaseType = RaytraceAccelerationStructurePoolProvider;

protected:
  OSSpinlock rtasSpinlock;

private:
  ska::flat_hash_map<size_t, dag::Vector<RaytraceAccelerationStructureHeap>> heapBuckets DAG_TS_GUARDED_BY(rtasSpinlock);

protected:
  uint64_t memoryUsed DAG_TS_GUARDED_BY(rtasSpinlock) = 0;

private:
  RaytraceAccelerationStructureHeap allocAccelStructHeap(Device &device, uint32_t size) DAG_TS_REQUIRES(rtasSpinlock);
  void freeAccelStructHeap(RaytraceAccelerationStructureHeap &&heap) DAG_TS_REQUIRES(rtasSpinlock);

  RaytraceAccelerationStructure *allocAccelStruct(Device &device, uint32_t size);
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

  RaytraceAccelerationStructure *newRaytraceTopAccelerationStructure(Device &device, uint64_t size);
  RaytraceAccelerationStructure *newRaytraceBottomAccelerationStructure(Device &device, uint64_t size);

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
} // namespace drv3d_dx12::resource_manager
#else
namespace drv3d_dx12::resource_manager
{
using RaytraceAccelerationStructurePoolProvider = BufferHeap;
using RaytraceAccelerationStructureObjectProvider = RaytraceAccelerationStructurePoolProvider;
} // namespace drv3d_dx12::resource_manager
#endif