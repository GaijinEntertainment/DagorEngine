// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "heapLayout.h"

#include <EASTL/algorithm.h>
#include <EASTL/utility.h>
#include <debug/dag_assert.h>


namespace dafg
{

bool do_arcs_overlap(uint32_t a_start, uint32_t a_end, uint32_t b_start, uint32_t b_end)
{
  const bool aWraps = a_end <= a_start;
  const bool bWraps = b_end <= b_start;
  if (aWraps && bWraps)
    return true;
  if (aWraps)
    return a_end > b_start || b_end > a_start;
  if (bWraps)
    return b_end > a_start || a_end > b_start;
  return a_end > b_start && b_end > a_start;
}

bool Timeline::overlap(const ResourceLifetime &a, const ResourceLifetime &b) const
{
  return do_arcs_overlap(moment(a.firstUse), moment(a.release), moment(b.firstUse), moment(b.release));
}

uint32_t Timeline::duration(const ResourceLifetime &lifetime) const
{
  const uint32_t start = moment(lifetime.firstUse);
  const uint32_t end = moment(lifetime.release);
  return end > start ? end - start : end + length() - start;
}

HeapIndex Layout::addHeap(HeapClass heap_class, eastl::span<const HeapIndex> in_use)
{
  HeapIndex idx = HeapIndex::Invalid;
  for (auto &&[candidate, heap] : heaps.enumerate())
    if (heap.rects.empty() && heap.reservedSize == 0 && eastl::find(in_use.begin(), in_use.end(), candidate) == in_use.end())
    {
      idx = candidate;
      break;
    }

  if (idx == HeapIndex::Invalid)
    idx = heaps.appendNew().first;
  else
  {
    auto &old = heapsOfClass[heaps[idx].heapClass];
    if (const auto it = eastl::find(old.begin(), old.end(), idx); it != old.end())
      old.erase(it);
    heaps[idx] = LayoutHeap{};
  }
  heaps[idx].heapClass = heap_class;
  heapsOfClass[heap_class].push_back(idx);
  return idx;
}

eastl::span<const HeapIndex> Layout::heapsOf(HeapClass heap_class) const
{
  const auto it = heapsOfClass.find(heap_class);
  return it == heapsOfClass.end() ? eastl::span<const HeapIndex>{} : eastl::span<const HeapIndex>{it->second};
}

void Layout::reserveResources(size_t resource_count)
{
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    if (where.locationOf[frame].totalKeys() < resource_count)
      where.locationOf[frame].resize(resource_count, NOWHERE);
    if (spanning[frame].size() < resource_count)
      spanning[frame].resize(resource_count, false);
  }
}

static const LayoutRect *find_in(const dag::Vector<LayoutRect> &rects, FrameResource what, uint64_t offset)
{
  auto it =
    eastl::lower_bound(rects.begin(), rects.end(), offset, [](const LayoutRect &rect, uint64_t value) { return rect.offset < value; });
  for (; it != rects.end() && it->offset == offset; ++it)
    if (it->what == what)
      return it;
  return nullptr;
}

const LayoutRect *Layout::find(FrameResource what) const
{
  const auto [heap, offset] = where[what];
  return heap == HeapIndex::Invalid ? nullptr : find_in(heaps[heap].rects, what, offset);
}

LayoutRect *Layout::find(FrameResource what) { return const_cast<LayoutRect *>(eastl::as_const(*this).find(what)); }

void Layout::add(HeapIndex heap, const LayoutRect &rect)
{
  G_ASSERTF(!has(rect.what), "daFG: frame resource %u of frame %u is already in the layout", eastl::to_underlying(rect.what.resIdx),
    rect.what.frame);
  auto &rects = heaps[heap].rects;
  const auto at = eastl::upper_bound(rects.begin(), rects.end(), rect.offset,
    [](uint64_t offset, const LayoutRect &other) { return offset < other.offset; });
  rects.insert(at, rect);
  track(heap, rect);
}

void Layout::track(HeapIndex heap, const LayoutRect &rect)
{
  where.put(rect.what, {
                         .heap = heap,
                         .offset = rect.offset,
                       });
  if (spans_frames(rect.lifetime))
  {
    ++heaps[heap].spanning[rect.what.frame];
    spanning[rect.what.frame].set(rect.what.resIdx, true);
  }
}

LayoutRect Layout::take(FrameResource what)
{
  const auto heap = where[what].heap;
  LayoutRect *it = find(what);
  G_ASSERT_RETURN(it != nullptr, {});
  const LayoutRect rect = *it;
  heaps[heap].rects.erase(it);
  where.put(what, NOWHERE);
  if (spans_frames(rect.lifetime))
  {
    --heaps[heap].spanning[what.frame];
    spanning[what.frame].set(what.resIdx, false);
  }
  return rect;
}

void Layout::setLifetime(FrameResource what, const ResourceLifetime &lifetime)
{
  const auto heap = where[what].heap;
  LayoutRect *rect = find(what);
  G_ASSERT_RETURN(rect != nullptr, );
  if (spans_frames(rect->lifetime))
    --heaps[heap].spanning[what.frame];
  rect->lifetime = lifetime;
  const bool spans = spans_frames(lifetime);
  if (spans)
    ++heaps[heap].spanning[what.frame];
  spanning[what.frame].set(what.resIdx, spans);
}

WantedRect convert_to_wanted(const LayoutRect &rect, HeapClass heap_class, AllocationLocation was_at)
{
  return {
    .what = rect.what,
    .size = rect.size,
    .align = rect.align,
    .lifetime = rect.lifetime,
    .hash = rect.hash,
    .heapClass = heap_class,
    .wasAt = was_at,
  };
}

static bool overlaps_in_memory(const LayoutRect &rect, uint64_t offset, uint64_t size)
{
  return rect.offset < offset + size && offset < rect.end();
}

uint64_t measure_used_size(const LayoutHeap &heap)
{
  uint64_t result = 0;
  for (const auto &rect : heap.rects)
    result = eastl::max(result, rect.end());
  return result;
}

size_t count_rects(const Layout &layout, HeapClass heap_class)
{
  size_t result = 0;
  for (const auto heapIdx : layout.heapsOf(heap_class))
    result += layout.heaps[heapIdx].rects.size();
  return result;
}

bool is_layout_valid(const Layout &layout, const Timeline &timeline)
{
  size_t listed = 0;
  for (const auto &[heapClass, indices] : layout.heapsOfClass)
    for (const auto heapIdx : indices)
    {
      if (!layout.heaps.isMapped(heapIdx) || !(layout.heaps[heapIdx].heapClass == heapClass))
        return false;
      ++listed;
    }
  if (listed != layout.heaps.size())
    return false;

  for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
  {
    if (measure_used_size(heap) > heap.reservedSize)
      return false;
    eastl::array<uint32_t, SCHEDULE_FRAME_WINDOW> spanning{};
    for (size_t i = 0; i < heap.rects.size(); ++i)
    {
      const auto &a = heap.rects[i];
      if (a.size == 0 || a.align == 0 || a.offset % a.align != 0)
        return false;
      if (i > 0 && heap.rects[i - 1].offset > a.offset)
        return false;
      if (const auto at = layout.where[a.what]; at.heap != heapIdx || at.offset != a.offset)
        return false;
      if (spans_frames(a.lifetime))
        ++spanning[a.what.frame];
      if (layout.spanning[a.what.frame].test(a.what.resIdx, false) != spans_frames(a.lifetime))
        return false;
      for (size_t j = i + 1; j < heap.rects.size(); ++j)
      {
        const auto &b = heap.rects[j];
        if (a.what == b.what)
          return false;
        if (overlaps_in_memory(b, a.offset, a.size) && timeline.overlap(a.lifetime, b.lifetime))
          return false;
      }
    }
    if (spanning != heap.spanning)
      return false;
  }
  return true;
}

} // namespace dafg
