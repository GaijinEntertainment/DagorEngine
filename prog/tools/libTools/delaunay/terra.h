#pragma once

#include "greedyInsert.h"
#include <delaunay/delaunay.h>

class Mesh;
class Point3;

namespace delaunay
{

extern GreedySubdivision *mesh;
extern ImportMask *MASK;

extern real error_threshold;
extern int point_limit;

extern int goal_not_met();
extern void greedy_insertion();
extern void display_greedy_insertion(void (*callback)());

extern void generate_output(Mesh &out_mesh, float cell, const Point3 &ofs, Map *DEM);

} // namespace delaunay
