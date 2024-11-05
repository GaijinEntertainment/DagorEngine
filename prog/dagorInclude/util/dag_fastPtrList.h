//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabSort.h>

class FastPtrList
{
public:
  FastPtrList() : list(midmem) {}

  int size() const { return list.size(); }
  void *operator[](int idx) const { return list[idx]; }

  bool empty() const { return list.size() == 0; }
  bool hasPtr(const void *key) const
  {
    int n = list.find((void *)key);
    return n != -1;
  }

  bool addPtr(void *key)
  {
    if (hasPtr(key))
      return false;
    list.insert(key);
    return true;
  }
  bool addPtrFiltered(void *key, const void *bad_key) { return key != bad_key ? addPtr(key) : false; }
  int addPtrList(const FastPtrList &l)
  {
    int added = false;
    for (int i = 0; i < l.list.size(); i++)
      if (addPtr(l.list[i]))
        added++;
    return added;
  }

  bool delPtr(void *key)
  {
    int n = list.find(key);
    if (n < 0)
      return false;
    erase_items(list, n, 1);
    return true;
  }
  int delPtrList(const FastPtrList &l)
  {
    int removed = false;
    for (int i = 0; i < l.list.size(); i++)
      if (delPtr(l.list[i]))
        removed++;
    return removed;
  }

  bool hasAny(const FastPtrList &l) const
  {
    for (int i = 0; i < l.list.size(); i++)
      if (hasPtr(l.list[i]))
        return true;
    return false;
  }
  bool hasAll(const FastPtrList &l) const
  {
    if (l.list.size() > list.size())
      return false;
    for (int i = 0; i < l.list.size(); i++)
      if (!hasPtr(l.list[i]))
        return false;
    return true;
  }
  bool cmpEq(const FastPtrList &l) const { return list.size() == l.list.size() && mem_eq(list, l.list.data()); }


  void reset(bool clr = false) { clr ? clear_and_shrink(list) : list.clear(); }
  dag::ConstSpan<void *> getList() const { return list; }

protected:
  struct PtrPredicate
  {
    static int order(void *a, void *b) { return ((uint8_t *)a) - ((uint8_t *)b); }
  };
  TabSortedInline<void *, PtrPredicate> list;
};
