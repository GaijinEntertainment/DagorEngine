// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "color_curve.h"
#include "../../c_constants.h"
#include <memory/dag_mem.h>
#include <windows.h>

#include <propPanel2/comWnd/dialog_window.h>
#include <propPanel2/comWnd/color_correction_info.h>

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


COLORREF CURVE_COLORS[COLOR_CURVE_MODE_COUNT] = {RGB(64, 64, 64), RGB(192, 0, 0), RGB(0, 192, 0), RGB(0, 0, 192)};


COLORREF BAR_COLORS[COLOR_CURVE_MODE_COUNT] = {RGB(162, 162, 162), RGB(255, 192, 192), RGB(192, 255, 192), RGB(192, 192, 255)};


WColorCurveControl::WColorCurveControl(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_NOTIFY | BS_OWNERDRAW, "", x, y, w, h),

  viewSize(w, h),
  startingPos(-1, -1),
  cachPolyLinesRGB(tmpmem),
  cachPolyLinesR(tmpmem),
  cachPolyLinesG(tmpmem),
  cachPolyLinesB(tmpmem),
  cancelMovement(false),
  startMovement(false),
  mTooltip(event_handler, this),
  mMode(COLOR_CURVE_MODE_RGB),
  hBitmap(NULL),
  hBitmapDC(NULL),
  showChartBars(true),
  showBaseLine(true)
{
  viewBox.lim[0].x = -9;
  viewBox.lim[0].y = 260;
  viewBox.lim[1].x = 260;
  viewBox.lim[1].y = -9;

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
  mTooltip.setText("");
  build();
}


WColorCurveControl::~WColorCurveControl()
{
  removeDC();

  for (int i = 0; i < COLOR_CURVE_MODE_COUNT; ++i)
    delete cbs[i];
}


void WColorCurveControl::setMode(int _mode)
{
  if (_mode < COLOR_CURVE_MODE_RGB || _mode > COLOR_CURVE_MODE_COUNT - 1)
    mMode = COLOR_CURVE_MODE_RGB;
  else
    mMode = _mode;

  build();
}


void WColorCurveControl::setBarChart(const unsigned char rgb_bar_charts[3][256])
{
  memcpy(bar_chart, rgb_bar_charts, 256 * 3);
  refresh(false);
}


void WColorCurveControl::build()
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

  refresh(false);
}


inline void cut_val(float &val) { val = (val < 0) ? 0 : ((val > 255) ? 255 : val); }


inline bool check_val(float val) { return (val >= 0) && (val < 256); }


void WColorCurveControl::cutCurve(int index)
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


bool WColorCurveControl::isSelection()
{
  Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
  cbs[mMode]->getControlPoints(points);

  for (int i = 0; i < points.size(); i++)
    if (points[i].isSelected())
      return true;

  return false;
}


void WColorCurveControl::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  viewSize.x = w;
  viewSize.y = h;
  build();
}


void WColorCurveControl::removeDC()
{
  if (hBitmap)
    DeleteObject((HBITMAP)hBitmap);

  if (hBitmapDC)
    DeleteDC((HDC)hBitmapDC);

  hBitmap = NULL;
  hBitmapDC = NULL;
}


intptr_t WColorCurveControl::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HDC hDC = draw->hDC;

  removeDC();
  hBitmapDC = CreateCompatibleDC(hDC);
  hBitmap = CreateCompatibleBitmap((HDC)hDC, mWidth, mHeight);
  HBITMAP oldBitmap = (HBITMAP)SelectObject((HDC)hBitmapDC, (HBITMAP)hBitmap);

  drawGeom(hBitmapDC);
  BitBlt(hDC, 0, 0, mWidth, mHeight, (HDC)hBitmapDC, 0, 0, SRCCOPY);

  SelectObject((HDC)hDC, oldBitmap);
  return 0;
}


void WColorCurveControl::drawGeom(void *hdc)
{
  HDC hDC = (HDC)hdc;

  HBRUSH white_brush = CreateSolidBrush(RGB(255, 255, 255));
  SelectObject(hDC, white_brush);
  MoveToEx(hDC, 0, 0, NULL);

  HPEN black_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HPEN white_pen = CreatePen(PS_DASH, 1, RGB(255, 255, 255));
  IPoint2 p0 = locate(Point2(0, 0), viewBox, viewSize);
  IPoint2 p1 = locate(Point2(255, 255), viewBox, viewSize);

  // fill area
  SelectObject(hDC, black_pen);
  Rectangle(hDC, 0, 0, mWidth, mHeight);

  // gradients
  SelectObject(hDC, GetStockObject(DC_PEN));
  for (int i = 0; i < (p1.x - p0.x); ++i)
  {
    float color_val = floor(255.0 * i / (p1.x - p0.x));
    SetDCPenColor(hDC, RGB(color_val, color_val, color_val));
    MoveToEx(hDC, p0.x + i + 1, mHeight - 2, NULL);
    LineTo(hDC, p0.x + i + 1, p0.y);
  }
  for (int i = 0; i < (p0.y - p1.y); ++i)
  {
    float color_val = 255 - floor(255.0 * i / (p0.y - p1.y));
    SetDCPenColor(hDC, RGB(color_val, color_val, color_val));
    MoveToEx(hDC, 1, i + p1.y, NULL);
    LineTo(hDC, p0.x, i + p1.y);
  }

  // lines
  SelectObject(hDC, GetStockObject(DC_PEN));
  SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
  SetDCPenColor(hDC, RGB(212, 212, 212));

  // grid
  IPoint2 p_step = (p1 - p0 + IPoint2(1, -1)) / 4;
  for (int i = 0; i < 4; ++i)
  {
    Rectangle(hDC, p0.x + i * p_step.x, p0.y + 1, p0.x + (i + 1) * p_step.x + 1, p1.y - 1);
    Rectangle(hDC, p0.x, p0.y + i * p_step.y + 1, p1.x + 2, p0.y + (i + 1) * p_step.y);
  }

  // base line
  if (showBaseLine)
  {
    MoveToEx(hDC, p0.x, p0.y, NULL);
    LineTo(hDC, p1.x, p1.y);
  }

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
    drawBarChart(hDC, mMode);

  // curves
  switch (mMode)
  {
    case COLOR_CURVE_MODE_RGB:
      for (int i = COLOR_CURVE_MODE_COUNT - 1; i >= 0; --i)
        drawCurve(hDC, i);
      break;

    default: drawCurve(hDC, mMode); break;
  }

  SelectObject(hDC, GetStockObject(DC_PEN));
  SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
  DeleteObject(white_brush);
  DeleteObject(black_pen);
  DeleteObject(white_pen);
}


void WColorCurveControl::drawCurve(void *hdc, int mode_index)
{
  HDC hDC = (HDC)hdc;
  HPEN pen = CreatePen(PS_SOLID, 1, CURVE_COLORS[mode_index]);
  HBRUSH brush = CreateSolidBrush(CURVE_COLORS[mode_index]);

  // curve
  Tab<IPoint2> linePts(tmpmem);

  SelectObject(hDC, pen);

  PolyLineTab &cach_poly_lines = *cachPolyLines[mode_index];

  for (int i = 0; i < cach_poly_lines.size(); i++)
  {
    linePts.resize(cach_poly_lines[i].points.size());
    for (int j = 0; j < linePts.size(); ++j)
      linePts[j] = locate(cach_poly_lines[i].points[j], viewBox, viewSize);

    Tab<POINT> t(tmpmem);
    t.resize(linePts.size());
    for (int i = 0; i < linePts.size(); i++)
    {
      t[i].x = linePts[i].x;
      t[i].y = linePts[i].y;
    }

    Polyline(hDC, &t[0], t.size());
  }

  // control points
  if (cbs[mode_index] && mode_index == mMode)
  {
    Tab<ICurveControlCallback::ControlPoint> points(tmpmem);
    cbs[mode_index]->getControlPoints(points);

    for (int i = 0; i < points.size(); i++)
    {
      IPoint2 p = locate(points[i].pos, viewBox, viewSize);
      RECT r;
      r.top = p.y - CURVE_POINT_RAD;
      r.bottom = p.y + CURVE_POINT_RAD;
      r.left = p.x - CURVE_POINT_RAD;
      r.right = p.x + CURVE_POINT_RAD;
      if (points[i].isSelected())
        FillRect(hDC, &r, brush);
      else
        Rectangle(hDC, r.left, r.top, r.right, r.bottom);
    }
    clear_and_shrink(points);
  }

  SelectObject(hDC, GetStockObject(DC_PEN));
  SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
  DeleteObject(pen);
  DeleteObject(brush);
}


void WColorCurveControl::drawBarChart(void *hdc, int mode_index)
{
  HDC hDC = (HDC)hdc;
  HPEN pen = CreatePen(PS_SOLID, 1, BAR_COLORS[mode_index]);
  SelectObject(hDC, pen);

  IPoint2 p0 = locate(Point2(0, 0), viewBox, viewSize);
  IPoint2 p1 = locate(Point2(255, 255), viewBox, viewSize);
  int max_ht = p0.y - p1.y;

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

    MoveToEx(hDC, x, p0.y, NULL);
    LineTo(hDC, x, y);
  }

  SelectObject(hDC, GetStockObject(DC_PEN));
  DeleteObject(pen);
}


intptr_t WColorCurveControl::onLButtonDown(long x, long y)
{
  if (!cbs[mMode])
    return WindowControlBase::onLButtonDown(x, y);

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

  if (cbs[mMode]->selectControlPoints(pointIdx))
    refresh(false);
  startingPos = Point2(x, y);
  cancelMovement = false;

  return 0;
}


intptr_t WColorCurveControl::onLButtonUp(long x, long y)
{
  if (!cbs[mMode])
    return WindowControlBase::onLButtonUp(x, y);

  startMovement = false;
  startingPos = Point2(-1, -1);
  if (cbs[mMode]->endMoveControlPoints(cancelMovement))
  {
    build();
  }
  refresh(false);
  return 0;
}


static HCURSOR hc_move_cursor = LoadCursor(NULL, IDC_CROSS /*IDC_SIZEALL*/);

intptr_t WColorCurveControl::onDrag(int x, int y)
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

    if (cbs[mMode]->moveSelectedControlPoints(fromStartingPos))
    {
      build();
      SetCursor(::hc_move_cursor);
      if (mEventHandler)
        mEventHandler->onWcChange(this);
    }
  }

  refresh(false);
  return 0;
}


intptr_t WColorCurveControl::onMouseMove(int x, int y)
{
  String text;

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

  if (idx > -1)
  {
    text = String(20, "Point %d (%.0f, %.0f)", idx, floor(points[idx].pos.x), floor(points[idx].pos.y));
  }

  mTooltip.setText(text);
  return 1;
}


intptr_t WColorCurveControl::onLButtonDClick(long x, long y)
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

  if (idx > -1) // edit dialog
  {
    CDialogWindow setxy_dialog(getHandle(), _pxScaled(220), _pxScaled(100), "Select point coordinates");

    PropertyContainerControlBase *panel = setxy_dialog.getPanel();
    G_ASSERT(panel && "WCurveControl::onLButtonDClick: panel = NULL");

    panel->createEditInt(DIALOG_ID_X, "x:", floor(points[idx].pos.x));
    panel->createEditInt(DIALOG_ID_Y, "y:", floor(points[idx].pos.y), true, false);

    if (setxy_dialog.showDialog() == DIALOG_ID_OK)
    {
      Point2 from_starting_pos = Point2(panel->getInt(DIALOG_ID_X), panel->getInt(DIALOG_ID_Y)) - points[idx].pos;

      if (mEventHandler)
        mEventHandler->onWcChanging(this);

      if (cbs[mMode]->moveSelectedControlPoints(from_starting_pos))
      {
        cbs[mMode]->endMoveControlPoints(false);
        build();
        refresh(false);
        if (mEventHandler)
          mEventHandler->onWcChange(this);
      }
    }
  }
  else // new point
  {
    if (mEventHandler)
      mEventHandler->onWcChanging(this);

    if (cbs[mMode]->addNewControlPoint(getCoords(IPoint2(x, y), viewBox, viewSize)))
    {
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(this);
    }

    // select new point
    this->onLButtonDown(x, y);
  }

  return 0;
}


intptr_t WColorCurveControl::onRButtonDown(long x, long y)
{
  if (!cbs[mMode])
    return 0;

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
      return 0;

    if (mEventHandler)
      mEventHandler->onWcChanging(this);

    if (cbs[mMode]->deleteSelectedControlPoints())
    {
      build();
      if (mEventHandler)
        mEventHandler->onWcChange(this);
      refresh(false);
    }
  }
  return 0;
}


intptr_t WColorCurveControl::onKeyDown(unsigned v_key, int flags)
{
  switch (v_key)
  {
    case VK_ESCAPE: cancelMovement = true; break;
  }

  refresh(false);
  return 0;
}


void WColorCurveControl::getColorTables(unsigned char rgb_r_g_b[4][256])
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


void WColorCurveControl::setCorrectionInfo(const ColorCorrectionInfo &settings)
{
  setPoints(COLOR_CURVE_MODE_RGB, settings.rgbPoints);
  setPoints(COLOR_CURVE_MODE_R, settings.rPoints);
  setPoints(COLOR_CURVE_MODE_G, settings.gPoints);
  setPoints(COLOR_CURVE_MODE_B, settings.bPoints);
};


void WColorCurveControl::getCorrectionInfo(ColorCorrectionInfo &settings)
{
  getPoints(COLOR_CURVE_MODE_RGB, settings.rgbPoints);
  getPoints(COLOR_CURVE_MODE_R, settings.rPoints);
  getPoints(COLOR_CURVE_MODE_G, settings.gPoints);
  getPoints(COLOR_CURVE_MODE_B, settings.bPoints);
};


void WColorCurveControl::setPoints(int index, const Tab<Point2> &points)
{
  if (index < COLOR_CURVE_MODE_COUNT && cbs[index])
  {
    cbs[index]->clear();
    for (int i = 0; i < points.size(); i++)
      cbs[index]->addNewControlPoint(points[i]);
    build();
  }
}


void WColorCurveControl::getPoints(int index, Tab<Point2> &points)
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
