// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>

template <class K, class V, class Hasher, uint32_t Limit>
class LruCache
{
  static_assert(Limit < UINT32_MAX / 2);

public:
  template <class F>
  const V &access(const K &key, const F &creator)
  {
    auto it = cache.find(key);
    if (it == cache.end())
      it = cache.emplace(key, eastl::pair<V, uint32_t>{creator(), generation}).first;
    else
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