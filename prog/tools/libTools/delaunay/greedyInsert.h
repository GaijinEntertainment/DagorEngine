// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "heap.h"
#include "subdivision.h"
#include <delaunay/delaunay.h>

namespace delaunay
{

class TrackedTriangle : public Triangle
{
  //
  // candidate position
  int sx, sy;


public:
  DAG_DECLARE_NEW(delmem)

  TrackedTriangle(Edge *e, int t = NOT_IN_HEAP) : Triangle(e, t) {}

  void update(Subdivision &);


  void setCandidate(int x, int y, real)
  {
    sx = x;
    sy = y;
  }
  void getCandidate(int *x, int *y)
  {
    *x = sx;
    *y = sy;
  }
};


class Candidate
{
public:
  DAG_DECLARE_NEW(delmem)

  int x, y;
  real import;

  Candidate() { import = -1e7; }

  void consider(int sx, int sy, real i)
  {
    if (i > import)
    {
      x = sx;
      y = sy;
      import = i;
    }
  }
};


class GreedySubdivision : public Subdivision
{
  Heap *heap;
  unsigned int count;

protected:
  Map *H;

  Triangle *allocFace(Edge *e);

  void compute_plane(Plane &, Triangle &, Map &);

  void scan_triangle_line(const Plane &plane, int y, real x1, real x2, Candidate &candidate);

public:
  DAG_DECLARE_NEW(delmem)

  GreedySubdivision(Map *map);
  ~GreedySubdivision();

  array2<char> is_used;

  Edge *select(int sx, int sy, Triangle *t = NULL);

  Map &getData() { return *H; }

  void scanTriangle(TrackedTriangle &t);
  int greedyInsert();

  unsigned int pointCount() { return count; }
  real maxError();
  real rmsError();
  real eval(int x, int y);
};

//
// These are the possible values of is_used(x,y):
#define DATA_POINT_UNUSED  0
#define DATA_POINT_USED    1
#define DATA_POINT_IGNORED 2
#define DATA_VALUE_UNKNOWN 3
}; // namespace delaunay
