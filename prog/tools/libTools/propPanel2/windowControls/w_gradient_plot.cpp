// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_gradient_plot.h"

#include <propPanel2/c_util.h>
#include <windows.h>
#include <generic/dag_tab.h>
#include <math/dag_mathBase.h>


//------------------------ GCS controller  -----------------------------


class IGradientPlotController
{
public:
  IGradientPlotController() : points(midmem)
  {
    clear_and_shrink(points);
    value = Point3(0, 0, 0);
    w = h = 0;
  }


  virtual ~IGradientPlotController() { clear_and_shrink(points); }

  virtual void resize(unsigned _w, unsigned _h)
  {
    w = _w;
    h = _h;
    points.resize(w * h);
    recalculate();
  }

  virtual E3DCOLOR getPointColor(int x, int y)
  {
    G_ASSERT(points.size() == (w * h));

    if (x >= 0 && x < w && y >= 0 && y < h)
      return points[x + y * w];

    return E3DCOLOR(0, 0, 0, 0);
  }

  virtual bool needUpdate() { return true; }

  virtual void setValue(Point3 _value, bool recalc = true)
  {
    value = _value;

    if (needUpdate() && recalc)
      recalculate();
  }

  virtual Point3 pickPlotValue(int x, int y) = 0;
  virtual Point2 getPlotPos() = 0;

protected:
  virtual void recalculate() = 0;

  Point3 value;
  unsigned w, h;
  Tab<E3DCOLOR> points;
};

// ---------------------- H linear plot -------------------------------

class GPCModeH : public IGradientPlotController
{
public:
  // virtual bool needUpdate() { return false; }

  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
      value.x = lerp(0.0, 360.0, x / (w - 1.0));
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.x / 360.0), 0);
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1)
      return;

    for (int i = 0; i < w; ++i)
    {
      Point3 color(360.0 * i / (w - 1.0), 1.0, 1.0);

      for (int j = 0; j < h; ++j)
        points[i + j * w] = p2util::rgb_to_e3(p2util::hsv2rgb(color));
    }
  }
};


// ---------------------- S linear plot -------------------------------

class GPCModeS : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
      value.y = 1.0 - lerp(0.0, 1.0, x / (w - 1.0));
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, 1.0 - value.y), 0);
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1)
      return;

    for (int i = 0; i < w; ++i)
    {
      Point3 color(value.x, 1.0 - i / (w - 1.0), value.z);

      for (int j = 0; j < h; ++j)
        points[i + j * w] = p2util::rgb_to_e3(p2util::hsv2rgb(color));
    }
  }
};

// ---------------------- V linear plot -------------------------------

class GPCModeV : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
      value.z = 1.0 - lerp(0.0, 1.0, x / (w - 1.0));
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, 1.0 - value.z), 0);
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1)
      return;

    for (int i = 0; i < w; ++i)
    {
      Point3 color(value.x, value.y, 1.0 - i / (w - 1.0));

      for (int j = 0; j < h; ++j)
        points[i + j * w] = p2util::rgb_to_e3(p2util::hsv2rgb(color));
    }
  }
};

// ---------------------- R linear plot -------------------------------

class GPCModeR : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
      value.x = 1.0 - lerp(0.0, 1.0, x / (w - 1.0));
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, 1.0 - value.x), 0);
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1)
      return;

    for (int i = 0; i < w; ++i)
    {
      Point3 color(1.0 - i / (w - 1.0), value.y, value.z);

      for (int j = 0; j < h; ++j)
        points[i + j * w] = p2util::rgb_to_e3(color);
    }
  }
};

// ---------------------- G linear plot -------------------------------

class GPCModeG : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
      value.y = 1.0 - lerp(0.0, 1.0, x / (w - 1.0));
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, 1.0 - value.y), 0);
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1)
      return;

    for (int i = 0; i < w; ++i)
    {
      Point3 color(value.x, 1.0 - i / (w - 1.0), value.z);

      for (int j = 0; j < h; ++j)
        points[i + j * w] = p2util::rgb_to_e3(color);
    }
  }
};


// ---------------------- B linear plot -------------------------------

class GPCModeB : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
      value.z = 1.0 - lerp(0.0, 1.0, x / (w - 1.0));
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, 1.0 - value.z), 0);
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1)
      return;

    for (int i = 0; i < w; ++i)
    {
      Point3 color(value.x, value.y, 1.0 - i / (w - 1.0));

      for (int j = 0; j < h; ++j)
        points[i + j * w] = p2util::rgb_to_e3(color);
    }
  }
};

// ---------------------- HS 2d plot -------------------------------

class GPCModeHS : public IGradientPlotController
{
public:
  // virtual bool needUpdate() { return false; }

  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
    {
      value.x = lerp(0.0, 360.0, x / (w - 1.0));
      value.y = 1.0 - lerp(0.0, 1.0, y / (h - 1.0));
    }
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1 || h <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.x / 360.0), lerp(0.0, h - 1.0, 1.0 - value.y));
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1 || h <= 1)
      return;

    for (int i = 0; i < w; ++i)
      for (int j = 0; j < h; ++j)
      {
        Point3 color(360.0 * i / (w - 1.0), 1.0 - j / (h - 1.0), 1.0);
        points[i + j * w] = p2util::rgb_to_e3(p2util::hsv2rgb(color));
      }
  }
};

// ---------------------- HV 2d plot -------------------------------

class GPCModeHV : public IGradientPlotController
{
public:
  // virtual bool needUpdate() { return false; }

  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
    {
      value.x = lerp(0.0, 360.0, x / (w - 1.0));
      value.z = 1.0 - lerp(0.0, 1.0, y / (h - 1.0));
    }
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1 || h <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.x / 360.0), lerp(0.0, h - 1.0, 1.0 - value.z));
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1 || h <= 1)
      return;

    for (int i = 0; i < w; ++i)
      for (int j = 0; j < h; ++j)
      {
        Point3 color(360.0 * i / (w - 1.0), 1.0, 1.0 - j / (h - 1.0));
        points[i + j * w] = p2util::rgb_to_e3(p2util::hsv2rgb(color));
      }
  }
};

// ---------------------- SV 2d plot -------------------------------

class GPCModeSV : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
    {
      value.y = lerp(0.0, 1.0, x / (w - 1.0));
      value.z = 1.0 - lerp(0.0, 1.0, y / (h - 1.0));
    }
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1 || h <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.y), lerp(0.0, h - 1.0, 1.0 - value.z));
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1 || h <= 1)
      return;

    for (int i = 0; i < w; ++i)
      for (int j = 0; j < h; ++j)
      {
        Point3 color(value.x, i / (w - 1.0), 1.0 - j / (h - 1.0));
        points[i + j * w] = p2util::rgb_to_e3(p2util::hsv2rgb(color));
      }
  }
};

// ---------------------- RG 2d plot -------------------------------

class GPCModeRG : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
    {
      value.x = lerp(0.0, 1.0, x / (w - 1.0));
      value.y = 1.0 - lerp(0.0, 1.0, y / (h - 1.0));
    }
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1 || h <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.x), lerp(0.0, h - 1.0, 1.0 - value.y));
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1 || h <= 1)
      return;

    for (int i = 0; i < w; ++i)
      for (int j = 0; j < h; ++j)
      {
        Point3 color(i / (w - 1.0), 1.0 - j / (h - 1.0), value.z);
        points[i + j * w] = p2util::rgb_to_e3(color);
      }
  }
};


// ---------------------- RB 2d plot -------------------------------

class GPCModeRB : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
    {
      value.z = lerp(0.0, 1.0, x / (w - 1.0));
      value.x = 1.0 - lerp(0.0, 1.0, y / (h - 1.0));
    }
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1 || h <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.z), lerp(0.0, h - 1.0, 1.0 - value.x));
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1 || h <= 1)
      return;

    for (int i = 0; i < w; ++i)
      for (int j = 0; j < h; ++j)
      {
        Point3 color(1.0 - j / (h - 1.0), value.y, i / (w - 1.0));
        points[i + j * w] = p2util::rgb_to_e3(color);
      }
  }
};


// ---------------------- GB 2d plot -------------------------------

class GPCModeGB : public IGradientPlotController
{
public:
  virtual Point3 pickPlotValue(int x, int y)
  {
    x = clamp(x, 0, (int)w - 1);
    y = clamp(y, 0, (int)h - 1);

    if (w >= 1 && y < h)
    {
      value.z = lerp(0.0, 1.0, x / (w - 1.0));
      value.y = 1.0 - lerp(0.0, 1.0, y / (h - 1.0));
    }
    return value;
  }

  virtual Point2 getPlotPos()
  {
    if (w <= 1 || h <= 1)
      return Point2(0, 0);

    return Point2(lerp(0.0, w - 1.0, value.z), lerp(0.0, h - 1.0, 1.0 - value.y));
  }

protected:
  virtual void recalculate()
  {
    if (w <= 1 || h <= 1)
      return;

    for (int i = 0; i < w; ++i)
      for (int j = 0; j < h; ++j)
      {
        Point3 color(value.x, 1.0 - j / (h - 1.0), i / (w - 1.0));
        points[i + j * w] = p2util::rgb_to_e3(color);
      }
  }
};


// ====================================================================

// ----------------- Gradient Color Selector --------------------------

WGradientPlot::WGradientPlot(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY, "", x, y, w, h),
  controller(NULL),
  dragMode(false),
  hBitmap(NULL),
  hBitmapDC(NULL)
{

  reset();
}


WGradientPlot::~WGradientPlot() { resetDC(); }


void WGradientPlot::reset()
{
  mMode = GRADIENT_MODE_NONE;
  setColorFValue(Point3(180, 0.5, 0.5));
}

//-------------------------------------------------------------
// value  routines

void WGradientPlot::setMode(int mode)
{
  int old_color = mMode & COLOR_MASK;
  int new_color = mode & COLOR_MASK;

  if (old_color < GRADIENT_COLOR_MODE_HSV && new_color > GRADIENT_COLOR_MODE_HSV)
    mValue = p2util::hsv2rgb(mValue);
  else if (old_color > GRADIENT_COLOR_MODE_HSV && new_color < GRADIENT_COLOR_MODE_HSV)
    mValue = p2util::rgb2hsv(mValue);

  mMode = mode;
  updateDrawParams();
}


void WGradientPlot::setColorFValue(Point3 value)
{
  mValue = value;
  if (!controller || controller->needUpdate())
    updateDrawParams();
  else
    controller->setValue(mValue);
}

Point3 WGradientPlot::getColorFValue() const { return mValue; }


void WGradientPlot::setColorValue(E3DCOLOR value)
{
  if ((mMode & COLOR_MASK) < GRADIENT_COLOR_MODE_HSV)
    setColorFValue(p2util::rgb2hsv(p2util::e3_to_rgb(value)));
  else
    setColorFValue(p2util::e3_to_rgb(value));
}


E3DCOLOR WGradientPlot::getColorValue() const
{
  if ((mMode & COLOR_MASK) < GRADIENT_COLOR_MODE_HSV)
    return p2util::rgb_to_e3(p2util::hsv2rgb(mValue));
  else
    return p2util::rgb_to_e3(mValue);
}

//-------------------------------------------------------------
// draw routines


void WGradientPlot::resetDC()
{
  if (hBitmap)
    DeleteObject((HBITMAP)hBitmap);

  if (hBitmapDC)
    DeleteDC((HDC)hBitmapDC);

  hBitmap = NULL;
  hBitmapDC = NULL;
}


void WGradientPlot::drawGradientToDC()
{
  resetDC();

  HDC hDC = GetDC((HWND)getHandle());
  hBitmapDC = CreateCompatibleDC(hDC);
  hBitmap = CreateCompatibleBitmap((HDC)hDC, x1 - x0, y1 - y0);
  HBITMAP oldBitmap = (HBITMAP)SelectObject((HDC)hBitmapDC, (HBITMAP)hBitmap);

  drawGradient(hBitmapDC, 0, 0);

  SelectObject((HDC)hDC, oldBitmap);
  ReleaseDC((HWND)getHandle(), hDC);
}


void WGradientPlot::updateDrawParams()
{
  if (mMode == GRADIENT_MODE_NONE)
  {
    x0 = 1.0;
    y0 = 1.0;
    x1 = mWidth - 1.0;
    y1 = mHeight - 1.0;
    resetDC();
    refresh(false);
    return;
  }

  switch (mMode & AXIS_MASK)
  {
    case GRADIENT_AXIS_MODE_X:
      x0 = 1.0 + _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
      y0 = 1.0 + _pxS(TRACK_GRADIENT_BUTTON_HEIGHT);
      x1 = mWidth - x0 - 1.0;
      y1 = mHeight - 1.0;
      break;

    case GRADIENT_AXIS_MODE_Y:
      x0 = 1.0 + _pxS(TRACK_GRADIENT_BUTTON_HEIGHT);
      y0 = 1.0 + _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
      x1 = mWidth - 1.0;
      y1 = mHeight - y0 - 1.0;
      break;

    default:
      x0 = 1.0;
      y0 = 1.0;
      x1 = mWidth - 1.0;
      y1 = mHeight - 1.0;
  }

  del_it(controller);

  switch (mMode)
  {
    case (GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_X):
    case (GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_Y): controller = new GPCModeH(); break;

    case (GRADIENT_COLOR_MODE_H + GRADIENT_AXIS_MODE_XY): controller = new GPCModeSV(); break;

    case (GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_X):
    case (GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_Y): controller = new GPCModeS(); break;

    case (GRADIENT_COLOR_MODE_S + GRADIENT_AXIS_MODE_XY): controller = new GPCModeHV(); break;

    case (GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_X):
    case (GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_Y): controller = new GPCModeV(); break;

    case (GRADIENT_COLOR_MODE_V + GRADIENT_AXIS_MODE_XY):
      controller = new GPCModeHS();
      break;

      //---

    case (GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_X):
    case (GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_Y): controller = new GPCModeR(); break;

    case (GRADIENT_COLOR_MODE_R + GRADIENT_AXIS_MODE_XY): controller = new GPCModeGB(); break;

    case (GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_X):
    case (GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_Y): controller = new GPCModeG(); break;

    case (GRADIENT_COLOR_MODE_G + GRADIENT_AXIS_MODE_XY): controller = new GPCModeRB(); break;

    case (GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_X):
    case (GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_Y): controller = new GPCModeB(); break;

    case (GRADIENT_COLOR_MODE_B + GRADIENT_AXIS_MODE_XY): controller = new GPCModeRG(); break;
  }

  if (controller)
  {
    controller->setValue(mValue, false);

    if ((mMode & AXIS_MASK) == GRADIENT_AXIS_MODE_X)
      controller->resize(x1 - x0, 1);
    else if ((mMode & AXIS_MASK) == GRADIENT_AXIS_MODE_Y)
      controller->resize(y1 - y0, 1);
    else
      controller->resize(x1 - x0, y1 - y0);

    drawGradientToDC();
    refresh(false);
  }
}


intptr_t WGradientPlot::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;
  HDC hDC = (HDC)draw->hDC;

  if (hBitmapDC)
    BitBlt(hDC, x0, y0, x1 - x0, y1 - y0, (HDC)hBitmapDC, 0, 0, SRCCOPY);
  else
    drawGradient(hDC, x0, y0);

  // draw rect

  HPEN black_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HPEN old_pen = (HPEN)SelectObject(hDC, black_pen);
  MoveToEx(hDC, x0 - 1, y0 - 1, NULL);

  LineTo(hDC, x0 - 1, y1);
  LineTo(hDC, x1, y1);
  LineTo(hDC, x1, y0 - 1);
  LineTo(hDC, x0 - 1, y0 - 1);

  SelectObject(hDC, old_pen);
  DeleteObject(black_pen);

  // pointers

  if (controller)
  {
    Point2 cur_pos = controller->getPlotPos();

    switch (mMode & AXIS_MASK)
    {
      case GRADIENT_AXIS_MODE_X: drawArrow(hDC, cur_pos, false); break;

      case GRADIENT_AXIS_MODE_Y: drawArrow(hDC, cur_pos, true); break;

      case GRADIENT_AXIS_MODE_XY: drawCross(hDC, cur_pos);
    }
  }

  return 0;
}


void WGradientPlot::drawGradient(void *hdc, int x, int y)
{
  HDC hDC = (HDC)hdc;

  int _x0 = x;
  int _x1 = _x0 + (x1 - x0);
  int _y0 = y;
  int _y1 = _y0 + (y1 - y0);

  // draw gradient

  if (mMode == GRADIENT_MODE_NONE)
  {
    RECT curRect;
    curRect.left = _x0;
    curRect.top = _y0;
    curRect.right = _x1;
    curRect.bottom = _y1;

    HBRUSH hBrush;
    E3DCOLOR rgb_color = getColorValue();
    hBrush = CreateSolidBrush(RGB(rgb_color.r, rgb_color.g, rgb_color.b));
    FillRect((HDC)hDC, &curRect, hBrush);
    DeleteObject(hBrush);
    return;
  }

  if (controller)
    switch (mMode & AXIS_MASK)
    {
      case GRADIENT_AXIS_MODE_X:

        SelectObject(hDC, GetStockObject(DC_PEN));
        for (int i = _x0; i < _x1; i++)
        {
          E3DCOLOR color = controller->getPointColor(i - _x0, 0);
          SetDCPenColor(hDC, RGB(color.r, color.g, color.b));
          MoveToEx(hDC, i, _y0, NULL);
          LineTo(hDC, i, _y1);
        }
        break;

      case GRADIENT_AXIS_MODE_Y:

        SelectObject(hDC, GetStockObject(DC_PEN));
        for (int j = _y0; j < _y1; j++)
        {
          E3DCOLOR color = controller->getPointColor(j - _y0, 0);
          SetDCPenColor(hDC, RGB(color.r, color.g, color.b));
          MoveToEx(hDC, _x0, j, NULL);
          LineTo(hDC, _x1, j);
        }
        break;

      case GRADIENT_AXIS_MODE_XY:

      {
        struct
        {
          BITMAPINFOHEADER bmiHeader;
          RGBQUAD bmiColors[16];
        } bmi;

        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        int result = GetDIBits((HDC)hBitmapDC, (HBITMAP)hBitmap, 0, 0, NULL, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);

        if (result && bmi.bmiHeader.biBitCount == 32)
        {
          int buf_size = (_x1 - _x0) * (_y1 - _y0) * 4;
          char *buffer = new char[buf_size];
          memset(buffer, 0, buf_size);

          result = GetDIBits((HDC)hBitmapDC, (HBITMAP)hBitmap, 0, 0, (void *)buffer, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);

          // direct paint to bitmap memory
          char *buf = buffer;
          for (int j = _y0; j < _y1; ++j)
            for (int i = _x0; i < _x1; ++i)
            {
              E3DCOLOR color = controller->getPointColor(i - _x0, _y1 - j - 1);

              *buf = color.b;
              ++buf;
              *buf = color.g;
              ++buf;
              *buf = color.r;
              ++buf;
              *buf = 0x00;
              ++buf;
            }

          result = SetDIBits((HDC)hBitmapDC, (HBITMAP)hBitmap, 0, _y1 - _y0, (void *)buffer, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);
          delete[] buffer;
        }
        else
          for (int i = _x0; i < x1; i++)
            for (int j = _y0; j < y1; j++)
            {
              E3DCOLOR color = controller->getPointColor(i - _x0, j - _y0);
              SetPixelV(hDC, i, j, RGB(color.r, color.g, color.b)); // we need to fix this!
            }
      }
    }
}


void WGradientPlot::drawArrow(void *hdc, Point2 pos, bool rotate)
{
  G_ASSERT(controller);
  HDC hDC = (HDC)hdc;

  // dot line

  HPEN white_pen = CreatePen(PS_DOT, 1, RGB(255, 255, 255));
  HPEN old_pen = (HPEN)SelectObject(hDC, white_pen);

  if (!rotate)
  {
    MoveToEx(hDC, x0 + pos.x, y0, NULL);
    LineTo(hDC, x0 + pos.x, y1);
  }
  else
  {
    MoveToEx(hDC, x0, y0 + pos.x, NULL);
    LineTo(hDC, x1, y0 + pos.x);
  }

  // back
  RECT curRect;
  curRect.left = 0;
  curRect.top = 0;
  curRect.right = (rotate) ? x0 - 1 : mWidth;
  curRect.bottom = (rotate) ? mHeight : y0 - 1;

  HBRUSH hBrush;
  hBrush = CreateSolidBrush(USER_GUI_COLOR);
  FillRect(hDC, &curRect, hBrush);
  DeleteObject(hBrush);

  // cursor

  E3DCOLOR e3color = controller->getPointColor(pos.x, pos.y);
  HPEN border_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  HPEN color_pen = CreatePen(PS_SOLID, 1, RGB(e3color.r, e3color.g, e3color.b));
  SelectObject(hDC, color_pen);

  Point2 cur_pos((rotate) ? x0 - 2 : pos.x + x0, (rotate) ? pos.x + y0 : y0 - 2);

  int mx = _pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
  int my = _pxS(TRACK_GRADIENT_BUTTON_HEIGHT) - 1;

  for (int i = 0; i < my; ++i)
  {
    int j = lerp(0, mx, i / (my - 1.0));

    if (!rotate)
    {
      MoveToEx(hDC, cur_pos.x - j, cur_pos.y - i, NULL);
      LineTo(hDC, cur_pos.x + j, cur_pos.y - i);
    }
    else
    {
      MoveToEx(hDC, cur_pos.x - i, cur_pos.y - j, NULL);
      LineTo(hDC, cur_pos.x - i, cur_pos.y + j);
    }
  }


  SelectObject(hDC, border_pen);
  MoveToEx(hDC, cur_pos.x, cur_pos.y, NULL);
  if (!rotate)
  {
    LineTo(hDC, cur_pos.x - mx, cur_pos.y - my);
    LineTo(hDC, cur_pos.x + mx - 1, cur_pos.y - my);
  }
  else
  {
    LineTo(hDC, cur_pos.x - my, cur_pos.y - mx);
    LineTo(hDC, cur_pos.x - my, cur_pos.y + mx - 1);
  }
  LineTo(hDC, cur_pos.x, cur_pos.y);

  SelectObject(hDC, old_pen);
  DeleteObject(border_pen);
  DeleteObject(white_pen);
  DeleteObject(color_pen);
}


void WGradientPlot::drawCross(void *hdc, Point2 pos)
{
  G_ASSERT(controller);
  HDC hDC = (HDC)hdc;

  E3DCOLOR e3color = controller->getPointColor(pos.x, pos.y);
  HPEN color_pen = CreatePen(PS_SOLID, 3, RGB(255 - e3color.r, 255 - e3color.g, 255 - e3color.b));
  HPEN old_pen = (HPEN)SelectObject(hDC, color_pen);
  Point2 cur_pos(pos.x + x0, pos.y + y0);

  MoveToEx(hDC, cur_pos.x - 7, cur_pos.y, NULL);
  LineTo(hDC, cur_pos.x - 3, cur_pos.y);

  MoveToEx(hDC, cur_pos.x + 7, cur_pos.y, NULL);
  LineTo(hDC, cur_pos.x + 3, cur_pos.y);

  MoveToEx(hDC, cur_pos.x, cur_pos.y - 7, NULL);
  LineTo(hDC, cur_pos.x, cur_pos.y - 3);

  MoveToEx(hDC, cur_pos.x, cur_pos.y + 7, NULL);
  LineTo(hDC, cur_pos.x, cur_pos.y + 3);

  SelectObject(hDC, old_pen);
  DeleteObject(color_pen);
}

//-------------------------------------------------------------
// event routines

intptr_t WGradientPlot::onDrag(int new_x, int new_y) { return onLButtonDown(new_x, new_y); }


intptr_t WGradientPlot::onLButtonDown(long x, long y)
{
  if (mMode == GRADIENT_MODE_NONE)
    return 0;

  G_ASSERT(controller);

  if ((mMode & AXIS_MASK) == GRADIENT_AXIS_MODE_XY)
    SetCursor(NULL);

  switch (mMode & AXIS_MASK)
  {
    case GRADIENT_AXIS_MODE_X: mValue = controller->pickPlotValue(x - x0, 0); break;

    case GRADIENT_AXIS_MODE_Y: mValue = controller->pickPlotValue(y - y0, 0); break;

    default: mValue = controller->pickPlotValue(x - x0, y - y0);
  }

  dragMode = true;
  refresh(false);
  if (mEventHandler)
    mEventHandler->onWcChange(this);
  return 0;
}

intptr_t WGradientPlot::onLButtonUp(long x, long y)
{
  dragMode = false;
  return 0;
}

//-------------------------------------------------------------
// gui routines

void WGradientPlot::moveWindow(int x, int y)
{
  WindowControlBase::moveWindow(x, y);
  refresh(true);
}


void WGradientPlot::resizeWindow(int w, int h)
{
  WindowBase::resizeWindow(w, h);
  updateDrawParams();
}
