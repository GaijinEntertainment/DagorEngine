// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/algorithm.h>
#include <memory/dag_framemem.h>

#include <backend/resourceScheduling/heapPolicy.h>
#include <catch2/catch_test_macros.hpp>

#include "layoutTestHelpers.h"


namespace
{

using namespace layout_test;

const dafg::Preservation PRESERVING{
  .prevFrame = 0,
};

uint64_t sum_reserved(const dafg::Layout &layout, dafg::HeapClass heap_class)
{
  uint64_t result = 0;
  for (const auto &heap : layout.heaps)
    if (heap.heapClass == heap_class)
      result += heap.reservedSize;
  return result;
}

dafg::HeapPolicySettings make_waste_settings()
{
  return {
    .defragmentMinWaste = KB,
    .relocationMaxBytes = 0,
    .defragmentDelayFrames = 300,
    .defragmentCheckIntervalFrames = 60,
    .defragmentWasteFraction = 0.25f,
  };
}

} // namespace


TEST_CASE("a class is defragmented once it wastes memory for long enough", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  const auto settings = make_waste_settings();
  const uint64_t reserved = sum_reserved(layout, TRACKED);
  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);
  CHECK(waste.free == 63 * KB);

  dafg::DefragmentationState state;
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 0).defragment);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 30).defragment);
  CHECK(dafg::decide_defragmentation(state, reserved, waste, settings, 360).defragment);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 720).defragment);
}

TEST_CASE("a started delay survives a dip of the waste under the start fraction", "[heapPolicy]")
{
  auto settings = make_waste_settings();
  settings.defragmentWasteFractionStop = 0.15f;
  const uint64_t reserved = 64 * KB;
  const dafg::ClassWaste high{.free = 16 * KB};
  const dafg::ClassWaste dip{.free = 12 * KB};
  const dafg::ClassWaste low{.free = 8 * KB};

  dafg::DefragmentationState state;
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, high, settings, 0).defragment);
  CHECK(state.wastefulSinceFrame == 0);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, dip, settings, 60).defragment);
  CHECK(state.wastefulSinceFrame == 0);
  CHECK(dafg::decide_defragmentation(state, reserved, dip, settings, 300).defragment);

  dafg::DefragmentationState fresh;
  CHECK_FALSE(dafg::decide_defragmentation(fresh, reserved, dip, settings, 0).defragment);
  CHECK(fresh.wastefulSinceFrame < 0);
  CHECK_FALSE(dafg::decide_defragmentation(fresh, reserved, high, settings, 60).defragment);
  CHECK(fresh.wastefulSinceFrame == 60);
  CHECK_FALSE(dafg::decide_defragmentation(fresh, reserved, low, settings, 120).defragment);
  CHECK(fresh.wastefulSinceFrame < 0);
  CHECK_FALSE(dafg::decide_defragmentation(fresh, reserved, high, settings, 360).defragment);
}

TEST_CASE("a class that wastes little memory is never defragmented", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, 63 * KB, live_within(0, 0, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  auto settings = make_waste_settings();
  settings.defragmentDelayFrames = 0;
  settings.defragmentCheckIntervalFrames = 0;

  const uint64_t reserved = sum_reserved(layout, TRACKED);
  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);

  dafg::DefragmentationState state;
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 0).defragment);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 3600).defragment);
}

TEST_CASE("the waste of a heap with a preserved frame resource stays when a copy is not allowed", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  auto settings = make_waste_settings();
  settings.defragmentDelayFrames = 0;
  settings.defragmentCheckIntervalFrames = 0;

  const uint64_t reserved = sum_reserved(layout, TRACKED);
  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);
  CHECK(waste.free == 0);
  CHECK(waste.locked == 63 * KB);
  CHECK(waste.preservedBytes == KB);

  const dafg::intermediate::ResourceIndex invalidFor[] = {make_frame_resource(0).resIdx};
  const dafg::Preservation invalidated{
    .prevFrame = 0,
    .historyInvalidFor = invalidFor,
  };
  const auto notCovered = dafg::measure_recoverable_waste(layout, TRACKED, invalidated);
  CHECK(notCovered.locked == 63 * KB);
  CHECK(notCovered.preservedBytes == 0);

  dafg::DefragmentationState state;
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 0).defragment);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 3600).defragment);
}

TEST_CASE("a class is not defragmented again until its recoverable waste grows", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_within(0, 0, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  auto settings = make_waste_settings();
  settings.defragmentDelayFrames = 0;

  const uint64_t reserved = sum_reserved(layout, TRACKED);
  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);

  dafg::DefragmentationState state{
    .recoverableAfterDefragmentation = waste.recoverable(false),
  };
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 60).defragment);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, waste, settings, 3600).defragment);
  const dafg::ClassWaste lockedGrew{
    .free = waste.free,
    .locked = KB,
  };
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, lockedGrew, settings, 7200).defragment);
  const dafg::ClassWaste grown{
    .free = 64 * KB,
  };
  CHECK(dafg::decide_defragmentation(state, reserved, grown, settings, 10800).defragment);
}

TEST_CASE("the waste of a heap with a preserved frame resource is defragmented by a relocation when the copy is worth it",
  "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  auto settings = make_waste_settings();
  settings.defragmentDelayFrames = 0;
  settings.defragmentCheckIntervalFrames = 0;
  settings.relocationMaxBytes = 64 * KB;

  const uint64_t reserved = sum_reserved(layout, TRACKED);
  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);

  dafg::DefragmentationState state;
  const auto decision = dafg::decide_defragmentation(state, reserved, waste, settings, 0);
  CHECK(decision.defragment);
  CHECK(decision.relocate);
  CHECK(decision.sizeToContent);
  CHECK_FALSE(state.pending.defragment);
  CHECK(state.pendingSince == -1);

  settings.relocationWasteRatio = 128.f;
  dafg::DefragmentationState strict;
  CHECK_FALSE(dafg::decide_defragmentation(strict, reserved, waste, settings, 0).defragment);
}

TEST_CASE("every heap of a class has its own waste", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect small[] = {make_rect(0, 0, KB, live_within(0, 0, 2))};
  const dafg::LayoutRect big[] = {make_rect(1, 0, 63 * KB, live_within(0, 0, 2))};
  const dafg::LayoutRect otherClass[] = {make_rect(2, 0, KB, live_within(0, 0, 2))};
  add_heap(layout, TRACKED, 64 * KB, small);
  add_heap(layout, TRACKED, 64 * KB, big);
  add_heap(layout, TRACKED_2, 64 * KB, otherClass);

  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);
  CHECK(waste.free == 64 * KB);
  REQUIRE(waste.heaps.size() == 2);
  CHECK(waste.heaps[0].heap == layout.heapsOf(TRACKED)[0]);
  CHECK(waste.heaps[0].free == 63 * KB);
  CHECK(waste.heaps[1].free == KB);
}

TEST_CASE("the most wasteful heaps are packed anew before the whole class", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  auto settings = make_waste_settings();
  settings.defragmentWasteFractionStop = 0.15f;
  const auto h0 = static_cast<dafg::HeapIndex>(0);
  const auto h1 = static_cast<dafg::HeapIndex>(1);

  dafg::ClassWaste oneBad{.free = 64 * KB};
  oneBad.heaps.push_back({.heap = h0, .free = 63 * KB});
  oneBad.heaps.push_back({.heap = h1, .free = KB});
  const auto chosen = dafg::select_heaps_to_defragment(oneBad, 128 * KB, false, settings);
  REQUIRE(chosen.size() == 1);
  CHECK(chosen[0] == h0);

  dafg::ClassWaste spread{.free = 64 * KB};
  spread.heaps.push_back({.heap = h0, .free = 32 * KB});
  spread.heaps.push_back({.heap = h1, .free = 32 * KB});
  CHECK(dafg::select_heaps_to_defragment(spread, 128 * KB, false, settings).size() == 2);

  dafg::ClassWaste locked{.locked = 64 * KB};
  locked.heaps.push_back({.heap = h0, .locked = 63 * KB});
  locked.heaps.push_back({.heap = h1, .locked = KB});
  CHECK(dafg::select_heaps_to_defragment(locked, 128 * KB, false, settings).empty());
  REQUIRE(dafg::select_heaps_to_defragment(locked, 128 * KB, true, settings).size() == 1);
}

TEST_CASE("every heap is packed anew when the heaps alone recover less than the class", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  auto settings = make_waste_settings();
  settings.defragmentWasteFractionStop = 0.15f;
  const auto h0 = static_cast<dafg::HeapIndex>(0);
  const auto h1 = static_cast<dafg::HeapIndex>(1);
  const auto h2 = static_cast<dafg::HeapIndex>(2);

  // The peaks of the heaps do not all fall on the same moment, so the class
  // wastes more than its heaps together. The heap that wastes nothing is packed
  // anew too.
  dafg::ClassWaste shared{.free = 64 * KB};
  shared.heaps.push_back({.heap = h0, .free = 20 * KB});
  shared.heaps.push_back({.heap = h1, .free = 20 * KB});
  shared.heaps.push_back({.heap = h2});
  const auto chosen = dafg::select_heaps_to_defragment(shared, 128 * KB, false, settings);
  REQUIRE(chosen.size() == 3);
  CHECK(eastl::find(chosen.begin(), chosen.end(), h2) != chosen.end());
}

TEST_CASE("a preserved rect locks no waste when a resize keeps the contents", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  const auto pinned = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);
  CHECK(pinned.free == 0);
  CHECK(pinned.locked == 63 * KB);
  CHECK(pinned.preservedBytes == KB);

  const auto kept = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING, true);
  CHECK(kept.free == 63 * KB);
  CHECK(kept.locked == 0);
  CHECK(kept.preservedBytes == 0);
}

TEST_CASE("a rect alive across the frame boundary counts from the start of the measure", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {
    make_rect(0, 0, 16 * KB, make_lifetime(1, 8, 0, 2)), make_rect(1, 16 * KB, 16 * KB, live_within(0, 0, 2))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING, true);
  CHECK(waste.free == 32 * KB);
  CHECK(waste.locked == 0);
  REQUIRE(waste.heaps.size() == 1);
  CHECK(waste.heaps[0].free == 32 * KB);
}

TEST_CASE("a rect released at the moment another is first used shares its memory in the measure", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, 16 * KB, live_within(0, 0, 4)), make_rect(1, 0, 16 * KB, live_within(0, 4, 8))};
  add_heap(layout, TRACKED, 64 * KB, rects);

  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);
  CHECK(waste.free == 48 * KB);
  REQUIRE(waste.heaps.size() == 1);
  CHECK(waste.heaps[0].free == 48 * KB);
  CHECK(waste.heaps[0].content == 16 * KB);
}

TEST_CASE("a class whose waste is under the minimum is not defragmented", "[heapPolicy]")
{
  auto settings = make_waste_settings();
  settings.defragmentDelayFrames = 0;
  settings.defragmentCheckIntervalFrames = 0;
  const uint64_t reserved = 64 * KB;
  const dafg::ClassWaste waste{
    .free = 20 * KB,
  };

  dafg::DefragmentationState loose;
  CHECK(dafg::decide_defragmentation(loose, reserved, waste, settings, 0).defragment);

  settings.defragmentMinWaste = 32 * KB;
  dafg::DefragmentationState strict;
  CHECK_FALSE(dafg::decide_defragmentation(strict, reserved, waste, settings, 0).defragment);
}

TEST_CASE("a class with its waste at the floor is wasteful and names its heaps", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  auto settings = make_waste_settings();
  settings.defragmentWasteFractionStop = 0.25f;
  const auto h0 = static_cast<dafg::HeapIndex>(0);

  dafg::ClassWaste atFloor{
    .free = 16 * KB,
  };
  atFloor.heaps.push_back({
    .heap = h0,
    .free = 16 * KB,
  });
  CHECK(dafg::is_class_wasteful(dafg::DefragmentationState{}, 64 * KB, atFloor, false, settings, true));
  CHECK(dafg::select_heaps_to_defragment(atFloor, 64 * KB, false, settings).size() == 1);

  dafg::ClassWaste belowFloor{
    .free = 16 * KB - 1,
  };
  belowFloor.heaps.push_back({
    .heap = h0,
    .free = 16 * KB - 1,
  });
  CHECK(dafg::select_heaps_to_defragment(belowFloor, 64 * KB, false, settings).empty());

  settings.defragmentMinWaste = 0;
  settings.defragmentWasteFractionStop = 0.f;
  dafg::ClassWaste nothing;
  nothing.heaps.push_back({
    .heap = h0,
  });
  CHECK(dafg::select_heaps_to_defragment(nothing, 64 * KB, false, settings).empty());
}

TEST_CASE("a defragmentation creates the heaps it packs anew only when the memory budget holds them", "[heapPolicy]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect plain[] = {make_rect(0, 0, 16 * KB, live_within(0, 0, 2))};
  const dafg::LayoutRect preserved[] = {make_rect(1, 0, 8 * KB, live_across_frames(8, 2))};
  const auto plainHeap = add_heap(layout, TRACKED, 64 * KB, plain);
  const auto pinnedHeap = add_heap(layout, TRACKED, 64 * KB, preserved);

  const auto waste = dafg::measure_recoverable_waste(layout, TRACKED, PRESERVING);
  REQUIRE(waste.heaps.size() == 2);
  REQUIRE(waste.heaps[0].heap == plainHeap);
  CHECK(waste.heaps[0].content == 16 * KB);
  CHECK_FALSE(waste.heaps[0].pinned);
  CHECK(waste.heaps[0].free == 48 * KB);
  CHECK(waste.heaps[0].locked == 0);
  CHECK(waste.heaps[1].content == 8 * KB);
  CHECK(waste.heaps[1].pinned);
  CHECK(waste.heaps[1].free == 0);
  CHECK(waste.heaps[1].locked == 56 * KB);
  CHECK(waste.free == 48 * KB);
  CHECK(waste.locked == 64 * KB);
  const auto chosen = dafg::select_heaps_to_defragment(waste, 128 * KB, true, make_waste_settings());
  REQUIRE(chosen.size() == 2);
  CHECK(chosen[0] == pinnedHeap);

  const dafg::HeapIndex both[] = {plainHeap, pinnedHeap};
  CHECK(dafg::measure_defragmentation_burst(waste, both, false) == 16 * KB);
  CHECK(dafg::measure_defragmentation_burst(waste, both, true) == 24 * KB);
  const dafg::HeapIndex onlyPinned[] = {pinnedHeap};
  CHECK(dafg::measure_defragmentation_burst(waste, onlyPinned, false) == 0);

  const dafg::HeapPolicySettings settings{
    .defragmentHeadroom = 0.5f,
  };
  CHECK(dafg::is_burst_affordable(16 * KB, 24 * KB, settings));
  CHECK_FALSE(dafg::is_burst_affordable(16 * KB, 24 * KB - 1, settings));
  CHECK_FALSE(dafg::is_burst_affordable(16 * KB, 8 * KB, settings));
  CHECK(dafg::is_burst_affordable(0, 0, settings));
  CHECK(dafg::is_burst_affordable(16 * KB, eastl::numeric_limits<uint64_t>::max(), settings));
}

TEST_CASE("the small policy rules hold at their boundaries", "[heapPolicy]")
{
  const dafg::HeapPolicySettings settings{
    .defragmentMaxWaitFrames = 100,
    .defragmentPlacedAnewFraction = 0.25f,
  };

  CHECK(dafg::should_defragment_on_placed_anew(3, 1, settings));
  CHECK_FALSE(dafg::should_defragment_on_placed_anew(4, 1, settings));
  CHECK_FALSE(dafg::should_defragment_on_placed_anew(0, 0, settings));

  CHECK(dafg::should_release_empty_class(true, 0));
  CHECK_FALSE(dafg::should_release_empty_class(true, KB));
  CHECK_FALSE(dafg::should_release_empty_class(false, 0));

  dafg::ClassWaste waste{.free = 10 * KB, .locked = 10 * KB};
  dafg::reserve_for_ghosts(waste, 15 * KB);
  CHECK(waste.free == 0);
  CHECK(waste.locked == 5 * KB);
  dafg::reserve_for_ghosts(waste, 25 * KB);
  CHECK(waste.locked == 0);

  dafg::DefragmentationState state;
  dafg::arm_defragmentation(state, {}, 100);
  CHECK(state.pendingSince == -1);
  CHECK_FALSE(dafg::is_pending_overdue(state, settings, 200));
  dafg::arm_defragmentation(state,
    {
      .defragment = true,
    },
    100);
  CHECK_FALSE(dafg::is_pending_overdue(state, settings, 199));
  CHECK(dafg::is_pending_overdue(state, settings, 200));
  CHECK(dafg::disarm_defragmentation(state).defragment);
  CHECK(state.pendingSince == -1);
  CHECK_FALSE(dafg::is_pending_overdue(state, settings, 200));
}

TEST_CASE("the waste of a class is measured only once per check interval", "[heapPolicy]")
{
  const auto settings = make_waste_settings();
  const uint64_t reserved = 64 * KB;
  const dafg::ClassWaste high{.free = 32 * KB};
  const dafg::ClassWaste low{.free = 0};

  dafg::DefragmentationState state;
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, high, settings, 0).defragment);
  REQUIRE(state.wastefulSinceFrame == 0);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, low, settings, 30).defragment);
  CHECK(state.wastefulSinceFrame == 0);
  CHECK(state.lastCheckFrame == 0);
  CHECK_FALSE(dafg::decide_defragmentation(state, reserved, low, settings, 60).defragment);
  CHECK(state.wastefulSinceFrame < 0);
}
