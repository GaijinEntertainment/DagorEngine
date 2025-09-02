//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/utility.h>
#include <EASTL/string.h>
#include <EASTL/algorithm.h>
#include <EASTL/type_traits.h>
#include <ska_hash_map/flat_hash_map2.hpp>


#if EASTL_VERSION_N < 31903
namespace eastl
{
inline bool operator==(const char *a, const eastl::string_view &b) { return strcmp(a, b.data()) == 0; }
} // namespace eastl
#endif

//
// This is by design RO, append-only, mostly read container, ideal for WORM-like ("write once read many") usage patterns
//
// It's better then (old) FastNameMap because:
//  * O(1) (on average) vs O(log(N)) with very slow string compares
//  * No CI branch in runtime
// It's better then HashNameMap because:
//  * Rehashing
//  * Open adressing hashmap collision resolution (instead of chaining), which is way more cache friendlier (especially in robin-good
//  flavour)
//  * Better (arguably) hash function (FNV1 instead of djb)
//
template <typename Str = eastl::string>
class FastHashNameMapT
{
public:
  using NameIdsType = ska::flat_hash_map<Str, int, ska::power_of_two_std_hash<Str>>;

  typename NameIdsType::const_iterator begin() const { return nameIds.begin(); }
  typename NameIdsType::const_iterator end() const { return nameIds.end(); }

  eastl::pair<int, bool> insert(const char *name, int len = -1) // return true in second element of pair if was inserted (STL
                                                                // insert-like bhv)
  {
    auto it = nameIds.find_as(name, eastl::hash<const char *>(), eastl::equal_to<>());
    if (it != nameIds.end())
      return eastl::make_pair(it->second, false);
    int newNameId = (int)nameIds.size();
    nameIds.emplace(Str(name, len < 0 ? (int)strlen(name) : len), newNameId);
    return eastl::make_pair(newNameId, true);
  }
  size_t size() const { return nameIds.size(); }
  int getNameId(const char *name) const
  {
    auto it = nameIds.find_as(name, eastl::hash<const char *>(), eastl::equal_to<>());
    return it != nameIds.end() ? it->second : -1;
  }
  int getNameId(const Str &name) const
  {
    auto it = nameIds.find(name);
    return it != nameIds.end() ? it->second : -1;
  }
  const char *getNameSlow(int id) const
  {
    auto pred = [id](const typename decltype(nameIds)::value_type &kv) { return kv.second == id; };
    auto it = eastl::find_if(nameIds.begin(), nameIds.end(), pred);
    return it != nameIds.end() ? it->first.data() : NULL;
  }
  void clear() { decltype(nameIds)().swap(nameIds); }
  int addNameId(const char *name, int len = -1) { return insert(name, len).first; }
  int nameCount() const { return (int)size(); }

private:
  NameIdsType nameIds;
};

using FastHashNameMap = FastHashNameMapT<eastl::string>;
