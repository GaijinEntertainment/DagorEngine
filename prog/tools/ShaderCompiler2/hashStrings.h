// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <util/dag_stdint.h>
#include <ska_hash_map/flat_hash_map2.hpp>

class HashStrings
{
public:
  int addNameId(const char *name)
  {
    uint64_t hash = hash_eastl((const unsigned char *)name);
    int count = names.size();
    auto it = hashTable.emplace(hash, count);
    if (it.second) // new item
      names.push_back() = name;
    return it.first->second;
  }
  int getNameId(const char *name) const
  {
    uint64_t hash = hash_eastl((const unsigned char *)name);
    auto it = hashTable.find(hash);
    if (it == hashTable.end())
      return -1;
    return it->second;
  }
  const char *getName(uint32_t id) const
  {
    if (id < names.size())
      return names.data()[id];
    return NULL;
  }
  void reset()
  {
    clear_and_shrink(names);
    hashTable.clear();
  }
  int nameCount() const { return names.size(); }
  HashStrings(int c = 0) { hashTable.reserve(c); }

protected:
  ska::flat_hash_map<uint64_t, int> hashTable;

  Tab<String> names;
  static inline uint64_t hash_eastl(const unsigned char *p)
  {
    uint64_t c, result = 2166136261ULL; // We implement an FNV-like string hash.
    while ((c = *p++) != 0)             // Using '!=' disables compiler warnings.
      result = (result * 16777619ULL) ^ c;
    return result;
  }
};
