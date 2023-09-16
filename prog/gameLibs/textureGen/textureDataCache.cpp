#include <textureGen/textureDataCache.h>

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>


namespace regcache
{
uint64_t memory_limit = 2ull << 30; // 2GB
uint64_t current_memory_usage = 0;
uint64_t current_index = 1;

eastl::hash_map<eastl::string, eastl::vector<uint8_t>> cache;

void clear() { cache.clear(); }

bool is_record_exists(const char *key) { return cache.find(eastl::string(key)) != cache.end(); }

void *get_record_data_ptr(const char *key)
{
  auto it = cache.find(eastl::string(key));
  return (it == cache.end()) ? nullptr : (void *)(it->second.data() + sizeof(uint64_t));
}

void set_record_data(const char *key, void *data, size_t size)
{
  auto it = cache.find(eastl::string(key));
  if (it != cache.end())
    cache.erase(it);

  eastl::vector<uint8_t> v;
  v.resize(size + sizeof(uint64_t));
  memcpy(v.data() + sizeof(uint64_t), data, size);
  current_index++;
  memcpy(v.data(), &current_index, sizeof(uint64_t));
  cache[eastl::string(key)] = eastl::move(v);
}

void update()
{
  current_memory_usage = 0;
  for (auto &&v : cache)
    current_memory_usage += v.second.size();

  while (current_memory_usage > memory_limit)
  {
    auto bestIt = cache.end();
    uint64_t minIndex = size_t(-1ll);
    for (auto it = cache.begin(); it != cache.end(); ++it)
    {
      uint64_t index = 0;
      memcpy(&index, it->second.data(), sizeof(index));
      if (index < minIndex)
      {
        minIndex = index;
        bestIt = it;
      }
    }

    if (bestIt != cache.end())
    {
      current_memory_usage -= bestIt->second.size();
      cache.erase(bestIt);
    }
  }
}
} // namespace regcache
