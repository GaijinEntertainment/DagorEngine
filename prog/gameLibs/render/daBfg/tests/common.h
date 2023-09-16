#pragma once

#include <UnitTest++/UnitTestPP.h>
#include <resourceScheduling/packer.h>
#include <math/random/dag_random.h>
#include <debug/dag_assert.h>
#include <dag/dag_vector.h>


struct PackerFixture
{
  PackerFixture() { dagor_random::set_rnd_seed(42); }

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
    if (offset == dabfg::PackerOutput::NOT_ALLOCATED)
      return false;

    if (offset == dabfg::PackerOutput::NOT_SCHEDULED)
      return false;

    return true;
  }

  void validateOutput() const
  {
    CHECK_EQUAL(input.resources.size(), output.offsets.size());
    CHECK(output.heapSize <= input.maxHeapSize);

    for (uint32_t i = 0; i < input.resources.size(); ++i)
    {
      auto offset = output.offsets[i];
      auto offsetHint = input.resources[i].offsetHint;
      auto resSizeWithPadding = input.resources[i].sizeWithPadding(offset);
      if (input.resources[i].size == 0)
      {
        CHECK(offset == dabfg::PackerOutput::NOT_ALLOCATED);
      }
      else
      {
        CHECK(offset != dabfg::PackerOutput::NOT_ALLOCATED);
        if (isOffsetValid(offset))
        {
          CHECK(offset % input.resources[i].align == 0);
          CHECK(offset <= output.heapSize - input.resources[i].size);
          if (offsetHint != dabfg::PackerInput::NO_HINT)
          {
            CHECK(offset == offsetHint);
          }
        }
      }

      if (offset == dabfg::PackerOutput::NOT_SCHEDULED)
      {
        CHECK(resSizeWithPadding > input.maxHeapSize || output.heapSize > input.maxHeapSize - resSizeWithPadding);
      }
    }

    for (uint32_t i = 0; i < output.offsets.size(); ++i)
    {
      if (!isOffsetValid(output.offsets[i]))
        continue;

      for (uint32_t j = i + 1; j < output.offsets.size(); ++j)
      {
        if (!isOffsetValid(output.offsets[j]))
          continue;

        auto &fst = input.resources[i];
        auto &snd = input.resources[j];

        if (!segmentsDisjoint({fst.start, fst.end}, {snd.start, snd.end}))
        {
          bool noMemoryOverlap =
            output.offsets[i] + fst.size <= output.offsets[j] || output.offsets[j] + snd.size <= output.offsets[i];

          CHECK(noMemoryOverlap);
          if (!noMemoryOverlap)
          {
            // Getting here almost always means a debug session :)
            G_DEBUG_BREAK;
          }
        }
      }
    }
  }

  void pack()
  {
    output = packer(input);
    validateOutput();
  }

  dag::Vector<dabfg::PackerInput::Resource> resources;
  dabfg::PackerInput input{};
  dabfg::PackerOutput output{};
  dabfg::Packer packer = dabfg::make_greedy_scanline_packer();
};
