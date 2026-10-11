// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/variant.h>

#include <3d/dag_resPtr.h>
#include <dag/dag_vectorSet.h>
#include <memory/dag_framemem.h>

#include <id/idIndexedFlags.h>
#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/barrierScheduler.h>


class framemem_allocator;

namespace dafg
{

inline constexpr uint32_t UNSCHEDULED = eastl::numeric_limits<uint32_t>::max();

// Fake heap group used for CPU resources
inline ResourceHeapGroup *const CPU_HEAP_GROUP = reinterpret_cast<ResourceHeapGroup *>(~uintptr_t{0});

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

inline constexpr AllocationLocation NOWHERE{
  .heap = HeapIndex::Invalid,
  .offset = UNSCHEDULED,
};

struct FrameResource
{
  intermediate::ResourceIndex resIdx{};
  uint32_t frame = 0;

  bool operator==(const FrameResource &other) const = default;
};

struct HeapRequest
{
  ResourceHeapGroup *group = nullptr;
  uint64_t size = 0;
  bool untracked = false;
};
using HeapRequests = IdIndexedMapping<HeapIndex, HeapRequest>;

struct BufferDeactivation
{
  Sbuffer *buffer;
  eastl::optional<d3d::BufferBarrier> release;
};

struct TextureDeactivation
{
  BaseTexture *texture;
  eastl::optional<d3d::TextureBarrier> release;
};

struct BlobDeactivationRequest
{
  intermediate::DtorFunc destructor;
  void *blob;
};

using PotentialResourceDeactivation =
  eastl::variant<eastl::monostate, TextureDeactivation, BufferDeactivation, BlobDeactivationRequest>;
using PotentialDeactivationSet = IdIndexedMapping<intermediate::ResourceIndex, PotentialResourceDeactivation>;

using IntermediateResources = IdSparseIndexedMapping<intermediate::ResourceIndex, intermediate::Resource>;

} // namespace dafg
