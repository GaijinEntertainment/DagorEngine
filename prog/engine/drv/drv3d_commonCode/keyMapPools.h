// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <EASTL/type_traits.h>
#include <EASTL/fixed_vector.h>
#include <generic/dag_tab.h>


__forceinline uint32_t hash32shiftmult(uint32_t key)
{
  key = (key ^ 61) ^ (key >> 16);
  key = key + (key << 3);
  key = key ^ (key >> 4);
  key = key * 0x27d4eb2d; // a prime or an odd constant
  key = key ^ (key >> 15);
  return key;
}
// keytype must be integer, value 0 is reserved
template <typename T, typename KeyType, size_t N = 0>
struct KeyMap
{
  static constexpr int INVALID_KEY = 0;

  struct Entry
  {
    KeyType key = INVALID_KEY;
    T val;

    Entry(KeyType &k, T &v) : key(k), val(v) {}
  };

  typename eastl::conditional<N == 0, eastl::vector<Entry>, eastl::fixed_vector<Entry, N>>::type table;

  KeyMap() = default;

  void clear()
  {
    decltype(table) t;
    t.swap(table);
    for (auto &e : t)
    {
      if (e.key != INVALID_KEY)
        e.val.destroyObject();
    }
  }
  ~KeyMap() { clear(); }
  bool empty() const { return table.empty(); }
  size_t size() const { return table.size(); }

  /*//return true if killed something
  bool kill(KeyType mask, KeyType match)
  {
    bool killed = false;
    for (int i=0; i<table.size(); i++)
    {
      if ((table[i].key & mask) == match)
      {
        table[i].key = INVALID_KEY;
        table[i].val.destroyObject();
        killed = true;
      }
    }
    return killed;
  }*/

  __forceinline T *find(KeyType key)
  {
    for (int index = (int)table.size() - 1; index >= 0; index--)
    {
      Entry &e = table[index];
      if (e.key == key)
        return &e.val;
    }
    return nullptr;
  }

  void addNoCheck(KeyType key, T &v, int /*step*/ = 8)
  {
    G_ASSERT(key != INVALID_KEY);
    table.emplace_back(key, v);
  }

  bool add(const KeyType &key, T &v, int /*step*/ = 8)
  {
    addNoCheck(key, v, 0); // TODO check key duplication/allocation
    return true;
  }
};

template <typename T, typename KeyType, unsigned Buckets>
struct KeyMapHashed
{
  static constexpr int BUCKETS = Buckets;
  static constexpr int INVALID_KEY = 0;

  struct Entry
  {
    KeyType key;
    T val = T();

    Entry() : key(INVALID_KEY) {}
  };

  Tab<Entry> table[Buckets];

  KeyMapHashed() {}

  void clear()
  {
    for (int j = 0; j < Buckets; j++)
    {
      for (Entry &e : table[j])
        if (e.key != INVALID_KEY)
          e.val.destroyObject();
      clear_and_shrink(table[j]);
    }
  }
  ~KeyMapHashed() { clear(); }
  size_t size() const
  {
    size_t sz = 0;
    for (int i = 0; i < Buckets; i++)
      sz += table[i].size();
    return sz;
  }

  __forceinline T *find(KeyType key)
  {
    uint32_t bucketNo = hash32shiftmult(key) % Buckets;
    for (Entry &e : table[bucketNo])
      if (e.key == key)
        return &e.val;
    return NULL;
  }

  void addNoCheck(KeyType key, T &v)
  {
    G_ASSERT(key != INVALID_KEY);
    Entry e;
    e.key = key;
    e.val = v;
    uint32_t bucketNo = hash32shiftmult(key) % Buckets;
    table[bucketNo].push_back(e);
  }

  void add(const KeyType &key, T &v)
  {
    G_ASSERT(!find(key));
    addNoCheck(key, v); // TODO check key duplication/allocation
  }
};

// keytype must be integer, value 0 is reserved
template <typename T, typename KeyType>
struct KeyMapWide
{
  static constexpr int INVALID_KEY = 0;

  struct Entry
  {
    KeyType key;
    T val;

    Entry() { memset(&key, INVALID_KEY, sizeof(key)); }
  };

  Tab<Entry> table;
  size_t size() const { return table.size(); }

  uint32_t cmpk(const KeyType *key_ptr) const
  {
    uint32_t diff = 0;
    const uint8_t *ptr = (const uint8_t *)(key_ptr);
    for (size_t i = 0; i < sizeof(*key_ptr); i++)
      diff |= ptr[i] ^ INVALID_KEY;
    return diff;
  }

  KeyMapWide() : table(midmem) {}

  void clear()
  {
    for (int i = 0; i < table.size(); i++)
    {
      if (cmpk(&table[i].key) != 0)
        table[i].val.destroyObject();
    }

    clear_and_shrink(table);
  }

  ~KeyMapWide() { clear(); }

  __forceinline T *find(const KeyType &key) const
  {
    for (Entry &e : table)
      if (memcmp(&e.key, &key, sizeof(key)) == 0)
        return &e.val;
    return NULL;
  }

  void addNoCheck(const KeyType &key, T &v)
  {
    G_ASSERT(cmpk(&key) != 0);
    Entry e;
    e.key = key;
    e.val = v;
    table.push_back(e);
  }

  bool add(const KeyType &key, T &v)
  {
    G_ASSERT(!find(key));
    addNoCheck(key, v); // TODO check key duplication/allocation
    return true;
  }
};

// keytype must be integer, value 0 is reserved
template <typename T, typename KeyType, unsigned Buckets>
struct KeyMapWideHashed
{
  static constexpr int BUCKETS = Buckets;
  static constexpr int INVALID_KEY = 0;

  struct Entry
  {
    KeyType key;
    T val = T();

    Entry() { memset(&key, INVALID_KEY, sizeof(key)); }
  };

  Tab<Entry> table[Buckets];

  size_t size() const
  {
    size_t sz = 0;
    for (int i = 0; i < Buckets; i++)
      sz += table[i].size();
    return sz;
  }

  uint32_t cmpk(const KeyType *key_ptr) const
  {
    uint32_t diff = 0;
    const uint8_t *ptr = (const uint8_t *)(key_ptr);
    for (size_t i = 0; i < sizeof(*key_ptr); i++)
      diff |= ptr[i] ^ INVALID_KEY;
    return diff;
  }

  KeyMapWideHashed() {}

  void clear()
  {
    for (int j = 0; j < Buckets; j++)
    {
      for (Entry &e : table[j])
        if (cmpk(&e.key) != 0)
          e.val.destroyObject();
      clear_and_shrink(table[j]);
    }
  }

  ~KeyMapWideHashed() { clear(); }

  __forceinline T *find(const KeyType &key)
  {
    uint32_t bucketNo = key.getHash() % Buckets;
    for (Entry &e : table[bucketNo])
    {
      if (memcmp(&e.key, &key, sizeof(key)) == 0) //-V1014
        return &e.val;
    }
    return NULL;
  }

  void addNoCheck(const KeyType &key, T &v)
  {
    G_ASSERT(cmpk(&key) != 0);
    Entry e;
    e.key = key;
    e.val = v;
    uint32_t bucketNo = key.getHash() % Buckets;
    table[bucketNo].push_back(e);
  }

  bool add(const KeyType &key, T &v)
  {
    G_ASSERT(!find(key));
    addNoCheck(key, v); // TODO check key duplication/allocation
    return true;
  }
};
