// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/sort.h>

#include <memory/dag_framemem.h>

#include <backend/resourceScheduling/layoutOptimizer.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "layoutTestHelpers.h"


namespace
{

using namespace layout_test;


dafg::ResourceLifetime live_wrapping(uint32_t first, uint32_t release) { return make_lifetime(1, first, 0, release); }

dafg::WantedRect make_wanted(uint32_t res, uint64_t size, dafg::ResourceLifetime lifetime)
{
  return {
    .what = make_frame_resource(res),
    .size = size,
    .align = ALIGN,
    .lifetime = lifetime,
    .hash = res,
  };
}

struct Input
{
  dafg::Layout layout;
  dafg::ProposedHeapsFmem heaps;
};

Input make_one_heap(dafg::HeapIndex index, uint64_t reserved_size, eastl::span<const dafg::LayoutRect> rects)
{
  Input result;
  auto [idx, heap] = result.layout.heaps.appendNew();
  REQUIRE(idx == index);
  heap.reservedSize = reserved_size;
  heap.rects.assign(rects.begin(), rects.end());
  result.heaps.push_back(dafg::ProposedHeap{
    .reservedSize = reserved_size,
    .heap = idx,
  });
  return result;
}

struct Packed
{
  dafg::Layout layout;
  dag::Vector<dafg::ProposedHeap> heaps;
  dag::Vector<bool> isNew;
  dag::Vector<uint32_t> unplaced;

  size_t countHeaps() const { return heaps.size(); }
  const dafg::LayoutHeap &getHeap(size_t i) const { return layout.heaps[heaps[i].heap]; }
};

Packed run_pack(Input &&input, eastl::span<const dafg::WantedRect> wanted, const dafg::HeapLimits &limits, uint64_t growth_step,
  const dafg::Timeline &timeline, uint32_t boundary, bool no_aliasing)
{
  Packed result;
  result.layout = eastl::move(input.layout);
  dafg::HeapLimits withStep = limits;
  withStep.growthStep = growth_step;
  withStep.noAliasing = no_aliasing;
  auto packed = dafg::try_pack_into_layout(result.layout, input.heaps, wanted, withStep, timeline, boundary);
  CHECK(packed.placed.size() + packed.unplaced.size() == wanted.size());
  result.unplaced.assign(packed.unplaced.begin(), packed.unplaced.end());
  result.heaps.assign(packed.heaps.begin(), packed.heaps.end());

  for (uint32_t i = 0; i < result.heaps.size(); ++i)
  {
    auto &heap = result.heaps[i];
    result.isNew.push_back(heap.heap == dafg::HeapIndex::Invalid);
    if (result.isNew.back())
      heap.heap = result.layout.heaps.appendNew().first;
    auto &target = result.layout.heaps[heap.heap];
    target.reservedSize = heap.reservedSize;
    for (const auto &rect : packed.placed)
      if (rect.proposedHeap == i)
        target.rects.push_back(rect.rect);
    eastl::sort(target.rects.begin(), target.rects.end(),
      [](const dafg::LayoutRect &a, const dafg::LayoutRect &b) { return a.offset < b.offset; });
  }
  return result;
}

const dafg::LayoutRect *find_rect(const Packed &result, uint32_t res, uint32_t frame = 0)
{
  for (size_t i = 0; i < result.countHeaps(); ++i)
    for (const auto &r : result.getHeap(i).rects)
      if (r.what == make_frame_resource(res, frame))
        return &r;
  return nullptr;
}

size_t find_heap_of(const Packed &result, uint32_t res, uint32_t frame = 0)
{
  for (size_t i = 0; i < result.countHeaps(); ++i)
    for (const auto &r : result.getHeap(i).rects)
      if (r.what == make_frame_resource(res, frame))
        return i;
  return ~size_t(0);
}

bool fits_the_heaps(const Packed &result)
{
  for (size_t h = 0; h < result.countHeaps(); ++h)
    for (const auto &r : result.getHeap(h).rects)
      if (r.end() > result.getHeap(h).reservedSize || r.offset % r.align != 0)
        return false;
  return true;
}

bool check_no_shared_memory(const Packed &result)
{
  for (size_t h = 0; h < result.countHeaps(); ++h)
  {
    const auto &rects = result.getHeap(h).rects;
    for (size_t i = 0; i < rects.size(); ++i)
      for (size_t j = i + 1; j < rects.size(); ++j)
        if (rects[i].offset < rects[j].end() && rects[j].offset < rects[i].end())
          return false;
  }
  return true;
}

bool check_no_live_aliases(const Packed &result)
{
  for (size_t h = 0; h < result.countHeaps(); ++h)
  {
    const auto &rects = result.getHeap(h).rects;
    for (size_t i = 0; i < rects.size(); ++i)
      for (size_t j = i + 1; j < rects.size(); ++j)
      {
        const auto &a = rects[i];
        const auto &b = rects[j];
        const bool memoryOverlap = a.offset < b.end() && b.offset < a.end();
        if (memoryOverlap && TIMELINE.overlap(a.lifetime, b.lifetime))
          return false;
      }
  }
  return true;
}

} // namespace


TEST_CASE("a pack of an empty class gives every frame resource memory", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::WantedRect wanted[] = {make_wanted(0, KB, live_within(0, 0, 4)), make_wanted(1, KB, live_within(0, 1, 5)),
    make_wanted(2, KB, live_within(0, 2, 6)), make_wanted(3, KB, live_within(0, 3, 7))};

  const auto result = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  CHECK(result.isNew[0]);
  CHECK(result.getHeap(0).reservedSize == 4 * KB);
  CHECK(result.getHeap(0).rects.size() == 4);
  CHECK(fits_the_heaps(result));
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a pack aliases frame resources that never share a moment", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::WantedRect wanted[] = {
    make_wanted(0, KB, live_within(0, 0, 4)), make_wanted(1, KB, live_within(0, 4, 8)), make_wanted(2, KB, live_within(1, 0, 4))};

  const auto result = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  CHECK(result.getHeap(0).reservedSize == KB);
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a pack never aliases a frame resource that wraps the frame ring", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::WantedRect wanted[] = {
    make_wanted(0, KB, live_wrapping(6, 2)), make_wanted(1, KB, live_within(0, 0, 4)), make_wanted(2, KB, live_within(1, 4, 8))};

  const auto result = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  CHECK(result.getHeap(0).reservedSize == 2 * KB);
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a pinned frame resource keeps its offset and its heap keeps its size", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 4 * KB, KB, live_wrapping(6, 2))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), 16 * KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 0, 4))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  CHECK(result.heaps[0].heap == static_cast<dafg::HeapIndex>(0));
  CHECK(result.getHeap(0).reservedSize == 16 * KB);
  REQUIRE(find_rect(result, 0) != nullptr);
  CHECK(find_rect(result, 0)->offset == 4 * KB);
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a heap without a pinned frame resource is sized to its new rects", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), 0, {});
  const dafg::WantedRect wanted[] = {make_wanted(0, KB, live_within(0, 0, 4)), make_wanted(1, KB, live_within(0, 2, 6))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 8 * KB, TIMELINE, 0, false);

  REQUIRE(result.countHeaps() == 1);
  CHECK(result.heaps[0].heap == static_cast<dafg::HeapIndex>(0));
  CHECK(result.getHeap(0).reservedSize == 2 * KB);
}

TEST_CASE("a heap with a pinned frame resource grows by the growth step", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_within(0, 0, 8))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 2, 6))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 8 * KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  CHECK(result.getHeap(0).reservedSize == 8 * KB);
  CHECK(fits_the_heaps(result));
}

TEST_CASE("a heap packed again keeps the rects its first run placed", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), 2 * KB, {});
  const dafg::WantedRect wanted[] = {make_wanted(0, 2 * KB, live_within(0, 0, 8)), make_wanted(1, KB, live_within(0, 2, 6))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  REQUIRE(find_rect(result, 0) != nullptr);
  REQUIRE(find_rect(result, 1) != nullptr);
  CHECK(find_rect(result, 0)->offset == 0);
  CHECK(find_rect(result, 1)->offset == 2 * KB);
  CHECK(result.getHeap(0).reservedSize == 3 * KB);
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a frame resource that fits no heap gets a heap of its own", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_within(0, 0, 8))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 2, 6))};

  const dafg::HeapLimits limits{
    .maxHeapSize = KB,
  };

  const auto result = run_pack(eastl::move(input), wanted, limits, KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 2);
  CHECK(result.getHeap(0).reservedSize == KB);
  CHECK(result.isNew[1]);
  CHECK(result.getHeap(1).reservedSize == KB);
  CHECK(find_heap_of(result, 1) == 1);
}

TEST_CASE("a pinned heap keeps its size and the unplaced frame resource goes to a new heap", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_wrapping(6, 2))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
  input.heaps[0].pinned = true;
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 0, 4))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 8 * KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 2);
  CHECK(result.heaps[0].heap == static_cast<dafg::HeapIndex>(0));
  CHECK(result.getHeap(0).reservedSize == KB);
  CHECK(result.isNew[1]);
  CHECK(find_heap_of(result, 1) == 1);
  CHECK(fits_the_heaps(result));
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a frame resource above the optimal heap size does not grow an existing heap", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_within(0, 0, 8))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, 16 * KB, live_within(0, 2, 6))};

  const dafg::HeapLimits limits{
    .optimalMaxHeapSize = 4 * KB,
  };

  const uint64_t growthStep = GENERATE(KB, 8 * KB);
  const auto result = run_pack(eastl::move(input), wanted, limits, growthStep, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 2);
  CHECK(result.getHeap(0).reservedSize == KB);
  CHECK(result.getHeap(1).reservedSize == 16 * KB);
  CHECK(find_heap_of(result, 1) == 1);
}

TEST_CASE("a frame resource bigger than the heap limit stays without memory", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::HeapLimits limits{
    .maxHeapSize = KB,
  };
  const dafg::WantedRect wanted[] = {make_wanted(0, 2 * KB, live_within(0, 0, 4))};

  const auto result = run_pack({}, wanted, limits, KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.size() == 1);
  CHECK((wanted[result.unplaced[0]].what == make_frame_resource(0)));
}

TEST_CASE("a class of a fixed size gets one heap and leftovers stay without memory", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::HeapLimits limits{
    .maxHeapSize = 2 * KB,
    .fixedSize = true,
  };
  const dafg::WantedRect wanted[] = {
    make_wanted(0, KB, live_within(0, 0, 8)), make_wanted(1, KB, live_within(0, 0, 8)), make_wanted(2, KB, live_within(0, 0, 8))};

  const auto result = run_pack({}, wanted, limits, KB, TIMELINE, 0, false);

  REQUIRE(result.countHeaps() == 1);
  CHECK(result.getHeap(0).reservedSize == 2 * KB);
  CHECK(result.getHeap(0).rects.size() == 2);
  CHECK(result.unplaced.size() == 1);
}

TEST_CASE("a pack of the same input gives the same result twice", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::WantedRect wanted[] = {make_wanted(0, KB, live_within(0, 0, 4)), make_wanted(1, 2 * KB, live_within(0, 1, 5)),
    make_wanted(2, KB, live_within(0, 2, 6)), make_wanted(3, 4 * KB, live_within(1, 3, 7)), make_wanted(4, KB, live_wrapping(8, 1))};

  const auto first = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);
  const auto second = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(first.countHeaps() == second.countHeaps());
  for (size_t i = 0; i < first.countHeaps(); ++i)
  {
    REQUIRE(first.getHeap(i).rects.size() == second.getHeap(i).rects.size());
    CHECK(first.getHeap(i).reservedSize == second.getHeap(i).reservedSize);
    for (size_t j = 0; j < first.getHeap(i).rects.size(); ++j)
    {
      CHECK((first.getHeap(i).rects[j].what == second.getHeap(i).rects[j].what));
      CHECK(first.getHeap(i).rects[j].offset == second.getHeap(i).rects[j].offset);
    }
  }
}

TEST_CASE("a pack respects the alignment of every frame resource", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  dafg::WantedRect wanted[] = {
    make_wanted(0, 100, live_within(0, 0, 8)), make_wanted(1, 100, live_within(0, 0, 8)), make_wanted(2, 100, live_within(0, 0, 8))};
  wanted[0].align = 64;
  wanted[1].align = 512;
  wanted[2].align = 128;

  const auto result = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  CHECK(fits_the_heaps(result));
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a pack with no aliasing gives every frame resource its own memory", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::WantedRect wanted[] = {make_wanted(0, KB, live_within(0, 0, 4)), make_wanted(1, KB, live_within(0, 4, 8)),
    make_wanted(2, KB, live_within(1, 0, 4)), make_wanted(3, KB, live_wrapping(6, 2))};

  const auto result = run_pack({}, wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, true);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  CHECK(result.getHeap(0).reservedSize == 4 * KB);
  CHECK(check_no_shared_memory(result));
  CHECK(fits_the_heaps(result));
}

TEST_CASE("an unplaced frame resource takes the memory of a pinned frame resource that is not alive with it", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 4, 8))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(find_rect(result, 1) != nullptr);
  CHECK(find_rect(result, 1)->offset == 0);
  CHECK(result.getHeap(0).reservedSize == KB);
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("an unplaced frame resource never takes the memory of a pinned frame resource alive with it", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 2, 6))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, KB, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(find_rect(result, 1) != nullptr);
  CHECK(find_rect(result, 1)->offset == KB);
  CHECK(result.getHeap(0).reservedSize == 2 * KB);
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a frame resource that ends before a pinned frame resource that wraps takes its memory", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, 0, KB, live_wrapping(6, 2))};
  const dafg::WantedRect early[] = {make_wanted(1, KB, live_within(0, 4, 8))};
  const dafg::WantedRect late[] = {make_wanted(1, KB, make_lifetime(1, 0, 1, 8))};

  {
    auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
    const auto result = run_pack(eastl::move(input), early, dafg::HeapLimits{}, KB, TIMELINE, 0, false);
    REQUIRE(result.unplaced.empty());
    REQUIRE(find_rect(result, 1) != nullptr);
    CHECK(find_rect(result, 1)->offset == 0);
    CHECK(result.getHeap(0).reservedSize == KB);
    CHECK(check_no_live_aliases(result));
  }

  {
    auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
    const auto result = run_pack(eastl::move(input), late, dafg::HeapLimits{}, KB, TIMELINE, 0, false);
    REQUIRE(result.unplaced.empty());
    REQUIRE(find_rect(result, 1) != nullptr);
    CHECK(find_rect(result, 1)->offset == KB);
    CHECK(result.getHeap(0).reservedSize == 2 * KB);
    CHECK(check_no_live_aliases(result));
  }
}

TEST_CASE("no aliasing keeps every unplaced frame resource off the memory of a pinned one", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect notWrapping[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  const dafg::LayoutRect wraps[] = {make_rect(0, 0, KB, live_wrapping(6, 2))};
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_within(0, 4, 8))};

  for (const auto &pinned : {eastl::span<const dafg::LayoutRect>{notWrapping}, eastl::span<const dafg::LayoutRect>{wraps}})
  {
    auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), KB, pinned);
    const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, KB, TIMELINE, 0, true);
    REQUIRE(result.unplaced.empty());
    REQUIRE(find_rect(result, 1) != nullptr);
    CHECK(find_rect(result, 1)->offset == KB);
    CHECK(result.getHeap(0).reservedSize == 2 * KB);
    CHECK(check_no_shared_memory(result));
  }
}

TEST_CASE("a sized-to-content heap that the pack never reaches keeps the size of its rects", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  Input input;
  for (uint32_t i = 0; i < 2; ++i)
  {
    auto [idx, heap] = input.layout.heaps.appendNew();
    heap.reservedSize = 4 * KB;
    heap.rects.push_back(make_rect(i, 0, KB, live_within(0, 0, 4)));
    input.heaps.push_back(dafg::ProposedHeap{
      .reservedSize = 0,
      .heap = idx,
    });
  }
  const dafg::WantedRect wanted[] = {make_wanted(2, KB, live_within(0, 5, 6))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  CHECK(result.getHeap(0).reservedSize == KB);
  CHECK(result.getHeap(1).reservedSize == KB);
  CHECK(fits_the_heaps(result));
}

TEST_CASE("a pack rotates the ring so that a pinned rect that wraps the old boundary is packed around, not above", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect pinned[] = {make_rect(0, KB, KB, live_wrapping(6, 2))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), 4 * KB, pinned);
  const dafg::WantedRect wanted[] = {make_wanted(1, 2 * KB, live_within(1, 2, 6)), make_wanted(2, KB, live_within(0, 2, 6))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 1, TIMELINE, TIMEPOINTS, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(find_rect(result, 1) != nullptr);
  CHECK(find_rect(result, 1)->offset == 0);
  CHECK(check_no_live_aliases(result));
  CHECK(fits_the_heaps(result));
}

TEST_CASE("a pack around two held rects that wrap uses the memory free between and below them", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const dafg::LayoutRect held[] = {make_rect(0, 0, KB, live_wrapping(6, 2)), make_rect(1, 2 * KB, KB, live_wrapping(5, 1))};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), 4 * KB, held);
  const dafg::WantedRect wanted[] = {make_wanted(2, KB, live_within(0, 3, 5)), make_wanted(3, KB, live_within(0, 0, 8))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(result.countHeaps() == 1);
  CHECK(result.getHeap(0).reservedSize == 4 * KB);
  CHECK(find_rect(result, 0)->offset == 0);
  CHECK(find_rect(result, 1)->offset == 2 * KB);
  REQUIRE(find_rect(result, 3) != nullptr);
  CHECK((find_rect(result, 3)->offset == KB || find_rect(result, 3)->offset == 3 * KB));
  CHECK(fits_the_heaps(result));
  CHECK(check_no_live_aliases(result));
}

TEST_CASE("a frame resource that crosses the frame boundary goes around the rects a heap holds", "[layoutOptimizer]")
{
  FRAMEMEM_REGION;
  const bool heldWraps = GENERATE(true, false);
  const auto heldLifetime = heldWraps ? live_wrapping(6, 2) : live_within(0, 0, 4);
  const dafg::LayoutRect held[] = {make_rect(0, 0, KB, heldLifetime)};
  auto input = make_one_heap(static_cast<dafg::HeapIndex>(0), 16 * KB, held);
  const dafg::WantedRect wanted[] = {make_wanted(1, KB, live_wrapping(7, 1)), make_wanted(2, KB, live_within(0, 5, 8))};

  const auto result = run_pack(eastl::move(input), wanted, dafg::HeapLimits{}, 1, TIMELINE, 0, false);

  REQUIRE(result.unplaced.empty());
  REQUIRE(find_rect(result, 0) != nullptr);
  CHECK(find_rect(result, 0)->offset == 0);
  REQUIRE(find_rect(result, 1) != nullptr);
  CHECK(find_rect(result, 1)->offset != 0);
  CHECK(fits_the_heaps(result));
  CHECK(check_no_live_aliases(result));
}
