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
  /*CtlColor backColor, backCreateColor, ptColor, selPtColor,
    handleColor, selHandleColor;*/


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

  // Контрол имеет методы для установки и получения указателя на ICurveControlCallback.
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


  // Контрол отображает область кривой, заданную положениями левого нижнего и правого верхнего углов вида в координатах кривой.
  // Контрол отображает управляющие точки.
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

  // Эти положения могут быть фиксированными, а могут меняться различными способами, в зависимости от того, как используется этот
  // контрол. Поэтому надо сделать методы для получения и установки положений этих двух углов вида.
  void setViewBox(const Point2 &left_bottom, const Point2 &right_top);
  Point2 getLeftBottom() const { return Point2(viewBox.lim[0].x, viewBox.lim[1].y); }
  Point2 getRightTop() const { return Point2(viewBox.lim[1].x, viewBox.lim[0].y); }

  // Контрол может быть переведен в режим создания точек. В этом режиме при нажатии левой кнопки мыши создается точка.
  // При нажатии правой кнопки мыши происходит выход из этого режима в обычный режим работы.
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

  // Контрол имеет метод для удаления выбранных точек. Этот метод будет вызываться извне,
  // при нажатии кнопки в интерфейсе, например.
  // void delSelection();
  void autoZoom();

protected:
  void drawAxisMarks(void *hdc);
  void drawPlotLine(void *hdc, const Point2 &p1, const Point2 &p2);
  void drawMarkText(void *hdc, const Point2 &pos, double _value, unsigned align);
  bool checkTextNeed(int count, int position);

  ICurveControlCallback *cb;

  // задает размер и положение области вида в координатах кривой.
  // Это может быть использовано для оптимизации ломаных линий,
  // например, они могут быть созданы только для этой видимой области или их детализация может зависеть от этого.
  BBox2 defaultViewBox;
  BBox2 viewBox;
  // задает размер вида в пикселях, может использоваться для определения необходимого уровня детализации кривой.
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
