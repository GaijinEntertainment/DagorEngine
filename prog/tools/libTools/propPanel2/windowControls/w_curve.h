// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include "w_simple_controls.h"
#include "w_curve_math.h"
#include <math/dag_e3dColor.h>

// -----------------------------------------------

typedef struct tagPOINT POINT;

struct MyRect
{
  int l, t, r, b;

  int operator!=(MyRect &r2)
  {
    if (l != r2.l || t != r2.t || r != r2.r || b != r2.b)
      return 1;

    return 0;
  }

  int operator==(MyRect &r2)
  {
    if (l == r2.l && t == r2.t && r == r2.r && b == r2.b)
      return 1;

    return 0;
  }

  int width() const { return r - l; }
  int height() const { return b - t; }
};


/////////////////////////////////////////////////////////////////////////////
class WCurveControl : public WindowControlBase
{
public:
  // CtlColor backColor, backCreateColor, ptColor, selPtColor, handleColor, selHandleColor;


  enum
  {
    MODE_SELECTION,
    MODE_CREATION,
    MODE_DRAG_COORD,
  };


  WCurveControl(WindowControlEventHandler *event_handler, WindowBase *parent, int left, int top, int w, int h, int id = -1,
    const Point2 &left_bottom = Point2(-0.05, -0.1), const Point2 &right_top = Point2(1.05, 1.1),
    ICurveControlCallback *curve_control_cb = NULL);

  virtual ~WCurveControl() { setCB(NULL); }

  // set and get ICurveControlCallback pointer
  ICurveControlCallback *getCB() const { return cb; }
  void setCB(ICurveControlCallback *curve_control_cb)
  {
    if (cb)
    {
      delete cb;
      cb = NULL;
    }
    cb = curve_control_cb;
    build();
  }


  // shows part of curve set by left-bottom and right-upper corners (and shows control points)
  virtual intptr_t onControlDrawItem(void *info);
  virtual void resizeWindow(int w, int h);
  virtual intptr_t onLButtonDown(long x, long y);
  virtual intptr_t onLButtonDClick(long x, long y);
  virtual intptr_t onLButtonUp(long x, long y);
  virtual intptr_t onDrag(int x, int y);
  virtual intptr_t onMouseMove(int x, int y);
  virtual intptr_t onRButtonDown(long x, long y);
  virtual intptr_t onVScroll(int dy, bool is_wheel);
  virtual intptr_t onKeyDown(unsigned v_key, int flags);

  void setValue(Tab<Point2> &source);
  void getValue(Tab<Point2> &dest) const;

  // set and get viewbox for curve
  void setViewBox(const Point2 &left_bottom, const Point2 &right_top);
  Point2 getLeftBottom() const { return Point2(viewBox.lim[0].x, viewBox.lim[1].y); }
  Point2 getRightTop() const { return Point2(viewBox.lim[1].x, viewBox.lim[0].y); }

  // set and get current mode (creation mode adds new point for each LMB click); RMB click resets mode to default
  void setMode(int _mode = MODE_CREATION)
  {
    mode = _mode;
    refresh(true);
  }
  int getMode() { return mode; }

  void setColor(E3DCOLOR color) { mColor = color; };
  void setFixed(bool value);
  void setMinMax(int min, int max);
  void setXMinMax(float min, float max);
  void setYMinMax(float min, float max);
  void setCurValue(float value);
  void setCycled(bool cycled);
  void setSelected(bool value) { mSelected = value; }

  void autoZoom();

protected:
  void drawAxisMarks(void *hdc);
  void drawPlotLine(void *hdc, const Point2 &p1, const Point2 &p2);
  void drawMarkText(void *hdc, const Point2 &pos, double _value, unsigned align);
  bool checkTextNeed(int count, int position);

  ICurveControlCallback *cb;

  // sets size and position of view area (in curve coordinates)
  BBox2 defaultViewBox;
  BBox2 viewBox;
  // sets size in pixels, can be used to specify required accuracy of curve
  Point2 viewSize;
  Tab<ICurveControlCallback::PolyLine> cachPolyLines;
  bool cancelMovement;
  bool startMovement;
  Point2 startingPos;
  struct
  {
    bool active;
    int type;
    MyRect sel;
  } rectSelect;
  int mode;
  float mCurValue;

  bool isSelection();
  int ProcessRectSelection(int x, int y);
  void startRectangularSelection(int mx, int my, int type);
  bool endRectangularSelection(MyRect *result, int *type);
  void paintSelectionRect();
  void build();

  E3DCOLOR mColor;
  WTooltip mTooltip;
  int mMinPtCount, mMaxPtCount;
  bool mDrawAMarks, mSelected;
};
