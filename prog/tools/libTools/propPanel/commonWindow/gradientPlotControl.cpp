// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "gradientPlotControl.h"
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/c_util.h>
#include "../c_constants.h"

#include <windows.h>
#include <generic/dag_tab.h>
#include <libTools/util/hdpiUtil.h>
#include <math/dag_mathBase.h>
#include <imgui/imgui.h>

#include <3d/dag_lockTexture.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_texture.h>
#include <util/dag_texMetaData.h>

namespace PropPanel
{

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

GradientPlotControl::GradientPlotControl(PropertyControlBase &control_holder, int x, int y, int w, int h) :
  controlHolder(control_holder),
  requestedWidth(w),
  mWidth(w),
  mHeight(h),
  mMode(GRADIENT_MODE_NONE),
  mValue(Point3::ZERO),
  x0(0.0f),
  y0(0.0f),
  x1(0.0f),
  y1(0.0f),
  controller(NULL),
  dragMode(false),
  lastMousePosInCanvas(-1.0f, -1.0f),
  viewOffset(0.0f, 0.0f),
  gradientTextureId(BAD_TEXTUREID)
{
  setColorFValue(Point3(180, 0.5, 0.5));
}

GradientPlotControl::~GradientPlotControl() { releaseGradientTexture(); }

//-------------------------------------------------------------
// value  routines

void GradientPlotControl::setMode(int mode)
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

void GradientPlotControl::setColorFValue(Point3 value)
{
  mValue = value;
  if (!controller || controller->needUpdate())
    updateDrawParams();
  else
    controller->setValue(mValue);
}

Point3 GradientPlotControl::getColorFValue() const { return mValue; }

void GradientPlotControl::setColorValue(E3DCOLOR value)
{
  if ((mMode & COLOR_MASK) < GRADIENT_COLOR_MODE_HSV)
    setColorFValue(p2util::rgb2hsv(p2util::e3_to_rgb(value)));
  else
    setColorFValue(p2util::e3_to_rgb(value));
}

E3DCOLOR GradientPlotControl::getColorValue() const
{
  if ((mMode & COLOR_MASK) < GRADIENT_COLOR_MODE_HSV)
    return p2util::rgb_to_e3(p2util::hsv2rgb(mValue));
  else
    return p2util::rgb_to_e3(mValue);
}

//-------------------------------------------------------------
// draw routines

void GradientPlotControl::releaseGradientTexture()
{
  if (gradientTextureId != BAD_TEXTUREID)
  {
    release_managed_tex(gradientTextureId);
    gradientTextureId = BAD_TEXTUREID;
  }
}

void GradientPlotControl::drawGradientToTexture()
{
  releaseGradientTexture();

  const int textureWidth = x1 - x0;
  const int textureHeight = y1 - y0;
  const String textureName(64, "GradientPlotControl_%p", this);

  BaseTexture *texture = d3d::create_tex(nullptr, textureWidth, textureHeight, TEXFMT_A8R8G8B8, 1, textureName);
  if (!texture)
    return;

  TextureMetaData textureMetaData;
  apply_gen_tex_props(texture, textureMetaData);

  LockedImage2DView<uint32_t> lockedTexture = lock_texture<uint32_t>(texture, 0, TEXLOCK_WRITE);
  if (!lockedTexture)
  {
    del_d3dres(texture);
    return;
  }

  drawGradient(lockedTexture.get(), textureWidth, textureHeight, lockedTexture.getByteStride());

  lockedTexture.close();

  gradientTextureId = register_managed_tex(textureName, texture);
  G_ASSERT(gradientTextureId != BAD_TEXTUREID);
}

void GradientPlotControl::updateDrawParams()
{
  if (mMode == GRADIENT_MODE_NONE)
  {
    x0 = 1.0;
    y0 = 1.0;
    x1 = mWidth - 1.0;
    y1 = mHeight - 1.0;
    releaseGradientTexture();
    return;
  }

  switch (mMode & AXIS_MASK)
  {
    case GRADIENT_AXIS_MODE_X:
      x0 = 1.0 + hdpi::_pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
      y0 = 1.0 + hdpi::_pxS(TRACK_GRADIENT_BUTTON_HEIGHT);
      x1 = mWidth - x0 - 1.0;
      y1 = mHeight - 1.0;
      break;

    case GRADIENT_AXIS_MODE_Y:
      x0 = 1.0 + hdpi::_pxS(TRACK_GRADIENT_BUTTON_HEIGHT);
      y0 = 1.0 + hdpi::_pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
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

    redrawGradientTexture = true;
  }
}

void GradientPlotControl::drawGradient(uint8_t *texture_buffer, int texture_width, int texture_height, uint32_t texture_stride)
{
  G_ASSERT(texture_buffer);
  G_ASSERT(texture_width == (x1 - x0));
  G_ASSERT(texture_height == (y1 - y0));
  const int _x0 = 0;
  const int _x1 = texture_width;
  const int _y0 = 0;
  const int _y1 = texture_height;

  if (mMode == GRADIENT_MODE_NONE)
  {
    const uint32_t color = getColorValue();

    for (int y = 0; y < texture_height; ++y)
    {
      uint32_t *texture = reinterpret_cast<uint32_t *>(&texture_buffer[y * texture_stride]);
      for (int x = 0; x < texture_width; ++x)
        texture[x] = color;
    }

    return;
  }

  if (!controller)
    return;

  switch (mMode & AXIS_MASK)
  {
    case GRADIENT_AXIS_MODE_X:
    {
      for (int i = _x0; i < _x1; i++)
      {
        const uint32_t color = controller->getPointColor(i - _x0, 0) | 0xff000000;

        for (int y = 0; y < texture_height; ++y)
        {
          uint32_t *texture = reinterpret_cast<uint32_t *>(&texture_buffer[y * texture_stride]);
          texture[i] = color;
        }
      }
    }
    break;

    case GRADIENT_AXIS_MODE_Y:
    {
      for (int i = _y0; i < _y1; i++)
      {
        const uint32_t color = controller->getPointColor(i - _y0, 0) | 0xff000000;

        uint32_t *texture = reinterpret_cast<uint32_t *>(&texture_buffer[i * texture_stride]);
        for (int x = 0; x < texture_width; ++x)
          texture[x] = color;
      }
    }
    break;

    case GRADIENT_AXIS_MODE_XY:
    {
      for (int y = _y0; y < _y1; ++y)
      {
        uint32_t *texture = reinterpret_cast<uint32_t *>(&texture_buffer[y * texture_stride]);

        for (int x = _x0; x < _x1; ++x)
        {
          const uint32_t color = controller->getPointColor(x - _x0, y - _y0) | 0xff000000;
          texture[x] = color;
        }
      }
    }
    break;
  }
}

void GradientPlotControl::drawArrow(Point2 pos, bool rotate)
{
  G_ASSERT(controller);
  if (!controller)
    return;

  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // dot line
  if (!rotate)
    drawList->AddLine(ImVec2(x0 + pos.x, y0) + viewOffset, ImVec2(x0 + pos.x, y1) + viewOffset, IM_COL32(255, 255, 255, 255));
  else
    drawList->AddLine(ImVec2(x0, y0 + pos.x) + viewOffset, ImVec2(x1, y0 + pos.x) + viewOffset, IM_COL32(255, 255, 255, 255));

  // cursor

  const E3DCOLOR e3color = controller->getPointColor(pos.x, pos.y);
  const ImU32 color IM_COL32(e3color.r, e3color.g, e3color.b, 255);

  const Point2 cur_pos(rotate ? x0 - 2 : pos.x + x0, rotate ? pos.x + y0 : y0 - 2);
  const int mx = hdpi::_pxS(TRACK_GRADIENT_BUTTON_WIDTH) / 2;
  const int my = hdpi::_pxS(TRACK_GRADIENT_BUTTON_HEIGHT) - 1;

  for (int i = 0; i < my; ++i)
  {
    int j = lerp(0, mx, i / (my - 1.0));

    if (!rotate)
      drawList->AddLine(ImVec2(cur_pos.x - j, cur_pos.y - i) + viewOffset, ImVec2(cur_pos.x + j, cur_pos.y - i) + viewOffset, color);
    else
      drawList->AddLine(ImVec2(cur_pos.x - i, cur_pos.y - j) + viewOffset, ImVec2(cur_pos.x - i, cur_pos.y + j) + viewOffset, color);
  }

  const ImU32 borderColor = IM_COL32(255, 255, 255, 255);
  const ImVec2 point1(cur_pos.x, cur_pos.y);
  if (!rotate)
  {
    const ImVec2 point2(cur_pos.x - mx, cur_pos.y - my);
    const ImVec2 point3(cur_pos.x + mx - 1, cur_pos.y - my);
    drawList->AddLine(point1 + viewOffset, point2 + viewOffset, borderColor);
    drawList->AddLine(point2 + viewOffset, point3 + viewOffset, borderColor);
    drawList->AddLine(point3 + viewOffset, point1 + viewOffset, borderColor);
  }
  else
  {
    const ImVec2 point2(cur_pos.x - my, cur_pos.y - mx);
    const ImVec2 point3(cur_pos.x - my, cur_pos.y + mx - 1);
    drawList->AddLine(point1 + viewOffset, point2 + viewOffset, borderColor);
    drawList->AddLine(point2 + viewOffset, point3 + viewOffset, borderColor);
    drawList->AddLine(point3 + viewOffset, point1 + viewOffset, borderColor);
  }
}

void GradientPlotControl::drawCross(Point2 pos)
{
  G_ASSERT(controller);
  if (!controller)
    return;

  const E3DCOLOR e3color = controller->getPointColor(pos.x, pos.y);
  const ImU32 color = IM_COL32(255 - e3color.r, 255 - e3color.g, 255 - e3color.b, 255);
  const float thickness = 3.0f;
  const Point2 cur_pos(pos.x + x0, pos.y + y0);
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->AddLine(ImVec2(cur_pos.x - 7, cur_pos.y) + viewOffset, ImVec2(cur_pos.x - 3, cur_pos.y) + viewOffset, color, thickness);
  drawList->AddLine(ImVec2(cur_pos.x + 7, cur_pos.y) + viewOffset, ImVec2(cur_pos.x + 3, cur_pos.y) + viewOffset, color, thickness);
  drawList->AddLine(ImVec2(cur_pos.x, cur_pos.y - 7) + viewOffset, ImVec2(cur_pos.x, cur_pos.y - 3) + viewOffset, color, thickness);
  drawList->AddLine(ImVec2(cur_pos.x, cur_pos.y + 7) + viewOffset, ImVec2(cur_pos.x, cur_pos.y + 3) + viewOffset, color, thickness);
}

//-------------------------------------------------------------
// event routines

void GradientPlotControl::onDrag(int new_x, int new_y) { onLButtonDown(new_x, new_y); }

void GradientPlotControl::onLButtonDown(long x, long y)
{
  if (mMode == GRADIENT_MODE_NONE)
    return;

  G_ASSERT(controller);
  if (!controller)
    return;

  switch (mMode & AXIS_MASK)
  {
    case GRADIENT_AXIS_MODE_X: mValue = controller->pickPlotValue(x - x0, 0); break;

    case GRADIENT_AXIS_MODE_Y: mValue = controller->pickPlotValue(y - y0, 0); break;

    default: mValue = controller->pickPlotValue(x - x0, y - y0);
  }

  dragMode = true;
  controlHolder.onWcChange(nullptr);
  return;
}

void GradientPlotControl::onLButtonUp(long x, long y)
{
  dragMode = false;
  return;
}

void GradientPlotControl::draw()
{
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  // We cannot redraw the texture anywhere because it might have been already used in this frame, so we redraw it before
  // the first use here.
  if (redrawGradientTexture)
  {
    redrawGradientTexture = false;
    drawGradientToTexture();
  }

  if (gradientTextureId != BAD_TEXTUREID)
    drawList->AddImage((ImTextureID)((unsigned)gradientTextureId), ImVec2(x0, y0) + viewOffset, ImVec2(x1, y1) + viewOffset);

  drawList->AddRect(ImVec2(x0 - 1, y0 - 1), ImVec2(x1, y1), IM_COL32(0, 0, 0, 255));

  // Draw the pointers.
  if (controller)
  {
    Point2 cur_pos = controller->getPlotPos();

    switch (mMode & AXIS_MASK)
    {
      case GRADIENT_AXIS_MODE_X: drawArrow(cur_pos, false); break;
      case GRADIENT_AXIS_MODE_Y: drawArrow(cur_pos, true); break;
      case GRADIENT_AXIS_MODE_XY: drawCross(cur_pos); break;
    }
  }
}

void GradientPlotControl::updateImgui()
{
  ImGui::PushID(this);

  const int widthToUse = min((int)floorf(ImGui::GetContentRegionAvail().x), requestedWidth);
  if (widthToUse != mWidth)
  {
    mWidth = widthToUse;
    updateDrawParams();
  }

  viewOffset = ImGui::GetCursorScreenPos();

  // This will catch the interactions.
  ImGui::InvisibleButton("canvas", ImVec2(mWidth, mHeight));

  const bool isHovered = ImGui::IsItemHovered();
  const ImVec2 mousePosInCanvas = ImGui::GetIO().MousePos - viewOffset;

  if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    onLButtonDown(mousePosInCanvas.x, mousePosInCanvas.y);
  else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    onLButtonUp(mousePosInCanvas.x, mousePosInCanvas.y);
  else if (mousePosInCanvas != lastMousePosInCanvas)
  {
    lastMousePosInCanvas = mousePosInCanvas;

    if (dragMode && ImGui::IsMouseDown(ImGuiMouseButton_Left))
      onDrag(mousePosInCanvas.x, mousePosInCanvas.y);
  }

  draw();

  if (dragMode && (mMode & AXIS_MASK) == GRADIENT_AXIS_MODE_XY)
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

  ImGui::PopID();
}

} // namespace PropPanel