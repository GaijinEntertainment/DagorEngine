// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <EASTL/unique_ptr.h>
#include <backend/resourceScheduling/lruCache.h>
#include <dag/dag_vector.h>
#include <util/dag_string.h>

// Perf test of ResourceLruCache against LruCache

namespace
{

// same footprint as NativeResourceAllocator::ResourceKey: two hashes plus ResourceDescription
struct BenchKey
{
  uint64_t hash;
  uint64_t rawHash;
  uint64_t payload[4];
  bool operator==(const BenchKey &) const = default;
};

struct BenchKeyHash
{
  size_t operator()(const BenchKey &k) const { return k.hash; }
};

struct BenchRes
{
  uint64_t dummy[4];
};

struct BenchDestroyer
{
  void operator()(BenchRes *r) const { delete r; }
};

constexpr uint32_t LIMIT = 128;
constexpr uint32_t EVICTION_LIMIT = 8;
constexpr uint32_t FRAMES = 2;
constexpr uint32_t SALT_CYCLE = 4; // shifted keys come back after this many recompiles, bounds the working set

using UniqueBenchRes = eastl::unique_ptr<BenchRes, BenchDestroyer>;
using NewCache = ResourceLruCache<BenchKey, BenchRes, BenchDestroyer, BenchKeyHash, LIMIT, EVICTION_LIMIT>;
using OldCache = LruCache<BenchKey, UniqueBenchRes, BenchKeyHash, LIMIT>;

UniqueBenchRes create_res() { return UniqueBenchRes(new BenchRes{}); }

BenchKey make_key(uint32_t texture, uint32_t variant, uint32_t salt)
{
  uint64_t h = (uint64_t(texture) << 40 | uint64_t(salt) << 32 | variant) * 0x9E3779B97F4A7C15ull;
  h ^= h >> 29;
  return BenchKey{h, uint64_t(texture) * 0x9E3779B97F4A7C15ull, {texture, variant, salt, 0}};
}

struct Scenario
{
  uint32_t textures;
  uint32_t resizedPerFrame;
  uint32_t variants;
  uint32_t shiftedOnRecompile = 0;
};

struct State
{
  dag::Vector<uint32_t> variant;
  uint32_t salt = 0;
  uint32_t recompiles = 0;

  explicit State(const Scenario &sc) : variant(sc.textures, 0) {}
};

template <class Cache>
struct Driver;

template <>
struct Driver<NewCache>
{
  static void place(NewCache &c, State &s, const Scenario &sc)
  {
    for (uint32_t t = 0; t < sc.textures; ++t)
      for (uint32_t f = 0; f < FRAMES; ++f)
        c.access(make_key(t, s.variant[t], t < sc.shiftedOnRecompile ? s.salt : 0), create_res);
    c.retireUnreferenced();
  }

  static void frame(NewCache &c, State &s, const Scenario &sc)
  {
    c.tick();
    for (uint32_t t = 0; t < sc.resizedPerFrame; ++t)
    {
      const uint32_t salt = t < sc.shiftedOnRecompile ? s.salt : 0;
      for (uint32_t f = 0; f < FRAMES; ++f)
        c.dropReference(make_key(t, s.variant[t], salt));
      s.variant[t] = (s.variant[t] + 1) % sc.variants;
      for (uint32_t f = 0; f < FRAMES; ++f)
        c.access(make_key(t, s.variant[t], salt), create_res);
    }
    c.retireUnreferenced();
  }

  static void unplace(NewCache &c, const State &s, const Scenario &sc)
  {
    for (uint32_t t = 0; t < sc.textures; ++t)
      for (uint32_t f = 0; f < FRAMES; ++f)
        c.dropReference(make_key(t, s.variant[t], t < sc.shiftedOnRecompile ? s.salt : 0));
  }

  static void recompile(NewCache &c, State &s, const Scenario &sc)
  {
    c.tick();
    unplace(c, s, sc);
    s.salt = (++s.recompiles) % SALT_CYCLE;
    place(c, s, sc);
    c.compact();
  }
};

template <>
struct Driver<OldCache>
{
  static void place(OldCache &c, State &s, const Scenario &sc)
  {
    for (uint32_t t = 0; t < sc.textures; ++t)
      for (uint32_t f = 0; f < FRAMES; ++f)
        c.access(make_key(t, s.variant[t], t < sc.shiftedOnRecompile ? s.salt : 0), create_res);
  }

  static void frame(OldCache &c, State &s, const Scenario &sc)
  {
    c.lruCleanup();
    for (uint32_t t = 0; t < sc.textures; ++t)
    {
      const uint32_t salt = t < sc.shiftedOnRecompile ? s.salt : 0;
      if (t < sc.resizedPerFrame)
      {
        s.variant[t] = (s.variant[t] + 1) % sc.variants;
        for (uint32_t f = 0; f < FRAMES; ++f)
          c.access(make_key(t, s.variant[t], salt), create_res);
      }
      else
        for (uint32_t f = 0; f < FRAMES; ++f)
          c.touch(make_key(t, s.variant[t], salt));
    }
  }

  static void recompile(OldCache &c, State &s, const Scenario &sc)
  {
    c.lruCleanup();
    s.salt = (++s.recompiles) % SALT_CYCLE;
    place(c, s, sc);
  }
};

template <class Cache>
void bench_frames(const char *label, const Scenario &sc)
{
  Cache cache;
  State state(sc);
  Driver<Cache>::place(cache, state, sc);
  // reach steady state: all variants exist and sit in the cache
  for (uint32_t i = 0; i < sc.variants + 1; ++i)
    Driver<Cache>::frame(cache, state, sc);

  String name(0, "%s frame: tex=%u resized=%u variants=%u", label, sc.textures, sc.resizedPerFrame, sc.variants);
  BENCHMARK(name.c_str()) { Driver<Cache>::frame(cache, state, sc); };
}

template <class Cache>
void bench_recompile(const char *label, const Scenario &sc)
{
  Cache cache;
  State state(sc);
  Driver<Cache>::place(cache, state, sc);
  for (uint32_t i = 0; i < SALT_CYCLE; ++i)
    Driver<Cache>::recompile(cache, state, sc);

  String name(0, "%s recompile: tex=%u shifted=%u", label, sc.textures, sc.shiftedOnRecompile);
  BENCHMARK(name.c_str()) { Driver<Cache>::recompile(cache, state, sc); };
}

} // namespace

TEST_CASE("resource lru cache benchmarks", "[!benchmark][resourceLruCache]")
{
  const Scenario frameScenarios[] = {
    {100, 0, 1},
    {500, 0, 1},
    {500, 10, 4},
    {500, 100, 4},
    {500, 100, 64},
    {2000, 0, 1},
    {2000, 200, 4},
  };
  const Scenario recompileScenarios[] = {
    {100, 0, 1, 0},
    {500, 0, 1, 0},
    {500, 0, 1, 250},
    {2000, 0, 1, 0},
    {2000, 0, 1, 1000},
  };

  SECTION("per frame")
  {
    for (const auto &sc : frameScenarios)
    {
      bench_frames<OldCache>("old", sc);
      bench_frames<NewCache>("new", sc);
    }
  }

  SECTION("recompile")
  {
    for (const auto &sc : recompileScenarios)
    {
      bench_recompile<OldCache>("old", sc);
      bench_recompile<NewCache>("new", sc);
    }
  }
}
