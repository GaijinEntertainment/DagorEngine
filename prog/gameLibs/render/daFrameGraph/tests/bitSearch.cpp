// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <id/bitSearch.h>
#include <vecmath/dag_vecMath.h>
#include <dag/dag_vector.h>
#include <random>

// Helper: allocate a vec4f-aligned bit array with zeroed padding.
struct AlignedBits
{
  static constexpr uint32_t WORD_BITS = 32;
  static constexpr uint32_t WORDS_PER_VEC4 = sizeof(vec4f) / sizeof(uint32_t);

  uint32_t *data = nullptr;
  uint32_t bitCount = 0;
  uint32_t wordCount = 0;

  AlignedBits() = default;
  explicit AlignedBits(uint32_t bits, bool fill = false)
  {
    bitCount = bits;
    wordCount = ((bits + WORD_BITS - 1) / WORD_BITS + WORDS_PER_VEC4 - 1) & ~(WORDS_PER_VEC4 - 1);
    if (wordCount == 0)
      wordCount = WORDS_PER_VEC4; // minimum allocation
    data = static_cast<uint32_t *>(_mm_malloc(wordCount * sizeof(uint32_t), alignof(vec4f)));
    if (fill && bits > 0)
    {
      memset(data, 0xFF, wordCount * sizeof(uint32_t));
      // Clear padding
      uint32_t rem = bits % WORD_BITS;
      if (rem != 0)
        data[(bits - 1) / WORD_BITS] &= ~0u >> (WORD_BITS - rem);
      for (uint32_t i = (bits + WORD_BITS - 1) / WORD_BITS; i < wordCount; ++i)
        data[i] = 0;
    }
    else
    {
      memset(data, 0, wordCount * sizeof(uint32_t));
    }
  }

  ~AlignedBits()
  {
    if (data)
      _mm_free(data);
  }

  AlignedBits(const AlignedBits &) = delete;
  AlignedBits &operator=(const AlignedBits &) = delete;

  void set(uint32_t pos)
  {
    if (pos < bitCount)
      data[pos / WORD_BITS] |= 1u << (pos % WORD_BITS);
  }

  void clear(uint32_t pos)
  {
    if (pos < bitCount)
      data[pos / WORD_BITS] &= ~(1u << (pos % WORD_BITS));
  }
};

static dag::Vector<uint32_t> collect_forward_true(const AlignedBits &ab)
{
  dag::Vector<uint32_t> result;
  uint32_t w = 0;
  uint32_t pos = bit_search<true, true>(ab.data, ab.bitCount, 0, w);
  while (pos < ab.bitCount)
  {
    result.push_back(pos);
    pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
  }
  return result;
}

static dag::Vector<uint32_t> collect_forward_false(const AlignedBits &ab)
{
  dag::Vector<uint32_t> result;
  uint32_t w = 0;
  uint32_t pos = bit_search<false, true>(ab.data, ab.bitCount, 0, w);
  while (pos < ab.bitCount)
  {
    result.push_back(pos);
    pos = bit_search<false, true>(ab.data, ab.bitCount, pos + 1, w);
  }
  return result;
}


// ---- bit_search<true, true> forward tests ----

TEST_CASE("bit_search<true,true> forward: all-zero", "[bit_search]")
{
  AlignedBits ab(200);
  auto result = collect_forward_true(ab);
  REQUIRE(result.empty());
}

TEST_CASE("bit_search<true,true> forward: single bit at various positions", "[bit_search]")
{
  for (uint32_t pos : {0u, 31u, 32u, 63u, 64u, 127u, 128u, 199u})
  {
    CAPTURE(pos);
    AlignedBits ab(200);
    ab.set(pos);
    auto result = collect_forward_true(ab);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == pos);
  }
}

TEST_CASE("bit_search<true,true> forward: scattered bits", "[bit_search]")
{
  AlignedBits ab(200);
  dag::Vector<uint32_t> expected = {3, 15, 31, 32, 63, 64, 100, 127, 128, 190, 199};
  for (auto pos : expected)
    ab.set(pos);

  auto result = collect_forward_true(ab);
  REQUIRE(result.size() == expected.size());
  for (size_t i = 0; i < expected.size(); ++i)
    REQUIRE(result[i] == expected[i]);
}

TEST_CASE("bit_search<true,true> forward: all-true", "[bit_search]")
{
  constexpr uint32_t N = 200;
  AlignedBits ab(N, true);
  auto result = collect_forward_true(ab);
  REQUIRE(result.size() == N);
  for (uint32_t i = 0; i < N; ++i)
    REQUIRE(result[i] == i);
}

TEST_CASE("bit_search<true,true> forward: non-power-of-two sizes", "[bit_search]")
{
  for (uint32_t bitCount : {33u, 65u, 129u, 137u})
  {
    CAPTURE(bitCount);
    AlignedBits ab(bitCount);
    ab.set(0);
    ab.set(bitCount - 1);

    auto result = collect_forward_true(ab);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == 0);
    REQUIRE(result[1] == bitCount - 1);
  }
}

// ---- bit_search<true, false> backward tests ----

TEST_CASE("bit_search<true,false> backward: small test", "[bit_search]")
{
  AlignedBits ab(4);
  ab.set(0);

  uint32_t w = 0;
  uint32_t pos = bit_search<true, false>(ab.data, ab.bitCount, 3, w);
  REQUIRE(pos == 0);
  REQUIRE(w == 0x1);
}

TEST_CASE("bit_search<true,false> backward: finds correct bit", "[bit_search]")
{
  AlignedBits ab(200);
  ab.set(50);
  ab.set(100);
  ab.set(150);

  uint32_t w = 0;

  // Backward from 199 should find 150
  uint32_t pos = bit_search<true, false>(ab.data, ab.bitCount, 199, w);
  REQUIRE(pos == 150);

  // Backward from 120 should find 100 (independent search, must init cache)
  w = ab.data[120 / detail::WORD_BITS];
  pos = bit_search<true, false>(ab.data, ab.bitCount, 120, w);
  REQUIRE(pos == 100);

  // Backward from 49 should find nothing (independent search, must init cache)
  w = ab.data[49 / detail::WORD_BITS];
  pos = bit_search<true, false>(ab.data, ab.bitCount, 49, w);
  REQUIRE(pos == ab.bitCount);
}

TEST_CASE("bit_search<true,false> backward: sequential backward", "[bit_search]")
{
  AlignedBits ab(200);
  dag::Vector<uint32_t> positions = {10, 50, 100, 150, 190};
  for (auto p : positions)
    ab.set(p);

  uint32_t w = 0;
  uint32_t pos = bit_search<true, false>(ab.data, ab.bitCount, 199, w);
  REQUIRE(pos == 190);

  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 150);

  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 100);

  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 50);

  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 10);

  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == ab.bitCount);
}

// ---- bit_search<false, true> forward tests ----

TEST_CASE("bit_search<false,true> forward: all-true means no false bits", "[bit_search]")
{
  AlignedBits ab(200, true);
  auto result = collect_forward_false(ab);
  REQUIRE(result.empty());
}

TEST_CASE("bit_search<false,true> forward: all-zero means all positions are false", "[bit_search]")
{
  constexpr uint32_t N = 128;
  AlignedBits ab(N);
  auto result = collect_forward_false(ab);
  REQUIRE(result.size() == N);
  for (uint32_t i = 0; i < N; ++i)
    REQUIRE(result[i] == i);
}

TEST_CASE("bit_search<false,true> forward: respects size boundary", "[bit_search]")
{
  // Size 33: padding bits beyond 33 should NOT appear as false bits
  AlignedBits ab(33, true);
  ab.clear(10);
  ab.clear(32);

  auto result = collect_forward_false(ab);
  REQUIRE(result.size() == 2);
  REQUIRE(result[0] == 10);
  REQUIRE(result[1] == 32);
}

TEST_CASE("bit_search<false,true> forward: non-power-of-two sizes", "[bit_search]")
{
  for (uint32_t bitCount : {33u, 65u, 129u, 137u})
  {
    CAPTURE(bitCount);
    AlignedBits ab(bitCount, true);
    ab.clear(0);
    ab.clear(bitCount - 1);

    auto result = collect_forward_false(ab);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == 0);
    REQUIRE(result[1] == bitCount - 1);
  }
}

// ---- bit_search<false, false> backward tests ----

TEST_CASE("bit_search<false,false> backward: finds correct bit", "[bit_search]")
{
  AlignedBits ab(200, true);
  ab.clear(50);
  ab.clear(100);

  uint32_t w = ab.data[199 / detail::WORD_BITS];

  uint32_t pos = bit_search<false, false>(ab.data, ab.bitCount, 199, w);
  REQUIRE(pos == 100);

  pos = bit_search<false, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 50);

  pos = bit_search<false, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == ab.bitCount);
}

// ---- current_word output tests ----

TEST_CASE("bit_search: current_word contains full word after search", "[bit_search]")
{
  AlignedBits ab(200);
  ab.set(3);
  ab.set(15);
  ab.set(31);

  uint32_t expectedW = (1u << 3) | (1u << 15) | (1u << 31);

  // Forward search: w should be the full word
  uint32_t w = 0;
  uint32_t pos = bit_search<true, true>(ab.data, ab.bitCount, 0, w);
  REQUIRE(pos == 3);
  REQUIRE(w == expectedW);

  // Successive forward searches within same word keep full w
  pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
  REQUIRE(pos == 15);
  REQUIRE(w == expectedW);

  pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
  REQUIRE(pos == 31);
  REQUIRE(w == expectedW);

  // Backward search: w should also be the full word
  w = 0;
  pos = bit_search<true, false>(ab.data, ab.bitCount, 31, w);
  REQUIRE(pos == 31);
  REQUIRE(w == expectedW);

  // Successive backward searches within same word keep full w
  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 15);
  REQUIRE(w == expectedW);

  pos = bit_search<true, false>(ab.data, ab.bitCount, pos - 1, w);
  REQUIRE(pos == 3);
  REQUIRE(w == expectedW);
}

// ---- Drain pattern tests ----

TEST_CASE("bit_search: drain pattern yields correct next positions", "[bit_search]")
{
  AlignedBits ab(200);
  dag::Vector<uint32_t> expected = {3, 15, 31, 32, 63, 64, 100, 127, 128, 190, 199};
  for (auto p : expected)
    ab.set(p);

  // Collect using bit_search iteration (same as collectForwardTrue)
  auto result = collect_forward_true(ab);
  REQUIRE(result.size() == expected.size());
  for (size_t i = 0; i < expected.size(); ++i)
    REQUIRE(result[i] == expected[i]);
}

// ---- Benchmarks ----

TEST_CASE("bit_search benchmarks", "[bit_search][!benchmark]")
{
  volatile uint32_t sink = 0;

  SECTION("sparse bits (1% density)")
  {
    constexpr uint32_t N = 10000;
    AlignedBits ab(N);
    std::mt19937 rng{42};
    for (uint32_t i = 0; i < N / 100; ++i)
      ab.set(rng() % N);

    BENCHMARK("bit_search<true,true> 10k bits, ~1% set")
    {
      uint32_t count = 0;
      uint32_t w = 0;
      uint32_t pos = bit_search<true, true>(ab.data, ab.bitCount, 0, w);
      while (pos < ab.bitCount)
      {
        ++count;
        pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
      }
      sink = count;
      return count;
    };
  }

  SECTION("dense bits (50% density)")
  {
    constexpr uint32_t N = 10000;
    AlignedBits ab(N);
    std::mt19937 rng{42};
    for (uint32_t i = 0; i < N / 2; ++i)
      ab.set(rng() % N);

    BENCHMARK("bit_search<true,true> 10k bits, ~50% set")
    {
      uint32_t count = 0;
      uint32_t w = 0;
      uint32_t pos = bit_search<true, true>(ab.data, ab.bitCount, 0, w);
      while (pos < ab.bitCount)
      {
        ++count;
        pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
      }
      sink = count;
      return count;
    };
  }

  SECTION("all true")
  {
    constexpr uint32_t N = 10000;
    AlignedBits ab(N, true);

    BENCHMARK("bit_search<true,true> 10k bits, 100% set")
    {
      uint32_t count = 0;
      uint32_t w = 0;
      uint32_t pos = bit_search<true, true>(ab.data, ab.bitCount, 0, w);
      while (pos < ab.bitCount)
      {
        ++count;
        pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
      }
      sink = count;
      return count;
    };
  }

  SECTION("very sparse (single bit at end)")
  {
    constexpr uint32_t N = 10000;
    AlignedBits ab(N);
    ab.set(N - 1);

    BENCHMARK("bit_search<true,true> 10k bits, 1 set at end")
    {
      uint32_t count = 0;
      uint32_t w = 0;
      uint32_t pos = bit_search<true, true>(ab.data, ab.bitCount, 0, w);
      while (pos < ab.bitCount)
      {
        ++count;
        pos = bit_search<true, true>(ab.data, ab.bitCount, pos + 1, w);
      }
      sink = count;
      return count;
    };
  }
}
