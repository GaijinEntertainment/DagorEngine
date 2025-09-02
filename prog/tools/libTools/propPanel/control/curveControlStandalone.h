// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../commonWindow/w_curve_math.h"
#include <propPanel/c_window_event_handler.h>
#include <propPanel/messageQueue.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>

namespace PropPanel
{

// -----------------------------------------------

typedef struct tagPOINT POINT;

struct MyRect
{
  int l, t, r, b;

  int operator!=(MyRect &r2)
  {
    if (l != r2.l || t != r2.t || r != r2.r || b != r2.b)
      return 1;

    return 0;
  }

  int operator==(MyRect &r2)
  {
    if (l == r2.l && t == r2.t && r == r2.r && b == r2.b)
      return 1;

    return 0;
  }

  int width() const { return r - l; }
  int height() const { return b - t; }
};


/////////////////////////////////////////////////////////////////////////////
class CurveControlStandalone : public IDelayedCallbackHandler
{
public:
  enum
  {
    MODE_SELECTION,
    MODE_CREATION,
    MODE_DRAG_COORD,
  };


  CurveControlStandalone(WindowControlEventHandler *event_handler, WindowBase *parent, int left, int top, int w, int h, int id = -1,
    const Point2 &left_bottom = Point2(-0.05, -0.1), const Point2 &right_top = Point2(1.05, 1.1),
    ICurveControlCallback *curve_control_cb = nullptr);

  virtual ~CurveControlStandalone() { setCB(nullptr); }

  // set and get ICurveControlCallback pointer
  ICurveControlCallback *getCB() const { return cb; }
  void setCB(ICurveControlCallback *curve_control_cb)
  {
    if (cb)
    {
      delete cb;
      cb = nullptr;
    }
    cb = curve_control_cb;
    build();
  }

  void setValue(Tab<Point2> &source);
  void getValue(Tab<Point2> &dest) const;

  // set and get viewbox for curve
  void setViewBox(const Point2 &left_bottom, const Point2 &right_top);
  Point2 getLeftBottom() const { return Point2(viewBox.lim[0].x, viewBox.lim[1].y); }
  Point2 getRightTop() const { return Point2(viewBox.lim[1].x, viewBox.lim[0].y); }

  // set and get current mode (creation mode adds new point for each LMB click); RMB click resets mode to default
  void setMode(int _mode = MODE_CREATION) { mode = _mode; }
  int getMode() { return mode; }

  void setColor(E3DCOLOR color) { mColor = color; };
  void setFixed(bool value);
  void setMinMax(int min, int max);
  void setXMinMax(float min, float max);
  void setYMinMax(float min, float max);
  void setCurValue(float value);
  void setCycled(bool cycled);
  void setSelected(bool value) { mSelected = value; }

  void autoZoom();

  void updateImgui(float width, float height);

protected:
  void onLButtonDown(long x, long y);
  void onLButtonDClick(long x, long y);
  void onLButtonUp(long x, long y);
  void onDrag(int x, int y);
  void onMouseMove(int x, int y);
  void onRButtonDown(long x, long y);
  void onVScroll(int dy, bool is_wheel);
  void handleKeyPresses(unsigned canvas_id);

  void drawAxisMarks(uint32_t color);
  void drawPlotLine(const Point2 &p1, const Point2 &p2, uint32_t color);
  void drawMarkText(const Point2 &pos, double _value);
  void drawHelp();
  void drawInternal();
  void draw();
  bool checkTextNeed(int count, int position);

  ICurveControlCallback *cb;

  // sets size and position of view area (in curve coordinates)
  BBox2 defaultViewBox;
  BBox2 viewBox;
  // sets size in pixels, can be used to specify required accuracy of curve
  Point2 viewSize;
  Point2 viewOffset;
  Tab<ICurveControlCallback::PolyLine> cachPolyLines;
  bool cancelMovement;
  bool startMovement;
  Point2 startingPos;
  struct
  {
    bool active;
    int type;
    MyRect sel;
  } rectSelect;
  int mode;
  float mCurValue;

  bool isSelection();
  int ProcessRectSelection(int x, int y);
  void startRectangularSelection(int mx, int my, int type);
  bool endRectangularSelection(MyRect *result, int *type);
  void paintSelectionRect();
  void build();

  void showEditDialog(int idx);

  void onImguiDelayedCallback(void *user_data) override;

  E3DCOLOR mColor;
  int mMinPtCount, mMaxPtCount;
  bool mDrawAMarks, mSelected;

  WindowControlEventHandler *mEventHandler;
  WindowBase *windowBaseForEventHandler = nullptr;
  Point2 lastMousePosInCanvas;
  String tooltipText;
  bool showHelp = false;
  int delayedEditDialogPointIndex = -1;
  ImGuiMouseCursor mouseCursor = ImGuiMouseCursor_Arrow;
};

} // namespace PropPanel