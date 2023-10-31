#include <sys/stat.h>
#include "common.h"

#include <libTools/staticGeom/geomResources.h>
#include <debug/dag_debug.h>


bool objectWasMoved = false, objectWasRotated = false, objectWasScaled = false;

float triang_area(dag::ConstSpan<Point2> pts)
{
  int n = pts.size();
  float a = 0.0f;
  for (int p = n - 1, q = 0; q < n; p = q++)
  {
    a += pts[p].x * pts[q].y - pts[q].x * pts[p].y;
  }

  return a * 0.5f;
}


bool triang_inside_triangle(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Px, float Py)
{
  float ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  float cCROSSap, bCROSScp, aCROSSbp;

  ax = Cx - Bx;
  ay = Cy - By;

  bx = Ax - Cx;
  by = Ay - Cy;

  cx = Bx - Ax;
  cy = By - Ay;

  apx = Px - Ax;
  apy = Py - Ay;

  bpx = Px - Bx;
  bpy = Py - By;

  cpx = Px - Cx;
  cpy = Py - Cy;

  aCROSSbp = ax * bpy - ay * bpx;
  cCROSSap = cx * apy - cy * apx;
  bCROSScp = bx * cpy - by * cpx;

  return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
};


bool triang_snip(dag::ConstSpan<Point2> pts, int u, int v, int w, int n, int *V)
{
  int p;
  float Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = pts[V[u]].x;
  Ay = pts[V[u]].y;

  Bx = pts[V[v]].x;
  By = pts[V[v]].y;

  Cx = pts[V[w]].x;
  Cy = pts[V[w]].y;

  if (1e-10 > (((Bx - Ax) * (Cy - Ay)) - ((By - Ay) * (Cx - Ax))))
    return false;

  for (p = 0; p < n; p++)
  {
    if ((p == u) || (p == v) || (p == w))
      continue;

    Px = pts[V[p]].x;
    Py = pts[V[p]].y;

    if (triang_inside_triangle(Ax, Ay, Bx, By, Cx, Cy, Px, Py))
      return false;
  }

  return true;
}

bool make_clockwise_coords(dag::Span<Point2> pts)
{
  int cnt = pts.size();

  Point2 center = Point2(0, 0);

  for (int i = 0; i < cnt; i++)
    center += pts[i];

  center /= cnt;

  Point2 p1 = center - pts[0];
  Point2 p2 = center - pts[1];

  Point3 cross = Point3(p1.x, 0, p1.y) % Point3(p2.x, 0, p2.y);

  if (cross.y < 0)
  {
    Tab<Point2> tmp(tmpmem_ptr());
    dag::set_allocator(tmp, tmpmem_ptr());
    tmp.resize(cnt);

    tmp = pts;

    for (int i = 0; i < cnt; i++)
      pts[i] = tmp[cnt - i - 1];

    return true;
  }

  return false;
}

bool make_clockwise_coords(dag::Span<Point3> pts3)
{
  Tab<Point2> pts(tmpmem);
  pts.resize(pts3.size());
  for (int i = 0; i < pts3.size(); i++)
    pts[i] = Point2(pts3[i].x, pts3[i].z);

  int cnt = pts.size();

  Point2 center = Point2(0, 0);

  for (int i = 0; i < cnt; i++)
    center += pts[i];

  center /= cnt;

  Point2 p1 = center - pts[0];
  Point2 p2 = center - pts[1];

  Point3 cross = Point3(p1.x, 0, p1.y) % Point3(p2.x, 0, p2.y);

  if (cross.y < 0)
  {
    Tab<Point3> tmp(tmpmem_ptr());
    dag::set_allocator(tmp, tmpmem_ptr());
    tmp.resize(cnt);

    tmp = pts3;

    for (int i = 0; i < cnt; i++)
      pts3[i] = tmp[cnt - i - 1];

    return true;
  }

  return false;
}

bool lines_inters_ignore_Y(Point3 &pf1, Point3 &pf2, Point3 &ps1, Point3 &ps2)
{
  real x1 = pf1.x, z1 = pf1.z, x2 = pf2.x, z2 = pf2.z;
  real x3 = ps1.x, z3 = ps1.z, x4 = ps2.x, z4 = ps2.z;

  return (((x3 - x1) * (z2 - z1) - (z3 - z1) * (x2 - x1)) * ((x4 - x1) * (z2 - z1) - (z4 - z1) * (x2 - x1)) <= 0) &&
         (((x1 - x3) * (z4 - z3) - (z1 - z3) * (x4 - x3)) * ((x2 - x3) * (z4 - z3) - (z2 - z3) * (x4 - x3)) <= 0);
}
