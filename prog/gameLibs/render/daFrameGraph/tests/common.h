// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <backend/resourceScheduling/packer.h>
#include <math/random/dag_random.h>
#include <debug/dag_assert.h>
#include <dag/dag_vector.h>

struct Segment
{
  uint32_t s, e;
};

// Non-wrapping with non-wrapping case
static bool segmentsDisjointNWNW(Segment a, Segment b) { return a.e <= b.s || b.e <= a.s; }

// Wrapping with non-wrapping case
static bool segmentsDisjointWNW(Segment a, Segment b) { return a.e <= b.s && b.e <= a.s; }

static bool isWrapping(Segment s) { return s.s >= s.e; }

static bool segmentsDisjoint(Segment a, Segment b)
{
  if (isWrapping(b))
    eastl::swap(a, b);

  if (isWrapping(b))
    return false;

  if (isWrapping(a))
    return segmentsDisjointWNW(a, b);
  else
    return segmentsDisjointNWNW(a, b);
}

static bool isOffsetValid(uint64_t offset)
{
  if (offset == dafg::PackerOutput::NOT_ALLOCATED)
    return false;

  if (offset == dafg::PackerOutput::NOT_SCHEDULED)
    return false;

  return true;
}

struct TestState
{
  dag::Vector<dafg::PackerInput::Resource> resources;
  dafg::PackerInput input{};
  dafg::PackerOutput output{};
  dafg::Packer packer;
};

inline TestState init_test()
{
  dagor_random::set_rnd_seed(Catch::getSeed());
  return TestState{{}, {}, {}, dafg::make_greedy_scanline_packer()};
}

inline void validate_test(const TestState &state)
{
  CHECK(state.input.resources.size() == state.output.offsets.size());
  CHECK(state.output.heapSize <= state.input.maxHeapSize);

  for (uint32_t i = 0; i < state.input.resources.size(); ++i)
  {
    auto offset = state.output.offsets[i];
    auto resSizeWithPadding = state.input.resources[i].sizeWithPadding(offset);
    if (state.input.resources[i].size == 0)
    {
      CHECK(offset == dafg::PackerOutput::NOT_ALLOCATED);
    }
    else
    {
      CHECK(offset != dafg::PackerOutput::NOT_ALLOCATED);
      if (isOffsetValid(offset))
      {
        CHECK(offset % state.input.resources[i].align == 0);
        CHECK(offset <= state.output.heapSize - state.input.resources[i].size);
      }
    }

    if (offset == dafg::PackerOutput::NOT_SCHEDULED)
      CHECK((resSizeWithPadding > state.input.maxHeapSize || state.output.heapSize > state.input.maxHeapSize - resSizeWithPadding));

    if (state.input.resources[i].pin != dafg::PackerInput::NO_PIN)
      CHECK(offset == state.input.resources[i].pin);
  }

  for (uint32_t i = 0; i < state.output.offsets.size(); ++i)
  {
    if (!isOffsetValid(state.output.offsets[i]))
      continue;

    for (uint32_t j = i + 1; j < state.output.offsets.size(); ++j)
    {
      if (!isOffsetValid(state.output.offsets[j]))
        continue;

      auto &fst = state.input.resources[i];
      auto &snd = state.input.resources[j];

      if (!segmentsDisjoint({fst.start, fst.end}, {snd.start, snd.end}))
      {
        const bool noMemoryOverlap = state.output.offsets[i] + fst.size <= state.output.offsets[j] ||
                                     state.output.offsets[j] + snd.size <= state.output.offsets[i];

        CHECK(noMemoryOverlap);
      }
    }
  }
}

inline void pack(TestState &state)
{
  state.input.resources = state.resources;
  state.output = state.packer(state.input);
  validate_test(state);
}
