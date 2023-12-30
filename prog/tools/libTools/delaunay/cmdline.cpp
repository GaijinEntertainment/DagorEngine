#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <memory/dag_regionMemAlloc.h>
#include "terra.h"
#include <util/dag_globDef.h>

using namespace std;
namespace delaunay
{
IMemAlloc *delmem = NULL;

GreedySubdivision *mesh = NULL;

static ImportMask default_mask;
ImportMask *MASK = &default_mask;


real error_threshold = 0.0;
int point_limit = -1;

void load(float error_threshold1, int point_limit1, Map *DEM, Mesh &m, float cell, const Point3 &ofs, ImportMask *MASK1)
{
  RegionMemAlloc rm_alloc(8 << 20, 4 << 20);
  delmem = &rm_alloc;
  point_limit = point_limit1;
  error_threshold = error_threshold1;
  MASK = MASK1 ? MASK1 : &default_mask;
  mesh = new (delmem) GreedySubdivision(DEM);
  if (point_limit < 0)
    point_limit = DEM->width() * DEM->height();
  greedy_insertion();
  generate_output(m, cell, ofs, DEM);
  delete_obj(mesh);
  debug_ctx("delmem used %dK (of %dK allocated in %d pools)", rm_alloc.getPoolUsedSize() >> 10, rm_alloc.getPoolAllocatedSize() >> 10,
    rm_alloc.getPoolsCount());
  delmem = NULL;
}

} // namespace delaunay