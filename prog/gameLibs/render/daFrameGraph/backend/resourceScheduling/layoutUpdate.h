// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <dag/dag_vector.h>

#include <backend/resourceScheduling/layoutOptimizer.h>


namespace dafg
{

using WantedRectsFmem = dag::Vector<WantedRect, framemem_allocator>;

using ResourceSetFmem = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

struct FrameResourceSet
{
  eastl::array<ResourceSetFmem, SCHEDULE_FRAME_WINDOW> perFrame;

  explicit FrameResourceSet(size_t count)
  {
    for (auto &flags : perFrame)
      flags.resize(count, false);
  }

  bool contains(FrameResource what) const { return perFrame[what.frame].test(what.resIdx, false); }
  void insert(FrameResource what) { perFrame[what.frame].set(what.resIdx, true); }
};

// A preserved rect holds history the next frame reads. Without `spanningBefore`
// the current spanning flags count. `holdsPreserved` is a cheap superset.
struct Preservation
{
  uint32_t prevFrame = 0;
  bool historyInvalid = false;
  eastl::span<const intermediate::ResourceIndex> historyInvalidFor;
  const ResourceSetFmem *spanningBefore = nullptr;

  bool covers(const Layout &layout, FrameResource what) const;
  bool holdsPreserved(const LayoutHeap &heap) const { return !historyInvalid && heap.spanning[prevFrame] > 0; }
};

using InverseOfPairingFmem = IdIndexedMapping<intermediate::ResourceIndex, intermediate::ResourceIndex, framemem_allocator>;

// Maps an old index to its new one. An old index without a new one maps to
// RESOURCE_NOT_MAPPED.
InverseOfPairingFmem invert_pairing(const intermediate::HistoryPairing &pairing);

// A full pass over the layout. The caller skips it when the pairing keeps
// every index.
void reindex(Layout &layout, const InverseOfPairingFmem &new_index_of, size_t new_resource_count);

void remove_rect(Layout &layout, FrameResource what, PlacementsChanged &changed);

void strip_rect(Layout &layout, const WantedRect &wanted, WantedRectsFmem &to_be_placed);

bool fits_in_place(const Layout &layout, FrameResource what, const ResourceLifetime &lifetime, const Timeline &timeline);

// Only for preserved history, which must not move. The rects it now overlaps
// stay in the layout and go to `evicted`. The caller takes them out.
void set_lifetime(Layout &layout, FrameResource what, const ResourceLifetime &lifetime, const Timeline &timeline,
  FrameResourceSet &evicted);

template <class Drop>
void strip_class(Layout &layout, HeapClass heap_class, WantedRectsFmem &to_be_placed, const Drop &drop)
{
  for (const auto heapIdx : layout.heapsOf(heap_class))
  {
    auto &heap = layout.heaps[heapIdx];
    for (size_t i = heap.rects.size(); i > 0;)
    {
      const auto &rect = heap.rects[--i];
      if (!drop(heapIdx, rect))
        continue;
      const AllocationLocation wasAt{
        .heap = heapIdx,
        .offset = rect.offset,
      };
      strip_rect(layout, convert_to_wanted(rect, heap_class, wasAt), to_be_placed);
    }
  }
}

// `relocate` unpins the heaps that hold preserved rects. `size_to_content`
// proposes size 0 only for a heap without rects, or for any heap of a class
// whose resize keeps the contents.
ProposedHeapsFmem propose_heaps_of(const Layout &layout, HeapClass heap_class, const Preservation &preservation, bool size_to_content,
  const HeapLimits &limits, bool relocate = false);

// Flags only the frame resources whose place differs from their wasAt.
// `wanted` is the span the packing was packed from. A stripped rect that the
// packing left unplaced gets no flag, the caller places it again or drops it.
// `packing` is changed: every new heap in it gets its layout heap index, and
// the placed rects are sorted. Apply a proposal before the next class is packed,
// since a new heap may take an empty slot that another proposal names.
void apply_packing(Layout &layout, HeapClass heap_class, ProposedPacking &packing, eastl::span<const WantedRect> wanted,
  PlacementsChanged &changed);

} // namespace dafg
