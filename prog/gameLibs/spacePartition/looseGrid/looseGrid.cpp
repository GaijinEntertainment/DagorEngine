#include <spacePartition/dag_looseGrid.h>
#include <math/dag_bounds3.h>
#include <math/dag_adjpow2.h>
#include <math/dag_math2d.h>
#include <osApiWrappers/dag_spinlock.h>
#include <startup/dag_globalSettings.h>
#include <vecmath/dag_vecMath_common.h>

#define GRID_SIZE_MASK (GRID_SIZE_CELLS - 1)
#if DAGOR_DBGLEVEL > 0 && defined(_DEBUG_TAB_)
#define VERIFY_LOOSE_GRID_INTEGRITY 1
#endif

namespace not_thread_safe_loose_grid
{
#define THREAD_SAFE_LOOSE_GRID 0
#include "looseGrid.inc.cpp"
#undef THREAD_SAFE_LOOSE_GRID
} // namespace not_thread_safe_loose_grid

#define THREAD_SAFE_LOOSE_GRID 1
#include "looseGrid.inc.cpp"
#undef THREAD_SAFE_LOOSE_GRID
