// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/variant.h>

#include <3d/dag_resPtr.h>
#include <dag/dag_vectorSet.h>
#include <memory/dag_framemem.h>

#include <generic/dag_fixedVectorMap.h>
#include <id/idIndexedFlags.h>
#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/barrierScheduler.h>


class framemem_allocator;

namespace dafg
{

static constexpr int SCHEDULE_FRAME_WINDOW = BarrierScheduler::SCHEDULE_FRAME_WINDOW;

static constexpr uint32_t UNSCHEDULED = eastl::numeric_limits<uint32_t>::max();

// Fake heap group used for CPU resources
inline static ResourceHeapGroup *const CPU_HEAP_GROUP = reinterpret_cast<ResourceHeapGroup *>(~uintptr_t{0});

using ResourceProperties = IdIndexedMapping<intermediate::ResourceIndex, ResourceAllocationProperties>;

enum class HeapIndex : uint32_t
{
  Invalid = ~0u
};

struct AllocationLocation
{
  HeapIndex heap;
  uint64_t offset;
};
using AllocationLocations = eastl::array<IdIndexedMapping<intermediate::ResourceIndex, AllocationLocation>, SCHEDULE_FRAME_WINDOW>;

struct HeapRequest
{
  ResourceHeapGroup *group;
  uint64_t size;
};
using HeapRequests = IdIndexedMapping<HeapIndex, HeapRequest>;

struct BlobDeactivationRequest
{
  intermediate::DtorFunc destructor;
  void *blob;
};

using PotentialResourceDeactivation = eastl::variant<eastl::monostate, BaseTexture *, Sbuffer *, BlobDeactivationRequest>;
using PotentialDeactivationSet = IdIndexedMapping<intermediate::ResourceIndex, PotentialResourceDeactivation>;

using IntermediateResources = IdSparseIndexedMapping<intermediate::ResourceIndex, intermediate::Resource>;
using IrHistoryPairing = IdIndexedMapping<intermediate::ResourceIndex, intermediate::ResourceIndex, framemem_allocator>;

// This lists the heaps that were either completely destroyed
// or recreated with a larger size (and their resources hence died)
using DestroyedHeapSet = dag::Vector<HeapIndex, framemem_allocator>;

struct ResourceLocation
{
  uint64_t offset;
  uint64_t size;
};
using PerHeapPerFrameHistoryResourceLocations = dag::FixedVectorMap<intermediate::ResourceIndex, ResourceLocation, 32>;
using PerHeapHistoryResourceLocations = eastl::array<PerHeapPerFrameHistoryResourceLocations, SCHEDULE_FRAME_WINDOW>;
using HistoryResourceLocations = IdIndexedMapping<HeapIndex, PerHeapHistoryResourceLocations>;

struct ResourceSchedule
{
  AllocationLocations allocationLocations;
  HeapRequests heapRequests;
  ResourceProperties resourceProperties;
};

} // namespace dafg
