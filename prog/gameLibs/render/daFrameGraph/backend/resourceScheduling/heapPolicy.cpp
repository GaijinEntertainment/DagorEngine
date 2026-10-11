// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "heapPolicy.h"

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>


namespace dafg
{

static bool is_before(LifetimePoint a, LifetimePoint b) { return a.frame != b.frame ? a.frame < b.frame : a.timepoint < b.timepoint; }

struct LiveEvent
{
  LifetimePoint at{};
  int64_t size = 0;
  uint32_t heapSlot = 0;
  bool inFreeHeap = false;
};

using LiveEvents = dag::Vector<LiveEvent, framemem_allocator>;

static uint64_t measure_waste(uint64_t reserved, int64_t peak) { return reserved > static_cast<uint64_t>(peak) ? reserved - peak : 0; }

ClassWaste measure_recoverable_waste(const Layout &layout, HeapClass heap_class, const Preservation &preservation,
  bool resize_keeps_contents)
{
  const auto heapsOfClass = layout.heapsOf(heap_class);
  ClassWaste result;
  result.heaps.reserve(heapsOfClass.size());
  dag::Vector<int64_t, framemem_allocator> heapLive(heapsOfClass.size(), 0);
  dag::Vector<int64_t, framemem_allocator> heapPeak(heapsOfClass.size(), 0);
  LiveEvents events;
  events.reserve(2 * count_rects(layout, heap_class));
  int64_t liveAtStart = 0;
  int64_t freeLiveAtStart = 0;
  uint64_t reserved = 0;
  uint64_t freeReserved = 0;
  for (uint32_t slot = 0; slot < heapsOfClass.size(); ++slot)
  {
    const auto heapIdx = heapsOfClass[slot];
    const auto &heap = layout.heaps[heapIdx];
    const bool inFreeHeap = resize_keeps_contents || !preservation.holdsPreserved(heap);
    result.heaps.push_back({
      .heap = heapIdx,
      .pinned = !inFreeHeap,
      .free = inFreeHeap ? heap.reservedSize : 0,
      .locked = inFreeHeap ? 0 : heap.reservedSize,
    });
    reserved += heap.reservedSize;
    if (inFreeHeap)
      freeReserved += heap.reservedSize;
    for (const auto &rect : heap.rects)
    {
      const int64_t size = static_cast<int64_t>(rect.size);
      if (!is_before(rect.lifetime.firstUse, rect.lifetime.release))
      {
        liveAtStart += size;
        heapLive[slot] += size;
        if (inFreeHeap)
          freeLiveAtStart += size;
      }
      events.push_back({
        .at = rect.lifetime.firstUse,
        .size = size,
        .heapSlot = slot,
        .inFreeHeap = inFreeHeap,
      });
      events.push_back({
        .at = rect.lifetime.release,
        .size = -size,
        .heapSlot = slot,
        .inFreeHeap = inFreeHeap,
      });
      if (!inFreeHeap && preservation.covers(layout, rect.what))
        result.preservedBytes += rect.size;
    }
    heapPeak[slot] = heapLive[slot];
  }

  eastl::sort(events.begin(), events.end(),
    [](const LiveEvent &a, const LiveEvent &b) { return is_before(a.at, b.at) || (a.at == b.at && a.size < b.size); });

  int64_t live = liveAtStart;
  int64_t freeLive = freeLiveAtStart;
  int64_t peak = live;
  int64_t freePeak = freeLive;
  for (const auto &event : events)
  {
    live += event.size;
    peak = eastl::max(peak, live);
    heapLive[event.heapSlot] += event.size;
    heapPeak[event.heapSlot] = eastl::max(heapPeak[event.heapSlot], heapLive[event.heapSlot]);
    if (event.inFreeHeap)
    {
      freeLive += event.size;
      freePeak = eastl::max(freePeak, freeLive);
    }
  }

  for (uint32_t slot = 0; slot < heapsOfClass.size(); ++slot)
  {
    auto &heap = result.heaps[slot];
    heap.free = measure_waste(heap.free, heapPeak[slot]);
    heap.locked = measure_waste(heap.locked, heapPeak[slot]);
    heap.content = eastl::min<uint64_t>(layout.heaps[heap.heap].reservedSize, static_cast<uint64_t>(heapPeak[slot]));
  }
  const uint64_t freeWaste = measure_waste(freeReserved, freePeak);
  const uint64_t total = measure_waste(reserved, peak);
  result.free = freeWaste;
  result.locked = total > freeWaste ? total - freeWaste : 0;
  return result;
}

uint64_t measure_defragmentation_burst(const ClassWaste &waste, eastl::span<const HeapIndex> chosen, bool relocate)
{
  uint64_t result = 0;
  for (const auto &heap : waste.heaps)
    if ((relocate || !heap.pinned) && eastl::find(chosen.begin(), chosen.end(), heap.heap) != chosen.end())
      result += heap.content;
  return result;
}

dag::Vector<HeapIndex, framemem_allocator> select_heaps_to_defragment(const ClassWaste &waste, uint64_t reserved_now, bool relocate,
  const HeapPolicySettings &settings)
{
  dag::Vector<HeapIndex, framemem_allocator> result;
  const uint64_t recoverable = waste.recoverable(relocate);
  const uint64_t floor =
    eastl::max<uint64_t>(settings.defragmentMinWaste, static_cast<uint64_t>(reserved_now * settings.defragmentWasteFractionStop));
  if (recoverable == 0 || recoverable < floor)
    return result;
  result.reserve(waste.heaps.size());
  const uint64_t needed = recoverable - floor;

  dag::Vector<HeapWaste, framemem_allocator> byWaste(waste.heaps.begin(), waste.heaps.end());
  eastl::sort(byWaste.begin(), byWaste.end(),
    [relocate](const HeapWaste &a, const HeapWaste &b) { return a.recoverable(relocate) > b.recoverable(relocate); });

  uint64_t accumulated = 0;
  for (const auto &heap : byWaste)
  {
    const uint64_t recovers = heap.recoverable(relocate);
    if (recovers == 0)
      break;
    result.push_back(heap.heap);
    accumulated += recovers;
    if (accumulated >= needed)
      return result;
  }
  result.clear();
  for (const auto &heap : waste.heaps)
    result.push_back(heap.heap);
  return result;
}

DefragmentationDecision decide_defragmentation(DefragmentationState &state, uint64_t reserved_now, const ClassWaste &waste,
  const HeapPolicySettings &settings, uint32_t frame)
{
  if (!is_defragmentation_check_due(state, settings, frame))
    return {};
  const int64_t now = frame;
  state.lastCheckFrame = now;

  const bool relocate = is_relocation_worth_it(waste, settings);
  if (!is_class_wasteful(state, reserved_now, waste, relocate, settings, state.wastefulSinceFrame >= 0))
  {
    state.wastefulSinceFrame = -1;
    return {};
  }

  if (state.wastefulSinceFrame < 0)
    state.wastefulSinceFrame = now;
  if (now - state.wastefulSinceFrame < settings.defragmentDelayFrames)
    return {};

  state.wastefulSinceFrame = -1;
  return {
    .defragment = true,
    .relocate = relocate,
    .sizeToContent = true,
  };
}

} // namespace dafg
