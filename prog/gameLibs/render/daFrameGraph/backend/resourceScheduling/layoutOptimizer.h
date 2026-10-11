// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>

#include <backend/resourceScheduling/heapLayout.h>


namespace dafg
{

struct HeapLimits
{
  uint64_t maxHeapSize = eastl::numeric_limits<uint64_t>::max();
  uint64_t optimalMaxHeapSize = 0;
  uint64_t growthStep = 1;
  bool fixedSize = false;
  // The caller reads it to decide ProposedHeap::pinned, the packer does not
  bool resizeKeepsContents = false;
  // Every wanted rect gets the whole frame ring as its lifetime, so no two share memory
  bool noAliasing = false;
};

uint64_t compute_heap_size_limit(const HeapLimits &limits, uint64_t largest_rect);

struct ProposedHeap
{
  // 0 sizes the heap to the rects it gets, except in a fixed size class
  // and for a pinned heap
  uint64_t reservedSize = 0;
  HeapIndex heap = HeapIndex::Invalid;
  // Keeps its size and gets no growth
  bool pinned = false;
};

// One heap class. A new heap has an invalid index.
using ProposedHeapsFmem = dag::Vector<ProposedHeap, framemem_allocator>;

struct ProposedRect
{
  LayoutRect rect{};
  // Index into the wanted span
  uint32_t item = 0;
  uint32_t proposedHeap = 0;
};

using UnplacedIndicesFmem = dag::Vector<uint32_t, framemem_allocator>;

// The heaps are allocated last. So the caller can add heaps without a copy.
struct ProposedPacking
{
  dag::Vector<ProposedRect, framemem_allocator> placed;
  UnplacedIndicesFmem unplaced;
  ProposedHeapsFmem heaps;
};

// The heaps take rects and grow in the order of the span. Rects already in the
// heaps never move. `unplaced` holds the indices into
// `wanted` of the rects that fit nowhere. `boundary` is the moment the next
// frame starts.
ProposedPacking try_pack_into_layout(const Layout &layout, eastl::span<const ProposedHeap> heaps, eastl::span<const WantedRect> wanted,
  const HeapLimits &limits, const Timeline &timeline, uint32_t boundary);

} // namespace dafg
