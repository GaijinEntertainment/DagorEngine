//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_patchTab.h>
#include <util/dag_stdint.h>
#include <stdlib.h>
#include <string.h>


/// Read-only fast direct name map (to be constructed from binary dump)
struct RoNameMap
{
public:
  PatchableTab<PatchablePtr<const char>> map;

public:
  int getNameId(const char *name) const
  {
    int lo = 0, hi = map.size() - 1, m;
    int cmp;

    // check bounds first
    if (hi < 0)
      return -1;

    cmp = strcmp(name, map[0]);
    if (cmp < 0)
      return -1;
    if (cmp == 0)
      return 0;

    if (hi != 0)
      cmp = strcmp(name, map[hi]);
    if (cmp > 0)
      return -1;
    if (cmp == 0)
      return hi;

    // binary search
    while (lo < hi)
    {
      m = (lo + hi) >> 1;
      cmp = strcmp(name, map[m]);

      if (cmp == 0)
        return m;
      if (m == lo)
        return -1;

      if (cmp < 0)
        hi = m;
      else
        lo = m;
    }
    return -1;
  }

  int nameCount() const { return map.size(); }

  void patchData(void *base)
  {
    map.patch(base);

    for (int i = 0; i < map.size(); i++)
      map[i].patch(base);
  }
};

/// Read-only fast name map with abitrary id mapping (to be constructed from binary dump)
struct RoNameMapEx : public RoNameMap
{
public:
  PatchableTab<uint16_t> id;

public:
  int getNameId(const char *name) const
  {
    int n = RoNameMap::getNameId(name);
    return (n >= 0) ? id[n] : -1;
  }

  void patchData(void *base)
  {
    RoNameMap::patchData(base);
    id.patch(base);
  }

  /// Walks through all map to find name_id (slow); returns NULL when name_id is not found
  const char *getNameSlow(int name_id) const
  {
    if (name_id >= 0 && name_id < id.size())
      for (int i = 0; i < id.size(); i++)
        if (name_id == id[i])
          return map[i];
    return NULL;
  }
};

#include <supp/dag_iterateStatus.h>

template <typename CB>
static inline void iterate_names(const RoNameMapEx &nm, CB cb /*(int id, const char *name)*/)
{
  for (uint32_t ord_idx = 0; ord_idx < nm.map.size(); ++ord_idx)
    cb(nm.id[ord_idx], nm.map[ord_idx].get());
}
template <typename CB>
static inline void iterate_names3(const RoNameMapEx &nm, CB cb /*(int id, const char *name, int ord_idx)*/)
{
  for (uint32_t ord_idx = 0; ord_idx < nm.map.size(); ++ord_idx)
    cb(nm.id[ord_idx], nm.map[ord_idx].get(), ord_idx);
}
template <typename CB, typename TAB>
static inline void iterate_names_in_order(const RoNameMapEx &nm, const TAB &idx, CB cb /*(int id, const char *name)*/)
{
  for (uint32_t ord_idx : idx)
    cb(nm.id[ord_idx], nm.map[ord_idx].get());
}
template <typename CB>
static inline IterateStatus iterate_names_breakable(const RoNameMapEx &nm, CB cb /*(int id, const char *name)*/)
{
  for (uint32_t ord_idx = 0; ord_idx < nm.map.size(); ++ord_idx)
  {
    IterateStatus s = cb(nm.id[ord_idx], nm.map[ord_idx].get());
    if (s != IterateStatus::Continue)
      return s;
  }
  return IterateStatus::StopDoneAll;
}
