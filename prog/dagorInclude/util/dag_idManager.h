//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>

/**
 * Allocates ids starting from 0 and allows freeing them.
 * Freed ids will be returned first on the next allocations.
 */
class IdManager
{
protected:
  int32_t lastAllocatedId = -1;
  dag::Vector<uint32_t> freeIds;

public:
  uint32_t allocate()
  {
    uint32_t id;
    if (freeIds.size())
    {
      id = freeIds.back();
      freeIds.pop_back();
    }
    else
    {
      id = ++lastAllocatedId;
    }
    return id;
  }

  void free(uint32_t id)
  {
    G_ASSERT(id <= uint32_t(lastAllocatedId) && lastAllocatedId >= 0);
    if (id == lastAllocatedId)
      lastAllocatedId--;
    else
      freeIds.push_back(id);
  }

  void clear()
  {
    lastAllocatedId = -1;
    freeIds.clear();
  }
};
