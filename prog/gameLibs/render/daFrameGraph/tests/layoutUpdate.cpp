// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <initializer_list>

#include <memory/dag_framemem.h>

#include <backend/resourceScheduling/layoutUpdate.h>
#include <catch2/catch_test_macros.hpp>

#include "layoutTestHelpers.h"


namespace
{

using namespace layout_test;

dafg::WantedRect make_wanted(uint32_t res, uint64_t size, dafg::ResourceLifetime lifetime, dafg::HeapClass heap_class = TRACKED,
  uint32_t frame = 0)
{
  return {
    .what = make_frame_resource(res, frame),
    .size = size,
    .align = ALIGN,
    .lifetime = lifetime,
    .hash = res,
    .heapClass = heap_class,
  };
}

const dafg::Preservation PRESERVING_FRAME_0{
  .prevFrame = 0,
};
const dafg::Preservation PRESERVING_FRAME_1{
  .prevFrame = 1,
};
const dafg::Preservation HISTORY_INVALID{
  .prevFrame = 0,
  .historyInvalid = true,
};

bool holds(eastl::span<const dafg::LayoutRect> rects, uint32_t res, uint32_t frame = 0)
{
  for (const auto &r : rects)
    if (r.what == make_frame_resource(res, frame))
      return true;
  return false;
}

bool holds(eastl::span<const dafg::WantedRect> rects, uint32_t res, uint32_t frame = 0)
{
  for (const auto &r : rects)
    if (r.what == make_frame_resource(res, frame))
      return true;
  return false;
}

dafg::intermediate::HistoryPairing make_pairing(std::initializer_list<int> old_indices)
{
  dafg::intermediate::HistoryPairing result;
  for (const int oldIdx : old_indices)
    result.push_back(oldIdx < 0 ? dafg::intermediate::RESOURCE_NOT_MAPPED
                                : static_cast<dafg::intermediate::ResourceIndex>(static_cast<uint32_t>(oldIdx)));
  return result;
}

void pack_and_apply(dafg::Layout &layout, dafg::HeapClass heap_class, const dafg::WantedRectsFmem &toBePlaced,
  dafg::PlacementsChanged &changed)
{
  const dafg::HeapLimits limits{
    .growthStep = KB,
  };
  const auto heaps = dafg::propose_heaps_of(layout, heap_class, PRESERVING_FRAME_0, false, limits);
  auto packing = dafg::try_pack_into_layout(layout, heaps, toBePlaced, limits, TIMELINE, 0);
  REQUIRE(packing.unplaced.empty());
  dafg::apply_packing(layout, heap_class, packing, toBePlaced, changed);
}

size_t count_flags(const dafg::PlacementsChanged &changed)
{
  size_t result = 0;
  for (const auto &perFrame : changed)
    for (const auto idx : perFrame.trueKeys())
    {
      G_UNUSED(idx);
      ++result;
    }
  return result;
}

} // namespace


TEST_CASE("only rects that share memory in an untracked heap are aliases", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  const dafg::HeapClass untracked{
    .group = GROUP,
    .untracked = true,
  };
  dafg::Layout layout;
  const dafg::LayoutRect aliasing[] = {make_rect(0, 0, 2 * KB, live_within(0, 0, 4)), make_rect(1, KB, KB, live_within(0, 4, 8)),
    make_rect(2, 2 * KB, KB, live_within(0, 0, 8))};
  add_heap(layout, untracked, 4 * KB, aliasing);
  const dafg::LayoutRect tracked[] = {make_rect(3, 0, KB, live_within(0, 0, 4)), make_rect(4, 0, KB, live_within(0, 4, 8))};
  add_heap(layout, TRACKED, 4 * KB, tracked);

  dag::Vector<eastl::pair<dafg::FrameResource, dafg::FrameResource>> pairs;
  dafg::for_each_untracked_alias(layout, [&](dafg::FrameResource a, dafg::FrameResource b) { pairs.push_back({a, b}); });

  REQUIRE(pairs.size() == 1);
  CHECK((pairs[0].first == make_frame_resource(0)));
  CHECK((pairs[0].second == make_frame_resource(1)));
}

TEST_CASE("the reindex moves the rects of paired resources and drops the rest", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 4)), make_rect(1, KB, KB, live_within(0, 0, 4)),
    make_rect(2, 2 * KB, KB, live_across_frames(0, 4)), make_rect(2, 3 * KB, KB, live_within(1, 0, 4), 1),
    make_rect(3, 4 * KB, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED, 5 * KB, rects);

  const auto pairing = make_pairing({2, -1});
  dafg::reindex(layout, dafg::invert_pairing(pairing), pairing.totalKeys());

  REQUIRE(get_heap_at(layout, 0).rects.size() == 2);
  CHECK(layout.where[make_frame_resource(0, 1)].heap == static_cast<dafg::HeapIndex>(0));
  CHECK(layout.where[make_frame_resource(0, 1)].offset == 3 * KB);
  CHECK_FALSE(layout.has(make_frame_resource(2, 1)));
  CHECK(holds(get_heap_at(layout, 0).rects, 0));
  CHECK(get_heap_at(layout, 0).rects[0].offset == 2 * KB);
  CHECK(layout.where[make_frame_resource(0)].heap == static_cast<dafg::HeapIndex>(0));
  CHECK(layout.where[make_frame_resource(0)].offset == 2 * KB);
  CHECK_FALSE(layout.has(make_frame_resource(1)));
  CHECK(layout.spanning[0].test(static_cast<dafg::intermediate::ResourceIndex>(0), false));
  CHECK_FALSE(layout.spanning[0].test(static_cast<dafg::intermediate::ResourceIndex>(2), false));
  CHECK(get_heap_at(layout, 0).spanning[0] == 1);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("a heap with a preserved frame resource is pinned when a resize loses its contents", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2)), make_rect(1, KB, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED, 4 * KB, rects);
  const dafg::LayoutRect other[] = {make_rect(2, 0, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED, 4 * KB, other);
  const dafg::LayoutRect otherClass[] = {make_rect(3, 0, KB, live_across_frames(8, 2))};
  const auto otherClassHeap = add_heap(layout, TRACKED_2, 4 * KB, otherClass);

  dafg::HeapLimits limits;
  const auto frozen = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, false, limits);
  REQUIRE(frozen.size() == 2);
  CHECK(frozen[0].heap != otherClassHeap);
  CHECK(frozen[1].heap != otherClassHeap);
  CHECK(frozen[0].pinned);
  CHECK_FALSE(frozen[1].pinned);

  const auto otherParity = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_1, false, limits);
  CHECK_FALSE(otherParity[0].pinned);

  const auto invalidated = dafg::propose_heaps_of(layout, TRACKED, HISTORY_INVALID, false, limits);
  CHECK_FALSE(invalidated[0].pinned);

  limits.resizeKeepsContents = true;
  const auto thawed = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, false, limits);
  CHECK_FALSE(thawed[0].pinned);
}

TEST_CASE("a proposal sized to content resets only the heaps that cannot lose their contents", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect preserved[] = {make_rect(0, 0, KB, live_across_frames(8, 2))};
  const dafg::LayoutRect plain[] = {make_rect(1, 0, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED, 4 * KB, preserved);
  add_heap(layout, TRACKED, 4 * KB, plain);
  add_heap(layout, TRACKED, 4 * KB, {});

  dafg::HeapLimits limits;
  const auto sized = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, true, limits);
  REQUIRE(sized.size() == 3);
  CHECK(sized[0].pinned);
  CHECK(sized[0].reservedSize == 4 * KB);
  CHECK_FALSE(sized[1].pinned);
  CHECK(sized[1].reservedSize == 4 * KB);
  CHECK(sized[2].reservedSize == 0);

  const auto relocated = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, true, limits, true);
  REQUIRE(relocated.size() == 3);
  CHECK_FALSE(relocated[0].pinned);
  CHECK(relocated[0].reservedSize == 4 * KB);
  CHECK(relocated[2].reservedSize == 0);

  limits.resizeKeepsContents = true;
  const auto kept = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, true, limits);
  REQUIRE(kept.size() == 3);
  for (const auto &heap : kept)
  {
    CHECK_FALSE(heap.pinned);
    CHECK(heap.reservedSize == 0);
  }
}

TEST_CASE("applying a pack merges the new rects around the ones the heap holds", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect held[] = {make_rect(0, KB, KB, live_within(0, 0, 8))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, held);

  dafg::WantedRectsFmem toBePlaced;
  toBePlaced.push_back(make_wanted(1, KB, live_within(0, 0, 8)));
  toBePlaced.push_back(make_wanted(2, KB, live_within(0, 0, 8)));
  dafg::PlacementsChanged changed;
  pack_and_apply(layout, TRACKED, toBePlaced, changed);

  const auto &rects = layout.heaps[heapIdx].rects;
  REQUIRE(rects.size() == 3);
  CHECK((rects[0].what == make_frame_resource(1)));
  CHECK(rects[0].offset == 0);
  CHECK((rects[1].what == make_frame_resource(0)));
  CHECK(rects[1].offset == KB);
  CHECK((rects[2].what == make_frame_resource(2)));
  CHECK(rects[2].offset == 2 * KB);
  CHECK(count_flags(changed) == 2);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("preservation follows the frame parity and the invalidation", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2)), make_rect(1, KB, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED, 4 * KB, rects);

  CHECK(PRESERVING_FRAME_0.covers(layout, make_frame_resource(0)));
  CHECK_FALSE(PRESERVING_FRAME_0.covers(layout, make_frame_resource(1)));
  CHECK_FALSE(PRESERVING_FRAME_1.covers(layout, make_frame_resource(0)));
  CHECK_FALSE(HISTORY_INVALID.covers(layout, make_frame_resource(0)));

  const dafg::intermediate::ResourceIndex invalidFor[] = {static_cast<dafg::intermediate::ResourceIndex>(0)};
  const dafg::Preservation partly{
    .prevFrame = 0,
    .historyInvalidFor = invalidFor,
  };
  CHECK_FALSE(partly.covers(layout, make_frame_resource(0)));

  const dafg::intermediate::ResourceIndex otherInvalid[] = {static_cast<dafg::intermediate::ResourceIndex>(1)};
  const dafg::Preservation elsewhere{
    .prevFrame = 0,
    .historyInvalidFor = otherInvalid,
  };
  CHECK(elsewhere.covers(layout, make_frame_resource(0)));

  const dafg::ResourceSetFmem spannedNothing(RESOURCE_COUNT, false);
  const dafg::Preservation newlySpanning{
    .prevFrame = 0,
    .spanningBefore = &spannedNothing,
  };
  CHECK_FALSE(newlySpanning.covers(layout, make_frame_resource(0)));

  dafg::ResourceSetFmem spannedRect(RESOURCE_COUNT, false);
  spannedRect.set(make_frame_resource(0).resIdx, true);
  const dafg::Preservation stillSpanning{
    .prevFrame = 0,
    .spanningBefore = &spannedRect,
  };
  CHECK(stillSpanning.covers(layout, make_frame_resource(0)));
}

TEST_CASE("a removed frame resource leaves the layout and is flagged", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 4)), make_rect(1, KB, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED, 4 * KB, rects);

  dafg::PlacementsChanged changed;
  dafg::remove_rect(layout, make_frame_resource(1), changed);

  REQUIRE(get_heap_at(layout, 0).rects.size() == 1);
  CHECK(holds(get_heap_at(layout, 0).rects, 0));
  CHECK(count_flags(changed) == 1);
  CHECK(changed[0].test(make_frame_resource(1).resIdx, false));
  CHECK_FALSE(layout.has(make_frame_resource(1)));
}

TEST_CASE("a stripped frame resource becomes unplaced in its new heap class", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, rects);

  dafg::WantedRectsFmem toBePlaced;
  auto moving = make_wanted(0, KB, live_within(0, 0, 4), TRACKED_2);
  moving.wasAt = {
    .heap = heapIdx,
    .offset = 0,
  };
  dafg::strip_rect(layout, moving, toBePlaced);

  CHECK(get_heap_at(layout, 0).rects.empty());
  REQUIRE(toBePlaced.size() == 1);
  CHECK((toBePlaced[0].heapClass == TRACKED_2));
  CHECK(toBePlaced[0].wasAt.heap == heapIdx);
}

TEST_CASE("a preserved lifetime that grows over a neighbour evicts the neighbour", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2)), make_rect(1, 0, KB, live_within(0, 4, 6)),
    make_rect(2, KB, KB, live_within(0, 0, 8)), make_rect(3, 2 * KB, KB, live_within(1, 0, 2), 1),
    make_rect(4, 2 * KB, KB, live_within(1, 4, 6), 1), make_rect(5, 0, KB, live_within(0, 1, 3))};
  add_heap(layout, TRACKED, 4 * KB, rects);
  REQUIRE(PRESERVING_FRAME_0.covers(layout, make_frame_resource(0)));

  dafg::FrameResourceSet evicted(RESOURCE_COUNT);
  dafg::set_lifetime(layout, make_frame_resource(0), live_across_frames(0, 2), TIMELINE, evicted);

  CHECK(evicted.contains(make_frame_resource(1)));
  CHECK(evicted.contains(make_frame_resource(5)));
  CHECK_FALSE(evicted.contains(make_frame_resource(2)));
  CHECK(layout.has(make_frame_resource(1)));
  CHECK((get_heap_at(layout, 0).rects[0].lifetime == live_across_frames(0, 2)));
  CHECK(get_heap_at(layout, 0).spanning[0] == 1);

  dafg::FrameResourceSet evictedAgain(RESOURCE_COUNT);
  dafg::set_lifetime(layout, make_frame_resource(0), live_across_frames(8, 2), TIMELINE, evictedAgain);
  CHECK_FALSE(evictedAgain.contains(make_frame_resource(1)));
  CHECK_FALSE(evictedAgain.contains(make_frame_resource(2)));
  CHECK_FALSE(evictedAgain.contains(make_frame_resource(5)));

  dafg::FrameResourceSet evictedOnFrame1(RESOURCE_COUNT);
  dafg::set_lifetime(layout, make_frame_resource(3, 1), live_within(1, 0, 6), TIMELINE, evictedOnFrame1);
  CHECK(evictedOnFrame1.contains(make_frame_resource(4, 1)));
  CHECK_FALSE(evictedOnFrame1.contains(make_frame_resource(4, 0)));
  CHECK_FALSE(evictedOnFrame1.contains(make_frame_resource(1)));
}

TEST_CASE("a new lifetime fits in place unless it overlaps a rect in the same memory", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {
    make_rect(0, 0, KB, live_within(0, 0, 2)), make_rect(1, 0, KB, live_within(0, 4, 6)), make_rect(2, KB, KB, live_within(0, 0, 8))};
  add_heap(layout, TRACKED, 4 * KB, rects);

  CHECK(dafg::fits_in_place(layout, make_frame_resource(0), live_within(0, 0, 1), TIMELINE));
  CHECK(dafg::fits_in_place(layout, make_frame_resource(0), live_within(0, 0, 4), TIMELINE));
  CHECK_FALSE(dafg::fits_in_place(layout, make_frame_resource(0), live_within(0, 0, 5), TIMELINE));
}

TEST_CASE("stripping a class keeps the rects for which the predicate is false", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2)), make_rect(1, KB, KB, live_within(0, 0, 4)),
    make_rect(2, 2 * KB, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, rects);
  const dafg::LayoutRect otherClass[] = {make_rect(4, 0, KB, live_within(0, 0, 4))};
  add_heap(layout, TRACKED_2, 4 * KB, otherClass);

  dafg::WantedRectsFmem toBePlaced;
  dafg::strip_class(layout, TRACKED, toBePlaced,
    [&](dafg::HeapIndex, const dafg::LayoutRect &r) { return !PRESERVING_FRAME_0.covers(layout, r.what); });

  REQUIRE(get_heap_at(layout, 0).rects.size() == 1);
  CHECK(holds(get_heap_at(layout, 0).rects, 0));
  REQUIRE(toBePlaced.size() == 2);
  CHECK(holds(toBePlaced, 1));
  CHECK(holds(toBePlaced, 2));
  CHECK(layout.has(make_frame_resource(4)));
  for (const auto &wanted : toBePlaced)
  {
    CHECK(wanted.wasAt.heap == heapIdx);
    CHECK(wanted.wasAt.offset == (wanted.what == make_frame_resource(1) ? KB : 2 * KB));
  }
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("applying the pack flags only the frame resources that moved", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, rects);

  dafg::WantedRectsFmem toBePlaced;
  toBePlaced.push_back(make_wanted(1, KB, live_within(0, 2, 6)));
  auto moving = make_wanted(0, KB, live_within(0, 0, 4));
  moving.wasAt = {
    .heap = heapIdx,
    .offset = 0,
  };
  dafg::strip_rect(layout, moving, toBePlaced);

  dafg::PlacementsChanged changed;
  pack_and_apply(layout, TRACKED, toBePlaced, changed);

  CHECK(count_flags(changed) == 1);
  CHECK(changed[0].test(make_frame_resource(1).resIdx, false));
  CHECK_FALSE(changed[0].test(make_frame_resource(0).resIdx, false));
  CHECK(layout.heaps[heapIdx].rects.size() == 2);
  CHECK(layout.where[make_frame_resource(1)].heap == heapIdx);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("frame resources of the second frame keep their own lookups and flags", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect held[] = {make_rect(0, 0, KB, make_lifetime(1, 2, 0, 4), 1)};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, held);

  CHECK(layout.where[make_frame_resource(0, 1)].heap == heapIdx);
  CHECK_FALSE(layout.has(make_frame_resource(0, 0)));
  CHECK(layout.spanning[1].test(make_frame_resource(0, 1).resIdx, false));
  CHECK_FALSE(layout.spanning[0].test(make_frame_resource(0, 1).resIdx, false));
  CHECK(get_heap_at(layout, 0).spanning[1] == 1);
  CHECK(get_heap_at(layout, 0).spanning[0] == 0);

  dafg::WantedRectsFmem toBePlaced;
  toBePlaced.push_back(make_wanted(1, KB, live_within(1, 5, 8), TRACKED, 1));
  dafg::PlacementsChanged changed;
  pack_and_apply(layout, TRACKED, toBePlaced, changed);

  CHECK(layout.where[make_frame_resource(1, 1)].heap == heapIdx);
  CHECK_FALSE(layout.has(make_frame_resource(1, 0)));
  CHECK(changed[1].test(make_frame_resource(1, 1).resIdx, false));
  CHECK_FALSE(changed[0].test(make_frame_resource(1, 1).resIdx, false));
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("applying a pack writes the proposed size of a heap the layout holds", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect held[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, held);

  dafg::ProposedPacking packing;
  packing.heaps.push_back(dafg::ProposedHeap{
    .reservedSize = 2 * KB,
    .heap = heapIdx,
  });
  dafg::PlacementsChanged changed;
  dafg::apply_packing(layout, TRACKED, packing, {}, changed);

  CHECK(layout.heaps[heapIdx].reservedSize == 2 * KB);
  CHECK(layout.heaps[heapIdx].rects.size() == 1);
  CHECK(count_flags(changed) == 0);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("applying a proposal adds its new heaps to the layout", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  layout.reserveResources(RESOURCE_COUNT);

  dafg::WantedRectsFmem toBePlaced;
  toBePlaced.push_back(make_wanted(0, KB, live_within(0, 0, 4)));
  toBePlaced.push_back(make_wanted(1, KB, live_across_frames(2, 6)));

  dafg::PlacementsChanged changed;
  pack_and_apply(layout, TRACKED, toBePlaced, changed);

  REQUIRE(layout.heaps.size() == 1);
  CHECK(get_heap_at(layout, 0).reservedSize == 2 * KB);
  CHECK(count_flags(changed) == 2);
  CHECK(get_heap_at(layout, 0).spanning[0] == 1);
  CHECK(PRESERVING_FRAME_0.covers(layout, make_frame_resource(1)));
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("applying the pack flags a frame resource that lands away from its old place", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 2 * KB, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, rects);

  dafg::WantedRectsFmem toBePlaced;
  auto moving = make_wanted(0, KB, live_within(0, 0, 4));
  moving.wasAt = {
    .heap = heapIdx,
    .offset = 2 * KB,
  };
  dafg::strip_rect(layout, moving, toBePlaced);

  dafg::PlacementsChanged changed;
  pack_and_apply(layout, TRACKED, toBePlaced, changed);

  REQUIRE(layout.where[make_frame_resource(0)].heap == heapIdx);
  REQUIRE(layout.where[make_frame_resource(0)].offset != 2 * KB);
  CHECK(count_flags(changed) == 1);
  CHECK(changed[0].test(make_frame_resource(0).resIdx, false));
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("a stripped rect that the pack leaves unplaced is out of the layout and has no flag", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, KB, rects);

  dafg::WantedRectsFmem toBePlaced;
  auto grown = make_wanted(0, 2 * KB, live_within(0, 0, 4));
  grown.wasAt = {
    .heap = heapIdx,
    .offset = 0,
  };
  dafg::strip_rect(layout, grown, toBePlaced);

  const dafg::HeapLimits limits{
    .maxHeapSize = KB,
    .growthStep = KB,
  };
  const auto heaps = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, false, limits);
  auto packing = dafg::try_pack_into_layout(layout, heaps, toBePlaced, limits, TIMELINE, 0);
  REQUIRE(packing.unplaced.size() == 1);
  dafg::PlacementsChanged changed;
  dafg::apply_packing(layout, TRACKED, packing, toBePlaced, changed);

  CHECK_FALSE(layout.has(make_frame_resource(0)));
  CHECK(count_flags(changed) == 0);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("applying a proposal gives each of its new heaps its own slot", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  layout.reserveResources(RESOURCE_COUNT);

  dafg::WantedRectsFmem toBePlaced;
  toBePlaced.push_back(make_wanted(0, KB, live_within(0, 0, 4)));
  toBePlaced.push_back(make_wanted(1, KB, live_within(0, 2, 6)));

  const dafg::HeapLimits limits{
    .maxHeapSize = KB,
    .growthStep = KB,
  };
  const auto heaps = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, false, limits);
  auto packing = dafg::try_pack_into_layout(layout, heaps, toBePlaced, limits, TIMELINE, 0);
  REQUIRE(packing.unplaced.empty());
  REQUIRE(packing.heaps.size() == 2);
  CHECK(packing.heaps[0].heap == dafg::HeapIndex::Invalid);
  CHECK(packing.heaps[1].heap == dafg::HeapIndex::Invalid);
  dafg::PlacementsChanged changed;
  dafg::apply_packing(layout, TRACKED, packing, toBePlaced, changed);

  CHECK(layout.heaps.size() == 2);
  CHECK(packing.heaps[0].heap != packing.heaps[1].heap);
  CHECK(layout.where[make_frame_resource(0)].heap != layout.where[make_frame_resource(1)].heap);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("a new heap of a proposal does not take the slot of an emptied heap that the proposal names", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const auto emptied = add_heap(layout, TRACKED, 0, {});

  dafg::WantedRectsFmem toBePlaced;
  toBePlaced.push_back(make_wanted(0, KB, live_within(0, 0, 4)));
  toBePlaced.push_back(make_wanted(1, KB, live_within(0, 2, 6)));

  const dafg::HeapLimits limits{
    .maxHeapSize = KB,
    .growthStep = KB,
  };
  const auto heaps = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, true, limits);
  auto packing = dafg::try_pack_into_layout(layout, heaps, toBePlaced, limits, TIMELINE, 0);
  REQUIRE(packing.unplaced.empty());
  REQUIRE(packing.heaps.size() == 2);
  REQUIRE(packing.heaps[0].heap == emptied);
  REQUIRE(packing.heaps[1].heap == dafg::HeapIndex::Invalid);
  dafg::PlacementsChanged changed;
  dafg::apply_packing(layout, TRACKED, packing, toBePlaced, changed);

  CHECK(layout.heaps.size() == 2);
  CHECK(packing.heaps[1].heap != emptied);
  CHECK(layout.where[make_frame_resource(0)].heap != layout.where[make_frame_resource(1)].heap);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("applying the pack flags a frame resource that lands in another heap at its old offset", "[layoutUpdate]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect full[] = {make_rect(1, 0, KB, live_within(0, 0, 8))};
  const auto fullHeap = add_heap(layout, TRACKED, KB, full);
  const auto emptyHeap = add_heap(layout, TRACKED, KB, {});

  dafg::WantedRectsFmem toBePlaced;
  auto moved = make_wanted(0, KB, live_within(0, 0, 4));
  moved.wasAt = {
    .heap = fullHeap,
    .offset = 0,
  };
  toBePlaced.push_back(moved);

  const dafg::HeapLimits limits{
    .maxHeapSize = KB,
    .growthStep = KB,
  };
  const auto heaps = dafg::propose_heaps_of(layout, TRACKED, PRESERVING_FRAME_0, false, limits);
  auto packing = dafg::try_pack_into_layout(layout, heaps, toBePlaced, limits, TIMELINE, 0);
  REQUIRE(packing.unplaced.empty());
  dafg::PlacementsChanged changed;
  dafg::apply_packing(layout, TRACKED, packing, toBePlaced, changed);

  REQUIRE(layout.where[make_frame_resource(0)].heap == emptyHeap);
  REQUIRE(layout.where[make_frame_resource(0)].offset == 0);
  CHECK(changed[0].test(make_frame_resource(0).resIdx, false));
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}
