// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/resourceSlot/detail/ids.h>

namespace resource_slot::detail
{

struct SlotUsage
{
  NodeId node;
  int priority;
  bool isOnlyRead;
};

struct LessPriority
{
  bool operator()(SlotUsage a, SlotUsage b) const
  {
    if (a.priority != b.priority)
      return a.priority < b.priority;

    // Read should be after update
    return a.isOnlyRead < b.isOnlyRead;
  }
};

} // namespace resource_slot::detail