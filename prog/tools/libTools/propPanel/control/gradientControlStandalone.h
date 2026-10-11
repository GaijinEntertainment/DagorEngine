// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_common.h>
#include <propPanel/c_window_event_handler.h>
#include <propPanel/commonWindow/colorDialog.h>
#include <propPanel/messageQueue.h>
#include <math/dag_e3dColor.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{

class GradientControlStandalone;
class IMenu;
class GradientControlContextMenuEventHandler;

class TrackGradientButton : public IColorDialogChangeHandler
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
  // IColorDialogChangeHandler
  void onColorDialogChange(E3DCOLOR color) override;

  void applyColor(E3DCOLOR new_color);

  bool colorChangeStarted = false;
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
  void setMinMax(float min, float max);
  float getMinValue() const { return minValue; }
  float getMaxValue() const { return maxValue; }
  void setMinMaxPointCount(int min, int max);
  void getValue(PGradient destGradient) const;
  float getNormalizedFromValue(float value) const;
  float getValueFromNormalized(float normalized_value) const;
  void reset();
  void updateCycled(TrackGradientButton *button);
  void processContextMenu(unsigned id);

  bool canRemove();
  void setSelected(bool value);

  // Remembers a key's open color picker, so this control can detach it before it deletes that key.
  void beginColorPicker(ColorDialog &dialog);

  int getGradientDisplayWidth() const { return gradientDisplaySize.x; }

  // True while a key is grabbed or its color picker is open. updateImgui() claims the mouse buttons
  // for its own canvas id, so this control never takes ImGui's active item.
  bool isGestureActive() const { return mouseClickKeyIndex >= 0 || showColorPickerForKeyIndex >= 0; }

  void updateImgui(int width, int height);

private:
  void draw();
  void onLButtonDClick(float x_pos_in_canvas_space);
  void onRButtonDown();
  void calculateSizes(int total_width, int total_height);
  void cancelColorPickerShowRequest();
  void detachOpenColorPicker();
  void handleKeyPresses(unsigned canvas_id);
  void copyGradient();
  void pasteGradient();

  void onImguiDelayedCallback(void *user_data) override;

  Tab<TrackGradientButton *> mKeys;
  float mCurValue = 0.0f;
  bool mCycled, mSelected;
  int mMinPtCount, mMaxPtCount;
  float minValue = 0.0f;
  float maxValue = 1.0f;

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
  ColorDialog *openColorPicker = nullptr; // a key's picker, for as long as its modal loop runs
  eastl::unique_ptr<IMenu> contextMenu;
  eastl::unique_ptr<GradientControlContextMenuEventHandler> contextMenuEventHandler;
};

} // namespace PropPanel