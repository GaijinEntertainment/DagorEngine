#include <math/dag_wooray3d.h>
#include <vecmath/dag_vecMath.h>

void WooRay3d::initWoo(const Point3 &start, const Point3 &dir, const Point3 &leafSize)
{
  p = start;
  wdir = dir;
  pt = IPoint3(floor(div(p, leafSize)));
  step = IPoint3(wdir.x >= 0.0f ? 1 : -1, wdir.y >= 0.0f ? 1 : -1, wdir.z >= 0.0f ? 1 : -1);
  const IPoint3 nextPt = pt + IPoint3(max(0, step.x), max(0, step.y), max(0, step.z));
  Point3 absDir(fabsf(wdir.x), fabsf(wdir.y), fabsf(wdir.z));
  tDelta = Point3(absDir.x > 1e-6f ? leafSize.x / absDir.x : 0, absDir.y > 1e-6f ? leafSize.y / absDir.y : 0,
    absDir.z > 1e-6f ? leafSize.z / absDir.z : 0);

  // this calculations should be made in doubles for precision
  tMax = Point3(absDir.x > 1e-6 ? double(nextPt.x * double(leafSize.x) - p.x) / double(wdir.x) : VERY_BIG_NUMBER,
    absDir.y > 1e-6 ? double(nextPt.y * double(leafSize.y) - p.y) / double(wdir.y) : VERY_BIG_NUMBER,
    absDir.z > 1e-6 ? double(nextPt.z * double(leafSize.z) - p.z) / double(wdir.z) : VERY_BIG_NUMBER);
}