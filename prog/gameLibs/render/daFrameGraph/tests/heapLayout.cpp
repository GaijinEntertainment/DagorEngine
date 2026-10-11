// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>

#include <catch2/catch_test_macros.hpp>

#include "layoutTestHelpers.h"


namespace
{
using namespace layout_test;
} // namespace


TEST_CASE("lifetimes are arcs of the frame ring", "[heapLayout]")
{
  CHECK_FALSE(TIMELINE.overlap(live_within(0, 2, 5), live_within(0, 5, 8)));
  CHECK(TIMELINE.overlap(live_within(0, 2, 6), live_within(0, 5, 8)));
  CHECK_FALSE(TIMELINE.overlap(live_within(0, 2, 5), live_within(1, 2, 5)));

  const auto wrapping = make_lifetime(1, 8, 0, 2);
  CHECK(TIMELINE.overlap(wrapping, live_within(0, 0, 1)));
  CHECK_FALSE(TIMELINE.overlap(wrapping, live_within(0, 4, 6)));
  CHECK(TIMELINE.overlap(wrapping, wrapping));

  CHECK(TIMELINE.duration(live_within(0, 2, 5)) == 3);
  CHECK(TIMELINE.duration(wrapping) == 4);
}

TEST_CASE("the layout tracks the heap and offset of every frame resource and the frame resources that span into the next frame",
  "[heapLayout]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  const dafg::LayoutRect rects[] = {make_rect(0, 0, KB, live_across_frames(8, 2)), make_rect(1, KB, KB, live_within(0, 0, 4))};
  const auto heapIdx = add_heap(layout, TRACKED, 4 * KB, rects);

  CHECK(layout.where[make_frame_resource(0)].heap == heapIdx);
  CHECK(layout.where[make_frame_resource(1)].heap == heapIdx);
  CHECK(layout.where[make_frame_resource(1)].offset == KB);
  CHECK(layout.where[make_frame_resource(2)].heap == dafg::HeapIndex::Invalid);
  CHECK(layout.find(make_frame_resource(1))->offset == KB);
  CHECK(get_heap_at(layout, 0).spanning[0] == 1);
  CHECK(get_heap_at(layout, 0).spanning[1] == 0);
  CHECK(layout.spanning[0].test(static_cast<dafg::intermediate::ResourceIndex>(0), false));
  CHECK_FALSE(layout.spanning[0].test(static_cast<dafg::intermediate::ResourceIndex>(1), false));
  CHECK(dafg::is_layout_valid(layout, TIMELINE));

  const auto taken = layout.take(make_frame_resource(0));
  CHECK(taken.offset == 0);
  CHECK(layout.where[make_frame_resource(0)].heap == dafg::HeapIndex::Invalid);
  CHECK(get_heap_at(layout, 0).spanning[0] == 0);
  CHECK_FALSE(layout.spanning[0].test(static_cast<dafg::intermediate::ResourceIndex>(0), false));

  layout.setLifetime(make_frame_resource(1), live_across_frames(0, 4));
  CHECK(get_heap_at(layout, 0).spanning[0] == 1);
  CHECK(layout.spanning[0].test(static_cast<dafg::intermediate::ResourceIndex>(1), false));
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}

TEST_CASE("a heap slot without rects and memory goes to the next class that opens a heap", "[heapLayout]")
{
  FRAMEMEM_REGION;
  dafg::Layout layout;
  layout.reserveResources(RESOURCE_COUNT);
  const auto first = layout.addHeap(TRACKED);
  const auto reused = layout.addHeap(TRACKED_2);

  CHECK(reused == first);
  CHECK(layout.heaps.size() == 1);
  CHECK(layout.heapsOf(TRACKED).empty());
  REQUIRE(layout.heapsOf(TRACKED_2).size() == 1);
  CHECK(layout.heapsOf(TRACKED_2)[0] == reused);

  const dafg::HeapIndex taken[] = {reused};
  const auto fresh = layout.addHeap(TRACKED, taken);
  CHECK(fresh != reused);
  CHECK(layout.heaps.size() == 2);
  CHECK(dafg::is_layout_valid(layout, TIMELINE));
}
