//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <memory/dag_mem.h>
#include <generic/dag_tabSort.h>
#include <osApiWrappers/dag_localConv.h>
#include <string.h>
#include <util/dag_stdint.h>


/// Fast string map (add string/strId, get strId by string)
template <typename T, intptr_t invalidId = -1>
class FastStrMapT
{
public:
  typedef T IdType;
  struct Entry
  {
    const char *name;
    IdType id;
  };

  struct LambdaStrcmp
  {
    static int order(const Entry &a, const Entry &b) { return strcmp(a.name, b.name); }
  };

  struct LambdaStricmp
  {
    static int order(const Entry &a, const Entry &b) { return dd_stricmp(a.name, b.name); }
  };


public:
  FastStrMapT(IMemAlloc *mem = NULL, bool ignore_case = false) : fastMap(mem ? mem : tmpmem) { ignoreCase = ignore_case; }
  FastStrMapT(const FastStrMapT &m) : fastMap(m.fastMap), ignoreCase(false)
  {
    for (int i = 0; i < fastMap.size(); i++)
      fastMap[i].name = str_dup(fastMap[i].name, dag::get_allocator(fastMap));
  }
  ~FastStrMapT() { reset(false); }

  FastStrMapT &operator=(const FastStrMapT &m)
  {
    reset();
    fastMap = m.fastMap;
    for (int i = 0; i < fastMap.size(); i++)
      fastMap[i].name = str_dup(fastMap[i].name, dag::get_allocator(fastMap));
    return *this;
  }

  /// Returns strId for given name or invalidId if name not found.
  IdType getStrId(const char *name) const
  {
    int n = getStrIndex(name);
    if (n == -1)
      return (IdType)invalidId;
    return fastMap[n].id;
  }

  /// Returns strId for given name (adds str to the list if not found)
  IdType addStrId(const char *str, IdType id, const char *&stored_str)
  {
    int str_index = getStrIndex(str);
    if (str_index < 0)
    {
      Entry key = {str_dup(str, dag::get_allocator(fastMap)), id};
      if (ignoreCase)
        fastMap.insert(key, LambdaStricmp());
      else
        fastMap.insert(key, LambdaStrcmp());

      stored_str = key.name;
      return id;
    }

    stored_str = fastMap[str_index].name;
    return fastMap[str_index].id;
  }

  /// Returns strId for given name (adds str to the list if not found)
  IdType addStrId(const char *str, IdType id)
  {
    IdType str_id = getStrId(str);
    if (str_id == (IdType)invalidId)
    {
      str_id = id;
      Entry key = {str_dup(str, dag::get_allocator(fastMap)), str_id};
      if (ignoreCase)
        fastMap.insert(key, LambdaStricmp());
      else
        fastMap.insert(key, LambdaStrcmp());
    }

    return str_id;
  }

  /// Set strId for given name (adds str to the list if not found and returns true)
  bool setStrId(const char *str, IdType id)
  {
    int idx = getStrIndex(str);

    if (idx == -1)
    {
      Entry key = {str_dup(str, dag::get_allocator(fastMap)), id};
      if (ignoreCase)
        fastMap.insert(key, LambdaStricmp());
      else
        fastMap.insert(key, LambdaStrcmp());
      return true;
    }
    else
    {
      fastMap[idx].id = id;
      return false;
    }
  }

  bool delStrId(const char *str)
  {
    int idx = getStrIndex(str);
    if (idx == -1)
      return false;
    memfree((void *)fastMap[idx].name, dag::get_allocator(fastMap));
    erase_items(fastMap, idx, 1);
    return true;
  }

  bool delStrId(IdType str_id)
  {
    int cnt = fastMap.size();
    for (int i = fastMap.size() - 1; i >= 0; i--)
      if (fastMap[i].id == str_id)
      {
        memfree((void *)fastMap[i].name, dag::get_allocator(fastMap));
        erase_items(fastMap, i, 1);
      }
    return fastMap.size() < cnt;
  }

  /// returns number of strings
  int strCount() const { return fastMap.size(); }

  /// Resets strmap to initial state (all previously issued ids become invalid)
  void reset(bool erase_only = true)
  {
    for (int i = fastMap.size() - 1; i >= 0; i--)
      memfree((void *)fastMap[i].name, dag::get_allocator(fastMap));

    erase_only ? fastMap.clear() : clear_and_shrink(fastMap);
  }

  /// Reserves memory for at least add_num additinal strs
  void reserve(int add_num) { fastMap.reserve(fastMap.size() + add_num); }

  /// Returns non-modifiable slice of entries
  dag::ConstSpan<Entry> getMapRaw() const { return fastMap; }

  /// Returns index in map array for given str or -1 if str not found.
  int getStrIndex(const char *str) const
  {
    Entry key = {str, (IdType)invalidId};
    if (ignoreCase)
      return fastMap.find(key, LambdaStricmp());
    else
      return fastMap.find(key, LambdaStrcmp());
  }

  /// Walks through all map to find str_id (slow); returns NULL when str_id is not found
  const char *getStrSlow(IdType str_id) const
  {
    for (const Entry &e : fastMap)
      if (e.id == str_id)
        return e.name;
    return NULL;
  }

protected:
  TabSortedFast<Entry> fastMap;
  bool ignoreCase;
};

#ifdef _TARGET_STATIC_LIB
extern template class FastStrMapT<int, -1>;
extern template class FastStrMapT<int, 0>;
extern template class FastStrMapT<char *, 0>;
#endif

typedef FastStrMapT<int, -1> FastStrMap;
typedef FastStrMapT<char *, 0> FastStrStrMap_;

#include <supp/dag_define_KRNLIMP.h>

class FastStrStrMap : public FastStrStrMap_
{
public:
  KRNLIMP FastStrStrMap(IMemAlloc *mem = NULL, bool ignore_case = false);
  KRNLIMP FastStrStrMap(const FastStrStrMap &m);
  KRNLIMP ~FastStrStrMap();

  /// Returns strId for given name (adds str to the list if not found)
  KRNLIMP IdType addStrId(const char *str, IdType id);

  /// Set strId for given name (adds str to the list if not found and returns true)
  KRNLIMP bool setStrId(const char *str, IdType id);

  KRNLIMP bool delStrId(const char *str);

  KRNLIMP bool delStrId(IdType str_id);

  KRNLIMP void reset(bool erase_only = true);
};

#include <supp/dag_undef_KRNLIMP.h>
