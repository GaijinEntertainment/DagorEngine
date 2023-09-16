//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tabSort.h>

class FastIntList
{
public:
  FastIntList(IMemAlloc *allocator = midmem) : list(allocator) {}

  int size() const { return list.size(); }
  int operator[](int idx) const { return list[idx]; }

  bool empty() const { return list.empty(); }
  bool hasInt(int key) const
  {
    int n = list.find(key);
    return n != -1;
  }

  bool addInt(int key)
  {
    if (hasInt(key))
      return false;
    list.insert(key);
    return true;
  }
  bool addIntFiltered(int key, int bad_key) { return key != bad_key ? addInt(key) : false; }
  int addIntList(const FastIntList &l)
  {
    int added = false;
    for (int i = 0; i < l.list.size(); i++)
      if (addInt(l.list[i]))
        added++;
    return added;
  }

  bool delInt(int key)
  {
    int n = list.find(key);
    if (n < 0)
      return false;
    erase_items(list, n, 1);
    return true;
  }
  int delIntList(const FastIntList &l)
  {
    int removed = false;
    for (int i = 0; i < l.list.size(); i++)
      if (delInt(l.list[i]))
        removed++;
    return removed;
  }

  bool hasAny(const FastIntList &l) const
  {
    for (int i = 0; i < l.list.size(); i++)
      if (hasInt(l.list[i]))
        return true;
    return false;
  }
  bool hasAll(const FastIntList &l) const
  {
    if (l.list.size() > list.size())
      return false;
    for (int i = 0; i < l.list.size(); i++)
      if (!hasInt(l.list[i]))
        return false;
    return true;
  }
  bool cmpEq(const FastIntList &l) const { return list.size() == l.list.size() && mem_eq(list, l.list.data()); }


  void reset(bool clr = false) { clr ? clear_and_shrink(list) : list.clear(); }
  dag::ConstSpan<int> getList() const { return list; }

protected:
  struct IntPredicate
  {
    static int order(int a, int b) { return a == b ? 0 : (a < b ? -1 : 1); }
  };
  TabSortedInline<int, IntPredicate> list;
};
