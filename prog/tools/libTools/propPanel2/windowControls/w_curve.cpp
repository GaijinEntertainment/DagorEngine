// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_curve.h"
#include "../c_constants.h"
#include <memory/dag_mem.h>
#include <windows.h>

#include <propPanel2/comWnd/dialog_window.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>

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

// -----------------------------------------------


enum
{
  CURVE_SENSITIVE = 5,
  CURVE_POINT_RAD = 3,
  CURVE_SCROLL_SPEED = 10,

  DIALOG_ID_X = 1000,
  DIALOG_ID_Y,
};

WCurveControl::WCurveControl(WindowControlEventHandler *event_handler, WindowBase *parent, int left, int top, int w, int h, int id,
  const Point2 &left_bottom, const Point2 &right_top, ICurveControlCallback *curve_control_cb)

  :
  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_NOTIFY | BS_OWNERDRAW, "", left, top, w, h),
  viewSize(w, h),
  defaultViewBox(left_bottom, right_top),
  startingPos(-1, -1),
  cb(NULL),
  cachPolyLines(tmpmem),
  mode(MODE_SELECTION),
  cancelMovement(false),
  startMovement(false),
  mColor(E3DCOLOR(0, 255, 0)),
  mCurValue(0),
  mTooltip(event_handler, this),
  mMinPtCount(2),
  mMaxPtCount(0),
  mDrawAMarks(false),
  mSelected(false)
{
  setCB(curve_control_cb);

  setViewBox(left_bottom, right_top);
  memset(&rectSelect, 0, sizeof(rectSelect));
  mTooltip.setText("");
}


void WCurveControl::setFixed(bool value)
{
  if (!cb)
    return;

  cb->setLockEnds(value);
  cb->setLockX(value);
}

void WCurveControl::setMinMax(int min, int max)
{
  mMinPtCount = min;
  mMaxPtCount = max;
}

void WCurveControl::setXMinMax(float min, float max)
{
  if (!cb)
    return;

  cb->setXMinMax(min, max);
}


void WCurveControl::setYMinMax(float min, float max)
{
  if (!cb)
    return;

  cb->setYMinMax(min, max);
}

void WCurveControl::setViewBox(const Point2 &left_bottom, const Point2 &right_top)
{
  viewBox.lim[0].x = left_bottom.x;
  viewBox.lim[0].y = right_top.y;
  viewBox.lim[1].x = right_top.x;
  viewBox.lim[1].y = left_bottom.y;
  build();
}

int WCurveControl::ProcessRectSelection(int x, int y)
{
  paintSelectionRect();
  rectSelect.sel.r = x;
  rectSelect.sel.b = y;
  return 0;
}

bool WCurveControl::isSelection()
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);

  for (int i = 0; i < points.size(); i++)
    if (points[i].isSelected())
      return true;

  return false;
}

void WCurveControl::startRectangularSelection(int mx, int my, int type)
{
  if (rectSelect.active)
    endRectangularSelection(NULL, NULL);

  rectSelect.active = true;
  rectSelect.type = type;
  rectSelect.sel.l = mx;
  rectSelect.sel.t = my;
  rectSelect.sel.r = mx;
  rectSelect.sel.b = my;
}

bool WCurveControl::endRectangularSelection(MyRect *result, int *type)
{
  if (!rectSelect.active)
    return false;

  paintSelectionRect();

  rectSelect.active = false;
  int temp;
  if (result)
  {
    if (rectSelect.sel.l > rectSelect.sel.r)
    {
      temp = rectSelect.sel.l;
      rectSelect.sel.l = rectSelect.sel.r;
      rectSelect.sel.r = temp;
    }
    if (rectSelect.sel.t > rectSelect.sel.b)
    {
      temp = rectSelect.sel.t;
      rectSelect.sel.t = rectSelect.sel.b;
      rectSelect.sel.b = temp;
    }
    *result = rectSelect.sel;
  }
  if (type)
    *type = rectSelect.type;
  return true;
}

void WCurveControl::paintSelectionRect()
{
  if (rectSelect.sel.l == rectSelect.sel.r && rectSelect.sel.t == rectSelect.sel.b)
    return;

  HDC dc = GetDC((HWND)getHandle());
  HPEN pen = CreatePen(PS_DASH, 0, RGB(~mColor.r, ~mColor.g, ~mColor.b));
  RECT rect;
  rect.right = rectSelect.sel.r < mWidth ? rectSelect.sel.r : rectSelect.sel.r > mWidth + 5000 /*some_big_number*/ ? 0 : mWidth - 1;
  rect.bottom = rectSelect.sel.b < mHeight - 1                          ? rectSelect.sel.b
                : rectSelect.sel.b > mHeight + 5000 /*some_big_number*/ ? 0
                                                                        : mHeight - 1;
  rect.left = rectSelect.sel.l;
  rect.top = rectSelect.sel.t;
  SelectObject(dc, pen);
  MoveToEx(dc, rect.left, rect.top, NULL);
  LineTo(dc, rect.left, rect.bottom);
  LineTo(dc, rect.right, rect.bottom);
  LineTo(dc, rect.right, rect.top);
  LineTo(dc, rect.left, rect.top);

  DeleteObject(pen);
  ReleaseDC((HWND)getHandle(), dc);
}

intptr_t WCurveControl::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HBRUSH black_brush = CreateSolidBrush(RGB(0, 0, 0));
  HBRUSH white_brush = CreateSolidBrush(RGB(255, 255, 255));
  SelectObject(draw->hDC, white_brush);
  MoveToEx(draw->hDC, 0, 0, NULL);

  HPEN pen = CreatePen(PS_SOLID, 1, RGB(mColor.r, mColor.g, mColor.b));
  HPEN black_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HPEN white_pen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
  HPEN gray_pen = CreatePen(PS_DOT, 1, RGB(128, 128, 128));
  HPEN old_pen = (HPEN)SelectObject(draw->hDC, pen);

  // draw current X value
  if (mCurValue)
  {
    SelectObject(draw->hDC, white_pen);
    drawPlotLine((void *)draw->hDC, Point2(mCurValue, viewBox.lim[0].y), Point2(mCurValue, viewBox.lim[1].y));
  }

  // draw minimum and maximum
  if (cb)
  {
    Point2 minXY, maxXY, lt, rb;
    minXY = cb->getMinXY();
    maxXY = cb->getMaxXY();
    SelectObject(draw->hDC, gray_pen);
    lt.x = (minXY.x == maxXY.x) ? viewBox.lim[0].x : minXY.x;
    rb.x = (minXY.x == maxXY.x) ? viewBox.lim[1].x : maxXY.x;
    lt.y = (minXY.y == maxXY.y) ? viewBox.lim[0].y : minXY.y;
    rb.y = (minXY.y == maxXY.y) ? viewBox.lim[1].y : maxXY.y;

    if (minXY.x != maxXY.x)
    {
      drawPlotLine((void *)draw->hDC, Point2(minXY.x, lt.y), Point2(minXY.x, rb.y));
      drawPlotLine((void *)draw->hDC, Point2(maxXY.x, lt.y), Point2(maxXY.x, rb.y));
    }
    if (minXY.y != maxXY.y)
    {
      drawPlotLine((void *)draw->hDC, Point2(lt.x, minXY.y), Point2(rb.x, minXY.y));
      drawPlotLine((void *)draw->hDC, Point2(lt.x, maxXY.y), Point2(rb.x, maxXY.y));
    }
  }

  // draw coord lines
  SelectObject(draw->hDC, black_pen);
  drawPlotLine((void *)draw->hDC, Point2(viewBox.lim[0].x, 0), Point2(viewBox.lim[1].x, 0));
  drawPlotLine((void *)draw->hDC, Point2(0, viewBox.lim[0].y), Point2(0, viewBox.lim[1].y));
  if (mDrawAMarks)
    drawAxisMarks((void *)draw->hDC);

  // curve
  Tab<IPoint2> linePts(tmpmem);

  SelectObject(draw->hDC, pen);
  for (int i = 0; i < cachPolyLines.size(); i++)
  {
    linePts.resize(cachPolyLines[i].points.size());
    for (int j = 0; j < linePts.size(); ++j)
      linePts[j] = locate(cachPolyLines[i].points[j], viewBox, viewSize);

    Tab<POINT> t(tmpmem);
    t.resize(linePts.size());
    for (int i = 0; i < linePts.size(); i++)
    {
      t[i].x = linePts[i].x;
      t[i].y = linePts[i].y;
    }
    Polyline(draw->hDC, &t[0], t.size());
  }

  // control lines & points
  if (!cb)
    return 0;

  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);

  //////////////////////
  /*for ( int i = 0; i < points.size(); i++ )
  {
    SelectObject(draw->hDC,pen2);
    if (points[i].isSmooth())
    {
      IPoint2 o = locate( points[i].pos, viewBox, viewSize);
      MoveToEx(draw->hDC, o.x, o.y, NULL);

      p = locate( points[i].pos, viewBox, viewSize);
      LineTo(draw->hDC, p.x, p.y );

      MoveToEx(draw->hDC, o.x, o.y, NULL);
      p = locate( points[i].out, viewBox, viewSize);
      LineTo(draw->hDC, p.x, p.y );
    }
  }*/

  for (int i = 0; i < points.size(); i++)
  {
    IPoint2 p = locate(points[i].pos, viewBox, viewSize);
    RECT r;
    r.top = p.y - CURVE_POINT_RAD;
    r.bottom = p.y + CURVE_POINT_RAD;
    r.left = p.x - CURVE_POINT_RAD;
    r.right = p.x + CURVE_POINT_RAD;
    FillRect(draw->hDC, &r, points[i].isSelected() ? white_brush : black_brush);
  }

  clear_and_shrink(points);

  // selectionRect
  if (rectSelect.active)
    paintSelectionRect();

  // draw around rect
  HPEN selected_pen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
  SelectObject(draw->hDC, mSelected ? selected_pen : black_pen);

  MoveToEx(draw->hDC, 0, 0, NULL);
  LineTo(draw->hDC, mWidth - 1, 0);
  LineTo(draw->hDC, mWidth - 1, mHeight - 1);
  LineTo(draw->hDC, 0, mHeight - 1);
  LineTo(draw->hDC, 0, 0);

  SelectObject(draw->hDC, old_pen);
  DeleteObject(black_brush);
  DeleteObject(white_brush);
  DeleteObject(pen);
  DeleteObject(black_pen);
  DeleteObject(white_pen);
  DeleteObject(gray_pen);
  DeleteObject(selected_pen);
  return 0;
}


void WCurveControl::drawAxisMarks(void *hdc)
{
  Point2 mark_step(1, 1);
  const int AXIS_MARK_SIZE = 3;

#define CALC_STEP(param)                                               \
  double interval = fabs(viewBox.lim[1].param - viewBox.lim[0].param); \
  double exp_val = floor(log10(interval / 2));                         \
  mark_step.param = pow(10.0, exp_val);

#define CALC_INTERVAL(param)                                 \
  int start = floor(viewBox.lim[0].param / mark_step.param); \
  int end = floor(viewBox.lim[1].param / mark_step.param);   \
  if (start < 0)                                             \
    ++start;                                                 \
  if (end < 0)                                               \
    ++end;                                                   \
  int count = abs(start - end) + 1;

  {
    CALC_STEP(x)
    if (mark_step.x > 0)
    {
      CALC_INTERVAL(x)
      for (int i = start; i <= end; ++i)
      {
        double value_ = i * mark_step.x;
        IPoint2 p = locate(Point2(value_, 0), viewBox, viewSize);
        MoveToEx((HDC)hdc, p.x, p.y - AXIS_MARK_SIZE, NULL);
        LineTo((HDC)hdc, p.x, p.y + AXIS_MARK_SIZE);

        if (checkTextNeed(count, i))
          drawMarkText(hdc, Point2(p.x, p.y + 2 * AXIS_MARK_SIZE), value_, TA_CENTER);
      }
    }
  }

  {
    CALC_STEP(y)
    if (mark_step.y > 0)
    {
      CALC_INTERVAL(y)
      for (int i = end; i <= start; ++i)
      {
        double value_ = i * mark_step.y;
        IPoint2 p = locate(Point2(0, value_), viewBox, viewSize);
        MoveToEx((HDC)hdc, p.x - AXIS_MARK_SIZE, p.y, NULL);
        LineTo((HDC)hdc, p.x + AXIS_MARK_SIZE, p.y);

        if (checkTextNeed(count, i))
          drawMarkText(hdc, Point2(p.x - 2 * AXIS_MARK_SIZE, p.y), value_, TA_RIGHT);
      }
    }
  }
}

bool WCurveControl::checkTextNeed(int count, int position)
{
  if (position == 0)
    return false;
  if (count > 8 && count <= 16 && position % 2 != 0)
    return false;
  if (count > 16 && position % 5 != 0)
    return false;
  return true;
}


void WCurveControl::drawPlotLine(void *hdc, const Point2 &p1, const Point2 &p2)
{
  IPoint2 p = locate(p1, viewBox, viewSize);
  MoveToEx((HDC)hdc, p.x, p.y, NULL);
  p = locate(p2, viewBox, viewSize);
  LineTo((HDC)hdc, p.x, p.y);
}

void WCurveControl::drawMarkText(void *hdc, const Point2 &pos, double _value, unsigned align)
{
  String txt(10, "%g", _value);
  HGDIOBJ old_font = SelectObject((HDC)hdc, (HGDIOBJ)this->getSmallPlotFont());
  SetTextColor((HDC)hdc, RGB(255, 255, 255));
  SetTextAlign((HDC)hdc, align);
  TextOut((HDC)hdc, pos.x, pos.y, txt.str(), txt.length());
  SelectObject((HDC)hdc, old_font);
}


intptr_t WCurveControl::onLButtonDown(long x, long y)
{
  if (!cb)
    return WindowControlBase::onLButtonDown(x, y);
  if (mode == MODE_SELECTION)
  {
    bool controlPressed = wingw::is_key_pressed(VK_CONTROL);
    bool altPressed = wingw::is_key_pressed(VK_MENU);

    Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
    Tab<int> pointIdx(tmpmem);
    cb->getControlPoints(points);
    int idx = -1;
    for (int i = 0; i < points.size(); i++)
    {
      if ((Point2(x, y) - locate(points[i].pos, viewBox, viewSize)).length() < CURVE_SENSITIVE)
      {
        idx = i;
      }
    }

    bool canRemoveSelection = cb->beginMoveControlPoints(idx);
    if (idx != -1)
      startMovement = true;

    // if ( altPressed || controlPressed || !canRemoveSelection )
    if (controlPressed)
    {
      for (int j = 0; j < points.size(); j++)
        if (points[j].isSelected() && !(altPressed && idx == j))
          pointIdx.push_back(j);
      if ((controlPressed || !canRemoveSelection) && idx > -1 && !points[idx].isSelected())
        pointIdx.push_back(idx);
    }
    else if (idx > -1)
    {
      if (points[idx].isSelected())
      {
        for (int j = 0; j < points.size(); j++)
          if (points[j].isSelected())
            pointIdx.push_back(j);
      }
      else
        pointIdx.push_back(idx);
    }

    if (cb->selectControlPoints(pointIdx))
      refresh(true);
    startingPos = Point2(x, y);
    cancelMovement = false;
    if (pointIdx.empty())
    {
      if (wingw::is_key_pressed(VK_CONTROL))
        mode = MODE_DRAG_COORD;
      else
        startRectangularSelection(x, y, 0);
    }
  }
  return 0;
}


intptr_t WCurveControl::onLButtonDClick(long x, long y)
{
  if (mode == MODE_SELECTION)
  {
    Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
    cb->getControlPoints(points);
    int idx = -1;
    for (int i = 0; i < points.size(); i++)
    {
      if ((Point2(x, y) - locate(points[i].pos, viewBox, viewSize)).length() < CURVE_SENSITIVE)
      {
        idx = i;
        break;
      }
    }

    if (idx > -1) // edit dialog
    {
      CDialogWindow setxy_dialog(getHandle(), _pxScaled(220), _pxScaled(100), "Select point coordinates");

      PropertyContainerControlBase *panel = setxy_dialog.getPanel();
      G_ASSERT(panel && "WCurveControl::onLButtonDClick: panel = NULL");

      panel->createEditFloat(DIALOG_ID_X, "x:", points[idx].pos.x, 3);
      panel->createEditFloat(DIALOG_ID_Y, "y:", points[idx].pos.y, 3, true, false);

      if (setxy_dialog.showDialog() == DIALOG_ID_OK)
      {
        Point2 from_starting_pos = Point2(panel->getFloat(DIALOG_ID_X), panel->getFloat(DIALOG_ID_Y)) - points[idx].pos;

        if (mEventHandler)
          mEventHandler->onWcChanging(this);

        if (cb->moveSelectedControlPoints(from_starting_pos))
        {
          cb->endMoveControlPoints(false);
          build();
          refresh(true);
          if (mEventHandler)
            mEventHandler->onWcChange(this);
        }
      }
    }
    else // new point
    {
      if (mMaxPtCount && points.size() >= mMaxPtCount)
        return 0;

      if (mEventHandler)
        mEventHandler->onWcChanging(this);

      if (cb->addNewControlPoint(getCoords(IPoint2(x, y), viewBox, viewSize)))
      {
        build();
        if (mEventHandler)
          mEventHandler->onWcChange(this);
      }

      // select new point
      this->onLButtonDown(x, y);
    }
  }

  return 0;
}


intptr_t WCurveControl::onLButtonUp(long x, long y)
{
  if (!cb)
    return WindowControlBase::onLButtonUp(x, y);
  if (mode == MODE_DRAG_COORD)
    mode = MODE_SELECTION;
  else if (mode == MODE_SELECTION)
  {
    startMovement = false;
    startingPos = Point2(-1, -1);
    if (cb->endMoveControlPoints(cancelMovement))
    {
      build();
    }
    MyRect rect;
    int seltype;
    if (endRectangularSelection(&rect, &seltype))
    {
      if (rect.r - rect.l > 1 && rect.b - rect.t > 1)
      {
        Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
        Tab<int> pointIdx(tmpmem);
        cb->getControlPoints(points);
        for (int i = 0; i < points.size(); i++)
        {
          IPoint2 p = locate(points[i].pos, viewBox, viewSize);
          if (rect.l <= p.x && rect.t <= p.y && rect.r >= p.x && rect.b >= p.y)
            pointIdx.push_back(i);
        }
        cb->selectControlPoints(pointIdx);
        clear_and_shrink(pointIdx);
        clear_and_shrink(points);
      }
    }
  }
  refresh(true);
  return 0;
}

static HCURSOR hc_select_cursor = LoadCursor(NULL, IDC_CROSS);
static HCURSOR hc_move_cursor = LoadCursor(NULL, IDC_SIZEALL);
static HCURSOR hc_hand_cursor = LoadCursor(NULL, IDC_HAND);

intptr_t WCurveControl::onDrag(int x, int y)
{
  if (mode == MODE_DRAG_COORD)
  {
    Point2 newMin(viewBox.leftTop());
    Point2 newMax(viewBox.rightBottom());

    newMin.y = viewBox.rightBottom().y;
    newMax.y = viewBox.leftTop().y;

    float step_x = (x - startingPos.x) * (newMax.x - newMin.x) / viewSize.x;
    float step_y = (y - startingPos.y) * (newMax.y - newMin.y) / viewSize.y;
    newMin.x -= step_x;
    newMin.y += step_y;
    newMax.x -= step_x;
    newMax.y += step_y;

    setViewBox(newMin, newMax);
    startingPos.x = x;
    startingPos.y = y;

    SetCursor(::hc_hand_cursor);
  }
  else if (mode == MODE_SELECTION)
  {
    if (startingPos != Point2(-1, -1))
    {
      if (startMovement)
      {
        startMovement = false;
        if (mEventHandler && isSelection())
          mEventHandler->onWcChanging(this);
      }

      Point2 fromStartingPos = Point2(x, y) - startingPos;
      fromStartingPos.x *= viewBox.width().x / viewSize.x;
      fromStartingPos.y *= viewBox.width().y / viewSize.y;

      if (cb->moveSelectedControlPoints(fromStartingPos))
      {
        build();
        SetCursor(::hc_move_cursor);
        if (mEventHandler)
          mEventHandler->onWcChange(this);
      }
    }
    if (rectSelect.active)
    {
      ProcessRectSelection(x, y);
      SetCursor(::hc_select_cursor);
    }
  }
  refresh(true);
  return 0;
}


intptr_t WCurveControl::onMouseMove(int x, int y)
{
  String text;

  if (mode == MODE_SELECTION)
  {
    Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
    cb->getControlPoints(points);
    int idx = -1;
    for (int i = 0; i < points.size(); i++)
    {
      if ((Point2(x, y) - locate(points[i].pos, viewBox, viewSize)).length() < CURVE_SENSITIVE)
      {
        idx = i;
        break;
      }
    }

    if (idx > -1)
    {
      text = String(20, "Point %d (%.2f, %.2f)", idx, points[idx].pos.x, points[idx].pos.y);
    }
  }

  mTooltip.setText(text);
  return 1;
}


intptr_t WCurveControl::onRButtonDown(long x, long y)
{
  if (!cb)
    return 0;

  Tab<int> pointIdx(tmpmem);
  cb->selectControlPoints(pointIdx);

  this->onLButtonDown(x, y);
  this->onLButtonUp(x, y);

  bool _remove = false;
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);
  for (int i = 0; i < points.size(); ++i)
    if (points[i].isSelected())
    {
      if (cb->getLockEnds() && (i == 0 || i == points.size() - 1))
        return 0;

      _remove = true;
      break;
    }

  if (_remove)
  {
    if (mMinPtCount && points.size() <= mMinPtCount)
      return 0;

    if (mEventHandler)
      mEventHandler->onWcChanging(this);

    if (cb->deleteSelectedControlPoints())
    {
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(this);
      refresh(true);
    }
  }

  return 0;
}

intptr_t WCurveControl::onVScroll(int dy, bool is_wheel)
{
  if (!is_wheel)
    return 0;

  Point2 newMin(viewBox.leftTop());
  Point2 newMax(viewBox.rightBottom());

  newMin.y = viewBox.rightBottom().y;
  newMax.y = viewBox.leftTop().y;

  float delta = (float)dy / CURVE_SCROLL_SPEED;

  float delta_x = (newMax.x - newMin.x);
  float delta_y = (newMax.y - newMin.y);

  newMin.x -= delta * delta_x;
  newMin.y -= delta * delta_y;
  newMax.x += delta * delta_x;
  newMax.y += delta * delta_y;

  setViewBox(newMin, newMax);
  refresh(true);

  return 1;
}

intptr_t WCurveControl::onKeyDown(unsigned v_key, int flags)
{
  switch (v_key)
  {
    case VK_ESCAPE:
      if (mode == MODE_SELECTION)
        cancelMovement = true;
      break;

    case 'Q':
      cb->setConnectedEnds(!cb->getConnectedEnds());
      build();
      break;

    case 'W': cb->setLockX(!cb->getLockX()); break;

    case 'E': cb->setLockEnds(!cb->getLockEnds()); break;

    case 'X': autoZoom(); break;

    case 'M': mDrawAMarks = !mDrawAMarks; break;

    case 'H':
      wingw::message_box(0, "Curve editor help",
        "Mouse: \r\n"
        " LB - select \r\n"
        " LB + CTRL - multiselect \r\n"
        " LB move + CTRL - move area \r\n"
        " RB - remove point \r\n"
        " WHEEL - zoom \r\n"
        " Double LB - edit point or new point \r\n"
        "\r\n"
        "Keyboard: \r\n"
        " ESC - cansel movement \r\n"
        " Q - connect ends \r\n"
        " W - no-loops mode \r\n"
        " E - lock ends \r\n"
        " X - autozoom\r\n"
        " M - show axis marks\r\n"
        " CTRL + C - copy points \r\n"
        " CTRL + V - paste points \r\n"
        " H - help\r\n");
      break;
  }

  refresh(true);
  return 0;
}


void WCurveControl::autoZoom()
{
  Point2 left_bottom = Point2(0, 0);
  Point2 right_top = Point2(1.0, 1.0);

  Point2 minXY, maxXY;
  if (cb)
  {
    minXY = cb->getMinXY();
    maxXY = cb->getMaxXY();
  }

  if (minXY != maxXY)
  {
    right_top.x = maxXY.x;
    right_top.y = maxXY.y;
    left_bottom.x = minXY.x;
    left_bottom.y = minXY.y;
  }
  else
  {
    Tab<Point2> _points(tmpmem);
    getValue(_points);
    for (int i = 0; i < _points.size(); ++i)
    {
      if (_points[i].x > right_top.x)
        right_top.x = _points[i].x;
      if (_points[i].y > right_top.y)
        right_top.y = _points[i].y;

      if (_points[i].x < left_bottom.x)
        left_bottom.x = _points[i].x;
      if (_points[i].y < left_bottom.y)
        left_bottom.y = _points[i].y;
    }
  }

  float x_delta = 0.1 * (right_top.x - left_bottom.x);
  float y_delta = 0.1 * (right_top.y - left_bottom.y);
  right_top.x += x_delta;
  right_top.y += y_delta;
  left_bottom.x -= x_delta;
  left_bottom.y -= y_delta;
  setViewBox(left_bottom, right_top);
}


void WCurveControl::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  viewSize.x = w;
  viewSize.y = h;
}

/*
void WCurveControl::delSelection()
{
  if (!cb)
    return;
  if ( cb->deleteSelectedControlPoints() )
    build();
}
*/

void WCurveControl::build()
{
  if (!cb)
    return;

  cb->buildPolylines(cachPolyLines, viewBox, viewSize);

  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);
  if (points.size() == 1)
  {
    ICurveControlCallback::PolyLine pl(mColor);
    pl.points.push_back(Point2(viewBox.lim[0].x, points[0].pos.y));
    pl.points.push_back(Point2(viewBox.lim[1].x, points[0].pos.y));
    cachPolyLines.push_back(pl);
  }
  refresh(true);
}

void WCurveControl::setValue(Tab<Point2> &source)
{
  if (!cb)
    return;

  if (cb->getCycled())
    source.back().y = source[0].y;

  cb->clear();
  for (int i = 0; i < source.size(); i++)
    cb->addNewControlPoint(source[i]);

  autoZoom();
  // build();
}

void WCurveControl::setCurValue(float value)
{
  mCurValue = value;
  refresh(true);
}

void WCurveControl::setCycled(bool cycled)
{
  if (!cb)
    return;

  cb->setCycled(cycled);
}

void WCurveControl::getValue(Tab<Point2> &dest) const
{
  if (!cb)
    return;
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);
  clear_and_shrink(dest);
  dest.resize(points.size());
  for (int i = 0; i < dest.size(); i++)
    dest[i] = points[i].pos;
}
