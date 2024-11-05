// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <util/dag_inttypes.h>

struct ResourceSizeInfo
{
  uint32_t sizeInBytes;
  uint32_t index : 30;
  bool isTex : 1;
  bool hasRecreationCallback : 1;
};

using FramememResourceSizeInfoCollection = dag::Vector<ResourceSizeInfo, framemem_allocator>;
