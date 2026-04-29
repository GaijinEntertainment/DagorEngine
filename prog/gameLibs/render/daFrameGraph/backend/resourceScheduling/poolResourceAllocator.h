// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "resourceAllocator.h"
#include <resourcePool/resourcePool.h>


namespace dafg
{

using PoolResourceType = ::ResourceType;

// Simulates heaps via resource pools
class PoolResourceAllocator final : public ResourceAllocator
{
public:
  PoolResourceAllocator(IGraphDumper &dumper) : ResourceAllocator{dumper} {}

  // NOTE: no dynamic resolution without native heaps. Could be
  // implemented later on if needed.
  void resizeAutoResTextures(int, const DynamicResolutionUpdates &) override
  {
    G_ASSERTF(false, "Dynamic resolution is not supported when "
                     "simulating heaps using resource pools!");
  }

  ~PoolResourceAllocator() override;

private:
  D3dResource *getD3dResource(int frame, intermediate::ResourceIndex res_idx) const override;

  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) override;
  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *) override;

  DestroyedHeapSet allocateHeaps(const HeapRequests &heap_requests) override;

  void resetResourcePlacement() override;
  void placeResource(int frame, intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc,
    uint64_t offset, const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) override;

  void shutdownInternal() override;

  template <ResourceType RES_TYPE>
  void prepareHeapType(eastl::span<uint32_t const> heapSizes);

private:
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, uint32_t>, SCHEDULE_FRAME_WINDOW> resourceIndexInCollection;

  IdIndexedMapping<HeapIndex, uint32_t> heapStartOffsets;

  dag::Vector<PoolResourceType> resourceTypes;
};

} // namespace dafg
