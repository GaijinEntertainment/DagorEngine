// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <textureGen/textureDataCache.h>

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <ska_hash_map/flat_hash_map2.hpp>


namespace regcache
{
uint64_t memory_limit = 2ull << 30; // 2GB
uint64_t current_memory_usage = 0;
uint64_t current_index = 1;

eastl::hash_map<eastl::string, eastl::vector<uint8_t>> cache;
ska::flat_hash_set<eastl::string> pinned;

void clear()
{
  cache.clear();
  current_memory_usage = 0;
}

bool is_record_exists(const char *key) { return cache.find(eastl::string(key)) != cache.end(); }

static void set_index(eastl::vector<uint8_t> &v)
{
  current_index++;
  memcpy(v.data(), &current_index, sizeof(uint64_t));
}

void *get_record_data_ptr(const char *key)
{
  auto it = cache.find(eastl::string(key));
  if (it == cache.end())
    return nullptr;
  set_index(it->second);
  return (void *)(it->second.data() + sizeof(uint64_t));
}

static void evict_over_limit(const eastl::string *keep)
{
  while (current_memory_usage > memory_limit)
  {
    auto bestIt = cache.end();
    uint64_t minIndex = size_t(-1ll);
    for (auto it = cache.begin(); it != cache.end(); ++it)
    {
      if ((keep && it->first == *keep) || pinned.count(it->first))
        continue;
      uint64_t index = 0;
      memcpy(&index, it->second.data(), sizeof(index));
      if (index < minIndex)
      {
        minIndex = index;
        bestIt = it;
      }
    }

    if (bestIt == cache.end())
      break;
    current_memory_usage -= bestIt->second.size();
    cache.erase(bestIt);
  }
}

void *create_record(const char *key, size_t size)
{
  eastl::string name(key);
  eastl::vector<uint8_t> &record = cache[name];
  current_memory_usage -= record.size();
  if (record.size() != size + sizeof(uint64_t))
    record = eastl::vector<uint8_t>(size + sizeof(uint64_t));
  set_index(record);
  current_memory_usage += record.size();
  // The caller fills the new record right after this call, so it is never evicted here.
  evict_over_limit(&name);
  return record.data() + sizeof(uint64_t);
}

void pin(const char *key) { pinned.insert(eastl::string(key)); }

void unpin_all() { pinned.clear(); }

void update() { evict_over_limit(nullptr); }
} // namespace regcache
