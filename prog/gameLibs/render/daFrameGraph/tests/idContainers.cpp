// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <id/idRange.h>
#include <id/idSparseIndexedMapping.h>
#include <id/idIndexedFlags.h>
#include <generic/dag_reverseView.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <numeric>
#include <random>

enum class IdType : uint32_t
{
  Invalid = UINT32_MAX
};

TEST_CASE("Fill up", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  constexpr uint32_t itemCount = 1000;

  for (IdType i : IdRange<IdType>(itemCount))
  {
    auto [k, v] = mapping.appendNew(static_cast<uint32_t>(i) * 10);
    REQUIRE(k == i);
    REQUIRE(v == static_cast<uint32_t>(i) * 10);
  }

  REQUIRE(mapping.totalKeys() == itemCount);
  for (IdType i : IdRange<IdType>(itemCount))
  {
    REQUIRE(mapping.isMapped(i));
    REQUIRE(mapping[i] == static_cast<uint32_t>(i) * 10);
  }

  for (uint32_t i = 0; auto k : mapping.keys())
  {
    REQUIRE(k == static_cast<IdType>(i));
    ++i;
  }

  for (uint32_t i = 0; auto v : mapping.values())
  {
    REQUIRE(v == static_cast<uint32_t>(i) * 10);
    ++i;
  }

  for (auto [i, v] : mapping.enumerate())
    REQUIRE(v == static_cast<uint32_t>(i) * 10);
}

void validate_empty(const auto &mapping, uint32_t itemCount)
{
  auto keys = mapping.keys();
  REQUIRE(eastl::distance(keys.begin(), keys.end()) == 0);
  auto values = mapping.values();
  REQUIRE(eastl::distance(values.begin(), values.end()) == 0);
  auto pairs = mapping.enumerate();
  REQUIRE(eastl::distance(pairs.begin(), pairs.end()) == 0);
  REQUIRE(mapping.totalKeys() == itemCount);
  for (IdType i : IdRange<IdType>(itemCount))
    REQUIRE_FALSE(mapping.isMapped(i));
}

TEST_CASE("Fill up and then erase all forwards", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  constexpr uint32_t itemCount = 1000;

  for (IdType i : IdRange<IdType>(itemCount))
    mapping.appendNew(static_cast<uint32_t>(i) * 10);

  for (IdType i : IdRange<IdType>(itemCount))
    mapping.erase(i);

  validate_empty(mapping, itemCount);
}

TEST_CASE("Fill up and erase in arbitrary order", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  constexpr uint32_t itemCount = 200;

  for (IdType i : IdRange<IdType>(itemCount))
    mapping.appendNew(static_cast<uint32_t>(i) * 10);

  dag::Vector<uint32_t> order(itemCount);
  std::iota(order.begin(), order.end(), 0);
  std::mt19937 rng{42};
  std::shuffle(order.begin(), order.end(), rng);

  for (uint32_t i : order)
    mapping.erase(static_cast<IdType>(i));

  validate_empty(mapping, itemCount);
}

TEST_CASE("Make holes and fill them", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  constexpr uint32_t itemCount = 200;

  for (IdType i : IdRange<IdType>(itemCount))
    mapping.appendNew(static_cast<uint32_t>(i) * 10);

  constexpr int holes[] = {7, 2, 5, 3, 43, 127, 128, 129, 160, 140, 199};

  for (uint32_t i : holes)
  {
    mapping.erase(static_cast<IdType>(i));
    REQUIRE_FALSE(mapping.isMapped(static_cast<IdType>(i)));
    for (IdType j : IdRange<IdType>(itemCount))
      if (j != static_cast<IdType>(i))
        REQUIRE(mapping.isMapped(j));
    mapping.appendNew(static_cast<uint32_t>(i) * 10);
    REQUIRE(mapping.totalKeys() == itemCount);
    REQUIRE(mapping.isMapped(static_cast<IdType>(i)));
  }
}

TEST_CASE("Make multiple holes and fill all of them up", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  constexpr uint32_t itemCount = 500;

  for (IdType i : IdRange<IdType>(itemCount))
    mapping.appendNew(static_cast<uint32_t>(i) * 10);

  constexpr int holes[] = {31, 32, 63, 64, 69, 126, 127, 128, 169, 255, 256, 300, 400};

  for (uint32_t i : holes)
    mapping.erase(static_cast<IdType>(i));

  REQUIRE(mapping.totalKeys() == itemCount);
  for (uint32_t i : holes)
    REQUIRE_FALSE(mapping.isMapped(static_cast<IdType>(i)));

  for (uint32_t i : holes)
    mapping.appendNew(static_cast<uint32_t>(i) * 10);

  REQUIRE(mapping.totalKeys() == itemCount);

  for (IdType i : IdRange<IdType>(itemCount))
  {
    REQUIRE(mapping.isMapped(i));
    REQUIRE(mapping[i] == static_cast<uint32_t>(i) * 10);
  }
}

TEST_CASE("Reverse iteration", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  SECTION("Single item")
  {
    mapping.appendNew(42);
    for (auto k : dag::ReverseView{mapping.keys()})
      REQUIRE(k == static_cast<IdType>(0));
  }

  SECTION("128 items")
  {
    constexpr uint32_t itemCount = 128;

    for (IdType i : IdRange<IdType>(itemCount))
      mapping.appendNew(static_cast<uint32_t>(i) * 10);

    uint32_t expectedKey = itemCount - 1;
    for (auto k : dag::ReverseView{mapping.keys()})
    {
      REQUIRE(k == static_cast<IdType>(expectedKey));
      --expectedKey;
    }
    REQUIRE(expectedKey == UINT32_MAX);
  }

  SECTION("2 items with holes in-between")
  {
    const size_t holeCount = GENERATE(10, 32, 64, 128, 160, 192, 256, 512, 1024);

    mapping.appendNew(10);
    for (uint32_t i = 0; i < holeCount; ++i)
      mapping.appendNew(0);
    mapping.appendNew(20);

    for (uint32_t i = 0; i < holeCount; ++i)
      mapping.erase(static_cast<IdType>(1 + i));

    auto keysView = dag::ReverseView{mapping.keys()};
    auto it = keysView.begin();
    REQUIRE((it != keysView.end()));
    REQUIRE(*it == static_cast<IdType>(holeCount + 1));
    ++it;
    REQUIRE((it != keysView.end()));
    REQUIRE(*it == static_cast<IdType>(0));
    ++it;
    REQUIRE((it == keysView.end()));
  }
}

TEST_CASE("Iterate with holes", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  constexpr uint32_t itemCount = 300;

  for (IdType i : IdRange<IdType>(itemCount))
    mapping.appendNew(static_cast<uint32_t>(i) * 10);

  constexpr int holes[] = {10, 31, 32, 64, 128, 129, 130, 200, 250};

  for (uint32_t i : holes)
    mapping.erase(static_cast<IdType>(i));

  REQUIRE(mapping.totalKeys() == itemCount);

  dag::Vector<bool> expectToFind(itemCount, true);
  for (auto k : holes)
    expectToFind[static_cast<uint32_t>(k)] = false;

  SECTION("Keys")
  {
    dag::Vector<bool> found(itemCount, false);
    for (auto k : mapping.keys())
      found[static_cast<uint32_t>(k)] = true;

    for (IdType i : IdRange<IdType>(itemCount))
    {
      CAPTURE(i);
      REQUIRE(expectToFind[static_cast<uint32_t>(i)] == found[static_cast<uint32_t>(i)]);
    }
  }

  SECTION("Values")
  {
    dag::Vector<bool> found(itemCount, false);
    for (auto v : mapping.values())
      found[v / 10] = true;

    for (IdType i : IdRange<IdType>(itemCount))
    {
      CAPTURE(i);
      REQUIRE(expectToFind[static_cast<uint32_t>(i)] == found[static_cast<uint32_t>(i)]);
    }
  }

  SECTION("Pairs")
  {
    dag::Vector<bool> found(itemCount, false);
    for (auto [k, v] : mapping.enumerate())
      found[static_cast<uint32_t>(k)] = true;

    for (IdType i : IdRange<IdType>(itemCount))
    {
      CAPTURE(i);
      REQUIRE(expectToFind[static_cast<uint32_t>(i)] == found[static_cast<uint32_t>(i)]);
    }
  }

  SECTION("Reverse keys")
  {
    dag::Vector<bool> found(itemCount, false);
    for (auto k : dag::ReverseView{mapping.keys()})
    {
      CAPTURE(k);
      REQUIRE(mapping.isMapped(k));
      found[static_cast<uint32_t>(k)] = true;
    }

    for (IdType i : IdRange<IdType>(itemCount))
    {
      CAPTURE(i);
      REQUIRE(expectToFind[static_cast<uint32_t>(i)] == found[static_cast<uint32_t>(i)]);
    }
  }
}

TEST_CASE("Erase returns iterator to next element", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> mapping;

  SECTION("Erase first of many")
  {
    for (uint32_t i = 0; i < 5; ++i)
      mapping.appendNew(i * 10);

    auto it = mapping.erase(static_cast<IdType>(0));
    auto [key, val] = *it;
    REQUIRE(key == static_cast<IdType>(1));
    REQUIRE(val == 10);
  }

  SECTION("Erase middle element")
  {
    for (uint32_t i = 0; i < 5; ++i)
      mapping.appendNew(i * 10);

    auto it = mapping.erase(static_cast<IdType>(2));
    auto [key, val] = *it;
    REQUIRE(key == static_cast<IdType>(3));
    REQUIRE(val == 30);
  }

  SECTION("Erase last element returns end")
  {
    for (uint32_t i = 0; i < 5; ++i)
      mapping.appendNew(i * 10);

    auto it = mapping.erase(static_cast<IdType>(4));
    REQUIRE((it == mapping.enumerate().end()));
  }

  SECTION("Erase sole element returns end")
  {
    mapping.appendNew(42);

    auto it = mapping.erase(static_cast<IdType>(0));
    REQUIRE((it == mapping.enumerate().end()));
  }

  SECTION("Erase skips holes to find next")
  {
    for (uint32_t i = 0; i < 10; ++i)
      mapping.appendNew(i * 10);
    // Create a hole at 3 and 4
    mapping.erase(static_cast<IdType>(3));
    mapping.erase(static_cast<IdType>(4));

    // Erase 2 - next used should be 5
    auto it = mapping.erase(static_cast<IdType>(2));
    auto [key, val] = *it;
    REQUIRE(key == static_cast<IdType>(5));
    REQUIRE(val == 50);
  }

  SECTION("Return value is consistent with enumerate end")
  {
    for (uint32_t i = 0; i < 5; ++i)
      mapping.appendNew(i * 10);

    // Erase middle elements and verify returned iterator matches next enumerate step
    auto it = mapping.erase(static_cast<IdType>(1));
    auto [k1, v1] = *it;
    REQUIRE(k1 == static_cast<IdType>(2));

    auto it2 = mapping.erase(static_cast<IdType>(3));
    auto [k2, v2] = *it2;
    REQUIRE(k2 == static_cast<IdType>(4));
    REQUIRE(v2 == 40);

    // Erase the last remaining, should be end
    mapping.erase(static_cast<IdType>(4));
    REQUIRE(mapping.used() == 2);
    // Remaining: 0 and 2
    REQUIRE(mapping.isMapped(static_cast<IdType>(0)));
    REQUIRE(mapping.isMapped(static_cast<IdType>(2)));
  }

  SECTION("Erase during manual iteration via assignment")
  {
    constexpr uint32_t itemCount = 100;
    for (uint32_t i = 0; i < itemCount; ++i)
      mapping.appendNew(i);

    // Erase all even-indexed entries using the returned iterator
    uint32_t eraseCount = 0;
    for (auto it = mapping.enumerate().begin(); it != mapping.enumerate().end();)
    {
      auto [key, val] = *it;
      if (static_cast<uint32_t>(key) % 2 == 0)
      {
        it = mapping.erase(key);
        ++eraseCount;
      }
      else
        ++it;
    }

    REQUIRE(eraseCount == 50);
    REQUIRE(mapping.used() == 50);
    for (uint32_t i = 0; i < itemCount; ++i)
    {
      if (i % 2 == 0)
        REQUIRE_FALSE(mapping.isMapped(static_cast<IdType>(i)));
      else
      {
        REQUIRE(mapping.isMapped(static_cast<IdType>(i)));
        REQUIRE(mapping[static_cast<IdType>(i)] == i);
      }
    }
  }
}

static void validate_mapping_equals(const IdSparseIndexedMapping<IdType, uint32_t> &mapping,
  const dag::Vector<eastl::pair<uint32_t, uint32_t>> &expected)
{
  REQUIRE(mapping.used() == expected.size());
  for (auto [key, val] : expected)
  {
    CAPTURE(key, val);
    REQUIRE(mapping.isMapped(static_cast<IdType>(key)));
    REQUIRE(mapping[static_cast<IdType>(key)] == val);
  }
}

TEST_CASE("updateFrom with identical source and no changes", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  for (uint32_t i = 0; i < 10; ++i)
  {
    dst.appendNew(i * 10);
    src.appendNew(i * 10);
  }
  changes.resize(10, false);

  dst.updateFrom(src, changes);

  // Everything should remain the same
  for (uint32_t i = 0; i < 10; ++i)
  {
    REQUIRE(dst.isMapped(static_cast<IdType>(i)));
    REQUIRE(dst[static_cast<IdType>(i)] == i * 10);
  }
}

TEST_CASE("updateFrom copies changed items", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  for (uint32_t i = 0; i < 10; ++i)
  {
    dst.appendNew(i * 10);
    src.appendNew(i * 100); // source has different values
  }
  changes.resize(10, false);
  // Mark indices 3 and 7 as changed
  changes.set(static_cast<IdType>(3), true);
  changes.set(static_cast<IdType>(7), true);

  dst.updateFrom(src, changes);

  for (uint32_t i = 0; i < 10; ++i)
  {
    REQUIRE(dst.isMapped(static_cast<IdType>(i)));
    if (i == 3 || i == 7)
      REQUIRE(dst[static_cast<IdType>(i)] == i * 100);
    else
      REQUIRE(dst[static_cast<IdType>(i)] == i * 10);
  }
}

TEST_CASE("updateFrom inserts new items from source", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  // dst has {0, 1, 2}
  for (uint32_t i = 0; i < 3; ++i)
    dst.appendNew(i * 10);

  // src has {0, 1, 2, 3, 4}
  for (uint32_t i = 0; i < 5; ++i)
    src.appendNew(i * 100);

  changes.resize(5, false);

  dst.updateFrom(src, changes);

  // Existing unchanged items keep old values
  for (uint32_t i = 0; i < 3; ++i)
    REQUIRE(dst[static_cast<IdType>(i)] == i * 10);
  // New items get source values
  for (uint32_t i = 3; i < 5; ++i)
    REQUIRE(dst[static_cast<IdType>(i)] == i * 100);
  REQUIRE(dst.used() == 5);
}

TEST_CASE("updateFrom removes items not in source", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  // dst has {0, 1, 2, 3, 4}
  for (uint32_t i = 0; i < 5; ++i)
    dst.appendNew(i * 10);

  // src has {0, 1, 2} only
  for (uint32_t i = 0; i < 3; ++i)
    src.appendNew(i * 100);

  changes.resize(3, false);

  dst.updateFrom(src, changes);

  REQUIRE(dst.used() == 3);
  for (uint32_t i = 0; i < 3; ++i)
    REQUIRE(dst.isMapped(static_cast<IdType>(i)));
  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(3)));
  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(4)));
}

TEST_CASE("updateFrom from empty source clears destination", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  for (uint32_t i = 0; i < 10; ++i)
    dst.appendNew(i * 10);

  dst.updateFrom(src, changes);

  REQUIRE(dst.used() == 0);
}

TEST_CASE("updateFrom into empty destination copies all", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  for (uint32_t i = 0; i < 10; ++i)
    src.appendNew(i * 10);

  changes.resize(10, false);

  dst.updateFrom(src, changes);

  REQUIRE(dst.used() == 10);
  for (uint32_t i = 0; i < 10; ++i)
  {
    REQUIRE(dst.isMapped(static_cast<IdType>(i)));
    REQUIRE(dst[static_cast<IdType>(i)] == i * 10);
  }
}

TEST_CASE("updateFrom with sparse mappings", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  // dst has holes: {0, 2, 4, 6, 8}
  for (uint32_t i = 0; i < 10; ++i)
    dst.appendNew(i * 10);
  for (uint32_t i : {1, 3, 5, 7, 9})
    dst.erase(static_cast<IdType>(i));

  // src has different holes: {1, 2, 3, 6, 7}
  for (uint32_t i = 0; i < 10; ++i)
    src.appendNew(i * 100);
  for (uint32_t i : {0, 4, 5, 8, 9})
    src.erase(static_cast<IdType>(i));

  changes.resize(10, false);
  changes.set(static_cast<IdType>(2), true); // common key, flagged changed

  dst.updateFrom(src, changes);

  // Keys only in dst (0, 4, 8) should be removed
  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(0)));
  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(4)));
  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(8)));

  // Keys only in src (1, 3, 7) should be inserted
  REQUIRE(dst[static_cast<IdType>(1)] == 100);
  REQUIRE(dst[static_cast<IdType>(3)] == 300);
  REQUIRE(dst[static_cast<IdType>(7)] == 700);

  // Common key 2, flagged changed - gets source value
  REQUIRE(dst[static_cast<IdType>(2)] == 200);

  // Common key 6, NOT flagged changed - keeps old value
  REQUIRE(dst[static_cast<IdType>(6)] == 60);

  REQUIRE(dst.used() == 5);
}

TEST_CASE("updateFrom all items changed", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  for (uint32_t i = 0; i < 10; ++i)
  {
    dst.appendNew(i * 10);
    src.appendNew(i * 100);
  }

  changes.resize(10, true);

  dst.updateFrom(src, changes);

  for (uint32_t i = 0; i < 10; ++i)
    REQUIRE(dst[static_cast<IdType>(i)] == i * 100);
}

TEST_CASE("updateFrom with changes flags smaller than source", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  for (uint32_t i = 0; i < 10; ++i)
  {
    dst.appendNew(i * 10);
    src.appendNew(i * 100);
  }

  // changes only covers first 5 items - the rest default to false
  changes.resize(5, false);
  changes.set(static_cast<IdType>(2), true);

  dst.updateFrom(src, changes);

  // Only index 2 should be updated
  REQUIRE(dst[static_cast<IdType>(2)] == 200);
  // Others keep old values
  REQUIRE(dst[static_cast<IdType>(0)] == 0);
  REQUIRE(dst[static_cast<IdType>(5)] == 50);
  REQUIRE(dst[static_cast<IdType>(9)] == 90);
}

TEST_CASE("updateFrom simultaneous add remove and update", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  // dst: {0=100, 1=110, 2=120, 3=130}
  for (uint32_t i = 0; i < 4; ++i)
    dst.appendNew(100 + i * 10);

  // src: {1=210, 2=220, 4=240}  (0 and 3 removed, 4 added)
  for (uint32_t i = 0; i < 5; ++i)
    src.appendNew(200 + i * 10);
  src.erase(static_cast<IdType>(0));
  src.erase(static_cast<IdType>(3));

  changes.resize(5, false);
  changes.set(static_cast<IdType>(2), true); // 2 changed

  dst.updateFrom(src, changes);

  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(0))); // removed
  REQUIRE(dst[static_cast<IdType>(1)] == 110);         // kept (unchanged)
  REQUIRE(dst[static_cast<IdType>(2)] == 220);         // updated (changed)
  REQUIRE_FALSE(dst.isMapped(static_cast<IdType>(3))); // removed
  REQUIRE(dst[static_cast<IdType>(4)] == 240);         // inserted (new)
  REQUIRE(dst.used() == 3);
}

TEST_CASE("updateFrom both empty", "[idSparseIndexedMapping]")
{
  IdSparseIndexedMapping<IdType, uint32_t> dst;
  IdSparseIndexedMapping<IdType, uint32_t> src;
  IdIndexedFlags<IdType> changes;

  dst.updateFrom(src, changes);

  REQUIRE(dst.used() == 0);
  REQUIRE(dst.empty());
}

TEST_CASE("IdSparseIndexedMapping keys() benchmark", "[idSparseIndexedMapping][!benchmark]")
{
  constexpr uint32_t BIT_COUNT = 10'000;
  const auto holePercent = GENERATE(1, 10, 50);

  IdSparseIndexedMapping<IdType, uint32_t> mapping;
  for (uint32_t i = 0; i < BIT_COUNT; ++i)
    mapping.appendNew(i);
  dag::Vector<uint32_t> indices(BIT_COUNT);
  std::iota(indices.begin(), indices.end(), 0);
  std::mt19937 rng{42};
  std::shuffle(indices.begin(), indices.end(), rng);
  for (uint32_t i = 0; i < BIT_COUNT * holePercent / 100; ++i)
    mapping.erase(static_cast<IdType>(indices[i]));

  BENCHMARK("keys() " + std::to_string(BIT_COUNT) + ", " + std::to_string(holePercent) + "% holes")
  {
    uint32_t count = 0;
    for ([[maybe_unused]] auto key : mapping.keys())
      ++count;
    return count;
  };
}

// -- IdIndexedFlags general tests --

TEST_CASE("default construction", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags;
  REQUIRE(flags.size() == 0);
  REQUIRE(flags.empty());
}

TEST_CASE("sized construction with false", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, false);
  REQUIRE(flags.size() == 100);
  REQUIRE_FALSE(flags.empty());
  for (uint32_t i = 0; i < 100; ++i)
  {
    CAPTURE(i);
    REQUIRE_FALSE(flags[static_cast<IdType>(i)]);
  }
}

TEST_CASE("sized construction with true", "[idIndexedFlags]")
{
  for (uint32_t count : {1u, 31u, 32u, 33u, 64u, 65u, 128u, 129u, 200u})
  {
    CAPTURE(count);
    IdIndexedFlags<IdType> flags(count, true);
    REQUIRE(flags.size() == count);
    for (uint32_t i = 0; i < count; ++i)
    {
      CAPTURE(i);
      REQUIRE(flags[static_cast<IdType>(i)]);
    }
  }
}

TEST_CASE("copy constructor", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, false);
  flags.set(static_cast<IdType>(5), true);
  flags.set(static_cast<IdType>(63), true);
  flags.set(static_cast<IdType>(99), true);

  IdIndexedFlags<IdType> copy(flags);
  REQUIRE(copy.size() == 100);
  REQUIRE(copy[static_cast<IdType>(5)]);
  REQUIRE(copy[static_cast<IdType>(63)]);
  REQUIRE(copy[static_cast<IdType>(99)]);
  REQUIRE_FALSE(copy[static_cast<IdType>(0)]);
  REQUIRE_FALSE(copy[static_cast<IdType>(64)]);
}

TEST_CASE("move constructor", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, false);
  flags.set(static_cast<IdType>(42), true);

  IdIndexedFlags<IdType> moved(eastl::move(flags));
  REQUIRE(moved.size() == 100);
  for (int i = 0; i < 100; ++i)
  {
    CAPTURE(i);
    REQUIRE(moved[static_cast<IdType>(i)] == (i == 42));
  }
  REQUIRE(flags.size() == 0);
  REQUIRE(flags.empty());
}

TEST_CASE("copy assignment", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, false);
  flags.set(static_cast<IdType>(10), true);

  IdIndexedFlags<IdType> other(50, true);
  other = flags;
  REQUIRE(other.size() == 100);
  for (int i = 0; i < 100; ++i)
  {
    CAPTURE(i);
    REQUIRE(other[static_cast<IdType>(i)] == (i == 10));
  }
}

TEST_CASE("move assignment", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, false);
  flags.set(static_cast<IdType>(10), true);

  IdIndexedFlags<IdType> other(150, true);
  other = eastl::move(flags);
  REQUIRE(other.size() == 100);
  for (int i = 0; i < 100; ++i)
  {
    CAPTURE(i);
    REQUIRE(other[static_cast<IdType>(i)] == (i == 10));
  }
  REQUIRE(flags.size() == 0);
}

TEST_CASE("operator[] read and write via BitReference", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(64, false);

  flags[static_cast<IdType>(0)] = true;
  flags[static_cast<IdType>(31)] = true;
  flags[static_cast<IdType>(32)] = true;
  flags[static_cast<IdType>(63)] = true;

  REQUIRE(flags[static_cast<IdType>(0)]);
  REQUIRE(flags[static_cast<IdType>(31)]);
  REQUIRE(flags[static_cast<IdType>(32)]);
  REQUIRE(flags[static_cast<IdType>(63)]);
  REQUIRE_FALSE(flags[static_cast<IdType>(1)]);
  REQUIRE_FALSE(flags[static_cast<IdType>(30)]);

  // Clear via BitReference
  flags[static_cast<IdType>(31)] = false;
  REQUIRE_FALSE(flags[static_cast<IdType>(31)]);
}

TEST_CASE("set auto-resizes", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags;
  REQUIRE(flags.size() == 0);

  flags.set(static_cast<IdType>(100), true);
  REQUIRE(flags.size() == 101);
  REQUIRE(flags[static_cast<IdType>(100)]);

  // All bits before should be false
  for (uint32_t i = 0; i < 100; ++i)
    REQUIRE_FALSE(flags[static_cast<IdType>(i)]);
}

TEST_CASE("test returns default for unmapped keys", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(10, false);
  flags.set(static_cast<IdType>(5), true);

  REQUIRE(flags.test(static_cast<IdType>(5)));
  REQUIRE_FALSE(flags.test(static_cast<IdType>(0)));

  // Out of range
  REQUIRE_FALSE(flags.test(static_cast<IdType>(100)));
  REQUIRE(flags.test(static_cast<IdType>(100), true));
}

TEST_CASE("resize grow preserves existing bits", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(32, false);
  flags.set(static_cast<IdType>(0), true);
  flags.set(static_cast<IdType>(31), true);

  flags.resize(128, false);
  REQUIRE(flags.size() == 128);
  REQUIRE(flags[static_cast<IdType>(0)]);
  REQUIRE(flags[static_cast<IdType>(31)]);
  REQUIRE_FALSE(flags[static_cast<IdType>(32)]);
  REQUIRE_FALSE(flags[static_cast<IdType>(127)]);
}

TEST_CASE("resize grow with true sets new bits", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(32, false);

  flags.resize(96, true);
  REQUIRE(flags.size() == 96);
  // Original bits stay false
  for (uint32_t i = 0; i < 32; ++i)
  {
    CAPTURE(i);
    REQUIRE_FALSE(flags[static_cast<IdType>(i)]);
  }
  // New bits are true
  for (uint32_t i = 32; i < 96; ++i)
  {
    CAPTURE(i);
    REQUIRE(flags[static_cast<IdType>(i)]);
  }
}

TEST_CASE("resize shrink clears removed bits", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(128, true);

  flags.resize(33);
  REQUIRE(flags.size() == 33);
  for (uint32_t i = 0; i < 33; ++i)
  {
    CAPTURE(i);
    REQUIRE(flags[static_cast<IdType>(i)]);
  }

  // Growing back should have zeros in the previously-shrunk region
  flags.resize(128);
  for (uint32_t i = 33; i < 128; ++i)
  {
    CAPTURE(i);
    REQUIRE_FALSE(flags[static_cast<IdType>(i)]);
  }
}

TEST_CASE("assign overwrites all bits", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, true);

  flags.assign(50, false);
  REQUIRE(flags.size() == 50);
  for (uint32_t i = 0; i < 50; ++i)
    REQUIRE_FALSE(flags[static_cast<IdType>(i)]);

  flags.assign(80, true);
  REQUIRE(flags.size() == 80);
  for (uint32_t i = 0; i < 80; ++i)
  {
    CAPTURE(i);
    REQUIRE(flags[static_cast<IdType>(i)]);
  }
}

TEST_CASE("clear zeros everything", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(100, true);
  flags.clear();
  REQUIRE(flags.size() == 0);
  REQUIRE(flags.empty());

  // Regrow and verify padding was zeroed
  flags.resize(100, false);
  for (uint32_t i = 0; i < 100; ++i)
  {
    CAPTURE(i);
    REQUIRE_FALSE(flags[static_cast<IdType>(i)]);
  }
}

TEST_CASE("reserve does not change size", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags;
  flags.reserve(1000);
  REQUIRE(flags.size() == 0);
  REQUIRE(flags.empty());

  flags.resize(500, true);
  REQUIRE(flags.size() == 500);
}

TEST_CASE("enumerate yields index-value pairs", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(64, false);
  flags.set(static_cast<IdType>(10), true);
  flags.set(static_cast<IdType>(63), true);

  uint32_t count = 0;
  for (auto [id, val] : flags.enumerate())
  {
    auto idx = static_cast<uint32_t>(id);
    if (idx == 10 || idx == 63)
      REQUIRE(val);
    else
      REQUIRE_FALSE(val);
    ++count;
  }
  REQUIRE(count == 64);
}

TEST_CASE("enumerate allows mutation via BitReference", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(32, false);
  for (auto [id, val] : flags.enumerate())
    val = (static_cast<uint32_t>(id) % 2 == 0);

  for (uint32_t i = 0; i < 32; ++i)
  {
    CAPTURE(i);
    REQUIRE(flags[static_cast<IdType>(i)] == (i % 2 == 0));
  }
}

TEST_CASE("non-power-of-two sizes maintain padding invariant", "[idIndexedFlags]")
{
  for (uint32_t count : {1u, 7u, 33u, 65u, 97u, 129u, 137u})
  {
    CAPTURE(count);
    IdIndexedFlags<IdType> flags(count, true);
    REQUIRE(flags.size() == count);

    // Shrink and regrow to verify padding was properly cleared
    flags.resize(count / 2);
    flags.resize(count, false);

    for (uint32_t i = 0; i < count / 2; ++i)
      REQUIRE(flags[static_cast<IdType>(i)]);
    for (uint32_t i = count / 2; i < count; ++i)
    {
      CAPTURE(i);
      REQUIRE_FALSE(flags[static_cast<IdType>(i)]);
    }
  }
}

TEST_CASE("isMapped boundary", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags(10, false);
  REQUIRE(flags.isMapped(static_cast<IdType>(0)));
  REQUIRE(flags.isMapped(static_cast<IdType>(9)));
  REQUIRE_FALSE(flags.isMapped(static_cast<IdType>(10)));
  REQUIRE_FALSE(flags.isMapped(static_cast<IdType>(100)));
}

// -- trueKeys tests --

TEST_CASE("trueKeys on empty bitvector", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags;
  auto view = flags.trueKeys();
  REQUIRE((view.begin() == view.end()));
}

TEST_CASE("trueKeys on all-false bitvector", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags;
  flags.resize(200, false);
  auto view = flags.trueKeys();
  REQUIRE((view.begin() == view.end()));
}

TEST_CASE("trueKeys with single set bit", "[idIndexedFlags]")
{
  for (uint32_t pos : {0u, 31u, 32u, 63u, 64u, 127u, 128u, 199u})
  {
    CAPTURE(pos);
    IdIndexedFlags<IdType> flags;
    flags.resize(200, false);
    flags.set(static_cast<IdType>(pos), true);

    dag::Vector<uint32_t> result;
    for (auto key : flags.trueKeys())
      result.push_back(static_cast<uint32_t>(key));

    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == pos);
  }
}

TEST_CASE("trueKeys with multiple scattered bits", "[idIndexedFlags]")
{
  IdIndexedFlags<IdType> flags;
  flags.resize(200, false);

  dag::Vector<uint32_t> expected = {3, 15, 31, 32, 63, 64, 100, 127, 128, 190, 199};
  for (auto pos : expected)
    flags.set(static_cast<IdType>(pos), true);

  dag::Vector<uint32_t> result;
  for (auto key : flags.trueKeys())
    result.push_back(static_cast<uint32_t>(key));

  REQUIRE(result.size() == expected.size());
  for (size_t i = 0; i < expected.size(); ++i)
  {
    CAPTURE(i);
    REQUIRE(result[i] == expected[i]);
  }
}

TEST_CASE("trueKeys on all-true bitvector", "[idIndexedFlags]")
{
  constexpr uint32_t BIT_COUNT = 200;
  IdIndexedFlags<IdType> flags;
  flags.resize(BIT_COUNT, true);

  dag::Vector<uint32_t> result;
  for (auto key : flags.trueKeys())
    result.push_back(static_cast<uint32_t>(key));

  REQUIRE(result.size() == BIT_COUNT);
  for (uint32_t i = 0; i < BIT_COUNT; ++i)
  {
    CAPTURE(i);
    REQUIRE(result[i] == i);
  }
}

TEST_CASE("trueKeys with non-power-of-two sizes", "[idIndexedFlags]")
{
  for (uint32_t bitCount : {33u, 65u, 129u, 137u})
  {
    CAPTURE(bitCount);
    IdIndexedFlags<IdType> flags;
    flags.resize(bitCount, false);

    // Set first and last bits
    flags.set(static_cast<IdType>(0), true);
    flags.set(static_cast<IdType>(bitCount - 1), true);

    dag::Vector<uint32_t> result;
    for (auto key : flags.trueKeys())
      result.push_back(static_cast<uint32_t>(key));

    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == 0);
    REQUIRE(result[1] == bitCount - 1);
  }
}

static IdIndexedFlags<IdType> make_flags_with_density(uint32_t bit_count, uint32_t density_percent)
{
  IdIndexedFlags<IdType> flags;
  if (density_percent >= 100)
  {
    flags.resize(bit_count, true);
    return flags;
  }
  flags.resize(bit_count, false);
  if (density_percent == 0)
    return flags;
  const uint32_t count = uint64_t(bit_count) * density_percent / 100;
  dag::Vector<uint32_t> indices(bit_count);
  std::iota(indices.begin(), indices.end(), 0u);
  std::mt19937 rng{42};
  std::shuffle(indices.begin(), indices.end(), rng);
  for (uint32_t i = 0; i < count; ++i)
    flags.set(static_cast<IdType>(indices[i]), true);
  return flags;
}

TEST_CASE("IdIndexedFlags trueKeys benchmark", "[idIndexedFlags][!benchmark]")
{
  volatile uint32_t sink = 0;
  constexpr uint32_t BIT_COUNT = 10'000;
  const auto density = GENERATE(1, 50, 100);
  auto flags = make_flags_with_density(BIT_COUNT, density);

  BENCHMARK("trueKeys 10k bits, " + std::to_string(density) + "% set")
  {
    uint32_t count = 0;
    for ([[maybe_unused]] auto key : flags.trueKeys())
      ++count;
    sink = count;
    return count;
  };
}

// Verify that trueKeys only yields keys within [0, bitCount).
// framemem is a bump allocator, so allocating right after the IdIndexedFlags
// places non-zero data adjacent to its internal buffer.
TEST_CASE("trueKeys iterator stops correctly with poisoned adjacent memory", "[idIndexedFlags]")
{
  FRAMEMEM_REGION;

  for (uint32_t bitCount : {128u, 256u, 512u})
  {
    CAPTURE(bitCount);

    IdIndexedFlags<IdType, framemem_allocator> flags(bitCount, false);
    flags.set(static_cast<IdType>(bitCount - 1), true);

    // Allocate from the same bump allocator -- lands right after flags' buffer.
    // 0xFFFFFFFE has ctz == 1, so bit_search returns bitCount + 1 instead of
    // bitCount, making the iterator overshoot (bitCount + 1 != end.pos).
    // 0xFFFFFFFF would accidentally hide the bug because ctz == 0.
    constexpr size_t POISON_BYTES = 16;
    auto *poison = static_cast<uint32_t *>(framemem_ptr()->alloc(POISON_BYTES));
    memset(poison, 0, POISON_BYTES);
    for (size_t i = 0; i < POISON_BYTES / sizeof(uint32_t); ++i)
      poison[i] = 0xFFFFFFFEu;

    dag::Vector<uint32_t> result;
    for (auto key : flags.trueKeys())
    {
      auto k = static_cast<uint32_t>(key);
      REQUIRE(k < bitCount);
      result.push_back(k);
      if (result.size() > bitCount)
        break; // prevent infinite loop on failure
    }

    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == bitCount - 1);
  }
}

TEST_CASE("IdIndexedFlags trueKeys benchmark: 1 bit at end", "[idIndexedFlags][!benchmark]")
{
  constexpr uint32_t BIT_COUNT = 10'000;
  IdIndexedFlags<IdType> flags;
  flags.resize(BIT_COUNT, false);
  flags.set(static_cast<IdType>(BIT_COUNT - 1), true);

  BENCHMARK("trueKeys 10k bits, 1 set at end")
  {
    uint32_t count = 0;
    for ([[maybe_unused]] auto key : flags.trueKeys())
      ++count;
    return count;
  };
}
