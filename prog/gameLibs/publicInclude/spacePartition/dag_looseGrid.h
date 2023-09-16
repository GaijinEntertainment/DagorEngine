//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <math/dag_bounds3.h>
#include <math/dag_bounds2.h>
#include <osApiWrappers/dag_rwLock.h>


class BBox3;
class Point3;

enum
{
  GLEV_SMALL_OBJECTS = 0,
  GLEV_LARGE_OBJECTS = 1,
  GLEV_MAX = 2
};

#define GRID_SIZE_CELLS (32)
#define NUM_CELLS_LEVEL (GRID_SIZE_CELLS * GRID_SIZE_CELLS)
#define NUM_CELLS_TOTAL (NUM_CELLS_LEVEL * GLEV_MAX)


namespace not_thread_safe_loose_grid
{
#define THREAD_SAFE_LOOSE_GRID 0
#include "looseGrid.inc.h"
#undef THREAD_SAFE_LOOSE_GRID
} // namespace not_thread_safe_loose_grid

#define THREAD_SAFE_LOOSE_GRID 1
#include "looseGrid.inc.h"
#undef THREAD_SAFE_LOOSE_GRID
