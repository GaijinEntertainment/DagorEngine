//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_unpatchedTab.h>


struct UnpatchedRoNameMap
{
public:
  UnpatchedTabReader<UnpatchedPtrReader<const char>> mapReader;

public:
  int getNameId(const char *name, const void *baseAddress) const
  {
    dag::ConstSpan<UnpatchedPtrReader<const char>> map = mapReader.getAccessor(baseAddress);
    int lo = 0, hi = map.size() - 1;

    // check bounds first
    if (hi < 0)
      return -1;

    int cmp = strcmp(name, map[0].get(baseAddress));
    if (cmp < 0)
      return -1;
    if (cmp == 0)
      return 0;

    if (hi != 0)
      cmp = strcmp(name, map[hi].get(baseAddress));
    if (cmp > 0)
      return -1;
    if (cmp == 0)
      return hi;

    // binary search
    while (lo < hi)
    {
      const int m = (lo + hi) >> 1;
      cmp = strcmp(name, map[m].get(baseAddress));

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
};
