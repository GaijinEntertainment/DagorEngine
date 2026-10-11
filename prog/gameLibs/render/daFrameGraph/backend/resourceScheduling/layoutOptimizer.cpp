// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "layoutOptimizer.h"

#include <EASTL/algorithm.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/sort.h>
#include <debug/dag_assert.h>


namespace dafg
{

uint64_t compute_heap_size_limit(const HeapLimits &limits, uint64_t largest_rect)
{
  if (limits.optimalMaxHeapSize == 0)
    return limits.maxHeapSize;
  return eastl::min(limits.maxHeapSize, eastl::max(limits.optimalMaxHeapSize, largest_rect));
}

namespace
{

template <class T>
using FmemVector = dag::Vector<T, framemem_allocator>;

constexpr uint32_t ALWAYS_AVAILABLE = eastl::numeric_limits<uint32_t>::max();
constexpr uint64_t NOT_PLACED = eastl::numeric_limits<uint64_t>::max();

struct Arc
{
  uint32_t start = 0;
  uint32_t end = 0;

  bool wraps() const { return end <= start; }
};

bool are_disjoint(Arc a, Arc b) { return !do_arcs_overlap(a.start, a.end, b.start, b.end); }

uint32_t rank_by_extent(Arc arc) { return arc.wraps() ? ~0u : arc.end - arc.start; }

struct Item
{
  Arc arc{};
  uint64_t size = 0;
  uint64_t align = 1;

  uint64_t alignUp(uint64_t offset) const { return (offset + align - 1) / align * align; }
  uint64_t sizeWithPadding(uint64_t offset) const { return size + alignUp(offset) - offset; }
};

struct Slot
{
  uint64_t offset = 0;
  uint64_t size = 0;
  Arc arc{};

  uint64_t end() const { return offset + size; }
};

using Slots = FmemVector<Slot>;

bool compare_by_slot_offset(const Slot &a, const Slot &b) { return a.offset < b.offset; }

class MomentSet
{
public:
  void reserve(uint32_t length) { words.reserve((length + 63) / 64); }
  void assign(uint32_t length)
  {
    words.assign((length + 63) / 64, 0);
    ringLength = length;
  }

  void mark(Arc arc)
  {
    if (arc.wraps())
    {
      markRange(arc.start, ringLength);
      markRange(0, arc.end);
    }
    else
      markRange(arc.start, arc.end);
  }

  bool meets(Arc arc) const
  {
    if (arc.wraps())
      return hasAnyInRange(arc.start, ringLength) || hasAnyInRange(0, arc.end);
    return hasAnyInRange(arc.start, arc.end);
  }

private:
  void markRange(uint32_t from, uint32_t to)
  {
    for (uint32_t at = from; at < to;)
    {
      const uint32_t word = at / 64;
      const uint32_t bit = at % 64;
      const uint32_t take = eastl::min<uint32_t>(64 - bit, to - at);
      words[word] |= take == 64 ? ~uint64_t{0} : (((uint64_t{1} << take) - 1) << bit);
      at += take;
    }
  }

  bool hasAnyInRange(uint32_t from, uint32_t to) const
  {
    for (uint32_t at = from; at < to;)
    {
      const uint32_t word = at / 64;
      const uint32_t bit = at % 64;
      const uint32_t take = eastl::min<uint32_t>(64 - bit, to - at);
      const uint64_t mask = take == 64 ? ~uint64_t{0} : (((uint64_t{1} << take) - 1) << bit);
      if ((words[word] & mask) != 0)
        return true;
      at += take;
    }
    return false;
  }

  FmemVector<uint64_t> words;
  uint32_t ringLength = 0;
};

struct Block
{
  uint64_t offset = 0;
  uint64_t size = 0;
  uint32_t availableUntil = ALWAYS_AVAILABLE;
};

class GreedyHeap
{
public:
  GreedyHeap(uint64_t max_size, FmemVector<Block> &free_list) : freeList(free_list), maxSize(max_size) { freeList.clear(); }

  EA_NODISCARD Block occupyPinned(uint64_t offset, uint64_t size, Arc arc, uint64_t smallest_item);
  EA_NODISCARD Block allocate(const Item &item);
  void release(Block block);

private:
  FmemVector<Block> &freeList;
  uint64_t currentSize = 0;
  uint64_t maxSize = 0;
};

Block GreedyHeap::occupyPinned(uint64_t offset, uint64_t size, Arc arc, uint64_t smallest_item)
{
  G_ASSERT(arc.wraps());
  G_ASSERT(offset >= currentSize);

  Block result{
    .offset = offset,
    .size = size,
    .availableUntil = arc.start,
  };
  const uint64_t pad = offset - currentSize;
  if (pad >= smallest_item)
    freeList.push_back(Block{
      .offset = currentSize,
      .size = pad,
      .availableUntil = ALWAYS_AVAILABLE,
    });
  else
  {
    result.offset = currentSize;
    result.size += pad;
  }
  currentSize = offset + size;
  return result;
}

Block GreedyHeap::allocate(const Item &item)
{
  size_t candidate = freeList.size();
  for (size_t i = 0; i < freeList.size(); ++i)
  {
    const auto &block = freeList[i];
    if (item.sizeWithPadding(block.offset) > block.size)
      continue;
    if (block.availableUntil != ALWAYS_AVAILABLE && !are_disjoint(item.arc, Arc{
                                                                              .start = block.availableUntil,
                                                                              .end = 0,
                                                                            }))
      continue;
    if (candidate == freeList.size() || block.size < freeList[candidate].size)
      candidate = i;
  }

  if (candidate == freeList.size())
  {
    const uint64_t allocSize = item.sizeWithPadding(currentSize);
    if (allocSize > maxSize || currentSize > maxSize - allocSize)
      return Block{
        .offset = 0,
        .size = 0,
        .availableUntil = ALWAYS_AVAILABLE,
      };

    const Block result{
      .offset = currentSize,
      .size = allocSize,
      .availableUntil = item.arc.wraps() ? item.arc.start : ALWAYS_AVAILABLE,
    };
    currentSize += allocSize;
    return result;
  }

  Block result = freeList[candidate];
  const uint64_t used = item.sizeWithPadding(result.offset);
  if (result.size > used)
  {
    freeList[candidate].offset += used;
    freeList[candidate].size -= used;
    result.size = used;
  }
  else
    freeList.erase(freeList.begin() + candidate);

  if (item.arc.wraps())
    result.availableUntil = item.arc.start;
  return result;
}

bool can_merge(const Block &a, const Block &b) { return a.offset + a.size == b.offset && a.availableUntil == b.availableUntil; }

void GreedyHeap::release(Block block)
{
  size_t at = 0;
  while (at < freeList.size() && freeList[at].offset < block.offset)
    ++at;

  if (at > 0 && can_merge(freeList[at - 1], block))
  {
    block.offset = freeList[at - 1].offset;
    block.size += freeList[at - 1].size;
    freeList.erase(freeList.begin() + (at - 1));
    --at;
  }
  if (at < freeList.size() && can_merge(block, freeList[at]))
  {
    block.size += freeList[at].size;
    freeList.erase(freeList.begin() + at);
  }

  if (block.offset + block.size == currentSize && block.availableUntil == ALWAYS_AVAILABLE)
    currentSize -= block.size;
  else
    freeList.insert(freeList.begin() + at, block);
}

enum class EventType : uint32_t
{
  End,
  Start
};

struct Event
{
  uint32_t item = 0;
  uint32_t moment = 0;
  EventType type = EventType::End;
  uint64_t size = 0;
  uint32_t extentRank = 0;
  bool pinned = false;
};

bool compare_events(const Event &a, const Event &b)
{
  if (a.moment != b.moment)
    return a.moment < b.moment;
  if (a.type != b.type)
    return a.type == EventType::End;
  if (a.size != b.size)
    return a.size > b.size;
  if (a.extentRank != b.extentRank)
    return a.extentRank > b.extentRank;
  if (a.pinned != b.pinned)
    return a.pinned;
  return a.item < b.item;
}

bool fits_below(uint64_t base, uint64_t size, uint64_t limit) { return size <= limit && base <= limit - size; }

uint64_t round_up(uint64_t value, uint64_t step) { return (value + step - 1) / step * step; }

// Reserved once per pack, in declaration order. No member grows, and frame
// memory is freed in reverse.
struct Scratch
{
  FmemVector<uint32_t> order;
  FmemVector<uint64_t> offsets;
  FmemVector<uint32_t> wrapping;
  FmemVector<Block> blocks;
  MomentSet covered;

  void reserve(size_t items, uint32_t length)
  {
    order.reserve(items);
    offsets.reserve(items);
    wrapping.reserve(items);
    blocks.reserve(items);
    covered.reserve(length);
  }
};

// Reserved for each heap. A heap can hold many more rects than the pack places.
struct HeapScratch
{
  Slots slots;
  FmemVector<Block> pinnedBlocks;
  FmemVector<Event> events;
  FmemVector<Block> freeList;

  void reserveScanline(size_t items)
  {
    pinnedBlocks.reserve(slots.size());
    events.reserve(2 * items + slots.size());
    // Each release of an item or a slot adds at most one free block. So does
    // each pad below a slot.
    freeList.reserve(items + 2 * slots.size());
  }
};

void insert_slot(Slots &slots, const Slot &slot)
{
  const size_t at = eastl::upper_bound(slots.begin(), slots.end(), slot, compare_by_slot_offset) - slots.begin();
  slots.insert(slots.begin() + at, slot);
}

void order_by_size(eastl::span<const uint32_t> queue, eastl::span<const Item> items, FmemVector<uint32_t> &order)
{
  order.clear();
  for (uint32_t i = 0; i < queue.size(); ++i)
    order.push_back(i);
  eastl::sort(order.begin(), order.end(), [queue, items](uint32_t a, uint32_t b) {
    const auto &first = items[queue[a]];
    const auto &second = items[queue[b]];
    if (first.size != second.size)
      return first.size > second.size;
    const uint32_t firstRank = rank_by_extent(first.arc);
    const uint32_t secondRank = rank_by_extent(second.arc);
    return firstRank != secondRank ? firstRank > secondRank : a < b;
  });
}

void run_scanline(eastl::span<const uint32_t> queue, eastl::span<const Item> items, HeapScratch &local, uint64_t max_size,
  Scratch &scratch)
{
  const Slots &slots = local.slots;
  uint64_t smallest = eastl::numeric_limits<uint64_t>::max();
  for (const auto i : queue)
    smallest = eastl::min(smallest, items[i].size);

  GreedyHeap heap(max_size, local.freeList);
  local.pinnedBlocks.clear();
  for (const auto &slot : slots)
    local.pinnedBlocks.push_back(heap.occupyPinned(slot.offset, slot.size, slot.arc, smallest));

  scratch.offsets.assign(queue.size(), NOT_PLACED);

  auto &events = local.events;
  events.clear();
  auto &wrapping = scratch.wrapping;
  wrapping.clear();
  for (uint32_t i = 0; i < queue.size(); ++i)
  {
    const auto &item = items[queue[i]];
    const uint32_t extentRank = rank_by_extent(item.arc);
    events.push_back(Event{
      .item = i,
      .moment = item.arc.start,
      .type = EventType::Start,
      .size = item.size,
      .extentRank = extentRank,
    });
    events.push_back(Event{
      .item = i,
      .moment = item.arc.end,
      .type = EventType::End,
      .size = item.size,
      .extentRank = extentRank,
    });
    if (item.arc.wraps())
      wrapping.push_back(i);
  }
  for (uint32_t i = 0; i < slots.size(); ++i)
    events.push_back(Event{
      .item = i,
      .moment = slots[i].arc.end,
      .type = EventType::End,
      .size = slots[i].size,
      .extentRank = rank_by_extent(slots[i].arc),
      .pinned = true,
    });
  eastl::sort(events.begin(), events.end(), compare_events);

  eastl::sort(wrapping.begin(), wrapping.end(), [queue, items](uint32_t a, uint32_t b) {
    const auto &first = items[queue[a]];
    const auto &second = items[queue[b]];
    return first.arc.start != second.arc.start ? first.arc.start > second.arc.start : a < b;
  });

  scratch.blocks.assign(queue.size(), Block{});
  for (const auto i : wrapping)
    scratch.blocks[i] = heap.allocate(items[queue[i]]);

  for (const auto &event : events)
  {
    if (event.pinned)
    {
      heap.release(local.pinnedBlocks[event.item]);
      continue;
    }

    const auto &item = items[queue[event.item]];
    if (event.type == EventType::Start)
    {
      if (!item.arc.wraps())
        scratch.blocks[event.item] = heap.allocate(item);
      continue;
    }

    const Block block = scratch.blocks[event.item];
    if (block.size == 0)
      continue;
    scratch.offsets[event.item] = item.alignUp(block.offset);
    heap.release(block);
  }
}

void run_holes(eastl::span<const uint32_t> queue, eastl::span<const Item> items, Slots &slots, uint64_t max_size, Scratch &scratch)
{
  scratch.offsets.assign(queue.size(), NOT_PLACED);
  order_by_size(queue, items, scratch.order);

  for (const auto k : scratch.order)
  {
    const auto &item = items[queue[k]];
    uint64_t cursor = 0;
    uint64_t offset = NOT_PLACED;
    for (size_t i = 0; i < slots.size(); ++i)
    {
      if (slots[i].end() <= cursor || are_disjoint(item.arc, slots[i].arc))
        continue;
      const uint64_t base = item.alignUp(cursor);
      if (fits_below(base, item.size, slots[i].offset) && fits_below(base, item.size, max_size))
      {
        offset = base;
        break;
      }
      cursor = slots[i].end();
    }
    if (offset == NOT_PLACED)
    {
      const uint64_t base = item.alignUp(cursor);
      if (fits_below(base, item.size, max_size))
        offset = base;
    }
    if (offset == NOT_PLACED)
      continue;

    scratch.offsets[k] = offset;
    insert_slot(slots, Slot{
                         .offset = offset,
                         .size = item.size,
                         .arc = item.arc,
                       });
  }
}

struct Packing
{
  const Layout *layout = nullptr;
  ProposedPacking *out = nullptr;
  eastl::span<const WantedRect> wanted;
  const Timeline *timeline = nullptr;
  uint32_t length = 0;
  uint32_t boundary = 0;
  FmemVector<Item> items;
  FmemVector<uint32_t> remaining;
  Scratch scratch;
  HeapLimits limits;

  uint32_t rotate(uint32_t moment) const { return moment >= boundary ? moment - boundary : moment + length - boundary; }

  Arc computeArc(const ResourceLifetime &lifetime) const
  {
    if (limits.noAliasing)
      return Arc{
        .start = 0,
        .end = length,
      };
    return Arc{
      .start = rotate(timeline->moment(lifetime.firstUse)),
      .end = rotate(timeline->moment(lifetime.release)),
    };
  }

  uint64_t computeLimitOfRest() const
  {
    uint64_t largest = 0;
    for (const auto i : remaining)
      largest = eastl::max(largest, items[i].size);
    return compute_heap_size_limit(limits, largest);
  }

  void reserve()
  {
    items.reserve(wanted.size());
    remaining.reserve(wanted.size());
    scratch.reserve(wanted.size(), length);
  }

  uint64_t measureUsedSize(size_t heap) const
  {
    const HeapIndex heapIdx = out->heaps[heap].heap;
    uint64_t result = heapIdx != HeapIndex::Invalid ? measure_used_size(layout->heaps[heapIdx]) : 0;
    for (const auto &placed : out->placed)
      if (placed.proposedHeap == heap)
        result = eastl::max(result, placed.rect.end());
    return result;
  }

  template <class Visit>
  void forEachSlot(size_t heap, const Visit &visit) const
  {
    const HeapIndex heapIdx = out->heaps[heap].heap;
    if (heapIdx != HeapIndex::Invalid)
      for (const auto &rect : layout->heaps[heapIdx].rects)
        visit(
          Slot{
            .offset = rect.offset,
            .size = rect.size,
            .arc = computeArc(rect.lifetime),
          },
          false);
    for (const auto &placed : out->placed)
      if (placed.proposedHeap == heap)
        visit(
          Slot{
            .offset = placed.rect.offset,
            .size = placed.rect.size,
            .arc = computeArc(placed.rect.lifetime),
          },
          true);
  }

  // The holes pass needs only the held rects that live at the same time as a
  // rect to place. We count them first to reserve the slots once.
  void collectSlots(size_t heap, Slots &slots, bool &scanline)
  {
    bool everyPinnedWraps = true;
    size_t all = 0;
    size_t met = 0;
    forEachSlot(heap, [&](const Slot &slot, bool placed) {
      everyPinnedWraps = everyPinnedWraps && slot.arc.wraps();
      ++all;
      met += placed || scratch.covered.meets(slot.arc) ? 1 : 0;
    });
    scanline = everyPinnedWraps;
    slots.reserve(scanline ? all : met + remaining.size());

    bool anyPlaced = false;
    forEachSlot(heap, [&](const Slot &slot, bool placed) {
      if (!scanline && !placed && !scratch.covered.meets(slot.arc))
        return;
      anyPlaced = anyPlaced || placed;
      slots.push_back(slot);
    });
    if (anyPlaced)
      eastl::sort(slots.begin(), slots.end(), compare_by_slot_offset);
  }

  void run(size_t heap, uint64_t max_size)
  {
    scratch.covered.assign(length);
    for (const auto i : remaining)
      scratch.covered.mark(items[i].arc);

    HeapScratch local;
    bool scanline = false;
    collectSlots(heap, local.slots, scanline);

    if (scanline)
    {
      local.reserveScanline(remaining.size());
      run_scanline(remaining, items, local, max_size, scratch);
    }
    else
      run_holes(remaining, items, local.slots, max_size, scratch);

    auto &placed = out->placed;
    size_t kept = 0;
    for (size_t k = 0; k < remaining.size(); ++k)
    {
      if (scratch.offsets[k] == NOT_PLACED)
      {
        remaining[kept++] = remaining[k];
        continue;
      }
      const auto &want = wanted[remaining[k]];
      placed.push_back(ProposedRect{
        .rect =
          {
            .what = want.what,
            .offset = scratch.offsets[k],
            .size = want.size,
            .align = want.align,
            .lifetime = want.lifetime,
            .hash = want.hash,
          },
        .item = remaining[k],
        .proposedHeap = static_cast<uint32_t>(heap),
      });
    }
    remaining.resize(kept);
  }
};

} // namespace

ProposedPacking try_pack_into_layout(const Layout &layout, eastl::span<const ProposedHeap> heaps, eastl::span<const WantedRect> wanted,
  const HeapLimits &limits, const Timeline &timeline, uint32_t boundary)
{
  G_ASSERT(limits.growthStep > 0);

  ProposedPacking result;
  result.placed.reserve(wanted.size());
  result.unplaced.reserve(wanted.size());
  // Each new heap gets at least one rect
  result.heaps.reserve(heaps.size() + wanted.size());
  result.heaps.assign(heaps.begin(), heaps.end());
  auto &proposal = result.heaps;
  FmemVector<bool> sizedToContent;
  sizedToContent.reserve(proposal.size() + 1);
  for (const auto &heap : proposal)
    sizedToContent.push_back(heap.reservedSize == 0 && !heap.pinned);

  const uint32_t length = timeline.length();
  if (DAGOR_UNLIKELY(length == 0))
  {
    for (uint32_t i = 0; i < wanted.size(); ++i)
      result.unplaced.push_back(i);
    return result;
  }

  Packing packing{
    .layout = &layout,
    .out = &result,
    .wanted = wanted,
    .timeline = &timeline,
    .length = length,
    .boundary = boundary,
    .limits = limits,
  };
  packing.reserve();
  for (uint32_t i = 0; i < wanted.size(); ++i)
  {
    const auto &want = wanted[i];
    G_ASSERT(want.size > 0 && want.align > 0);
    packing.items.push_back(Item{
      .arc = packing.computeArc(want.lifetime),
      .size = want.size,
      .align = want.align,
    });
    packing.remaining.push_back(i);
  }

  if (limits.fixedSize)
  {
    if (proposal.empty() && !packing.remaining.empty())
    {
      proposal.push_back(ProposedHeap{});
      sizedToContent.push_back(false);
    }
    for (size_t i = 0; i < proposal.size(); ++i)
    {
      if (!proposal[i].pinned)
        proposal[i].reservedSize = limits.maxHeapSize;
      sizedToContent[i] = false;
    }
  }

  for (size_t i = 0; i < proposal.size(); ++i)
  {
    if (!packing.remaining.empty())
      packing.run(i, sizedToContent[i] ? packing.computeLimitOfRest() : proposal[i].reservedSize);
    if (sizedToContent[i])
      proposal[i].reservedSize = packing.measureUsedSize(i);
  }

  if (!limits.fixedSize)
  {
    const uint64_t limit = compute_heap_size_limit(limits, 0);
    for (size_t i = 0; i < proposal.size() && !packing.remaining.empty(); ++i)
    {
      if (proposal[i].pinned || sizedToContent[i] || proposal[i].reservedSize >= limit)
        continue;
      const size_t placedBefore = result.placed.size();
      packing.run(i, limit);
      if (result.placed.size() == placedBefore)
        continue;
      proposal[i].reservedSize =
        eastl::max(proposal[i].reservedSize, eastl::min(limit, round_up(packing.measureUsedSize(i), limits.growthStep)));
    }
  }

  while (!limits.fixedSize && !packing.remaining.empty())
  {
    proposal.push_back(ProposedHeap{});

    const size_t heap = proposal.size() - 1;
    const size_t placedBefore = result.placed.size();
    packing.run(heap, packing.computeLimitOfRest());
    if (result.placed.size() == placedBefore)
    {
      proposal.pop_back();
      break;
    }
    proposal[heap].reservedSize = packing.measureUsedSize(heap);
  }

  result.unplaced.assign(packing.remaining.begin(), packing.remaining.end());

  return result;
}

} // namespace dafg
