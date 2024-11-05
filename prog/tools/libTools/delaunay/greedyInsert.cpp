// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>

#include "terra.h"

namespace delaunay
{

extern ImportMask *MASK;

void TrackedTriangle::update(Subdivision &s)
{
  GreedySubdivision &gs = (GreedySubdivision &)s;
  gs.scanTriangle(*this);
}


GreedySubdivision::GreedySubdivision(Map *map)
{
  H = map;
  heap = new (delmem) Heap(1 << 20);

  int w = H->width();
  int h = H->height();

  is_used.init(w, h);
  int x, y;
  for (x = 0; x < w; x++)
    for (y = 0; y < h; y++)
      is_used(x, y) = DATA_POINT_UNUSED;


  initMesh(Vec2(0, 0), Vec2(0, h - 1), Vec2(w - 1, h - 1), Vec2(w - 1, 0));

  is_used(0, 0) = DATA_POINT_USED;
  is_used(0, h - 1) = DATA_POINT_USED;
  is_used(w - 1, h - 1) = DATA_POINT_USED;
  is_used(w - 1, 0) = DATA_POINT_USED;

  count = 4;
}
GreedySubdivision::~GreedySubdivision() { delete_obj(heap); }


Triangle *GreedySubdivision::allocFace(Edge *e)
{
  Triangle *t = new (delmem) TrackedTriangle(e);

  heap->insert(t, -1.0);

  return t;
}


void GreedySubdivision::compute_plane(Plane &plane, Triangle &T, Map &map)
{
  const Vec2 &p1 = T.point1();
  const Vec2 &p2 = T.point2();
  const Vec2 &p3 = T.point3();

  Vec3 v1(p1, map(p1[X], p1[Y]));
  Vec3 v2(p2, map(p2[X], p2[Y]));
  Vec3 v3(p3, map(p3[X], p3[Y]));

  plane.init(v1, v2, v3);
}

///////////////////////////

void GreedySubdivision::scan_triangle_line(const Plane &plane, int y, real x1, real x2, Candidate &candidate)
{
  int startx = (int)ceil(MIN(x1, x2));
  int endx = (int)floor(MAX(x1, x2));

  if (startx > endx)
    return;

  real z0 = plane(startx, y);
  real dz = plane.a;
  real z, diff;

  for (int x = startx; x <= endx; x++)
  {
    if (!is_used(x, y))
    {
      z = H->eval(x, y);
      diff = std::abs(z - z0);
      candidate.consider(x, y, MASK->apply(x, y, diff));
    }
    z0 += dz;
  }
}

template <class T>
static void inline swapi(T &a, T &b)
{
  T c = a;
  a = b;
  b = c;
}

#define FLT_ERROR 1e-5f

void GreedySubdivision::scanTriangle(TrackedTriangle &T)
{
  Plane z_plane;
  compute_plane(z_plane, T, *H);

  Vec2 by_y[3] = {T.point1(), T.point2(), T.point3()};

  int i0 = 0, i1 = 1, i2 = 2;
  if (by_y[i0][Y] > by_y[i1][Y])
    swapi(i0, i1);
  if (by_y[i0][Y] > by_y[i2][Y])
    swapi(i0, i2);
  if (by_y[i1][Y] > by_y[i2][Y])
    swapi(i1, i2);

  Vec2 &v0 = by_y[i0];
  Vec2 &v1 = by_y[i1];
  Vec2 &v2 = by_y[i2];


  int y;
  int starty, endy;
  Candidate candidate;
  float idy10 = (v1[Y] - v0[Y]), idy20 = (v2[Y] - v0[Y]), idy21 = (v2[Y] - v1[Y]);
  idy10 = fsel(idy10 - FLT_ERROR, 1.0f / idy10, 1.0f);
  idy20 = fsel(idy20 - FLT_ERROR, 1.0f / idy20, 1.0f);
  idy21 = fsel(idy21 - FLT_ERROR, 1.0f / idy21, 1.0f);
  real dx1 = (v1[X] - v0[X]) * idy10;
  real dx2 = (v2[X] - v0[X]) * idy20;

  real x1 = v0[X];
  real x2 = v0[X];

  starty = (int)v0[Y];
  endy = (int)v1[Y];
  for (y = starty; y < endy; y++)
  {
    scan_triangle_line(z_plane, y, x1, x2, candidate);
    x1 += dx1;
    x2 += dx2;
  }

  /////////////////////////////

  dx1 = (v2[X] - v1[X]) * idy21;
  x1 = v1[X];

  starty = endy;
  endy = (int)v2[Y];
  for (y = starty; y <= endy; y++)
  {
    scan_triangle_line(z_plane, y, x1, x2, candidate);
    x1 += dx1;
    x2 += dx2;
  }

  /////////////////////////////////
  //
  // We have now found the appropriate candidate point.
  //
  if (candidate.import < 1e-4)
  {
    if (T.token != NOT_IN_HEAP)
      heap->kill(T.token);

#ifdef SAFETY
    T.setCandidate(-69, -69, 0.0);
#endif
  }
  else
  {
    assert(!is_used(candidate.x, candidate.y));

    T.setCandidate(candidate.x, candidate.y, candidate.import);
    if (T.token == NOT_IN_HEAP)
      heap->insert(&T, candidate.import);
    else
      heap->update(&T, candidate.import);
  }
}

Edge *GreedySubdivision::select(int sx, int sy, Triangle *t)
{
  if (is_used(sx, sy))
  {
    debug("   WARNING: Tried to reinsert point: %d %d", sx, sy);
    return NULL;
  }

  is_used(sx, sy) = DATA_POINT_USED;
  count++;
  return insert(Vec2(sx, sy), t);
}


int GreedySubdivision::greedyInsert()
{
  heap_node *node = heap->extract();

  if (!node)
    return False;


  TrackedTriangle &T = *(TrackedTriangle *)node->obj;
  int sx, sy;
  T.getCandidate(&sx, &sy);

  select(sx, sy, &T);

  return True;
}

real GreedySubdivision::maxError()
{
  heap_node *node = heap->top();

  if (!node)
    return 0.0;

  return node->import;
}

real GreedySubdivision::rmsError()
{
  real err = 0.0;
  int width = H->width();
  int height = H->height();


  for (int i = 0; i < width; i++)
    for (int j = 0; j < height; j++)
    {
      real diff = eval(i, j) - H->eval(i, j);
      err += diff * diff;
    }

  return sqrt(err / (width * height));
}


real GreedySubdivision::eval(int x, int y)
{
  Vec2 p(x, y);
  Triangle *T = locate(p)->Lface();

  Plane z_plane;
  compute_plane(z_plane, *T, *H);

  return z_plane(x, y);
}

}; // namespace delaunay