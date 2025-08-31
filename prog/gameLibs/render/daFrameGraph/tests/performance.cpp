// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "common.h"
#include <util/dag_string.h>
#include <EASTL/vector_multiset.h>
#include <EASTL/numeric.h>
#include <EASTL/unordered_map.h>


// These statistics were collected from enlisted at the end of feb 2023
// Making them loadable from configs is too much work for how often
// this functionality is actually used (almost never)
// clang-format off
inline static const eastl::unordered_map<uint32_t, uint32_t> timelineSizeDistr
  {{130, 4}, {140, 8}, {144, 4}, {146, 4}};
inline static const eastl::unordered_map<uint32_t, uint32_t> lifetimeDistr
  {
    {0, 108}, {1, 120}, {2, 36}, {6, 6}, {7, 6}, {9, 4}, {13, 4},
    {28, 4}, {33, 12}, {35, 8}, {37, 4}, {38, 4}, {39, 8}, {41, 4},
    {42, 12}, {43, 4}, {44, 4}, {45, 4}, {55, 4}, {56, 8}, {60, 8},
    {62, 16}, {63, 4}, {66, 4}, {67, 16}, {69, 8}, {70, 8}, {71, 16},
    {73, 8}, {74, 8}, {79, 6}, {80, 8}, {82, 4}, {84, 4}, {85, 16},
    {89, 16}, {91, 12}, {92, 4}
  };
inline static const eastl::unordered_map<uint32_t, uint32_t> sizeDistr
  {
    {1, 28}, {4, 60}, {880, 20}, {12288, 20}, {131072, 12}, {524288, 20},
    {589824, 4}, {917504, 20}, {1245184, 40}, {2228224, 36}, {2490368, 82},
    {3538944, 96}, {4915200, 16}, {8847360, 76}
  };
// clang-format on

template <class T>
uint32_t generateFromDistr(const T &distr)
{
  dag::Vector<uint32_t> bounds;
  bounds.push_back(0);
  for (auto [k, v] : distr)
    bounds.push_back(bounds.back() + v);
  auto r = static_cast<uint32_t>(dagor_random::rnd_int(0, bounds.back() - 1));
  auto idx = eastl::upper_bound(bounds.begin(), bounds.end(), r) - bounds.begin() - 1;
  return eastl::next(distr.begin(), idx)->first;
}

static constexpr uint32_t ALIGNMENT = 8;

struct ProdTestState
{
  eastl::vector_multiset<float> ratios;
  uint32_t runs = 0;
};

static void test(ProdTestState &pst, uint32_t resCount, bool pack_wraparound)
{
  TestState state = init_test();

  uint32_t unpackedWraparoundTotal = 0;

  uint32_t timelineSize = generateFromDistr(timelineSizeDistr);
  uint32_t wraparoundTotalSize = 0;
  dag::Vector<int> balance(timelineSize, 0);
  for (uint32_t i = 0; i < resCount; ++i)
  {
    const uint32_t start = dagor_random::rnd_int(0, timelineSize - 1);
    const uint32_t end = (start + generateFromDistr(lifetimeDistr)) % timelineSize;
    const uint32_t size = generateFromDistr(sizeDistr);
    if (start < end || pack_wraparound)
      state.resources.push_back({start, end, size, ALIGNMENT, dafg::PackerInput::NO_PIN});
    else
      unpackedWraparoundTotal += size;

    if (start >= end)
      wraparoundTotalSize += size;

    balance[start] += size;
    balance[end] -= size;
  }

  eastl::partial_sum(balance.begin(), balance.end(), balance.begin());

  for (auto &x : balance)
    x += wraparoundTotalSize;

  uint32_t load = *eastl::max_element(balance.begin(), balance.end());

  state.input.timelineSize = timelineSize;
  state.input.maxHeapSize = UINT64_MAX;

  pack(state);

  const auto opt = state.output.heapSize + unpackedWraparoundTotal;
  const auto ratio = static_cast<float>(opt) / static_cast<float>(load);
  pst.ratios.insert(ratio);
  ++pst.runs;
}

TEST_CASE("Production-like input")
{
  printf("OPT/LOAD performance tests\n");
  printf("Count, avg, p90, no hist avg, no hist p90\n");

  constexpr uint32_t AVG_OVER = 100;
  ProdTestState pst;
  pst.ratios.reserve(AVG_OVER);

  for (uint32_t count = 25; count <= 200; count += 25)
  {
    for (uint32_t i = 0; i < AVG_OVER; ++i)
      test(pst, count, true);

    const float avg = eastl::accumulate(pst.ratios.begin(), pst.ratios.end(), 0.f) / pst.runs;
    const float p90 = pst.ratios[static_cast<uint32_t>(static_cast<float>(pst.ratios.size()) * 0.9f)];
    pst.ratios.clear();
    pst.runs = 0;

    for (uint32_t i = 0; i < AVG_OVER; ++i)
      test(pst, count, false);

    const float badavg = eastl::accumulate(pst.ratios.begin(), pst.ratios.end(), 0.f) / pst.runs;
    const float badp90 = pst.ratios[static_cast<uint32_t>(static_cast<float>(pst.ratios.size()) * 0.9f)];
    pst.ratios.clear();
    pst.runs = 0;

    printf("%d, %f, %f, %f, %f\n", count, avg, p90, badavg, badp90);
  }
}
