//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>

namespace rendinst
{
struct DestroyedInstanceRange
{
  uint32_t startOffs;
  uint32_t endOffs;

  DestroyedInstanceRange() : startOffs(0), endOffs(0) {}
  DestroyedInstanceRange(uint32_t start_offs, uint32_t end_offs) : startOffs(start_offs), endOffs(end_offs) {}
};

struct DestroyedPoolData
{
  Tab<DestroyedInstanceRange> destroyedInstances;
  uint16_t poolIdx;

  bool isInRange(uint32_t offs) const;
};

struct DestroyedCellData
{
  Tab<DestroyedPoolData> destroyedPoolInfo;
  int cellId;

  const DestroyedPoolData *getPool(uint16_t pool_idx) const;
};

void debug_print_destrs(const Tab<rendinst::DestroyedCellData> &cellsNewDestrInfo);
} // namespace rendinst
