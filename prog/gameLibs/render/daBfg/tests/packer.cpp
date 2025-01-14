// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "common.h"
#include <util/dag_string.h>
#include <dag/dag_vectorSet.h>
#include <EASTL/numeric.h>


TEST_FIXTURE(PackerFixture, TestEmptyInput)
{
  input.timelineSize = 10;
  input.maxHeapSize = 0xFFFFFFFF;
  pack();

  CHECK(output.offsets.empty());
  CHECK_EQUAL(0, output.heapSize);
}

TEST_FIXTURE(PackerFixture, TestZeroSizeInput)
{
  resources.assign(10, {0, 0, 0, 1, dabfg::PackerInput::NO_HINT});

  input.timelineSize = 10;
  input.resources = resources;
  input.maxHeapSize = 0xFFFFFFFF;

  pack();

  CHECK_EQUAL(0, output.heapSize);
}

TEST_FIXTURE(PackerFixture, TestHeapLimitInput)
{
  resources.assign(10, {0, 0, 1, 1, dabfg::PackerInput::NO_HINT});
  input.timelineSize = 10;
  input.resources = resources;
  input.maxHeapSize = 7;
  pack();
  validateOutput();
}

struct WraparoundStripTestsFixture : PackerFixture
{
  uint32_t testSize = 0;
  uint32_t expectedTotalSize = 0;

  static constexpr uint32_t ALIGNMENT = 8;

  void generate(uint32_t size)
  {
    testSize = size;
    for (uint32_t i = 0; i < testSize; ++i)
    {
      uint32_t tear = dagor_random::rnd_int(0, testSize - 1);
      uint32_t resSize = dagor_random::rnd_int(4096, 8192);
      uint32_t offsetHint = dabfg::PackerInput::NO_HINT;
      resources.push_back({tear, tear, resSize, ALIGNMENT, offsetHint});
      expectedTotalSize += resSize;
    }

    input.timelineSize = testSize;
    input.resources = resources;
    input.maxHeapSize = 0xFFFFFFFF;
  }

  void check() { CHECK_CLOSE(expectedTotalSize, output.heapSize, ALIGNMENT * output.offsets.size()); }
};

#define STRIPS_TEST(x)                                               \
  TEST_FIXTURE(WraparoundStripTestsFixture, TestWraparoundStrips##x) \
  {                                                                  \
    generate((x));                                                   \
    pack();                                                          \
    check();                                                         \
  }

STRIPS_TEST(1);
STRIPS_TEST(10);
STRIPS_TEST(100);
STRIPS_TEST(1000);

#undef STRIPS_TEST


struct RandomTestsFixture : PackerFixture
{
  uint32_t timelineSize = 0;
  uint32_t totalSizeLowerBound = 0;
  uint32_t totalSizeUpperBound = 0;

  static constexpr uint32_t ALIGNMENT = 8;

  // How bad the answer is allowed to be compared to strict lower bound
  static constexpr float MAX_QUALITY_BADNESS = 0.5f;

  void generate(uint32_t time, uint32_t resCount, float pinnedFrac = 0.0)
  {
    timelineSize = time;
    uint32_t wraparoundTotalSize = 0;
    dag::Vector<int> balance(timelineSize, 0);
    uint32_t freeOffsetHint = 0;
    const uint32_t invalidTimePoint = static_cast<uint32_t>(-1);
    Segment prevHintedSegment = {invalidTimePoint, invalidTimePoint};
    uint32_t prevHintedSize = 0;
    for (uint32_t i = 0; i < resCount; ++i)
    {
      uint32_t start = dagor_random::rnd_int(0, timelineSize - 1);
      uint32_t end = dagor_random::rnd_int(0, timelineSize - 1);
      uint32_t size = dagor_random::rnd_int(4096, 8192);
      uint32_t offsetHint = dabfg::PackerInput::NO_HINT;
      if (dagor_random::rnd_float(0.0, 1.0) < pinnedFrac)
      {
        offsetHint = ((freeOffsetHint + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;
        if (prevHintedSegment.s != invalidTimePoint && !segmentsDisjoint(Segment({start, end}), prevHintedSegment))
        {
          freeOffsetHint += prevHintedSize + ALIGNMENT;
          offsetHint = ((freeOffsetHint + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;
          prevHintedSize = size;
          prevHintedSegment = {start, end};
        }
        prevHintedSize = eastl::max(size, prevHintedSize);
        prevHintedSegment = {start, prevHintedSegment.e == invalidTimePoint ? end : prevHintedSegment.e};
      }

      resources.push_back({start, end, size, ALIGNMENT, offsetHint});
      if (start >= end)
        wraparoundTotalSize += size;

      balance[start] += size;
      balance[end] -= size;

      totalSizeUpperBound += size;
    }

    eastl::partial_sum(balance.begin(), balance.end(), balance.begin());

    for (auto &x : balance)
      x += wraparoundTotalSize;

    totalSizeLowerBound = *eastl::max_element(balance.begin(), balance.end());

    input.timelineSize = timelineSize;
    input.resources = resources;
    input.maxHeapSize = 0xFFFFFFFF;
  }

  void check()
  {
    float badness =
      static_cast<float>(output.heapSize - totalSizeLowerBound) / static_cast<float>(totalSizeUpperBound - totalSizeLowerBound);
    CHECK_CLOSE(0, badness, MAX_QUALITY_BADNESS);
    printf("Badness on timeline size %u"
           " and resource count %" PRIuPTR ": %f\n",
      timelineSize, input.resources.size(), badness);
  }
};

#define RANDOM_TEST(t, r)                                \
  TEST_FIXTURE(RandomTestsFixture, TestRandomT##t##R##r) \
  {                                                      \
    generate((t), (r));                                  \
    pack();                                              \
    check();                                             \
  }


RANDOM_TEST(10, 10);
RANDOM_TEST(100, 100);
RANDOM_TEST(1000, 1000);

RANDOM_TEST(10, 50);
RANDOM_TEST(100, 500);
RANDOM_TEST(1000, 5000);


#undef RANDOM_TEST

struct RandomHeapLimitTestsFixture : RandomTestsFixture
{
  void generateWithHeapLimit(uint32_t time, uint32_t resCount)
  {
    generate(time, resCount);
    input.maxHeapSize = resCount * dagor_random::rnd_int(1024, 4096);
  }
};

#define HEAP_LIMIT_TEST(t, r)                                              \
  TEST_FIXTURE(RandomHeapLimitTestsFixture, TestRandomHeapLimitT##t##R##r) \
  {                                                                        \
    generateWithHeapLimit((t), (r));                                       \
    pack();                                                                \
  }

HEAP_LIMIT_TEST(10, 100);
HEAP_LIMIT_TEST(100, 1000);
HEAP_LIMIT_TEST(1000, 10000);

#undef HEAP_LIMIT_TEST

struct RandomOffsetHintTestsFixture : RandomTestsFixture
{
  void generateWithOffsetHints(uint32_t time, uint32_t resCount, float pinnedFrac)
  {
    generate(time, resCount, pinnedFrac);
    input.maxHeapSize = 0xFFFFFFFF;
  }
};

#define OFFSET_HINT_TEST(t, r, f)                                            \
  TEST_FIXTURE(RandomOffsetHintTestsFixture, TestRandomOffsetHintT##t##R##r) \
  {                                                                          \
    generateWithOffsetHints((t), (r), (f));                                  \
    pack();                                                                  \
  }

OFFSET_HINT_TEST(10, 100, 0.1);
OFFSET_HINT_TEST(100, 1000, 0.5);
OFFSET_HINT_TEST(1000, 10000, 0.8);

#undef OFFSET_HINT_TEST

TEST_FIXTURE(PackerFixture, TestPreservationEdgeCase)
{
  input.timelineSize = 300;
  input.maxHeapSize = 0x4000000;

  resources = {
    {100, 281, 0x2B60000, 0x10000, 0},
    {281, 89, 0X1FE0000, 0x10000, 0},
    {281, 89, 0X1FE0000, 0x10000, 0X1FE0000},
  };

  input.resources = resources;

  pack();
}
