// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#pragma once

#include "../../windowControls/w_simple_controls.h"
#include "../../windowControls/w_curve_math.h"

#include <math/dag_e3dColor.h>

enum
{
  COLOR_CURVE_MODE_RGB,
  COLOR_CURVE_MODE_R,
  COLOR_CURVE_MODE_G,
  COLOR_CURVE_MODE_B,
  COLOR_CURVE_MODE_COUNT,
};

// -----------------------------------------------

typedef Tab<ICurveControlCallback::PolyLine> PolyLineTab;
struct ColorCorrectionInfo;

class WColorCurveControl : public WindowControlBase
{
public:
  WColorCurveControl(WindowControlEventHandler *event_handler, WindowBase *parent, int left, int x, int y, int h);

  virtual ~WColorCurveControl();

  virtual intptr_t onControlDrawItem(void *info);
  virtual void resizeWindow(int w, int h);
  virtual intptr_t onLButtonDown(long x, long y);
  virtual intptr_t onLButtonDClick(long x, long y);
  virtual intptr_t onLButtonUp(long x, long y);
  virtual intptr_t onDrag(int x, int y);
  virtual intptr_t onMouseMove(int x, int y);
  virtual intptr_t onRButtonDown(long x, long y);
  virtual intptr_t onKeyDown(unsigned v_key, int flags);

  void setMode(int _mode);
  void setBarChart(const unsigned char rgb_bar_charts[3][256]);
  void getColorTables(unsigned char rgb_r_g_b[4][256]);

  void setCorrectionInfo(const ColorCorrectionInfo &settings);
  void getCorrectionInfo(ColorCorrectionInfo &settings);

  void setChartBarsVisible(bool show_bar_chart)
  {
    showChartBars = show_bar_chart;
    refresh(false);
  }
  void setBaseLineVisible(bool show_base_line)
  {
    showBaseLine = show_base_line;
    refresh(false);
  }

protected:
  bool isSelection();
  void build();
  void drawGeom(void *hdc);
  void drawCurve(void *hdc, int mode_index);
  void drawBarChart(void *hdc, int mode_index);
  void removeDC();
  void cutCurve(int index);

  void setPoints(int index, const Tab<Point2> &points);
  void getPoints(int index, Tab<Point2> &points);

  ICurveControlCallback *cbs[COLOR_CURVE_MODE_COUNT];
  PolyLineTab *cachPolyLines[COLOR_CURVE_MODE_COUNT];
  int cCurInd;
  PolyLineTab cachPolyLinesRGB, cachPolyLinesR, cachPolyLinesG, cachPolyLinesB;
  bool cancelMovement;
  bool startMovement;
  Point2 startingPos;
  BBox2 viewBox;
  Point2 viewSize;
  WTooltip mTooltip;
  int mMode;
  void *hBitmap, *hBitmapDC;
  unsigned char bar_chart[3][256];
  bool showChartBars, showBaseLine; // TODO static
};
