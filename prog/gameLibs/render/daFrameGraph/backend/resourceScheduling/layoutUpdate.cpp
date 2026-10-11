// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "layoutUpdate.h"

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <debug/dag_assert.h>
#include <memory/dag_framemem.h>


namespace dafg
{

bool Preservation::covers(const Layout &layout, FrameResource what) const
{
  if (historyInvalid || what.frame != prevFrame || !layout.spanning[what.frame].test(what.resIdx, false))
    return false;
  if (spanningBefore != nullptr && !spanningBefore->test(what.resIdx, false))
    return false;
  return eastl::find(historyInvalidFor.begin(), historyInvalidFor.end(), what.resIdx) == historyInvalidFor.end();
}

InverseOfPairingFmem invert_pairing(const intermediate::HistoryPairing &pairing)
{
  size_t oldCount = 0;
  for (const auto oldIdx : pairing)
    if (oldIdx != intermediate::RESOURCE_NOT_MAPPED)
      oldCount = eastl::max<size_t>(oldCount, eastl::to_underlying(oldIdx) + 1);

  InverseOfPairingFmem result(oldCount, intermediate::RESOURCE_NOT_MAPPED);
  for (auto &&[newIdx, oldIdx] : pairing.enumerate())
    if (oldIdx != intermediate::RESOURCE_NOT_MAPPED)
    {
      G_ASSERT(result[oldIdx] == intermediate::RESOURCE_NOT_MAPPED);
      result[oldIdx] = newIdx;
    }
  return result;
}

void reindex(Layout &layout, const InverseOfPairingFmem &new_index_of, size_t new_resource_count)
{
  for (auto &heap : layout.heaps)
  {
    size_t kept = 0;
    for (size_t i = 0; i < heap.rects.size(); ++i)
    {
      const auto oldIdx = heap.rects[i].what.resIdx;
      const auto newIdx = new_index_of.isMapped(oldIdx) ? new_index_of[oldIdx] : intermediate::RESOURCE_NOT_MAPPED;
      if (newIdx == intermediate::RESOURCE_NOT_MAPPED)
        continue;
      heap.rects[kept] = heap.rects[i];
      heap.rects[kept].what.resIdx = newIdx;
      ++kept;
    }
    heap.rects.resize(kept);
  }

  for (auto &perFrame : layout.where.locationOf)
    perFrame.assign(new_resource_count, NOWHERE);
  for (auto &perFrame : layout.spanning)
    perFrame.assign(new_resource_count, false);
  for (auto [heapIdx, heap] : layout.heaps.enumerate())
  {
    heap.spanning = {};
    for (const auto &rect : heap.rects)
      layout.track(heapIdx, rect);
  }
}

void remove_rect(Layout &layout, FrameResource what, PlacementsChanged &changed)
{
  layout.take(what);
  changed[what.frame].set(what.resIdx, true);
}

void strip_rect(Layout &layout, const WantedRect &wanted, WantedRectsFmem &to_be_placed)
{
  layout.take(wanted.what);
  to_be_placed.push_back(wanted);
}

template <class OnOverlap>
static void for_each_overlap(const Layout &layout, const LayoutRect &mine, const ResourceLifetime &lifetime, const Timeline &timeline,
  const OnOverlap &on_overlap)
{
  for (const auto &rect : layout.heaps[layout.where[mine.what].heap].rects)
    if (rect.what != mine.what && rect.offset < mine.end() && mine.offset < rect.end() && timeline.overlap(rect.lifetime, lifetime))
      if (!on_overlap(rect))
        return;
}

bool fits_in_place(const Layout &layout, FrameResource what, const ResourceLifetime &lifetime, const Timeline &timeline)
{
  const LayoutRect *mine = layout.find(what);
  G_ASSERT_RETURN(mine != nullptr, false);
  bool fits = true;
  for_each_overlap(layout, *mine, lifetime, timeline, [&fits](const LayoutRect &) { return fits = false; });
  return fits;
}

void set_lifetime(Layout &layout, FrameResource what, const ResourceLifetime &lifetime, const Timeline &timeline,
  FrameResourceSet &evicted)
{
  const LayoutRect *mine = layout.find(what);
  G_ASSERT_RETURN(mine != nullptr, );
  layout.setLifetime(what, lifetime);
  for_each_overlap(layout, *mine, lifetime, timeline, [&evicted](const LayoutRect &rect) {
    evicted.insert(rect.what);
    return true;
  });
}

ProposedHeapsFmem propose_heaps_of(const Layout &layout, HeapClass heap_class, const Preservation &preservation, bool size_to_content,
  const HeapLimits &limits, bool relocate)
{
  ProposedHeapsFmem result;
  result.reserve(layout.heapsOf(heap_class).size());
  for (const auto heapIdx : layout.heapsOf(heap_class))
  {
    const auto &heap = layout.heaps[heapIdx];
    const bool pinned = !relocate && !limits.resizeKeepsContents && preservation.holdsPreserved(heap);
    const bool sizedToContent = size_to_content && (heap.rects.empty() || limits.resizeKeepsContents);
    result.push_back(ProposedHeap{
      .reservedSize = sizedToContent ? 0 : heap.reservedSize,
      .heap = heapIdx,
      .pinned = pinned,
    });
  }
  return result;
}

static bool compare_by_heap_then_offset(const ProposedRect &a, const ProposedRect &b)
{
  return a.proposedHeap != b.proposedHeap ? a.proposedHeap < b.proposedHeap : a.rect.offset < b.rect.offset;
}

void apply_packing(Layout &layout, HeapClass heap_class, ProposedPacking &packing, eastl::span<const WantedRect> wanted,
  PlacementsChanged &changed)
{
  auto &proposal = packing.heaps;
  dag::Vector<HeapIndex, framemem_allocator> inUse;
  inUse.reserve(proposal.size());
  for (const auto &packed : proposal)
    if (packed.heap != HeapIndex::Invalid)
      inUse.push_back(packed.heap);
  for (auto &packed : proposal)
    if (packed.heap == HeapIndex::Invalid)
    {
      packed.heap = layout.addHeap(heap_class, inUse);
      inUse.push_back(packed.heap);
    }

  eastl::sort(packing.placed.begin(), packing.placed.end(), compare_by_heap_then_offset);
  size_t first = 0;
  for (uint32_t slot = 0; slot < proposal.size(); ++slot)
  {
    const auto &packed = proposal[slot];
    auto &heap = layout.heaps[packed.heap];
    G_ASSERT(heap.heapClass == heap_class);
    heap.reservedSize = packed.reservedSize;

    size_t last = first;
    while (last < packing.placed.size() && packing.placed[last].proposedHeap == slot)
      ++last;
    const eastl::span<const ProposedRect> placedHere(packing.placed.data() + first, last - first);
    first = last;

    for (const auto &placed : placedHere)
    {
      const auto &rect = placed.rect;
      G_ASSERT(!layout.has(rect.what));
      layout.track(packed.heap, rect);
      const auto &wasAt = wanted[placed.item].wasAt;
      if (wasAt.heap != packed.heap || wasAt.offset != rect.offset)
        changed[rect.what.frame].set(rect.what.resIdx, true);
    }

    const size_t oldSize = heap.rects.size();
    heap.rects.resize(oldSize + placedHere.size());
    size_t write = heap.rects.size();
    size_t stays = oldSize;
    size_t fresh = placedHere.size();
    while (fresh > 0)
    {
      if (stays > 0 && placedHere[fresh - 1].rect.offset < heap.rects[stays - 1].offset)
        heap.rects[--write] = heap.rects[--stays];
      else
        heap.rects[--write] = placedHere[--fresh].rect;
    }
  }
}

} // namespace dafg
