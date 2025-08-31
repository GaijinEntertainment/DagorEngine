//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

namespace dacoll
{

enum CollType : uint32_t
{
  ETF_LMESH = 1 << 0,
  ETF_FRT = 1 << 1,
  ETF_RI = 1 << 2,
  ETF_RESTORABLES = 1 << 3,
  ETF_OBJECTS_GROUP = 1 << 4,
  ETF_STRUCTURES = 1 << 5,
  ETF_HEIGHTMAP = 1 << 6,
  ETF_STATIC = 1 << 7,
  ETF_RI_TREES = 1 << 8,
  ETF_RI_PHYS = 1 << 9, // Trace against PHYS_COLLIDABLE instead of TRACEABLE
  ETF_DEFAULT = ETF_LMESH | ETF_HEIGHTMAP | ETF_FRT | ETF_RI | ETF_RESTORABLES | ETF_OBJECTS_GROUP | ETF_STRUCTURES,
  ETF_ALL = -1 & ~ETF_RI_PHYS // Always specify use of phys collision explicitly
};

}; // namespace dacoll
