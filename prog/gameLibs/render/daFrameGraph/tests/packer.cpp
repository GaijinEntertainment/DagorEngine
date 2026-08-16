// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "common.h"
#include <catch2/generators/catch_generators.hpp>
#include <util/dag_string.h>
#include <dag/dag_vectorSet.h>
#include <EASTL/numeric.h>


TEST_CASE("Empty input")
{
  TestState state = init_test();
  state.input.timelineSize = 10;
  state.input.maxHeapSize = UINT64_MAX;
  pack(state);

  CHECK(state.output.offsets.empty());
  CHECK(state.output.heapSize == 0);
}

TEST_CASE("Zero size input")
{
  TestState state = init_test();

  state.resources.assign(10, {0, 0, 0, 1, dafg::PackerInput::NO_PIN});

  state.input.timelineSize = 10;
  state.input.maxHeapSize = UINT64_MAX;

  pack(state);

  CHECK(state.output.heapSize == 0);
}

TEST_CASE("Test heap limit")
{
  TestState state = init_test();

  state.resources.assign(10, {0, 0, 1, 1, dafg::PackerInput::NO_PIN});
  state.input.timelineSize = 10;
  state.input.maxHeapSize = 7;

  pack(state);

  CHECK(state.output.heapSize <= 7);
}

template <class T>
T abs(T x)
{
  return x < 0 ? -x : x;
}

TEST_CASE("Wraparound strips")
{

  static constexpr size_t ALIGNMENT = 8;

  TestState state = init_test();

  size_t size = GENERATE(1, 10, 100, 1000);

  size_t expectedTotalSize = 0;
  for (uint32_t i = 0; i < size; ++i)
  {
    uint32_t tear = dagor_random::rnd_int(0, size - 1);
    uint32_t resSize = dagor_random::rnd_int(4096, 8192);
    state.resources.push_back({tear, tear, resSize, ALIGNMENT, dafg::PackerInput::NO_PIN});
    expectedTotalSize += resSize;
  }

  state.input.timelineSize = size;
  state.input.maxHeapSize = UINT64_MAX;

  pack(state);

  CHECK(state.output.heapSize >= expectedTotalSize);
  CHECK(state.output.heapSize <= expectedTotalSize + ALIGNMENT * state.output.offsets.size());
}

TEST_CASE("Randomized")
{

  TestState state = init_test();

  static constexpr uint32_t ALIGNMENT = 8;

  uint64_t totalSizeLowerBound = 0;
  uint64_t totalSizeUpperBound = 0;

  uint32_t wraparoundTotalSize = 0;

  uint32_t timelineSize = GENERATE(10, 100, 1000);
  dag::Vector<int> balance(timelineSize, 0);

  uint32_t resourceCount = GENERATE(10, 50, 100, 500, 1000);

  for (uint32_t i = 0; i < resourceCount; ++i)
  {
    uint32_t start = dagor_random::rnd_int(0, timelineSize - 1);
    uint32_t end = dagor_random::rnd_int(0, timelineSize - 1);
    uint32_t size = dagor_random::rnd_int(4096, 8192);

    state.resources.push_back({start, end, size, ALIGNMENT, dafg::PackerInput::NO_PIN});
    if (start >= end)
      wraparoundTotalSize += size;

    balance[start] += size;
    balance[end] -= size;

    totalSizeUpperBound += size + ALIGNMENT;
  }

  eastl::partial_sum(balance.begin(), balance.end(), balance.begin());

  for (auto &x : balance)
    x += wraparoundTotalSize;

  totalSizeLowerBound = *eastl::max_element(balance.begin(), balance.end());

  state.input.timelineSize = timelineSize;
  const bool doLimit = GENERATE(true, false);
  if (doLimit)
  {
    state.input.maxHeapSize = resourceCount * dagor_random::rnd_int(1024, 4096);
  }
  else
  {
    state.input.maxHeapSize = UINT64_MAX;
  }

  pack(state);

  if (!doLimit)
  {
    CHECK(state.output.heapSize >= totalSizeLowerBound);
    CHECK(state.output.heapSize <= totalSizeUpperBound);
  }
}

TEST_CASE("One pinned")
{
  TestState state = init_test();

  state.input.timelineSize = 300;
  state.input.maxHeapSize = 0x4000000;

  state.resources = {{281, 100, 0x2000000, 1, 0}};

  pack(state);

  CHECK(state.output.heapSize == 0x2000000);
}

TEST_CASE("Pinned and regular alias")
{
  TestState state = init_test();

  state.input.timelineSize = 300;
  state.input.maxHeapSize = 0x4000000;

  state.resources = {
    {100, 200, 0x2000000, 8, dafg::PackerInput::NO_PIN},
    {200, 100, 0x2000000, 8, 0},
  };

  pack(state);

  CHECK(state.output.heapSize == 0x2000000);
}

TEST_CASE("Two pinned")
{
  TestState state = init_test();

  state.input.timelineSize = 300;
  state.input.maxHeapSize = 0x4000000;

  state.resources = {
    {281, 89, 0X1FE0000, 8, 0},
    {281, 89, 0X1FE0000, 8, 0X1FE0000},
  };

  pack(state);
}

TEST_CASE("Alias with two pinned and gap")
{
  TestState state = init_test();

  state.input.timelineSize = 10;
  state.input.maxHeapSize = 0x4000000;

  state.resources = {
    {3, 7, 3000, 8, dafg::PackerInput::NO_PIN},
    {9, 3, 1000, 8, 0},
    {9, 0, 1000, 8, 2000},
  };

  pack(state);

  CHECK(state.output.heapSize == 3000);
}

TEST_CASE("Randomized recompilation with pinning")
{
  TestState state = init_test();

  static constexpr uint32_t ALIGNMENT = 8;

  {

    uint32_t timelineSize = GENERATE(10, 100, 1000);
    uint32_t resourceCount = GENERATE(10, 100, 1000);
    for (uint32_t i = 0; i < resourceCount; ++i)
    {
      uint32_t start = dagor_random::rnd_int(0, timelineSize - 1);
      uint32_t end = dagor_random::rnd_int(0, timelineSize - 1);
      uint32_t size = dagor_random::rnd_int(4096, 8192);
      state.resources.push_back({start, end, size, ALIGNMENT, dafg::PackerInput::NO_PIN});
    }

    state.input.timelineSize = timelineSize;
    state.input.maxHeapSize = UINT64_MAX;

    pack(state);
  }

  const auto prevHeapSize = state.output.heapSize;

  // Pin all wraparounds
  for (int i = 0; i < state.resources.size(); ++i)
  {
    auto &res = state.resources[i];
    if (res.end <= res.start)
      res.pin = state.output.offsets[i];
  }

  pack(state);

  CHECK(prevHeapSize == state.output.heapSize);
}

// Optional resources share the heap with the mandatory ones, so they have to
// be checked against both groups.
static void validate_optional(const TestState &state)
{
  REQUIRE(state.output.optionalOffsets.size() == state.input.optionalResources.size());

  for (uint32_t i = 0; i < state.input.optionalResources.size(); ++i)
  {
    const auto &res = state.input.optionalResources[i];
    const auto offset = state.output.optionalOffsets[i];

    if (res.size == 0)
    {
      CHECK(offset == dafg::PackerOutput::NOT_ALLOCATED);
      continue;
    }

    // Leaving an optional resource unplaced is always a valid outcome
    if (!isOffsetValid(offset))
      continue;

    CHECK(offset % res.align == 0);
    CHECK(offset + res.size <= state.output.heapSize);

    const auto checkNoOverlap = [&](const dafg::PackerInput::Resource &other, uint64_t otherOffset) {
      if (!isOffsetValid(otherOffset) || segmentsDisjoint({res.start, res.end}, {other.start, other.end}))
        return;

      CHECK((offset + res.size <= otherOffset || otherOffset + other.size <= offset));
    };

    for (uint32_t j = 0; j < state.input.resources.size(); ++j)
      checkNoOverlap(state.input.resources[j], state.output.offsets[j]);
    for (uint32_t j = i + 1; j < state.input.optionalResources.size(); ++j)
      checkNoOverlap(state.input.optionalResources[j], state.output.optionalOffsets[j]);
  }
}

TEST_CASE("Optional fills a gap")
{
  TestState state = init_test();

  state.input.timelineSize = 10;
  state.input.maxHeapSize = UINT64_MAX;

  // Both are alive at the start of the timeline and hence cannot alias, so
  // the shorter one leaves a hole for the rest of the timeline.
  state.resources = {
    {0, 10, 1000, 1, dafg::PackerInput::NO_PIN},
    {0, 5, 1000, 1, dafg::PackerInput::NO_PIN},
  };

  dag::Vector<dafg::PackerInput::Resource> optional = {{5, 10, 1000, 1, dafg::PackerInput::NO_PIN}};
  state.input.optionalResources = optional;

  pack(state);
  validate_optional(state);

  CHECK(isOffsetValid(state.output.optionalOffsets[0]));
  // The optional resource went into the hole instead of growing the heap
  CHECK(state.output.heapSize == 2000);
}

TEST_CASE("Optional does not fit")
{
  TestState state = init_test();

  state.input.timelineSize = 10;
  state.input.maxHeapSize = 1000;

  state.resources = {{0, 10, 1000, 1, dafg::PackerInput::NO_PIN}};

  dag::Vector<dafg::PackerInput::Resource> optional = {{0, 10, 1000, 1, dafg::PackerInput::NO_PIN}};
  state.input.optionalResources = optional;

  pack(state);
  validate_optional(state);

  CHECK(state.output.optionalOffsets[0] == dafg::PackerOutput::NOT_SCHEDULED);
  // Mandatory resources are not disturbed by an optional one that did not fit
  CHECK(state.output.offsets[0] == 0);
  CHECK(state.output.heapSize == 1000);
}

TEST_CASE("Zero size optional")
{
  TestState state = init_test();

  state.input.timelineSize = 10;
  state.input.maxHeapSize = UINT64_MAX;

  state.resources = {{0, 10, 1000, 1, dafg::PackerInput::NO_PIN}};

  dag::Vector<dafg::PackerInput::Resource> optional = {{0, 5, 0, 1, dafg::PackerInput::NO_PIN}};
  state.input.optionalResources = optional;

  pack(state);
  validate_optional(state);

  CHECK(state.output.optionalOffsets[0] == dafg::PackerOutput::NOT_ALLOCATED);
}

TEST_CASE("Randomized with optional")
{
  TestState state = init_test();

  static constexpr uint32_t ALIGNMENT = 8;

  uint32_t timelineSize = GENERATE(10, 100);
  uint32_t resourceCount = GENERATE(10, 100, 500);

  for (uint32_t i = 0; i < resourceCount; ++i)
  {
    uint32_t start = dagor_random::rnd_int(0, timelineSize - 1);
    uint32_t end = dagor_random::rnd_int(0, timelineSize - 1);
    uint32_t size = dagor_random::rnd_int(4096, 8192);
    state.resources.push_back({start, end, size, ALIGNMENT, dafg::PackerInput::NO_PIN});
  }

  dag::Vector<dafg::PackerInput::Resource> optional;
  for (uint32_t i = 0; i < resourceCount; ++i)
  {
    // Optional resources are never allowed to wrap around
    uint32_t start = dagor_random::rnd_int(0, timelineSize - 2);
    uint32_t end = dagor_random::rnd_int(start + 1, timelineSize - 1);
    uint32_t size = dagor_random::rnd_int(4096, 8192);
    optional.push_back({start, end, size, ALIGNMENT, dafg::PackerInput::NO_PIN});
  }
  state.input.optionalResources = optional;

  state.input.timelineSize = timelineSize;
  state.input.maxHeapSize = resourceCount * dagor_random::rnd_int(1024, 4096);

  pack(state);
  validate_optional(state);
}
