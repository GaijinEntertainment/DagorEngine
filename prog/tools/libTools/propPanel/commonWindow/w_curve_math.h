// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_math3d.h>
#include <generic/dag_tab.h>

namespace PropPanel
{

class ICurveControlCallback
{
public:
  struct ControlPoint
  {
    Point2 pos, in, out;
    unsigned flags;
    enum
    {
      SELECTED = 1,
      SMOOTH = 2,
    };
    void select(bool sel)
    {
      if (sel)
        flags |= SELECTED;
      else
        flags &= ~SELECTED;
    }
    bool isSelected() const { return flags & SELECTED; }
    bool isSmooth() const { return flags & SMOOTH; }
    ControlPoint(const Point2 &_pos, unsigned _flags) : pos(_pos), flags(_flags) {}
  };


  class PolyLine
  {
  public:
    Tab<Point2> points;
    E3DCOLOR color;
    PolyLine(E3DCOLOR _color) : points(tmpmem), color(_color) {}
    PolyLine() : points(tmpmem) {}
  };

  virtual ~ICurveControlCallback() {}

  virtual void getControlPoints(Tab<ControlPoint> &points) = 0;

  virtual bool selectControlPoints(const Tab<int> &point_ids) = 0;

  virtual bool beginMoveControlPoints(int picked_point_id) = 0;
  virtual bool moveSelectedControlPoints(const Point2 &_from_starting_pos) = 0;
  virtual bool endMoveControlPoints(bool cancel_movement) = 0;

  virtual bool deleteSelectedControlPoints() = 0;
  virtual bool addNewControlPoint(const Point2 &at_pos) = 0;
  virtual void clear() = 0;

  virtual void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) = 0;

  void setConnectedEnds(bool connect) { connected_ends = connect; }
  bool getConnectedEnds() { return connected_ends; }
  virtual void setLockX(bool _lock_x) { lock_x = _lock_x; }
  bool getLockX() { return lock_x; };
  virtual void setLockEnds(bool _lock_ends) { lock_ends = _lock_ends; }
  bool getLockEnds() { return lock_ends; }
  void setXMinMax(float _min, float _max)
  {
    mMin.x = _min;
    mMax.x = _max;
  }
  void setYMinMax(float _min, float _max)
  {
    mMin.y = _min;
    mMax.y = _max;
  }

  const Point2 &getMinXY() { return mMin; }
  const Point2 &getMaxXY() { return mMax; }

  virtual void setCycled(bool cycled) { mCycled = cycled; }
  bool getCycled() { return mCycled; }

  virtual bool getCoefs(Tab<Point2> &xy_4c_per_seg) const = 0;

protected:
  bool connected_ends;
  bool lock_x;
  bool lock_ends;
  Point2 mMin, mMax;
  bool mCycled;
};


class CatmullRomCBTest : public ICurveControlCallback
{
protected:
  Tab<ControlPoint> controlPoints;
  Point2 fromStartingPos;

public:
  CatmullRomCBTest();
  void getControlPoints(Tab<ControlPoint> &points) override;

  bool selectControlPoints(const Tab<int> &point_ids) override;

  bool beginMoveControlPoints(int picked_point_id) override;
  bool moveSelectedControlPoints(const Point2 &_from_starting_pos) override;
  bool endMoveControlPoints(bool cancel_movement) override;

  bool deleteSelectedControlPoints() override;
  bool addNewControlPoint(const Point2 &at_pos) override;
  void clear() override { clear_and_shrink(controlPoints); };

  void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) override;

  bool getCoefs(Tab<Point2> &xy) const override
  {
    xy.clear();
    return false;
  }
};


class LinearCB : public CatmullRomCBTest
{
  void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) override;
};


class CubicPolynomCB : public CatmullRomCBTest
{
public:
  CubicPolynomCB() { lock_x = lock_ends = true; }
  void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) override;
  void setLockX(bool) override { lock_x = true; }
  void setLockEnds(bool) override { lock_ends = true; }

  bool getCoefs(Tab<Point2> &xy_4c_per_seg) const override;
};


class CubicPSplineCB : public CatmullRomCBTest
{
public:
  CubicPSplineCB() { lock_x = lock_ends = true; }
  void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) override;
  void setLockX(bool) override { lock_x = true; }
  void setLockEnds(bool) override { lock_ends = true; }

  bool getCoefs(Tab<Point2> &xy_4c_per_seg) const override;
};


//-------------------------------

class NurbCBTest : public ICurveControlCallback
{
  Tab<ControlPoint> controlPoints;
  Point2 fromStartingPos;
  Point2 tangentSize;
  int tangentIdx;

public:
  NurbCBTest();
  void getControlPoints(Tab<ControlPoint> &points) override;

  bool selectControlPoints(const Tab<int> &point_ids) override;

  bool beginMoveControlPoints(int picked_point_id) override;
  bool moveSelectedControlPoints(const Point2 &_from_starting_pos) override;
  bool endMoveControlPoints(bool cancel_movement) override;

  bool deleteSelectedControlPoints() override;
  bool addNewControlPoint(const Point2 &at_pos) override;

  void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) override;

  bool getCoefs(Tab<Point2> &xy) const override
  {
    xy.clear();
    return false;
  }
};

class QuadraticCBTest : public ICurveControlCallback
{
  Tab<ControlPoint> controlPoints;
  Point2 fromStartingPos;

public:
  QuadraticCBTest();
  void getControlPoints(Tab<ControlPoint> &points) override;

  bool selectControlPoints(const Tab<int> &point_ids) override;

  bool beginMoveControlPoints(int picked_point_id) override;
  bool moveSelectedControlPoints(const Point2 &_from_starting_pos) override;
  bool endMoveControlPoints(bool cancel_movement) override;

  bool deleteSelectedControlPoints() override;
  bool addNewControlPoint(const Point2 &at_pos) override;

  void buildPolylines(Tab<PolyLine> &poly_lines, const BBox2 &view_box, const Point2 &view_size) override;

  bool getCoefs(Tab<Point2> &xy) const override
  {
    xy.clear();
    return false;
  }
};

} // namespace PropPanel