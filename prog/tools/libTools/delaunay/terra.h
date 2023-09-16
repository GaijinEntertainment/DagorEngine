#ifndef TERRA_INCLUDED // -*- C++ -*-
#define TERRA_INCLUDED

#include "greedyInsert.h"
#include <delaunay/delaunay.h>

class Mesh;
class Point3;

namespace delaunay
{

extern GreedySubdivision *mesh;
// extern Map *DEM;
extern ImportMask *MASK;

extern real error_threshold;
extern int point_limit;
// extern char *script_filename;

extern int goal_not_met();
extern void greedy_insertion();
extern void display_greedy_insertion(void (*callback)());
// extern void scripted_preinsertion(istream&);
// extern void subsample_insertion(int target_width);

extern void generate_output(Mesh &out_mesh, float cell, const Point3 &ofs, Map *DEM);

} // namespace delaunay
#endif
