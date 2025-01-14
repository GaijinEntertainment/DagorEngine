// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "curveControlStandalone.h"
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include "../c_constants.h"
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/blkUtil.h>
#include <memory/dag_mem.h>
#include <windows.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

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

// This is a more convenient version of pre-registering all owned keys and then checking them with ImGui::IsKeyChordPressed.
static bool is_key_chord_pressed_owned(ImGuiKeyChord key_chord, ImGuiID canvas_id)
{
  if (!ImGui::IsKeyChordPressed(key_chord, ImGuiKeyOwner_Any))
    return false;

  ImGui::SetKeyOwnersForKeyChord(key_chord, canvas_id);
  return true;
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

CurveControlStandalone::CurveControlStandalone(WindowControlEventHandler *event_handler, WindowBase *parent, int left, int top, int w,
  int h, int id, const Point2 &left_bottom, const Point2 &right_top, ICurveControlCallback *curve_control_cb) :
  mEventHandler(event_handler),
  viewSize(w, h),
  viewOffset(0, 0),
  defaultViewBox(left_bottom, right_top),
  startingPos(-1, -1),
  cb(nullptr),
  cachPolyLines(tmpmem),
  mode(MODE_SELECTION),
  cancelMovement(false),
  startMovement(false),
  mColor(E3DCOLOR(0, 255, 0)),
  mCurValue(0),
  mMinPtCount(2),
  mMaxPtCount(0),
  mDrawAMarks(false),
  mSelected(false),
  lastMousePosInCanvas(-1.0f, -1.0f)
{
  setCB(curve_control_cb);

  setViewBox(left_bottom, right_top);
  memset(&rectSelect, 0, sizeof(rectSelect));
}

void CurveControlStandalone::setFixed(bool value)
{
  if (!cb)
    return;

  cb->setLockEnds(value);
  cb->setLockX(value);
}

void CurveControlStandalone::setMinMax(int min, int max)
{
  mMinPtCount = min;
  mMaxPtCount = max;
}

void CurveControlStandalone::setXMinMax(float min, float max)
{
  if (!cb)
    return;

  cb->setXMinMax(min, max);
}

void CurveControlStandalone::setYMinMax(float min, float max)
{
  if (!cb)
    return;

  cb->setYMinMax(min, max);
}

void CurveControlStandalone::setViewBox(const Point2 &left_bottom, const Point2 &right_top)
{
  viewBox.lim[0].x = left_bottom.x;
  viewBox.lim[0].y = right_top.y;
  viewBox.lim[1].x = right_top.x;
  viewBox.lim[1].y = left_bottom.y;
  build();
}

int CurveControlStandalone::ProcessRectSelection(int x, int y)
{
  rectSelect.sel.r = x;
  rectSelect.sel.b = y;
  return 0;
}

bool CurveControlStandalone::isSelection()
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);

  for (int i = 0; i < points.size(); i++)
    if (points[i].isSelected())
      return true;

  return false;
}

void CurveControlStandalone::startRectangularSelection(int mx, int my, int type)
{
  if (rectSelect.active)
    endRectangularSelection(nullptr, nullptr);

  rectSelect.active = true;
  rectSelect.type = type;
  rectSelect.sel.l = mx;
  rectSelect.sel.t = my;
  rectSelect.sel.r = mx;
  rectSelect.sel.b = my;
}

bool CurveControlStandalone::endRectangularSelection(MyRect *result, int *type)
{
  if (!rectSelect.active)
    return false;

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

void CurveControlStandalone::paintSelectionRect()
{
  if (rectSelect.sel.l == rectSelect.sel.r && rectSelect.sel.t == rectSelect.sel.b)
    return;

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  const ImVec2 leftTop(rectSelect.sel.l, rectSelect.sel.t);

  ImVec2 rightBottom;
  rightBottom.x = rectSelect.sel.r < viewSize.x                              ? rectSelect.sel.r
                  : rectSelect.sel.r > viewSize.x + 5000 /*some_big_number*/ ? 0
                                                                             : viewSize.x - 1;
  rightBottom.y = rectSelect.sel.b < viewSize.y - 1                          ? rectSelect.sel.b
                  : rectSelect.sel.b > viewSize.y + 5000 /*some_big_number*/ ? 0
                                                                             : viewSize.y - 1;
  // NOTE: ImGui porting: it was using PS_DASH.
  drawList->AddRect(leftTop + viewOffset, rightBottom + viewOffset, IM_COL32(~mColor.r, ~mColor.g, ~mColor.b, 255));
}

void CurveControlStandalone::drawHelp()
{
  if (!showHelp)
    return;

  ImGui::SetTooltip("Mouse: \n"
                    " LB - select \n"
                    " LB + CTRL - multiselect \n"
                    " LB move + CTRL - move area \n"
                    " RB - remove point \n"
                    " WHEEL - zoom \n"
                    " Double LB - edit point or new point \n"
                    "\n"
                    "Keyboard: \n"
                    " ESC - cancel movement \n"
                    " Q - connect ends \n"
                    " W - no-loops mode \n"
                    " E - lock ends \n"
                    " X - autozoom \n"
                    " M - show axis marks \n"
                    " CTRL + C - copy points \n"
                    " CTRL + V - paste points \n"
                    " H - toggle help \n");
}

void CurveControlStandalone::drawInternal()
{
  const ImU32 pen = IM_COL32(mColor.r, mColor.g, mColor.b, 255);
  const ImU32 black_pen = IM_COL32(0, 0, 0, 255);
  const ImU32 white_pen = IM_COL32(255, 255, 255, 255); // NOTE: ImGui porting: it was using PS_DASH.
  const ImU32 gray_pen = IM_COL32(128, 128, 128, 255);  // NOTE: ImGui porting: it was using PS_DOT.
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // draw current X value
  if (mCurValue)
    drawPlotLine(Point2(mCurValue, viewBox.lim[0].y), Point2(mCurValue, viewBox.lim[1].y), white_pen);

  // draw minimum and maximum
  if (cb)
  {
    Point2 minXY, maxXY, lt, rb;
    minXY = cb->getMinXY();
    maxXY = cb->getMaxXY();
    lt.x = (minXY.x == maxXY.x) ? viewBox.lim[0].x : minXY.x;
    rb.x = (minXY.x == maxXY.x) ? viewBox.lim[1].x : maxXY.x;
    lt.y = (minXY.y == maxXY.y) ? viewBox.lim[0].y : minXY.y;
    rb.y = (minXY.y == maxXY.y) ? viewBox.lim[1].y : maxXY.y;

    if (minXY.x != maxXY.x)
    {
      drawPlotLine(Point2(minXY.x, lt.y), Point2(minXY.x, rb.y), gray_pen);
      drawPlotLine(Point2(maxXY.x, lt.y), Point2(maxXY.x, rb.y), gray_pen);
    }
    if (minXY.y != maxXY.y)
    {
      drawPlotLine(Point2(lt.x, minXY.y), Point2(rb.x, minXY.y), gray_pen);
      drawPlotLine(Point2(lt.x, maxXY.y), Point2(rb.x, maxXY.y), gray_pen);
    }
  }

  // draw coord lines
  drawPlotLine(Point2(viewBox.lim[0].x, 0), Point2(viewBox.lim[1].x, 0), black_pen);
  drawPlotLine(Point2(0, viewBox.lim[0].y), Point2(0, viewBox.lim[1].y), black_pen);
  if (mDrawAMarks)
    drawAxisMarks(black_pen);

  // curve
  Tab<IPoint2> linePts(tmpmem);

  for (int i = 0; i < cachPolyLines.size(); i++)
  {
    linePts.resize(cachPolyLines[i].points.size());
    for (int j = 0; j < linePts.size(); ++j)
      linePts[j] = locate(cachPolyLines[i].points[j], viewBox, viewSize);

    Tab<ImVec2> t(tmpmem);
    t.resize(linePts.size());
    for (int i = 0; i < linePts.size(); i++)
    {
      t[i].x = linePts[i].x + viewOffset.x;
      t[i].y = linePts[i].y + viewOffset.y;
    }
    drawList->AddPolyline(&t[0], t.size(), pen, ImDrawFlags_None, 1.0f);
  }

  // control lines & points
  if (!cb)
    return;

  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);

  //////////////////////
  /*for ( int i = 0; i < points.size(); i++ )
  {
    SelectObject(draw->hDC,pen2);
    if (points[i].isSmooth())
    {
      IPoint2 o = locate( points[i].pos, viewBox, viewSize);
      MoveToEx(draw->hDC, o.x, o.y, nullptr);

      p = locate( points[i].pos, viewBox, viewSize);
      LineTo(draw->hDC, p.x, p.y );

      MoveToEx(draw->hDC, o.x, o.y, nullptr);
      p = locate( points[i].out, viewBox, viewSize);
      LineTo(draw->hDC, p.x, p.y );
    }
  }*/

  for (int i = 0; i < points.size(); i++)
  {
    const IPoint2 p = locate(points[i].pos, viewBox, viewSize);
    const ImVec2 leftTop(p.x - CURVE_POINT_RAD, p.y - CURVE_POINT_RAD);
    const ImVec2 rightBottom(p.x + CURVE_POINT_RAD, p.y + CURVE_POINT_RAD);
    const ImU32 color = points[i].isSelected() ? IM_COL32(255, 255, 255, 255) : IM_COL32(0, 0, 0, 255);
    drawList->AddRectFilled(leftTop + viewOffset, rightBottom + viewOffset, color);
  }

  clear_and_shrink(points);

  // selectionRect
  if (rectSelect.active)
    paintSelectionRect();

  // draw around rect
  const ImVec2 leftTop(0, 0);
  const ImVec2 rightBottom(viewSize.x - 1, viewSize.y - 1);
  drawList->AddRect(leftTop + viewOffset, rightBottom + viewOffset, mSelected ? IM_COL32(255, 0, 0, 255) : black_pen);

  drawHelp();
}

void CurveControlStandalone::draw()
{
  // Setting the clipping rectangle is needed because with panning the coordinates can get out of our draw area.
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  drawList->PushClipRect(viewOffset, viewSize + viewOffset, true);

  drawInternal();

  drawList->PopClipRect();
}

void CurveControlStandalone::drawAxisMarks(uint32_t color)
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();

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
        drawList->AddLine(ImVec2(p.x, p.y - AXIS_MARK_SIZE) + viewOffset, ImVec2(p.x, p.y + AXIS_MARK_SIZE) + viewOffset, color);

        if (checkTextNeed(count, i))
          drawMarkText(Point2(p.x, p.y + 2 * AXIS_MARK_SIZE), value_, TA_CENTER);
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
        drawList->AddLine(ImVec2(p.x - AXIS_MARK_SIZE, p.y) + viewOffset, ImVec2(p.x + AXIS_MARK_SIZE, p.y) + viewOffset, color);

        if (checkTextNeed(count, i))
          drawMarkText(Point2(p.x - 2 * AXIS_MARK_SIZE, p.y), value_, TA_RIGHT);
      }
    }
  }
}

bool CurveControlStandalone::checkTextNeed(int count, int position)
{
  if (position == 0)
    return false;
  if (count > 8 && count <= 16 && position % 2 != 0)
    return false;
  if (count > 16 && position % 5 != 0)
    return false;
  return true;
}

void CurveControlStandalone::drawPlotLine(const Point2 &p1, const Point2 &p2, uint32_t color)
{
  G_STATIC_ASSERT(sizeof(color) == sizeof(ImU32));

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  IPoint2 p1Loc = locate(p1, viewBox, viewSize);
  IPoint2 p2Loc = locate(p2, viewBox, viewSize);
  drawList->AddLine(ImVec2(p1Loc.x, p1Loc.y) + viewOffset, ImVec2(p2Loc.x, p2Loc.y) + viewOffset, color);
}

void CurveControlStandalone::drawMarkText(const Point2 &pos, double _value, unsigned align)
{
  String txt(10, "%g", _value);
  ImDrawList *drawList = ImGui::GetWindowDrawList();
  drawList->AddText(pos + viewOffset, IM_COL32(255, 255, 255, 255), txt.c_str());
}

void CurveControlStandalone::onLButtonDown(long x, long y)
{
  if (!cb)
    return;

  if (mode == MODE_SELECTION)
  {
    const bool controlPressed = ImGui::IsKeyDown(ImGuiMod_Ctrl);
    const bool altPressed = ImGui::IsKeyDown(ImGuiMod_Alt);

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

    cb->selectControlPoints(pointIdx);

    startingPos = Point2(x, y);
    cancelMovement = false;
    if (pointIdx.empty())
    {
      if (controlPressed)
        mode = MODE_DRAG_COORD;
      else
        startRectangularSelection(x, y, 0);
    }
  }
}

void CurveControlStandalone::showEditDialog(int idx)
{
  if (mode != MODE_SELECTION)
    return;

  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);
  if (idx < 0 || idx >= points.size())
    return;

  DialogWindow setxy_dialog(nullptr, hdpi::_pxScaled(220), hdpi::_pxScaled(100), "Select point coordinates");

  PropPanel::ContainerPropertyControl *panel = setxy_dialog.getPanel();
  G_ASSERT(panel && "CurveControlStandalone::onLButtonDClick: panel = nullptr");

  panel->createEditFloat(DIALOG_ID_X, "x:", points[idx].pos.x, 3);
  panel->createEditFloat(DIALOG_ID_Y, "y:", points[idx].pos.y, 3, true, false);

  if (setxy_dialog.showDialog() == DIALOG_ID_OK)
  {
    Point2 from_starting_pos = Point2(panel->getFloat(DIALOG_ID_X), panel->getFloat(DIALOG_ID_Y)) - points[idx].pos;

    if (mEventHandler)
      mEventHandler->onWcChanging(windowBaseForEventHandler);

    if (cb->moveSelectedControlPoints(from_starting_pos))
    {
      cb->endMoveControlPoints(false);
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }
}

void CurveControlStandalone::onLButtonDClick(long x, long y)
{
  if (mode == MODE_SELECTION)
  {
    remove_delayed_callback(*this);
    delayedEditDialogPointIndex = -1;

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
      delayedEditDialogPointIndex = idx;
      request_delayed_callback(*this);
    }
    else // new point
    {
      if (mMaxPtCount && points.size() >= mMaxPtCount)
        return;

      if (mEventHandler)
        mEventHandler->onWcChanging(windowBaseForEventHandler);

      if (cb->addNewControlPoint(getCoords(IPoint2(x, y), viewBox, viewSize)))
      {
        build();
        if (mEventHandler)
          mEventHandler->onWcChange(windowBaseForEventHandler);
      }

      // select new point
      onLButtonDown(x, y);
    }
  }
}

void CurveControlStandalone::onLButtonUp(long x, long y)
{
  if (!cb)
    return;

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
}

// TODO: ImGui porting: use ImGui for cursor changing (but it does not have all the cursor types)
static HCURSOR hc_select_cursor = LoadCursor(nullptr, IDC_CROSS);
static HCURSOR hc_move_cursor = LoadCursor(nullptr, IDC_SIZEALL);
static HCURSOR hc_hand_cursor = LoadCursor(nullptr, IDC_HAND);

void CurveControlStandalone::onDrag(int x, int y)
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

    SetCursor(hc_hand_cursor);
  }
  else if (mode == MODE_SELECTION)
  {
    if (startingPos != Point2(-1, -1))
    {
      if (startMovement)
      {
        startMovement = false;
        if (mEventHandler && isSelection())
          mEventHandler->onWcChanging(windowBaseForEventHandler);
      }

      Point2 fromStartingPos = Point2(x, y) - startingPos;
      fromStartingPos.x *= viewBox.width().x / viewSize.x;
      fromStartingPos.y *= viewBox.width().y / viewSize.y;

      if (cb->moveSelectedControlPoints(fromStartingPos))
      {
        build();
        SetCursor(hc_move_cursor);
        if (mEventHandler)
          mEventHandler->onWcChange(windowBaseForEventHandler);
      }
    }
    if (rectSelect.active)
    {
      ProcessRectSelection(x, y);
      SetCursor(hc_select_cursor);
    }
  }
}

void CurveControlStandalone::onMouseMove(int x, int y)
{
  tooltipText.clear();

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
      tooltipText.printf(20, "Point %d (%.2f, %.2f)", idx, points[idx].pos.x, points[idx].pos.y);
  }
}

void CurveControlStandalone::onRButtonDown(long x, long y)
{
  if (!cb)
    return;

  Tab<int> pointIdx(tmpmem);
  cb->selectControlPoints(pointIdx);

  onLButtonDown(x, y);
  onLButtonUp(x, y);

  bool _remove = false;
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cb->getControlPoints(points);
  for (int i = 0; i < points.size(); ++i)
    if (points[i].isSelected())
    {
      if (cb->getLockEnds() && (i == 0 || i == points.size() - 1))
        return;

      _remove = true;
      break;
    }

  if (_remove)
  {
    if (mMinPtCount && points.size() <= mMinPtCount)
      return;

    if (mEventHandler)
      mEventHandler->onWcChanging(windowBaseForEventHandler);

    if (cb->deleteSelectedControlPoints())
    {
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }
}

void CurveControlStandalone::onVScroll(int dy, bool is_wheel)
{
  if (!is_wheel)
    return;

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
}

void CurveControlStandalone::handleKeyPresses(unsigned canvas_id)
{
  G_STATIC_ASSERT(sizeof(canvas_id) == sizeof(ImGuiID));

  if (is_key_chord_pressed_owned(ImGuiKey_Escape, canvas_id))
  {
    if (mode == MODE_SELECTION)
      cancelMovement = true;
  }
  else if (is_key_chord_pressed_owned(ImGuiKey_Q, canvas_id))
  {
    cb->setConnectedEnds(!cb->getConnectedEnds());
    build();
  }
  else if (is_key_chord_pressed_owned(ImGuiKey_W, canvas_id))
  {
    cb->setLockX(!cb->getLockX());
  }
  else if (is_key_chord_pressed_owned(ImGuiKey_E, canvas_id))
  {
    cb->setLockEnds(!cb->getLockEnds());
  }
  else if (is_key_chord_pressed_owned(ImGuiKey_X, canvas_id))
  {
    autoZoom();
  }
  else if (is_key_chord_pressed_owned(ImGuiKey_M, canvas_id))
  {
    mDrawAMarks = !mDrawAMarks;
  }
  else if (is_key_chord_pressed_owned(ImGuiKey_H, canvas_id))
  {
    showHelp = !showHelp;
  }
  else if (mEventHandler && is_key_chord_pressed_owned(ImGuiMod_Ctrl | ImGuiKey_C, canvas_id))
  {
    DataBlock blk;
    mEventHandler->onWcClipboardCopy(nullptr, blk);

    SimpleString text = blk_util::blkTextData(blk);
    if (!text.empty())
      ImGui::SetClipboardText(text);
  }
  else if (mEventHandler && is_key_chord_pressed_owned(ImGuiMod_Ctrl | ImGuiKey_V, canvas_id))
  {
    if (const char *text = ImGui::GetClipboardText())
    {
      DataBlock blk;
      dblk::load_text(blk, make_span(text, i_strlen(text)), dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS);
      if (blk.isValid())
        mEventHandler->onWcClipboardPaste(nullptr, blk);
    }
  }
}

void CurveControlStandalone::autoZoom()
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

/*
void CurveControlStandalone::delSelection()
{
  if (!cb)
    return;
  if ( cb->deleteSelectedControlPoints() )
    build();
}
*/

void CurveControlStandalone::build()
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
}

void CurveControlStandalone::setValue(Tab<Point2> &source)
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

void CurveControlStandalone::setCurValue(float value) { mCurValue = value; }

void CurveControlStandalone::setCycled(bool cycled)
{
  if (!cb)
    return;

  cb->setCycled(cycled);
}

void CurveControlStandalone::getValue(Tab<Point2> &dest) const
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

void CurveControlStandalone::onImguiDelayedCallback(void *user_data)
{
  showEditDialog(delayedEditDialogPointIndex);
  delayedEditDialogPointIndex = -1;
}

void CurveControlStandalone::updateImgui(float width, float height)
{
  ImGui::PushID(this);

  if (viewSize.x != width || viewSize.y != height)
  {
    viewSize.x = width;
    viewSize.y = height;
    build();
  }

  viewOffset = ImGui::GetCursorScreenPos();

  // This will catch the interactions.
  const ImGuiID canvasId = ImGui::GetCurrentWindow()->GetID("canvas");
  ImGui::PushID(canvasId);
  ImGui::InvisibleButton("", viewSize);
  ImGui::PopID();

  const bool isHovered = ImGui::IsItemHovered();
  const ImVec2 mousePosInCanvas = ImGui::GetIO().MousePos - viewOffset;

  if (isHovered)
  {
    if (mousePosInCanvas != lastMousePosInCanvas)
    {
      lastMousePosInCanvas = mousePosInCanvas;

      if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        onDrag(mousePosInCanvas.x, mousePosInCanvas.y);
      else
        onMouseMove(mousePosInCanvas.x, mousePosInCanvas.y);
    }

    if (!tooltipText.empty() && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
      ImGui::SetItemTooltip("%s", tooltipText.c_str());

    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      onLButtonDClick(mousePosInCanvas.x, mousePosInCanvas.y);
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
      onLButtonDown(mousePosInCanvas.x, mousePosInCanvas.y);
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
      onLButtonUp(mousePosInCanvas.x, mousePosInCanvas.y);
    else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
      onRButtonDown(mousePosInCanvas.x, mousePosInCanvas.y);

    handleKeyPresses(canvasId);
  }
  else
  {
    showHelp = false;
  }

  draw();

  ImGui::PopID();
}

} // namespace PropPanel