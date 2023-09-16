#ifndef __GAIJIN_DAG_DYNMAP__
#define __GAIJIN_DAG_DYNMAP__
#pragma once


#include <generic/dag_tab.h>
#include <generic/dag_qsort.h>


#define INLINEMAP __forceinline


template <class TKey, class TVal, class TCompare = SimpleAscentCompare<TKey>>
class DynMap
{
public:
  DynMap(IMemAlloc *mem) : data(mem) {}
  DynMap(const DynMap &from) : data(from.data) {}

  INLINEMAP bool add(const TKey &key, const TVal &val)
  {
    if (!data.size() || find(key) == -1)
    {
      DynMapRec rec(key, val);
      const int idx = data.size();
      data.push_back(rec);

      if (data.size() > 1)
        push_back(key, idx, 0);

      return true;
    }

    return false;
  }

  // very slow method
  // TODO: make it faster
  bool erase(const TKey &key)
  {
    Tab<DynMapRec> oldData(data);

    data.clear();
    bool erased = false;

    for (int i = 0; i < oldData.size(); ++i)
      if (oldData[i].key != key)
        add(oldData[i].key, oldData[i].val);
      else
        erased = true;

    return erased;
  }

  bool erase(dag::ConstSpan<TKey> keys)
  {
    Tab<DynMapRec> oldData(data);

    data.clear();

    for (int i = 0; i < oldData.size(); ++i)
    {
      bool ignore = false;
      for (int j = 0; j < keys.size(); ++j)
        if (oldData[i].key == keys[j])
        {
          ignore = true;
          break;
        }

      if (!ignore)
        add(oldData[i].key, oldData[i].val);
    }

    return true;
  }

  INLINEMAP bool presented(const TKey &key) const
  {
    const int idx = find(key);
    return idx > -1;
  }

  INLINEMAP bool get(const TKey &key, TVal &val) const
  {
    const int idx = find(key);
    if (idx > -1)
    {
      val = data[idx].val;
      return true;
    }

    return false;
  }

  INLINEMAP bool set(const TKey &key, const TVal &val)
  {
    const int idx = find(key);
    if (idx > -1)
    {
      data[idx].val = val;
      return true;
    }

    return false;
  }

  INLINEMAP void clear()
  {
    curPos = 0;
    clear_and_shrink(data);
  }

  INLINEMAP void eraseall()
  {
    curPos = 0;
    data.clear();
  }

  INLINEMAP bool getFirst(TKey &key, TVal &val) const
  {
    curPos = 0;
    return getInCurPos(key, val);
  }

  INLINEMAP bool getFirst(TVal &val) const
  {
    curPos = 0;
    TKey k;
    return getInCurPos(k, val);
  }

  INLINEMAP bool getNext(TKey &key, TVal &val) const
  {
    ++curPos;
    return getInCurPos(key, val);
  }

  INLINEMAP bool getNext(TVal &val) const
  {
    ++curPos;
    TKey k;
    return getInCurPos(k, val);
  }

  INLINEMAP unsigned size() const { return data.size(); }
  INLINEMAP int dataSize() const { return data_size(data); }

protected:
  struct DynMapRec
  {
    TKey key;
    TVal val;
    int lo;
    int hi;

    DynMapRec(const TKey &k, const TVal &v) : key(k), val(v), lo(-1), hi(-1) {}
    DynMapRec(const DynMapRec &from) : key(from.key), val(from.val), lo(from.lo), hi(from.hi) {}
  };

  Tab<DynMapRec> data;
  mutable int curPos = 0;

  int find(const TKey &key, int idx = 0) const
  {
    if (data.size())
    {
      const int comp = TCompare::compare(data[idx].key, key);
      if (!comp)
        return idx;

      if (comp < 0)
        return (data[idx].lo > -1) ? find(key, data[idx].lo) : -1;
      else
        return (data[idx].hi > -1) ? find(key, data[idx].hi) : -1;
    }

    return -1;
  }

  void push_back(const TKey &key, int item_idx, int comp_idx)
  {
    const int comp = TCompare::compare(data[comp_idx].key, key);

    if (comp < 0)
    {
      if (data[comp_idx].lo > -1)
        push_back(key, item_idx, data[comp_idx].lo);
      else
        data[comp_idx].lo = item_idx;
    }
    else if (comp > 0)
    {
      if (data[comp_idx].hi > -1)
        push_back(key, item_idx, data[comp_idx].hi);
      else
        data[comp_idx].hi = item_idx;
    }
  }

  INLINEMAP bool getInCurPos(TKey &key, TVal &val) const
  {
    if (curPos > -1 && curPos < data.size())
    {
      key = data[curPos].key;
      val = data[curPos].val;
      return true;
    }

    return false;
  }
};


#endif //__GAIJIN_DAG_DYNMAP__
