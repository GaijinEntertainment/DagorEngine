#include "w_curve_math.h"
#include <memory/dag_mem.h>
#include <generic/dag_qsort.h>
#include <math/srcc_msu/srcc_msu.h>
#include <debug/dag_debug.h>


static IPoint2 locate(const Point2 &point, const BBox2 &view_box, const Point2 &view_size)
{
  IPoint2 res;
  res.x = (point.x - view_box.lim[0].x) * view_size.x / view_box.width().x;
  res.y = (point.y - view_box.lim[0].y) * view_size.y / view_box.width().y;
  return res;
}
static Point2 getCoords(const IPoint2 &point, const BBox2 &view_box, const Point2 &view_size)
{
  Point2 res;
  res.x = point.x * view_box.width().x / view_size.x + view_box.lim[0].x;
  res.y = point.y * view_box.width().y / view_size.y + view_box.lim[0].y;
  return res;
}

struct SplineKnot
{
  Point2 i, p, o;
  Point2 k1, k2, k3;

  void calc_ks(SplineKnot &nk)
  {
    k1 = (o - p) * 3;
    k2 = (p + nk.i - o * 2) * 3;
    k3 = (o - nk.i) * 3 + nk.p - p;
  }
  Point2 ippos(real t) { return ((k3 * t + k2) * t + k1) * t + p; }
  Point2 iptang(real t) { return (k3 * (t * 1.5f) + k2) * (t * 2) + k1; }
};

class Spline
{
public:
  DAG_DECLARE_NEW(midmem)

  Tab<SplineKnot> knot;
  char closed;

  Spline(IMemAlloc *m = midmem) : knot(m), closed(0) {}
  int segcount() { return closed ? knot.size() : knot.size() - 1; }
  void recalc()
  {
    for (int i = 1; i < knot.size(); ++i)
      knot[i - 1].calc_ks(knot[i]);
    if (closed)
      knot.back().calc_ks(knot[0]);
  }
};

class PointXSorter
{
public:
  static int compare(const ICurveControlCallback::ControlPoint &p1, const ICurveControlCallback::ControlPoint &p2)
  {
    if (p1.pos.x < p2.pos.x)
      return -1;
    else if (p1.pos.x > p2.pos.x)
      return 1;
    else
      return 0;
  }
};

//===================================================================

void LinearCB::buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size)
{
  PolyLine line(E3DCOLOR(255, 255, 0, 255));

  for (int i = 0; i < controlPoints.size(); ++i)
    line.points.push_back(controlPoints[i].pos);

  clear_and_shrink(poly_lines);
  poly_lines.push_back(line);
}

//===================================================================

CatmullRomCBTest::CatmullRomCBTest() : controlPoints(tmpmem), fromStartingPos(0, 0)
{
  mMin = mMax = Point2(0, 0);
  setConnectedEnds(false);
  setLockX(false);
  setLockEnds(false);
  setCycled(false);
}

void CatmullRomCBTest::getControlPoints(Tab<ControlPoint> &points) { points = controlPoints; }

bool CatmullRomCBTest::selectControlPoints(const Tab<int> &point_idx)
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
    if (controlPoints[i].isSelected())
    {
      controlPoints[i].select(false);
      changed = true;
    }

  for (int j = 0; j < point_idx.size(); j++)
    if (point_idx[j] >= 0 && point_idx[j] < controlPoints.size())
    {
      controlPoints[point_idx[j]].select(true);
      changed = true;
    }
  return changed;
}

bool CatmullRomCBTest::beginMoveControlPoints(int picked_point_idx)
{
  if (picked_point_idx >= 0 && picked_point_idx < controlPoints.size())
    return false;
  return true;
}

bool CatmullRomCBTest::moveSelectedControlPoints(const Point2 &_from_starting_pos)
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if (controlPoints[i].isSelected())
    {
      Point2 delta = _from_starting_pos - fromStartingPos;

      // if enabled lock_x can not move rightpoint lefter then left
      if (lock_x)
      {
        if (i != 0 && controlPoints[i].pos.x + delta.x < controlPoints[i - 1].pos.x)
          delta.x = controlPoints[i - 1].pos.x - controlPoints[i].pos.x;
        else if (i != controlPoints.size() - 1 && controlPoints[i].pos.x + delta.x > controlPoints[i + 1].pos.x)
          delta.x = controlPoints[i + 1].pos.x - controlPoints[i].pos.x;
      }

      // min and max set

      if (mMin.x != mMax.x)
      {
        if ((controlPoints[i].pos.x + delta.x > mMax.x) || (controlPoints[i].pos.x + delta.x < mMin.x))
          delta.x = 0;
      }

      if (mMin.y != mMax.y)
      {
        if ((controlPoints[i].pos.y + delta.y > mMax.y) || (controlPoints[i].pos.y + delta.y < mMin.y))
          delta.y = 0;
      }

      // if lock_ends enabled - change only y position of end points
      if (!(lock_ends && (i == 0 || i == controlPoints.size() - 1)))
      {
        controlPoints[i].pos += delta;
      }
      else
      {
        controlPoints[i].pos.y += delta.y;
      }
      changed = true;

      // sync firs and last points
      if (mCycled && (i == 0 || i == controlPoints.size() - 1))
      {
        int i_other = (i == 0) ? controlPoints.size() - 1 : 0;

        if (!controlPoints[i_other].isSelected()) // for both ends select
          controlPoints[i_other].pos.y = controlPoints[i].pos.y;
      }
    }
  }
  fromStartingPos = _from_starting_pos;
  return changed;
}

bool CatmullRomCBTest::endMoveControlPoints(bool cancel_movement)
{
  bool changed = false;
  if (cancel_movement && !mCycled)
    for (int i = 0; i < controlPoints.size(); i++)
    {
      if (controlPoints[i].isSelected())
      {
        Point2 delta = -fromStartingPos;
        controlPoints[i].pos += delta;
        changed = true;
      }
    }
  fromStartingPos = Point2(0, 0);
  return changed;
}

bool CatmullRomCBTest::deleteSelectedControlPoints()
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if (controlPoints[i].isSelected())
    {
      erase_items(controlPoints, i, 1);
      --i;
      changed = true;
      continue;
    }
  }
  return changed;
}

bool CatmullRomCBTest::addNewControlPoint(const Point2 &at_pos)
{
  if (mMin.x != mMax.x && (at_pos.x < mMin.x || at_pos.x > mMax.x))
    return false;
  if (mMin.y != mMax.y && (at_pos.y < mMin.y || at_pos.y > mMax.y))
    return false;

  for (int i = 0; i < controlPoints.size(); i++)
    controlPoints[i].select(false);
  controlPoints.push_back(ControlPoint(at_pos, ControlPoint::SMOOTH));

  if (lock_x)
  {
    SimpleQsort<ICurveControlCallback::ControlPoint, PointXSorter>::sort(&controlPoints[0], controlPoints.size());
  }
  return true;
}


void CatmullRomCBTest::buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size)
{
  float CURVE_ACCURACY = 0.05;
  PolyLine line(E3DCOLOR(255, 255, 0, 255));

  Tab<Point2> work_points(tmpmem);
  work_points.resize(controlPoints.size() + 4);
  for (int i = 0; i < controlPoints.size(); i++)
    work_points[i + 2] = controlPoints[i].pos;
  if (!connected_ends)
  {
    work_points[0] = work_points[1] = work_points[2];
    work_points[work_points.size() - 1] = work_points[work_points.size() - 2] = work_points[work_points.size() - 3];
  }
  else
  {
    work_points[0] = work_points[work_points.size() - 4];
    work_points[1] = work_points[work_points.size() - 3];
    work_points[work_points.size() - 2] = work_points[2];
    work_points[work_points.size() - 1] = work_points[3];
  }

  for (int i = 0; i < (int)work_points.size() - 3; i++)
  {
    Point2 &p0 = work_points[i + 0];
    Point2 &p1 = work_points[i + 1];
    Point2 &p2 = work_points[i + 2];
    Point2 &p3 = work_points[i + 3];
    float ax = (-1 * p0.x + 3 * p1.x - 3 * p2.x + 1 * p3.x) / 2.0;
    float bx = (2 * p0.x - 5 * p1.x + 4 * p2.x - 1 * p3.x) / 2.0;
    float cx = (-1 * p0.x + 0 * p1.x + 1 * p2.x + 0 * p3.x) / 2.0;
    float dx = (0 * p0.x + 2 * p1.x + 0 * p2.x + 0 * p3.x) / 2.0;
    float ay = (-1 * p0.y + 3 * p1.y - 3 * p2.y + 1 * p3.y) / 2.0;
    float by = (2 * p0.y - 5 * p1.y + 4 * p2.y - 1 * p3.y) / 2.0;
    float cy = (-1 * p0.y + 0 * p1.y + 1 * p2.y + 0 * p3.y) / 2.0;
    float dy = (0 * p0.y + 2 * p1.y + 0 * p2.y + 0 * p3.y) / 2.0;

    float x, y;
    for (float t = 0; t < 1; t += CURVE_ACCURACY) //-V1034
    {
      x = (((ax * t) + bx) * t + cx) * t + dx;
      y = (((ay * t) + by) * t + cy) * t + dy;
      line.points.push_back(Point2(x, y));
    }
  }
  clear_and_shrink(poly_lines);
  poly_lines.push_back(line);
}


//===================================================================

bool CubicPolynomCB::getCoefs(Tab<Point2> &xy_4c_per_seg) const
{
  xy_4c_per_seg.clear();
  if (controlPoints.size() != 4)
    return false;

  double cc[4];
#define ADD_BASIS(i0, i1, i2, i3)                                                   \
  if (controlPoints.size() >= 4)                                                    \
  {                                                                                 \
    real x0 = controlPoints[i0].pos.x;                                              \
    real x1 = controlPoints[i1].pos.x;                                              \
    real x2 = controlPoints[i2].pos.x;                                              \
    real x3 = controlPoints[i3].pos.x;                                              \
    real k = safediv(controlPoints[i0].pos.y, ((x0 - x1) * (x0 - x2) * (x0 - x3))); \
    cc[0] -= x1 * x2 * x3 * k;                                                      \
    cc[1] += (x1 * x2 + x2 * x3 + x3 * x1) * k;                                     \
    cc[2] -= (x1 + x2 + x3) * k;                                                    \
    cc[3] += k;                                                                     \
  }

  memset(cc, 0, sizeof(cc));
  ADD_BASIS(0, 1, 2, 3);
  ADD_BASIS(1, 2, 3, 0);
  ADD_BASIS(2, 3, 0, 1);
  ADD_BASIS(3, 0, 1, 2);

#undef ADD_BASIS

  xy_4c_per_seg.resize(4);
  xy_4c_per_seg[0].set(0, cc[0]);
  xy_4c_per_seg[1].set(1, cc[1]);
  xy_4c_per_seg[2].set(0, cc[2]);
  xy_4c_per_seg[3].set(0, cc[3]);
  return false; // x==t (direct) and need not be computed
}

void CubicPolynomCB::buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size)
{
  clear_and_shrink(poly_lines);
  if (controlPoints.size() != 4)
    return;

  Tab<Point2> cc(tmpmem);
  getCoefs(cc);

  static const float CURVE_ACCURACY = 0.025;

  PolyLine &line = poly_lines.push_back();
  line.color = E3DCOLOR(255, 255, 0, 255);
  line.points.reserve(1 / CURVE_ACCURACY + 2);
  for (float x = 0; x <= 1 + CURVE_ACCURACY; x += CURVE_ACCURACY) //-V1034
    line.points.push_back().set(x, ((cc[3].y * x + cc[2].y) * x + cc[1].y) * x + cc[0].y);
}


//===================================================================

bool CubicPSplineCB::getCoefs(Tab<Point2> &xy_4c_per_seg) const
{
  xy_4c_per_seg.clear();
  if (controlPoints.size() < 2)
    return false;

  SmallTab<real, TmpmemAlloc> x, f, df, sm, y, yc, rab;
  int nx = controlPoints.size(), ret = 0;
  if (connected_ends)
    nx += 4;
  clear_and_resize(x, nx);
  clear_and_resize(f, nx);
  clear_and_resize(df, nx);
  clear_and_resize(sm, nx);
  clear_and_resize(y, nx);
  clear_and_resize(yc, nx * 3);
  clear_and_resize(rab, 7 * (nx + 2));

  // build 1D-spline using monotonic X as parameter
  for (int i = 0; i < nx; i++)
  {
    if (!connected_ends)
    {
      x[i] = controlPoints[i].pos.x;
      f[i] = controlPoints[i].pos.y;
    }
    else if (i < 2)
    {
      x[i] = controlPoints[controlPoints.size() - 1 + i - 1].pos.x - 1.0f;
      f[i] = controlPoints[controlPoints.size() - 1 + i - 1].pos.y;
    }
    else if (i >= nx - 2)
    {
      x[i] = controlPoints[i - nx + 2].pos.x + 1.0f;
      f[i] = controlPoints[i - nx + 2].pos.y;
    }
    else
    {
      x[i] = controlPoints[i - 2].pos.x;
      f[i] = controlPoints[i - 2].pos.y;
    }

    df[i] = 1e-4;
    sm[i] = 2;
  }
  is01r_c(x.data(), f.data(), df.data(), &nx, sm.data(), y.data(), yc.data(), rab.data(), &ret);
  if (ret != 0)
  {
    logerr("failed to compute spline!");
    return false;
  }

  xy_4c_per_seg.resize(4 * (controlPoints.size() - 1));
  for (int i = 0; i + 1 < controlPoints.size(); i++)
  {
    const Point2 &p0 = controlPoints[i + 0].pos;
    const Point2 &p1 = controlPoints[i + 1].pos;
    int b = connected_ends ? i + 2 : i;
    float cc[4];

    xy_4c_per_seg[i * 4 + 0].set(0, y[b]);
    xy_4c_per_seg[i * 4 + 1].set(1, yc[b * 3 + 0]);
    xy_4c_per_seg[i * 4 + 2].set(0, yc[b * 3 + 1]);
    xy_4c_per_seg[i * 4 + 3].set(0, yc[b * 3 + 2]);
  }
  return false; // x==t (direct) and need not be computed
}

void CubicPSplineCB::buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size)
{
  Tab<Point2> cc(tmpmem);
  getCoefs(cc);
  if (!cc.size())
    return;

  const float CURVE_ACCURACY = 0.01 * (controlPoints.size() - 1);
  PolyLine line(E3DCOLOR(255, 255, 0, 255));
  clear_and_shrink(poly_lines);

  for (int i = 0; i + 1 < controlPoints.size(); i++)
  {
    const Point2 &p0 = controlPoints[i + 0].pos;
    const Point2 &p1 = controlPoints[i + 1].pos;

    float dx = p1.x - p0.x;
    for (float x = 0; x <= dx; x += CURVE_ACCURACY * dx) //-V1034
      line.points.push_back().set(p0.x + x, ((cc[i * 4 + 3].y * x + cc[i * 4 + 2].y) * x + cc[i * 4 + 1].y) * x + cc[i * 4 + 0].y);
  }
  poly_lines.push_back(line);
}

////===================================================================

NurbCBTest::NurbCBTest() : controlPoints(tmpmem), fromStartingPos(0, 0), tangentSize(0, 0), tangentIdx(-1) {}

void NurbCBTest::getControlPoints(Tab<ControlPoint> &points) { points = controlPoints; }

bool NurbCBTest::selectControlPoints(const Tab<int> &point_idx)
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
    if (controlPoints[i].isSelected())
    {
      controlPoints[i].select(false);
      changed = true;
    }

  for (int j = 0; j < point_idx.size(); j++)
    if (point_idx[j] >= 0 && point_idx[j] < controlPoints.size())
    {
      controlPoints[point_idx[j]].select(true);
      changed = true;
    }
  return changed;
}

bool NurbCBTest::beginMoveControlPoints(int picked_point_idx)
{
  if (picked_point_idx >= 0 && picked_point_idx < controlPoints.size())
  {
    tangentIdx = picked_point_idx;
    return false;
  }
  tangentIdx = -1;
  return true;
}

bool NurbCBTest::moveSelectedControlPoints(const Point2 &_from_starting_pos)
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if ((controlPoints[i].isSelected() && tangentIdx == -1) || tangentIdx == i)
    {
      Point2 delta = _from_starting_pos - fromStartingPos;
      controlPoints[i].pos += delta;
      changed = true;
    }
  }
  fromStartingPos = _from_starting_pos;
  return changed;
}

bool NurbCBTest::endMoveControlPoints(bool cancel_movement)
{
  bool changed = false;
  if (cancel_movement)
    for (int i = 0; i < controlPoints.size(); i++)
    {
      if ((controlPoints[i].isSelected() && tangentIdx == -1) || tangentIdx == i)
      {
        Point2 delta = -fromStartingPos;
        controlPoints[i].pos += delta;
        changed = true;
      }
    }
  fromStartingPos = Point2(0, 0);
  return changed;
}

bool NurbCBTest::deleteSelectedControlPoints()
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if (controlPoints[i].isSelected())
    {
      erase_items(controlPoints, i, 1);
      --i;
      changed = true;
      continue;
    }
  }
  return changed;
}

bool NurbCBTest::addNewControlPoint(const Point2 &at_pos)
{
  for (int i = 0; i < controlPoints.size(); i++)
    controlPoints[i].select(false);
  controlPoints.push_back(ControlPoint(at_pos, ControlPoint::SMOOTH));
  return true;
}


void NurbCBTest::buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size)
{
  tangentSize = view_box.width() / 10;
  float step = view_box.width().x / view_size.x;
  clear_and_shrink(poly_lines);
  int idx = -1;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if (idx > -1)
    {
      PolyLine line(E3DCOLOR(255, 255, 0, 255));

      Spline s;
      s.knot.resize(2);
      SplineKnot &k0 = s.knot[0], &k1 = s.knot[1];
      k0.p = controlPoints[idx].pos;
      k0.i = controlPoints[idx + 1].pos;
      k0.o = controlPoints[idx + 2].pos;
      k1.p = controlPoints[i].pos;
      k1.i = controlPoints[i + 1].pos;
      k1.o = controlPoints[i + 2].pos;
      s.recalc();

      real l = length(k1.p - k0.p);
      int n = ceilf(l / step);
      if (n < 3)
        n = 3;
      else if (n > 100)
        n = 100;
      int p = append_items(line.points, n);
      real n1 = 1.f / n;
      for (int j = 0; j < n; ++j, ++p)
      {
        Point2 p2 = k0.ippos(j * n1);
        line.points[p] = Point2(p2.x, p2.y);
      }

      poly_lines.push_back(line);
    }
    idx = i;
  }
}

//===================================================================


QuadraticCBTest::QuadraticCBTest() : controlPoints(tmpmem), fromStartingPos(0, 0)
{
  controlPoints.push_back(ControlPoint(Point2(0, 0.5), 0));
  controlPoints.push_back(ControlPoint(Point2(0.5, 0), 0));
  controlPoints.push_back(ControlPoint(Point2(1, 0.5), 0));
}

void QuadraticCBTest::getControlPoints(Tab<ControlPoint> &points) { points = controlPoints; }

bool QuadraticCBTest::selectControlPoints(const Tab<int> &point_idx)
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if (controlPoints[i].isSelected())
    {
      controlPoints[i].select(false);
      changed = true;
    }
    for (int j = 0; j < point_idx.size(); j++)
      if (i == point_idx[j])
      {
        controlPoints[i].select(true);
        changed = true;
      }
  }
  return changed;
}

bool QuadraticCBTest::beginMoveControlPoints(int picked_point_id) { return true; }

bool QuadraticCBTest::moveSelectedControlPoints(const Point2 &_from_starting_pos)
{
  bool changed = false;
  for (int i = 0; i < controlPoints.size(); i++)
  {
    if (controlPoints[i].isSelected())
    {
      Point2 delta = _from_starting_pos - fromStartingPos;
      if (i == 0 || i == 2)
        delta.x = 0;
      controlPoints[i].pos += delta;
      changed = true;
    }
  }
  fromStartingPos = _from_starting_pos;
  return changed;
}

bool QuadraticCBTest::endMoveControlPoints(bool cancel_movement)
{
  bool changed = false;
  if (cancel_movement)
    for (int i = 0; i < controlPoints.size(); i++)
    {
      if (controlPoints[i].isSelected())
      {
        Point2 delta = -fromStartingPos;
        if (i == 0 || i == 2)
          delta.x = 0;
        controlPoints[i].pos += delta;
        changed = true;
      }
    }
  fromStartingPos = Point2(0, 0);
  return changed;
}

bool QuadraticCBTest::deleteSelectedControlPoints() { return false; }

bool QuadraticCBTest::addNewControlPoint(const Point2 &at_pos) { return false; }

void QuadraticCBTest::buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size)
{
  // y=a*x*x+b*x+c;
  Point2 p0 = controlPoints[0].pos;
  Point2 p1 = controlPoints[1].pos;
  Point2 p2 = controlPoints[2].pos;
  float a = (p1.y - p0.y - p1.x * (p2.y - p0.y)) / (p1.x * p1.x - p1.x);
  float b = p2.y - p0.y - a;
  float c = p0.y;

  PolyLine line(E3DCOLOR(255, 255, 0, 255));

  float step = view_box.width().x / view_size.x;
  float x = view_box.lim[0].x;
  line.points.push_back(Point2(x, a * x * x + b * x + c));
  for (x += step; x < view_box.width().x; x += step)
  {
    line.points.push_back(Point2(x, a * x * x + b * x + c));
  }
  clear_and_shrink(poly_lines);
  poly_lines.push_back(line);
}