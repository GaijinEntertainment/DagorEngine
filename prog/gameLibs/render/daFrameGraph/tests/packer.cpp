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
