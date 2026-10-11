// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_get_random_seed.hpp>
#include <EASTL/unique_ptr.h>
#include <backend/resourceScheduling/lruCache.h>
#include <math/random/dag_random.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/sort.h>
#include <dag/dag_vector.h>
#include <util/dag_globDef.h>

namespace
{

struct FakeRes
{
  uint32_t key;
  uint32_t serial;
};

struct Tracker
{
  uint32_t created = 0;
  dag::Vector<uint32_t> destroyedSinceTake;
  eastl::unordered_set<uint32_t> everDestroyed;
  bool doubleDestroy = false;

  void reset() { *this = Tracker{}; }
  dag::Vector<uint32_t> take() { return eastl::exchange(destroyedSinceTake, {}); }
};

Tracker g_tracker;

struct FakeDestroyer
{
  void operator()(FakeRes *res) const
  {
    if (!g_tracker.everDestroyed.insert(res->serial).second)
      g_tracker.doubleDestroy = true;
    g_tracker.destroyedSinceTake.push_back(res->serial);
    delete res;
  }
};

struct KeyHash
{
  size_t operator()(uint32_t k) const { return size_t(k) * 0x9E3779B97F4A7C15ull; }
};

constexpr uint32_t LIMIT = 8;
using Cache = ResourceLruCache<uint32_t, FakeRes, FakeDestroyer, KeyHash, LIMIT>;
using UniqueFakeRes = eastl::unique_ptr<FakeRes, FakeDestroyer>;

UniqueFakeRes create_fake(uint32_t key)
{
  ++g_tracker.created;
  return UniqueFakeRes(new FakeRes{key, g_tracker.created});
}

struct Model
{
  struct Live
  {
    FakeRes *ptr;
    uint32_t refs;
  };
  struct Cached
  {
    FakeRes *ptr;
    uint32_t retiredAt;
  };
  eastl::unordered_map<uint32_t, Live> live;
  eastl::unordered_map<uint32_t, Cached> cached;
  uint32_t epoch = 0;

  dag::Vector<uint32_t> tick()
  {
    ++epoch;
    dag::Vector<uint32_t> expired;
    for (auto it = cached.begin(); it != cached.end();)
      if (it->second.retiredAt + LIMIT < epoch)
      {
        expired.push_back(it->second.ptr->serial);
        it = cached.erase(it);
      }
      else
        ++it;
    return expired;
  }

  // nullptr means a creation is expected
  FakeRes *expectedOnAccess(uint32_t key) const
  {
    if (auto it = live.find(key); it != live.end())
      return it->second.ptr;
    if (auto it = cached.find(key); it != cached.end())
      return it->second.ptr;
    return nullptr;
  }

  void accessed(uint32_t key, FakeRes *ptr)
  {
    if (auto it = live.find(key); it != live.end())
    {
      ++it->second.refs;
      return;
    }
    cached.erase(key);
    live[key] = Live{ptr, 1};
  }

  void drop(uint32_t key)
  {
    auto it = live.find(key);
    REQUIRE((it != live.end()));
    REQUIRE(it->second.refs > 0);
    --it->second.refs;
  }

  void clearRefs()
  {
    for (auto &[_, entry] : live)
      entry.refs = 0;
  }

  void retire()
  {
    for (auto it = live.begin(); it != live.end();)
      if (it->second.refs == 0)
      {
        cached[it->first] = Cached{it->second.ptr, epoch};
        it = live.erase(it);
      }
      else
        ++it;
  }
};

struct Harness
{
  Cache cache;
  Model model;

  Harness() { g_tracker.reset(); }

  void tick()
  {
    auto expected = model.tick();
    cache.tick();
    auto destroyed = g_tracker.take();
    eastl::sort(expected.begin(), expected.end());
    eastl::sort(destroyed.begin(), destroyed.end());
    CAPTURE(model.epoch, expected.size(), destroyed.size());
    REQUIRE((destroyed == expected));
    REQUIRE(!g_tracker.doubleDestroy);
    REQUIRE(cache.validate());
  }

  FakeRes *access(uint32_t key)
  {
    FakeRes *expected = model.expectedOnAccess(key);
    const uint32_t createdBefore = g_tracker.created;
    FakeRes *ptr = cache.access(key, [key]() { return create_fake(key); });
    REQUIRE(ptr != nullptr);
    REQUIRE(ptr->key == key);
    if (expected)
    {
      REQUIRE(ptr == expected);
      REQUIRE(g_tracker.created == createdBefore);
    }
    else
      REQUIRE(g_tracker.created == createdBefore + 1);
    model.accessed(key, ptr);
    REQUIRE(cache.validate());
    return ptr;
  }

  void drop(uint32_t key)
  {
    model.drop(key);
    cache.dropReference(key);
    REQUIRE(cache.validate());
  }

  void retire()
  {
    model.retire();
    cache.retireUnreferenced();
    REQUIRE(cache.validate());
  }

  void compact()
  {
    cache.compact();
    REQUIRE(cache.validate());
  }

  void recompileBegin()
  {
    tick();
    for (const auto &[key, entry] : model.live)
      for (uint32_t i = 0; i < entry.refs; ++i)
        cache.dropReference(key);
    model.clearRefs();
  }

  void idleFrames(uint32_t count)
  {
    for (uint32_t i = 0; i < count; ++i)
    {
      tick();
      retire();
    }
  }
};

} // namespace

TEST_CASE("access creates once and shares by key", "[resourceLruCache]")
{
  Harness h;
  FakeRes *a = h.access(1);
  FakeRes *b = h.access(1);
  CHECK(a == b);
  CHECK(g_tracker.created == 1);
  h.access(2);
  CHECK(g_tracker.created == 2);
}

TEST_CASE("shared entry survives while one reference remains", "[resourceLruCache]")
{
  Harness h;
  FakeRes *a = h.access(1);
  h.access(1);
  h.drop(1);
  h.retire();
  h.idleFrames(LIMIT * 2);
  CHECK(g_tracker.everDestroyed.empty());
  CHECK(h.access(1) == a);
}

TEST_CASE("retired entry expires exactly after Limit ticks", "[resourceLruCache]")
{
  Harness h;
  h.access(1);
  h.drop(1);
  h.retire();
  // ticks 1..LIMIT keep it, tick LIMIT + 1 destroys it; the harness checks each tick
  h.idleFrames(LIMIT);
  CHECK(g_tracker.everDestroyed.empty());
  h.idleFrames(1);
  CHECK(g_tracker.everDestroyed.size() == 1);
  // the list is now empty, more ticks must be harmless
  h.idleFrames(3);
}

TEST_CASE("cache hit revives retired entry without creation", "[resourceLruCache]")
{
  Harness h;
  FakeRes *a = h.access(1);
  h.drop(1);
  h.retire();
  h.idleFrames(LIMIT / 2);
  h.tick();
  CHECK(h.access(1) == a);
  CHECK(g_tracker.created == 1);
  h.retire();
  h.idleFrames(LIMIT * 2);
  CHECK(g_tracker.everDestroyed.empty());
  h.drop(1);
  h.retire();
  h.idleFrames(LIMIT + 1);
  CHECK(g_tracker.everDestroyed.size() == 1);
}

TEST_CASE("recompile keeps re-accessed entries and retires the rest", "[resourceLruCache]")
{
  Harness h;
  for (uint32_t k = 0; k < 10; ++k)
    h.access(k);
  h.retire();

  h.recompileBegin();
  for (uint32_t k = 0; k < 10; k += 2)
    h.access(k);
  h.retire();
  CHECK(g_tracker.created == 10);

  h.idleFrames(LIMIT + 1);
  CHECK(g_tracker.everDestroyed.size() == 5);

  // even keys are still live and shared
  for (uint32_t k = 0; k < 10; k += 2)
    h.access(k);
  CHECK(g_tracker.created == 10);
}

TEST_CASE("oscillating keys never recreate", "[resourceLruCache]")
{
  Harness h;
  const uint32_t keys[] = {10, 11, 12};
  uint32_t current = 0;
  h.access(keys[current]);
  h.retire();
  for (uint32_t frame = 0; frame < LIMIT * 8; ++frame)
  {
    h.tick();
    h.drop(keys[current]);
    current = (current + 1) % countof(keys);
    h.access(keys[current]);
    h.retire();
  }
  CHECK(g_tracker.created == countof(keys));
  CHECK(g_tracker.everDestroyed.empty());
}

TEST_CASE("compaction keeps lookups consistent", "[resourceLruCache]")
{
  Harness h;
  // enough churn to leave many free slots, then a compaction must remap everything
  constexpr uint32_t SLOTS = 2000;
  constexpr uint32_t VARIANTS = 3;
  dag::Vector<uint32_t> variant(SLOTS, 0);
  const auto keyOf = [](uint32_t slot, uint32_t v) { return slot * VARIANTS + v; };

  for (uint32_t s = 0; s < SLOTS; ++s)
    h.access(keyOf(s, 0));
  h.retire();

  for (uint32_t frame = 0; frame < LIMIT * 4; ++frame)
  {
    h.tick();
    for (uint32_t s = frame % 4; s < SLOTS; s += 4)
    {
      h.drop(keyOf(s, variant[s]));
      variant[s] = (variant[s] + 1) % VARIANTS;
      h.access(keyOf(s, variant[s]));
    }
    h.retire();
    if (frame % 8 == 7)
      h.compact();
  }
  // expire the whole cache so compaction has a lot to remove
  h.recompileBegin();
  for (uint32_t s = 0; s < SLOTS; s += 3)
    h.access(keyOf(s, variant[s]));
  h.retire();
  h.idleFrames(LIMIT + 1);
  h.compact();

  // every live slot must still resolve to the object the model remembers
  for (uint32_t s = 0; s < SLOTS; s += 3)
    h.access(keyOf(s, variant[s]));
  CHECK(!g_tracker.doubleDestroy);
}

TEST_CASE("failed creation does not poison the cache", "[resourceLruCache]")
{
  Cache cache;
  g_tracker.reset();
  FakeRes *res = cache.access(1u, []() { return UniqueFakeRes{}; });
  CHECK(res == nullptr);

  // production drops the reference on resize and re-accesses on recompile
  cache.dropReference(1u);
  cache.retireUnreferenced();
  cache.tick();
  cache.retireUnreferenced();

  FakeRes *retry = cache.access(1u, []() { return create_fake(1); });
  CHECK(retry != nullptr);
  CHECK(g_tracker.created == 1);
  cache.dropReference(1u);
  cache.retireUnreferenced();
  for (uint32_t i = 0; i < LIMIT + 1; ++i)
    cache.tick();
  CHECK(g_tracker.everDestroyed.size() == 1);
}

TEST_CASE("clear resets all state", "[resourceLruCache]")
{
  Harness h;
  h.access(1);
  h.access(2);
  h.drop(1);
  h.retire();
  h.idleFrames(2);

  h.cache.clear();
  REQUIRE(h.cache.validate());
  h.model = Model{};
  g_tracker.take();
  // production ticks right after clear on heap re-allocation
  h.idleFrames(3);

  h.access(1);
  h.drop(1);
  h.retire();
  h.idleFrames(LIMIT + 1);
  CHECK(g_tracker.everDestroyed.size() == 3);
}

TEST_CASE("random stress against the reference model", "[resourceLruCache]")
{
  dagor_random::set_rnd_seed(Catch::getSeed());
  Harness h;

  constexpr uint32_t TEXTURES = 48;
  constexpr uint32_t FRAMES_PER_TEXTURE = 2; // slots sharing one key, like the two scheduled frames
  constexpr uint32_t VARIANTS = 4;
  constexpr uint32_t SLOTS = TEXTURES * FRAMES_PER_TEXTURE;

  struct Slot
  {
    uint32_t texture;
    uint32_t variant = 0;
    bool active = false;
  };
  dag::Vector<Slot> slots;
  for (uint32_t s = 0; s < SLOTS; ++s)
    slots.push_back(Slot{s / FRAMES_PER_TEXTURE});
  const auto keyOf = [](const Slot &s) { return s.texture * VARIANTS + s.variant; };

  const auto recompile = [&]() {
    h.recompileBegin();
    for (auto &slot : slots)
    {
      slot.active = dagor_random::rnd_int(0, 7) != 0;
      if (!slot.active)
        continue;
      const uint32_t dynamic = dagor_random::rnd_int(0, VARIANTS - 1);
      slot.variant = dynamic;
      h.access(keyOf(slot));
    }
    h.retire();
    h.compact();
  };

  recompile();
  for (uint32_t frame = 0; frame < 3000; ++frame)
  {
    if (dagor_random::rnd_int(0, 60) == 0)
    {
      recompile();
      continue;
    }
    h.tick();
    const int resizeChance = dagor_random::rnd_int(0, 3); // bursts and quiet frames
    for (auto &slot : slots)
    {
      if (!slot.active || dagor_random::rnd_int(0, 3) >= resizeChance)
        continue;
      h.drop(keyOf(slot));
      slot.variant = dagor_random::rnd_int(0, VARIANTS - 1);
      h.access(keyOf(slot));
    }
    h.retire();
  }

  // drain: everything not live must die within Limit + 1 ticks
  h.recompileBegin();
  h.retire();
  h.idleFrames(LIMIT + 1);
  CHECK(h.model.cached.empty());
  CHECK(g_tracker.everDestroyed.size() == g_tracker.created);
  CHECK(!g_tracker.doubleDestroy);
}

TEST_CASE("eviction is capped per tick and stays in fifo order", "[resourceLruCache]")
{
  constexpr uint32_t CAP = 4;
  ResourceLruCache<uint32_t, FakeRes, FakeDestroyer, KeyHash, LIMIT, CAP> cache;
  g_tracker.reset();
  for (uint32_t k = 0; k < 10; ++k)
  {
    cache.access(k, [k]() { return create_fake(k); });
    cache.dropReference(k);
  }
  cache.retireUnreferenced();
  for (uint32_t i = 0; i < LIMIT; ++i)
    cache.tick();
  CHECK(g_tracker.everDestroyed.empty());

  const uint32_t expectedPerTick[] = {4, 4, 2, 0};
  uint32_t nextSerial = 1;
  for (uint32_t expected : expectedPerTick)
  {
    cache.tick();
    REQUIRE(cache.validate());
    auto destroyed = g_tracker.take();
    REQUIRE(destroyed.size() == expected);
    for (uint32_t serial : destroyed)
      CHECK(serial == nextSerial++);
  }
  CHECK(g_tracker.everDestroyed.size() == 10);
}

TEST_CASE("recompile every frame with new keys keeps the cache bounded", "[resourceLruCache]")
{
  constexpr uint32_t PROD_LIMIT = 128;
  constexpr uint32_t PROD_CAP = 8;
  constexpr uint32_t KEYS_PER_FRAME = 300;
  ResourceLruCache<uint32_t, FakeRes, FakeDestroyer, KeyHash, PROD_LIMIT, PROD_CAP> cache;
  g_tracker.reset();

  uint32_t nextKey = 0;
  for (uint32_t frame = 0; frame < 1000; ++frame)
  {
    cache.tick();
    for (uint32_t i = 0; i < KEYS_PER_FRAME; ++i, ++nextKey)
    {
      REQUIRE(cache.access(nextKey, [k = nextKey]() { return create_fake(k); }) != nullptr);
      cache.dropReference(nextKey);
    }
    cache.retireUnreferenced();
    cache.compact();
    cache.tick();

    const size_t resident = g_tracker.created - g_tracker.everDestroyed.size();
    CAPTURE(frame, resident);
    REQUIRE(resident <= KEYS_PER_FRAME * (PROD_LIMIT / 2 + 8));
  }
  REQUIRE(cache.validate());
  CHECK(!g_tracker.doubleDestroy);
}

TEST_CASE("a kept entry outlives the limit and does not hold up newer ones", "[resourceLruCache]")
{
  Cache cache;
  g_tracker.reset();
  cache.access(1, []() { return create_fake(1); });
  cache.access(2, []() { return create_fake(2); });
  cache.dropReference(1, 3 * LIMIT);
  cache.dropReference(2);
  cache.retireUnreferenced();

  for (uint32_t i = 0; i < LIMIT + 1; ++i)
    cache.tick();
  REQUIRE(cache.validate());
  CHECK((g_tracker.take() == dag::Vector<uint32_t>{2}));

  for (uint32_t i = LIMIT + 1; i < 3 * LIMIT; ++i)
    cache.tick();
  REQUIRE(cache.validate());
  CHECK(g_tracker.take().empty());

  bool created = false;
  cache.access(1, [&]() {
    created = true;
    return create_fake(1);
  });
  CHECK_FALSE(created);
  cache.dropReference(1);
  cache.retireUnreferenced();
  for (uint32_t i = 0; i < LIMIT + 1; ++i)
    cache.tick();
  REQUIRE(cache.validate());
  CHECK((g_tracker.take() == dag::Vector<uint32_t>{1}));
}

TEST_CASE("a kept entry is destroyed once its keep time is over", "[resourceLruCache]")
{
  Cache cache;
  g_tracker.reset();
  cache.access(1, []() { return create_fake(1); });
  cache.dropReference(1, 3 * LIMIT);
  cache.retireUnreferenced();
  for (uint32_t i = 0; i < 3 * LIMIT; ++i)
    cache.tick();
  CHECK(g_tracker.everDestroyed.empty());
  cache.tick();
  REQUIRE(cache.validate());
  CHECK(g_tracker.everDestroyed.size() == 1);
}
