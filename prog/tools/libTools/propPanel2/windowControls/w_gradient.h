#pragma once

#include "w_simple_controls.h"


class WGradient;

class WTrackGradientButton : public WindowControlBase
{
public:
  WTrackGradientButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, E3DCOLOR color, bool moveable,
    WGradient *owner, float _value);

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onDrag(int new_x, int new_y);
  virtual intptr_t onRButtonUp(long, long);
  virtual intptr_t onControlDrawItem(void *info);

  int getPos();
  bool useless() { return !useful; }
  E3DCOLOR getColorValue() const { return mColor; }
  void setColorValue(E3DCOLOR value)
  {
    mColor = value;
    refresh(true);
  }
  float getValue();
  void setValue(float value);
  void updateTooltipText();

private:
  bool canMove;
  bool moving;
  bool useful;
  bool firstDragMsg;
  float value;
  E3DCOLOR mColor;
  WGradient *mOwner;
  WTooltip mTooltip;
};


class WGradient : public WindowControlBase
{
public:
  WGradient(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w);
  ~WGradient();

  virtual intptr_t onControlDrawItem(void *info);
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);

  void addKey(float position, E3DCOLOR *color = NULL);
  E3DCOLOR getColor(float position);
  void moveWindow(int x, int y);
  void getLimits(WTrackGradientButton *key, int &left_limit, int &right_limit);
  void setValue(PGradient source);
  void setCurValue(float value);
  void setCycled(bool cycled) { mCycled = cycled; }
  void setMinMax(int min, int max);
  void getValue(PGradient destGradient) const;
  void resizeWindow(int w, int h);
  void reset();
  void redrawKeys();
  void updateCycled(WTrackGradientButton *button);

  bool canRemove();
  void setSelected(bool value);

private:
  Tab<WTrackGradientButton *> mKeys;
  float mCurValue;
  bool mCycled, mSelected;
  int mMinPtCount, mMaxPtCount;
};
