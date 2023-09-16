#pragma once

#include "w_simple_controls.h"

#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>


class IGradientPlotController;

class WGradientPlot : public WindowControlBase
{
public:
  WGradientPlot(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  ~WGradientPlot();

  virtual void setMode(int mode);
  virtual int getMode() const { return mMode; }

  // uses HSV (HSB) or RBG (0..1 each value), take mode to check
  virtual Point3 getColorFValue() const;
  virtual E3DCOLOR getColorValue() const;

  virtual void setColorFValue(Point3 value);
  virtual void setColorValue(E3DCOLOR value);

  // standard w_control routines
  void moveWindow(int x, int y);
  void resizeWindow(int w, int h);
  void reset();

  // events
  virtual intptr_t onControlDrawItem(void *info);
  virtual intptr_t onDrag(int new_x, int new_y);
  virtual intptr_t onLButtonDown(long x, long y);
  virtual intptr_t onLButtonUp(long x, long y);

protected:
  void updateDrawParams();

  void drawGradient(void *hdc, int x, int y);
  void drawArrow(void *hdc, Point2 pos, bool rotate);
  void drawCross(void *hdc, Point2 pos);

  void resetDC();
  void drawGradientToDC();

  int mMode;
  Point3 mValue;
  int x0, x1, y0, y1;
  bool dragMode;
  void *hBitmap, *hBitmapDC;

  IGradientPlotController *controller;
};
