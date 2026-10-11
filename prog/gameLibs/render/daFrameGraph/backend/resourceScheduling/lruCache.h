// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>

template <class K, class V, class Hasher, uint32_t Limit>
class LruCache
{
  static_assert(Limit < UINT32_MAX / 2);

public:
  template <class F>
  const V &access(const K &key, const F &creator)
  {
    auto [it, emplaced] = cache.emplace(key, eastl::pair<V, uint32_t>{});
    if (emplaced)
      it->second.first = creator();
    it->second.second = generation;
    return it->second.first;
  }

  void touch(const K &key)
  {
    auto it = cache.find(key);
    G_ASSERT(it != cache.end());
    it->second.second = generation;
  }

  void lruCleanup()
  {
    ++generation;

    for (auto it = cache.begin(); it != cache.end();)
      if (it->second.second + Limit < generation)
        it = cache.erase(it);
      else
        ++it;

    if (cache.size() * 2 < cache.bucket_count())
      cache.shrink_to_fit();

    if (generation >= 2 * Limit)
    {
      for (auto it = cache.begin(); it != cache.end(); ++it)
      {
        G_ASSERT(it->second.second >= Limit && it->second.second < generation);
        it->second.second -= Limit;
      }
      generation -= Limit;
    }
  }

  void clear() { cache.clear(); }

  size_t getMemoryUsage() const { return cache.bucket_count() * sizeof(decltype(cache)::value_type); }

private:
  ska::flat_hash_map<K, eastl::pair<V, uint32_t>, Hasher> cache;
  uint32_t generation = 0;
};

// This class is an more optimized lru cache impl for unique-ptr held resources which are
// 1) slow to create, so should be cached for reuse
// 2) take up a bunch of host ram, so should not be kept around indefinitely
// 3) slow to delete, so deletion should be amortized
// And for
// 1) Fast per-frame management
// 2) Fast(ish) codepath for full live resource set rebuild (scheduling)
// 3) Fast(ish) codepath for burst of replacements (texture resize batch)
//
// Unlike LruCache, we keep a flat entry array + a hash map key->index for lookups.
//
// Each array entry is in one of three states:
//  live (the resource is currently used)
//  retired (the resource is not used, but not deleted yet, and can be reused)
//  empty (the resident resource is deleted, the slot can be reused for a new resource)
//
// Live resources have a ref count to understand when they should be retired (may be >1 for resources shared between frames)
// Retired resources have a retiredAt time to know when to evict them, and are connected into a doubly linked list ordered by age
// Empty resources are connected into a singly linked free list, separate from the retired list
//
// The user keeps pointers to resources from the unique pointers for their usual work
//
// Per frame management is done by tick() -- it walks the retired list from oldest to newest and evicts (deletes the res and moves to
// empty state) all slots with age over Limit, but with a non-zero MaxEvictionsPerTick not more than
// max(MaxEvictionsPerTick, retired count / Limit) per tick (resource requirement 3).
// It is optimized to do the least amount of work for bookkeeping
// (as opposed to LruCache, which walks the full hash map AND requires to refresh all live resources).
//
// Rescheduling and resizing are done by
//  calling dropReference() on every old texture key
//  calling access() on every new one
//  calling retireUnreferenced() once after all of them
//  (optional) calling compact() to control memory footprint (up to 2 pool walks, hash table walk and rebuild, fires rarely)

template <class K, class R, class RDestroyer, class Hasher, uint32_t Limit, uint32_t MaxEvictionsPerTick = 0>
class ResourceLruCache
{
  static_assert(Limit != 0);
  using Index = uint16_t;
  static constexpr Index NONE = 0xFFFF;

  struct Entry
  {
    eastl::unique_ptr<R, RDestroyer> res;
    K key;
    uint32_t retiredAt = 0;
    uint32_t keepUntil = 0;
    Index older = NONE;
    Index newer = NONE;
    uint16_t liveRefs = 0;
    bool retired = false;
  };

  dag::Vector<Entry> pool;
  ska::flat_hash_map<K, Index, Hasher> lookupTable;
  dag::Vector<Index> unreferenced;
  Index oldest = NONE;
  Index newest = NONE;
  Index freeHead = NONE;
  uint32_t retiredCount = 0;
  uint32_t epoch = 0;

public:
  template <class F>
  R *access(const K &key, const F &creator)
  {
    auto [it, emplaced] = lookupTable.emplace(key, NONE);
    if (emplaced)
    {
      auto res = creator();
      const Index idx = res ? allocSlot() : NONE;
      if (DAGOR_UNLIKELY(idx == NONE))
      {
        // a null entry would be taken for a free slot
        lookupTable.erase(it);
        return nullptr;
      }
      it->second = idx;
      Entry &entry = pool[idx];
      entry.res = eastl::move(res);
      entry.key = key;
      entry.liveRefs = 1;
      return entry.res.get();
    }

    Entry &entry = pool[it->second];
    if (entry.retired)
    {
      unlinkRetired(it->second);
      entry.retired = false;
      entry.keepUntil = 0;
      G_ASSERT(entry.liveRefs == 0);
    }
    ++entry.liveRefs;
    return entry.res.get();
  }

  // A retired entry lives at least Limit ticks and at least keep_ticks
  void dropReference(const K &key, uint32_t keep_ticks = 0)
  {
    auto it = lookupTable.find(key);
    if (DAGOR_UNLIKELY(it == lookupTable.end()))
      return; // creation failed for this key, nothing was inserted
    Entry &entry = pool[it->second];
    G_ASSERT_RETURN(entry.liveRefs > 0 && !entry.retired, );
    if (keep_ticks > 0)
      entry.keepUntil = eastl::max(entry.keepUntil, epoch + keep_ticks);
    if (--entry.liveRefs == 0)
      unreferenced.push_back(it->second);
  }

  void retireUnreferenced()
  {
    for (Index idx : unreferenced)
    {
      Entry &entry = pool[idx];
      if (entry.liveRefs > 0 || entry.retired || !entry.res)
        continue;
      entry.retired = true;
      entry.retiredAt = epoch;
      linkNewest(idx);
    }
    unreferenced.clear();
  }

  void tick()
  {
    ++epoch;
    const uint32_t budget = eastl::max(MaxEvictionsPerTick, retiredCount / Limit);
    uint32_t evicted = 0;
    for (Index idx = oldest; idx != NONE;)
    {
      Entry &entry = pool[idx];
      if (epoch - entry.retiredAt <= Limit)
        break;
      const Index next = entry.newer;
      if (epoch <= entry.keepUntil)
      {
        idx = next;
        continue;
      }
      evict(idx);
      if (MaxEvictionsPerTick != 0 && ++evicted >= budget)
        break;
      idx = next;
    }
  }

  void compact()
  {
    const size_t used = lookupTable.size();

    if (lookupTable.bucket_count() >= 256 && used * 2 < lookupTable.bucket_count())
      lookupTable.shrink_to_fit();

    if (pool.size() < 256 || used * 2 > pool.size())
      return;

    G_ASSERT(unreferenced.empty());

    dag::Vector<Index, framemem_allocator> remap(pool.size(), NONE);
    Index next = 0;
    for (size_t i = 0, n = pool.size(); i < n; ++i)
    {
      if (!pool[i].res)
        continue;
      remap[i] = next;
      if (next != i)
        pool[next] = eastl::move(pool[i]);
      ++next;
    }
    pool.resize(next);
    pool.shrink_to_fit();

    const auto fix = [&remap](Index &idx) {
      if (idx != NONE)
        idx = remap[idx];
    };
    for (Entry &entry : pool)
    {
      fix(entry.older);
      fix(entry.newer);
    }
    for (auto &[_, idx] : lookupTable)
      fix(idx);
    fix(oldest);
    fix(newest);
    freeHead = NONE;
  }

  void clear()
  {
    pool.clear();
    lookupTable.clear();
    unreferenced.clear();
    oldest = newest = freeHead = NONE;
    retiredCount = 0;
    epoch = 0;
  }

  // Self check for tests and debugging
  bool validate() const
  {
    size_t retiredListed = 0;
    uint32_t lastRetiredAt = 0;
    Index prev = NONE;
    for (Index idx = oldest; idx != NONE; idx = pool[idx].newer)
    {
      const Entry &entry = pool[idx];
      if (++retiredListed > pool.size() || !entry.retired || !entry.res || entry.liveRefs != 0)
        return false;
      if (entry.older != prev || entry.retiredAt < lastRetiredAt)
        return false;
      lastRetiredAt = entry.retiredAt;
      prev = idx;
    }
    if (prev != newest || retiredListed != retiredCount)
      return false;
    size_t freeCount = 0;
    for (Index idx = freeHead; idx != NONE; idx = pool[idx].older)
      if (++freeCount > pool.size() || pool[idx].res)
        return false;

    size_t used = 0, retiredFlagged = 0;
    for (const Entry &entry : pool)
    {
      if (entry.res)
      {
        ++used;
        retiredFlagged += entry.retired ? 1 : 0;
      }
      else if (entry.retired || entry.liveRefs != 0)
        return false;
    }
    if (retiredFlagged != retiredListed || used + freeCount != pool.size() || lookupTable.size() != used)
      return false;

    for (const auto &[key, idx] : lookupTable)
      if (idx >= pool.size() || !pool[idx].res || !(pool[idx].key == key))
        return false;
    return true;
  }

  void dumpPoolState(auto &&printer) const
  {
    printer("[0, %d]", pool.size());
    for (size_t i = 0, n = pool.size(); i < n; ++i)
    {
      const Entry &entry = pool[i];
      if (!entry.res)
        printer("[%d]: empty, older=%d lr=%d", i, entry.older, entry.liveRefs);
      else if (entry.retired)
        printer("[%d]: retired at %d, older=%d newer=%d lr=%d", i, entry.retiredAt, entry.older, entry.newer, entry.liveRefs);
      else
        printer("[%d]: live, lr=%d", i, entry.liveRefs);
    }
  }

private:
  Index allocSlot()
  {
    if (freeHead != NONE)
    {
      const Index idx = freeHead;
      freeHead = pool[idx].older;
      pool[idx].older = NONE;
      return idx;
    }
    G_ASSERT_RETURN(pool.size() < NONE, NONE);
    pool.push_back();
    return Index(pool.size() - 1);
  }

  void linkNewest(Index idx)
  {
    Entry &entry = pool[idx];
    entry.older = newest;
    entry.newer = NONE;
    if (newest != NONE)
      pool[newest].newer = idx;
    else
      oldest = idx;
    newest = idx;
    ++retiredCount;
  }

  void unlinkRetired(Index idx)
  {
    Entry &entry = pool[idx];
    if (entry.older != NONE)
      pool[entry.older].newer = entry.newer;
    else
      oldest = entry.newer;
    if (entry.newer != NONE)
      pool[entry.newer].older = entry.older;
    else
      newest = entry.older;
    entry.older = entry.newer = NONE;
    --retiredCount;
  }

  void evict(Index idx)
  {
    Entry &entry = pool[idx];
    unlinkRetired(idx);
    entry.retired = false;
    lookupTable.erase(entry.key);
    entry.res.reset();
    entry.older = freeHead;
    freeHead = idx;
  }
};
