// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>
#include <propPanel/c_control_event_handler.h>

namespace PropPanel
{

class IGradientPlotController;
class PropertyControlBase;

class GradientPlotControl
{
public:
  GradientPlotControl(PropertyControlBase &control_holder, int x, int y, int w, int h);
  ~GradientPlotControl();

  void setMode(int mode);
  int getMode() const { return mMode; }

  // uses HSV (HSB) or RBG (0..1 each value), take mode to check
  Point3 getColorFValue() const;
  E3DCOLOR getColorValue() const;

  void setColorFValue(Point3 value);
  void setColorValue(E3DCOLOR value);

  void updateImgui();

protected:
  void updateDrawParams();

  void onDrag(int new_x, int new_y);
  void onLButtonDown(long x, long y);
  void onLButtonUp(long x, long y);

  void drawGradient(uint8_t *texture_buffer, int texture_width, int texture_height, uint32_t texture_stride);
  void drawArrow(Point2 pos, bool rotate);
  void drawCross(Point2 pos);
  void draw();

  void releaseGradientTexture();
  void drawGradientToTexture();

  const int requestedWidth;
  int mWidth;
  int mHeight;
  int mMode;
  Point3 mValue;
  int x0, x1, y0, y1;
  bool dragMode;

  IGradientPlotController *controller;
  PropertyControlBase &controlHolder;
  Point2 lastMousePosInCanvas;
  Point2 viewOffset;

  TEXTUREID gradientTextureId;
  bool redrawGradientTexture = false;
};

} // namespace PropPanel