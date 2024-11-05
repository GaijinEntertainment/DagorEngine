// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "w_curve_math.h"
#include <propPanel/c_window_event_handler.h>
#include <util/dag_string.h>

namespace PropPanel
{

struct ColorCorrectionInfo;

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

class ColorCurveControl
{
public:
  ColorCurveControl(WindowControlEventHandler *event_handler, WindowBase *parent, int left, int x, int y, int h);
  ~ColorCurveControl();

  void setMode(int _mode);
  void setBarChart(const unsigned char rgb_bar_charts[3][256]);
  void getColorTables(unsigned char rgb_r_g_b[4][256]);

  void setCorrectionInfo(const ColorCorrectionInfo &settings);
  void getCorrectionInfo(ColorCorrectionInfo &settings);

  void setChartBarsVisible(bool show_bar_chart) { showChartBars = show_bar_chart; }
  void setBaseLineVisible(bool show_base_line) { showBaseLine = show_base_line; }

  void showEditDialog();

  void updateImgui();

private:
  void onLButtonDown(long x, long y);
  void onLButtonDClick(long x, long y);
  void onLButtonUp(long x, long y);
  void onDrag(int x, int y);
  void onMouseMove(int x, int y);
  void onRButtonDown(long x, long y);

  bool isSelection();
  void build();
  void drawGeom();
  void drawCurve(int mode_index);
  void drawBarChart(int mode_index);
  void cutCurve(int index);

  void setPoints(int index, const Tab<Point2> &points);
  void getPoints(int index, Tab<Point2> &points);

  ICurveControlCallback *cbs[COLOR_CURVE_MODE_COUNT];
  PolyLineTab *cachPolyLines[COLOR_CURVE_MODE_COUNT];
  PolyLineTab cachPolyLinesRGB, cachPolyLinesR, cachPolyLinesG, cachPolyLinesB;
  bool cancelMovement;
  bool startMovement;
  Point2 startingPos;
  BBox2 viewBox;
  Point2 viewOffset;
  Point2 viewSize;
  int mMode;
  unsigned char bar_chart[3][256];
  bool showChartBars, showBaseLine; // TODO static
  WindowControlEventHandler *mEventHandler;
  WindowBase *windowBaseForEventHandler = nullptr;
  Point2 lastMousePosInCanvas;
  String tooltipText;
  int pointsIndexForShowEditDialog = -1;
};

} // namespace PropPanel