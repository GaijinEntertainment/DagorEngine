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
  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc,
    bool force_not_on_chip = false) override;
  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *) override;
  bool doHeapsCountObjects() const override { return true; }
  bool doesResizeKeepContents() const override { return true; }
  uint64_t getFreeMemoryBudget() const override { return UNKNOWN_MEMORY_BUDGET; }

  void allocateHeaps(const LayoutHeaps &layout_heaps, bool defer_destruction, ChangedHeaps &changed) override;
  bool doesHeapAgree(HeapIndex heap_idx, const HeapRequest &req) const override;

  D3dResource *placeResource(intermediate::ResourceIndex res_idx, HeapIndex heap_idx, const ResourceDescription &desc, uint64_t offset,
    const ResourceAllocationProperties &properties, const DynamicResolution &dyn_resolution) override;

  void shutdownInternal() override;
};

} // namespace dafg
