// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_common.h>
#include <propPanel/c_window_event_handler.h>
#include <propPanel/messageQueue.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>

namespace PropPanel
{

class GradientControlStandalone;

class TrackGradientButton
{
public:
  TrackGradientButton(WindowControlEventHandler *event_handler, E3DCOLOR color, bool moveable, GradientControlStandalone *owner,
    float _value);

  void showColorPicker();
  intptr_t onDrag(int new_x, int new_y);

  int getPosInGradientDisplaySpace() const;
  bool isMoving() const { return moving; }
  bool useless() const { return !useful; }
  E3DCOLOR getColorValue() const { return mColor; }
  void setColorValue(E3DCOLOR new_value) { mColor = new_value; }
  float getValue() const { return value; }
  void setValue(float new_value);
  void updateTooltipText();

  void cancelMoving();
  void markForDeletion();

  // Returns with true if the cursor is hovered over the key.
  bool updateImgui(const Point2 &view_offset, const Point2 &mouse_pos, bool gradient_control_hovered);

private:
  bool canMove;
  bool moving;
  bool useful;
  bool firstDragMsg;
  float value;
  E3DCOLOR mColor;
  GradientControlStandalone *mOwner;

  WindowControlEventHandler *mEventHandler;
  WindowBase *windowBaseForEventHandler = nullptr;
  String tooltipText;
};

class GradientControlStandalone : public IDelayedCallbackHandler
{
public:
  GradientControlStandalone(WindowControlEventHandler *event_handler, int w);
  ~GradientControlStandalone();

  void addKey(float position, E3DCOLOR *color, bool check_distance);
  E3DCOLOR getColor(float position);
  void getLimits(TrackGradientButton *key, int &left_limit, int &right_limit);
  void setValue(PGradient source);
  void setCurValue(float value);
  void setCycled(bool cycled) { mCycled = cycled; }
  void setMinMax(int min, int max);
  void getValue(PGradient destGradient) const;
  void reset();
  void updateCycled(TrackGradientButton *button);

  bool canRemove();
  void setSelected(bool value);

  int getGradientDisplayWidth() const { return gradientDisplaySize.x; }

  void updateImgui(int width, int height);

private:
  void draw();
  void onLButtonDClick(float x_pos_in_canvas_space);
  void calculateSizes(int total_width, int total_height);
  void cancelColorPickerShowRequest();

  void onImguiDelayedCallback(void *user_data) override;

  Tab<TrackGradientButton *> mKeys;
  float mCurValue;
  bool mCycled, mSelected;
  int mMinPtCount, mMaxPtCount;

  WindowControlEventHandler *mEventHandler;
  WindowBase *windowBaseForEventHandler = nullptr;
  int totalControlWidth = 0;
  int totalControlHeight = 0;
  IPoint2 gradientDisplayStart = IPoint2(0, 0);
  IPoint2 gradientDisplaySize = IPoint2(0, 0);
  Point2 viewOffset = Point2(0.0f, 0.0f);
  Point2 lastMouseDragPosInCanvas = Point2(-1.0f, -1.0f);
  int mouseClickKeyIndex = -1;
  int showColorPickerForKeyIndex = -1;
};

} // namespace PropPanel