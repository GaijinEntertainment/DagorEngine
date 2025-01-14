// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "colorCurveControl.h"
#include "../c_constants.h"
#include <memory/dag_mem.h>

#include <propPanel/commonWindow/color_correction_info.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>

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

// -----------------------------------------------


enum
{
  CURVE_SENSITIVE = 5,
  CURVE_POINT_RAD = 3,
  CURVE_SCROLL_SPEED = 10,

  DIALOG_ID_X = 1000,
  DIALOG_ID_Y,
};


ImU32 CURVE_COLORS[COLOR_CURVE_MODE_COUNT] = {
  IM_COL32(64, 64, 64, 255), IM_COL32(192, 0, 0, 255), IM_COL32(0, 192, 0, 255), IM_COL32(0, 0, 192, 255)};


ImU32 BAR_COLORS[COLOR_CURVE_MODE_COUNT] = {
  IM_COL32(162, 162, 162, 255), IM_COL32(255, 192, 192, 255), IM_COL32(192, 255, 192, 255), IM_COL32(192, 192, 255, 255)};


ColorCurveControl::ColorCurveControl(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :
  mEventHandler(event_handler),
  viewOffset(0, 0),
  startingPos(-1, -1),
  cachPolyLinesRGB(tmpmem),
  cachPolyLinesR(tmpmem),
  cachPolyLinesG(tmpmem),
  cachPolyLinesB(tmpmem),
  cancelMovement(false),
  startMovement(false),
  mMode(COLOR_CURVE_MODE_RGB),
  showChartBars(true),
  showBaseLine(true),
  lastMousePosInCanvas(-1.0f, -1.0f)
{
  viewBox.lim[0].x = -9;
  viewBox.lim[0].y = 260;
  viewBox.lim[1].x = 260;
  viewBox.lim[1].y = -9;
  viewSize = Point2(269, 269);

  cachPolyLines[COLOR_CURVE_MODE_RGB] = &cachPolyLinesRGB;
  cachPolyLines[COLOR_CURVE_MODE_R] = &cachPolyLinesR;
  cachPolyLines[COLOR_CURVE_MODE_G] = &cachPolyLinesG;
  cachPolyLines[COLOR_CURVE_MODE_B] = &cachPolyLinesB;

  for (int i = 0; i < COLOR_CURVE_MODE_COUNT; ++i)
  {
    cbs[i] = new CatmullRomCBTest();
    cbs[i]->setYMinMax(0, 255);
    cbs[i]->setXMinMax(0, 255);
    cbs[i]->setLockX(true);
    cbs[i]->addNewControlPoint(Point2(0, 0));
    cbs[i]->addNewControlPoint(Point2(255, 255));
  }

  memset(bar_chart, 0, 256 * 3);
  build();
}


ColorCurveControl::~ColorCurveControl()
{
  for (int i = 0; i < COLOR_CURVE_MODE_COUNT; ++i)
    delete cbs[i];
}


void ColorCurveControl::setMode(int _mode)
{
  if (_mode < COLOR_CURVE_MODE_RGB || _mode > COLOR_CURVE_MODE_COUNT - 1)
    mMode = COLOR_CURVE_MODE_RGB;
  else
    mMode = _mode;

  build();
}


void ColorCurveControl::setBarChart(const unsigned char rgb_bar_charts[3][256]) { memcpy(bar_chart, rgb_bar_charts, 256 * 3); }


void ColorCurveControl::build()
{
  if (mMode == COLOR_CURVE_MODE_RGB)
    for (int i = 0; i < COLOR_CURVE_MODE_COUNT; ++i)
    {
      cbs[i]->buildPolylines(*cachPolyLines[i], viewBox, viewSize);
      cutCurve(i);
    }
  else
  {
    cbs[mMode]->buildPolylines(*cachPolyLines[mMode], viewBox, viewSize);
    cutCurve(mMode);
  }

  lastMousePosInCanvas = Point2(-1.0f, -1.0f); // Force recreating the tooltip.
}


inline void cut_val(float &val) { val = (val < 0) ? 0 : ((val > 255) ? 255 : val); }


inline bool check_val(float val) { return (val >= 0) && (val < 256); }


void ColorCurveControl::cutCurve(int index)
{
  PolyLineTab &cpl = *cachPolyLines[index];

  // cut curves [0,0]:[255,255]
  for (int i = 0; i < cpl.size();)
  {
    for (int j = 0; j < cpl[i].points.size();)
    {
      Point2 &pt = cpl[i].points[j];

      if (!check_val(pt.x) && !check_val(pt.y))
        erase_items(cpl[i].points, j, 1);
      else
      {
        cut_val(pt.x);
        cut_val(pt.y);
        ++j;
      }
    }

    if (cpl[i].points.size() > 1)
      i++;
    else
      erase_items(cpl, i, 1);
  }

  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[index]->getControlPoints(points);
  int pc = points.size();
  if (pc > 0 && cpl.size())
  {
    // cut border point lines
    for (int i = 0; i < cpl[0].points.size();)
    {
      Point2 &pt = cpl[0].points[i];
      if (pt.x < points[0].pos.x)
        erase_items(cpl[0].points, i, 1);
      else
        ++i;
    }

    for (int i = 0; i < cpl.back().points.size();)
    {
      Point2 &pt = cpl.back().points[i];
      if (pt.x > points[pc - 1].pos.x)
        erase_items(cpl.back().points, i, 1);
      else
        ++i;
    }

    // and append end lines
    if (points[0].pos.x != 0)
    {
      ICurveControlCallback::PolyLine line;
      line.points.push_back(Point2(0, points[0].pos.y));
      line.points.push_back(points[0].pos);
      insert_items(*cachPolyLines[index], 0, 1, &line);
    }
    if (points[pc - 1].pos.x != 255)
    {
      ICurveControlCallback::PolyLine line;
      line.points.push_back(points[pc - 1].pos);
      line.points.push_back(Point2(255, points[pc - 1].pos.y));
      cachPolyLines[index]->push_back(line);
    }
  }
}


bool ColorCurveControl::isSelection()
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[mMode]->getControlPoints(points);

  for (int i = 0; i < points.size(); i++)
    if (points[i].isSelected())
      return true;

  return false;
}


void ColorCurveControl::drawGeom()
{
  const IPoint2 p0 = locate(Point2(0, 0), viewBox, viewSize);
  const IPoint2 p1 = locate(Point2(255, 255), viewBox, viewSize);
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(ImVec2(0.0f, 0.0f) + viewOffset, viewSize + viewOffset, IM_COL32(255, 255, 255, 255));
  drawList->AddRect(ImVec2(0.0f, 0.0f) + viewOffset, ImVec2(viewSize.x - 0.0f, viewSize.y - 0.0f) + viewOffset,
    IM_COL32(0, 0, 0, 255));

  // gradients
  for (int i = 0; i < (p1.x - p0.x); ++i)
  {
    float color_val = floor(255.0 * i / (p1.x - p0.x));
    ImU32 color = IM_COL32(color_val, color_val, color_val, 255);
    drawList->AddLine(ImVec2(p0.x + i + 1, viewSize.y - 1) + viewOffset, ImVec2(p0.x + i + 1, p0.y) + viewOffset, color);
  }
  for (int i = 0; i < (p0.y - p1.y); ++i)
  {
    float color_val = 255 - floor(255.0 * i / (p0.y - p1.y));
    ImU32 color = IM_COL32(color_val, color_val, color_val, 255);
    drawList->AddLine(ImVec2(1, i + p1.y - 1) + viewOffset, ImVec2(p0.x, i + p1.y - 1) + viewOffset, color);
  }

  // lines
  const ImU32 lineColor = IM_COL32(212, 212, 212, 255);

  // grid
  const IPoint2 p_step = (p1 - p0 + IPoint2(1, -1)) / 4;
  for (int i = 0; i <= 4; ++i)
  {
    const float x = p0.x + (i * p_step.x);
    drawList->AddLine(ImVec2(x, p0.y) + viewOffset, ImVec2(x, p1.y - 1) + viewOffset, lineColor);

    const float y = p0.y + (i * p_step.y) - 1;
    drawList->AddLine(ImVec2(p0.x, y) + viewOffset, ImVec2(p1.x + 2, y) + viewOffset, lineColor);
  }

  // base line
  if (showBaseLine)
    drawList->AddLine(ImVec2(p0.x, p0.y) + viewOffset, ImVec2(p1.x, p1.y) + viewOffset, lineColor);

  // draw coord lines
  /*
  SelectObject(hDC, black_pen);
  IPoint2 p = locate(Point2(viewBox.lim[0].x, 0), viewBox, viewSize);
  MoveToEx( hDC, p.x, p.y, NULL );
  p = locate(Point2(viewBox.lim[1].x, 0), viewBox, viewSize);
  LineTo( hDC, p.x, p.y );
  p = locate(Point2(0, viewBox.lim[0].y), viewBox, viewSize);
  MoveToEx(hDC, p.x, p.y, NULL );
  p = locate(Point2(0, viewBox.lim[1].y), viewBox, viewSize);
  LineTo( hDC, p.x, p.y );
  */

  // bars
  if (showChartBars)
    drawBarChart(mMode);

  // curves
  if (mMode == COLOR_CURVE_MODE_RGB)
  {
    for (int i = COLOR_CURVE_MODE_COUNT - 1; i >= 0; --i)
      drawCurve(i);
  }
  else
  {
    drawCurve(mMode);
  }
}


void ColorCurveControl::drawCurve(int mode_index)
{
  const ImU32 color = CURVE_COLORS[mode_index];
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // curve
  Tab<IPoint2> linePts(tmpmem);
  PolyLineTab &cach_poly_lines = *cachPolyLines[mode_index];

  for (int i = 0; i < cach_poly_lines.size(); i++)
  {
    linePts.resize(cach_poly_lines[i].points.size());
    for (int j = 0; j < linePts.size(); ++j)
      linePts[j] = locate(cach_poly_lines[i].points[j], viewBox, viewSize);

    Tab<ImVec2> t(tmpmem);
    t.resize(linePts.size());
    for (int i = 0; i < linePts.size(); i++)
    {
      t[i].x = linePts[i].x + viewOffset.x;
      t[i].y = linePts[i].y + viewOffset.y;
    }

    drawList->AddPolyline(&t[0], t.size(), color, ImDrawFlags_None, 1.0f);
  }

  // control points
  if (cbs[mode_index] && mode_index == mMode)
  {
    Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
    cbs[mode_index]->getControlPoints(points);

    for (int i = 0; i < points.size(); i++)
    {
      const IPoint2 p = locate(points[i].pos, viewBox, viewSize);
      const ImVec2 leftTop(p.x - CURVE_POINT_RAD, p.y - CURVE_POINT_RAD);
      const ImVec2 rightBottom(p.x + CURVE_POINT_RAD, p.y + CURVE_POINT_RAD);

      if (points[i].isSelected())
        drawList->AddRectFilled(leftTop + viewOffset, rightBottom + viewOffset, color);
      else
        drawList->AddRect(leftTop + viewOffset, rightBottom + viewOffset, color);
    }
    clear_and_shrink(points);
  }
}


void ColorCurveControl::drawBarChart(int mode_index)
{
  const ImU32 color = BAR_COLORS[mode_index];
  const IPoint2 p0 = locate(Point2(0, 0), viewBox, viewSize);
  const IPoint2 p1 = locate(Point2(255, 255), viewBox, viewSize);
  const int max_ht = p0.y - p1.y;
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  for (int i = 0; i < 256; ++i)
  {
    int x = p0.x + (p1.x - p0.x) * i / 255;
    int y = p0.y;

    if (mode_index > 0)
      y -= max_ht * bar_chart[mode_index - 1][i] / 255;
    else
    {
      int rgb_val = 0;
      for (int j = 0; j < 3; ++j)
        rgb_val += bar_chart[j][i];

      y -= max_ht * rgb_val / 255 / 3;
    }

    drawList->AddLine(ImVec2(x, p0.y) + viewOffset, ImVec2(x, y) + viewOffset, color);
  }
}


void ColorCurveControl::onLButtonDown(long x, long y)
{
  if (!cbs[mMode])
    return;

  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  Tab<int> pointIdx(tmpmem);
  cbs[mMode]->getControlPoints(points);

  int idx = -1;
  for (int i = 0; i < points.size(); i++)
  {
    if ((Point2(x, y) - locate(points[i].pos, viewBox, viewSize)).length() < CURVE_SENSITIVE)
    {
      idx = i;
    }
  }

  bool canRemoveSelection = cbs[mMode]->beginMoveControlPoints(idx);
  if (idx != -1)
  {
    startMovement = true;
    pointIdx.push_back(idx);
  }

  cbs[mMode]->selectControlPoints(pointIdx);
  startingPos = Point2(x, y);
  cancelMovement = false;
}


void ColorCurveControl::onLButtonUp(long x, long y)
{
  if (!cbs[mMode])
    return;

  startMovement = false;
  startingPos = Point2(-1, -1);
  if (cbs[mMode]->endMoveControlPoints(cancelMovement))
    build();
}


void ColorCurveControl::onDrag(int x, int y)
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

    if (cbs[mMode]->moveSelectedControlPoints(fromStartingPos))
    {
      build();
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); // TODO: ImGui porting: the original used IDC_CROSS here
      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }
}


void ColorCurveControl::onMouseMove(int x, int y)
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[mMode]->getControlPoints(points);
  int idx = -1;
  for (int i = 0; i < points.size(); i++)
  {
    if ((Point2(x, y) - locate(points[i].pos, viewBox, viewSize)).length() < CURVE_SENSITIVE)
    {
      idx = i;
      break;
    }
  }

  if (idx >= 0)
    tooltipText.printf(20, "Point %d (%.0f, %.0f)", idx, floor(points[idx].pos.x), floor(points[idx].pos.y));
  else
    tooltipText.clear();
}


void ColorCurveControl::onLButtonDClick(long x, long y)
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[mMode]->getControlPoints(points);
  pointsIndexForShowEditDialog = -1;
  for (int i = 0; i < points.size(); i++)
  {
    if ((Point2(x, y) - locate(points[i].pos, viewBox, viewSize)).length() < CURVE_SENSITIVE)
    {
      pointsIndexForShowEditDialog = i;
      break;
    }
  }

  if (pointsIndexForShowEditDialog > -1) // edit dialog
  {
    // The dialog cannot be opened from updateImgui, it must be delayed. This requires a really roundabout way to do it...
    if (mEventHandler)
      mEventHandler->onWcDoubleClick(windowBaseForEventHandler);
  }
  else // new point
  {
    if (mEventHandler)
      mEventHandler->onWcChanging(windowBaseForEventHandler);

    if (cbs[mMode]->addNewControlPoint(getCoords(IPoint2(x, y), viewBox, viewSize)))
    {
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }

    // select new point
    this->onLButtonDown(x, y);
  }
}


void ColorCurveControl::onRButtonDown(long x, long y)
{
  if (!cbs[mMode])
    return;

  Tab<int> pointIdx(tmpmem);
  cbs[mMode]->selectControlPoints(pointIdx);

  this->onLButtonDown(x, y);
  this->onLButtonUp(x, y);

  bool _remove = false;
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[mMode]->getControlPoints(points);
  for (int i = 0; i < points.size(); ++i)
    if (points[i].isSelected())
    {
      _remove = true;
      break;
    }

  if (_remove)
  {
    if (points.size() <= 2)
      return;

    if (mEventHandler)
      mEventHandler->onWcChanging(windowBaseForEventHandler);

    if (cbs[mMode]->deleteSelectedControlPoints())
    {
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }
}


void ColorCurveControl::getColorTables(unsigned char rgb_r_g_b[4][256])
{
  memset(rgb_r_g_b, 0, 256 * 4);

  for (int ch_ind = 0; ch_ind < 4; ++ch_ind)
  {
    PolyLineTab &cpl = *cachPolyLines[ch_ind];
    int pt_ind = 0, pl_ind = 0, pl_pt_ind = 0, last_val = 0;
    Point2 *pt0 = NULL, *pt1 = NULL;
    while (pt_ind < 256)
    {
      if (pt0 && pt1)
      {
        if (pt_ind <= pt1->x)
        {
          if (pt_ind >= pt0->x)
          {
            if ((pt0->x == pt1->x) || (pt0->y == pt1->y))
              last_val = pt0->y;
            else
            {
              float f = (pt_ind - pt0->x) / (pt1->x - pt0->x);
              last_val = lerp(pt0->y, pt1->y, f);
            }
          }

          rgb_r_g_b[ch_ind][pt_ind] = last_val;
          ++pt_ind;
          continue;
        }
        ++pl_pt_ind;
      }


      if (pl_pt_ind + 2 > cpl[pl_ind].points.size())
      {
        pl_pt_ind = 0;
        ++pl_ind;
      }

      if (pl_ind + 1 > cpl.size())
        while (pt_ind < 256)
        {
          rgb_r_g_b[ch_ind][pt_ind] = last_val;
          ++pt_ind;
        }

      pt0 = &cpl[pl_ind].points[pl_pt_ind];
      pt1 = &cpl[pl_ind].points[pl_pt_ind + 1];
    }
  }
}


void ColorCurveControl::setCorrectionInfo(const ColorCorrectionInfo &settings)
{
  setPoints(COLOR_CURVE_MODE_RGB, settings.rgbPoints);
  setPoints(COLOR_CURVE_MODE_R, settings.rPoints);
  setPoints(COLOR_CURVE_MODE_G, settings.gPoints);
  setPoints(COLOR_CURVE_MODE_B, settings.bPoints);
}


void ColorCurveControl::getCorrectionInfo(ColorCorrectionInfo &settings)
{
  getPoints(COLOR_CURVE_MODE_RGB, settings.rgbPoints);
  getPoints(COLOR_CURVE_MODE_R, settings.rPoints);
  getPoints(COLOR_CURVE_MODE_G, settings.gPoints);
  getPoints(COLOR_CURVE_MODE_B, settings.bPoints);
}


void ColorCurveControl::setPoints(int index, const Tab<Point2> &points)
{
  if (index < COLOR_CURVE_MODE_COUNT && cbs[index])
  {
    cbs[index]->clear();
    for (int i = 0; i < points.size(); i++)
      cbs[index]->addNewControlPoint(points[i]);
    build();
  }
}


void ColorCurveControl::getPoints(int index, Tab<Point2> &points)
{
  if (index < COLOR_CURVE_MODE_COUNT && cbs[index])
  {
    Tab<ICurveControlCallback::ControlPoint> _points(tmpmem);
    cbs[index]->getControlPoints(_points);
    clear_and_shrink(points);
    points.resize(_points.size());
    for (int i = 0; i < points.size(); i++)
      points[i] = _points[i].pos;
  }
}


void ColorCurveControl::showEditDialog()
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[mMode]->getControlPoints(points);

  if (pointsIndexForShowEditDialog < 0 || pointsIndexForShowEditDialog >= points.size())
    return;

  DialogWindow setxy_dialog(nullptr, hdpi::_pxScaled(220), hdpi::_pxScaled(100), "Select point coordinates");

  PropPanel::ContainerPropertyControl *panel = setxy_dialog.getPanel();
  G_ASSERT(panel && "ColorCurveControl::showEditDialog: panel = NULL");

  const Point2 pointPos = points[pointsIndexForShowEditDialog].pos;
  panel->createEditInt(DIALOG_ID_X, "x:", floor(pointPos.x));
  panel->createEditInt(DIALOG_ID_Y, "y:", floor(pointPos.y), true, false);

  if (setxy_dialog.showDialog() == DIALOG_ID_OK)
  {
    const Point2 from_starting_pos = Point2(panel->getInt(DIALOG_ID_X), panel->getInt(DIALOG_ID_Y)) - pointPos;

    if (mEventHandler)
      mEventHandler->onWcChanging(windowBaseForEventHandler);

    if (cbs[mMode]->moveSelectedControlPoints(from_starting_pos))
    {
      cbs[mMode]->endMoveControlPoints(false);
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(windowBaseForEventHandler);
    }
  }
}


void ColorCurveControl::updateImgui()
{
  ImGui::PushID(this);

  // Center horizontally.
  const float padding = (ImGui::GetContentRegionAvail().x - viewSize.x) * 0.5f;
  if (padding > 0.0f)
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding);

  viewOffset = ImGui::GetCursorScreenPos();

  // This will catch the interactions.
  ImGui::InvisibleButton("canvas", viewSize);

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
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Escape, ImGuiInputFlags_None, ImGuiKeyOwner_Any))
  {
    // Prevent Escape reaching the dialog because it would close it.
    ImGui::SetKeyOwner(ImGuiKey_Escape, ImGuiKeyOwner_Any, ImGuiInputFlags_LockUntilRelease);

    cancelMovement = true;
  }

  drawGeom();

  ImGui::PopID();
}

} // namespace PropPanel