#pragma once

#include "w_simple_controls.h"

class WTextGradient : public WindowControlBase
{
public:
  WTextGradient(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w);

  void setMinMax(int min, int max);
  void setValues(const TextGradient &source);
  void getValues(TextGradient &destGradient) const;
  void setCurrentKeyText(const char *value);
  void setCurValue(float value);
  const char *getCurrentKeyText() const;

  void moveWindow(int x, int y);
  void resizeWindow(int w, int h);
  void reset();
  void setSelected(bool value);

protected:
  intptr_t onControlDrawItem(void *info);
  intptr_t onRButtonUp(long x, long y);
  intptr_t onLButtonDown(long x, long y);
  intptr_t onLButtonUp(long x, long y);
  intptr_t onLButtonDClick(long x, long y);
  intptr_t onDrag(int x, int y);
  intptr_t onMouseMove(int x, int y);

private:
  void addKey(float position, const char *_text);
  void drawKeys(void *hdc);
  void clearForDraw(void *hdc);
  void drawRuller(void *hdc, int x, int y, int w);
  void drawCurrent(void *hdc, int x, int y, int w);

  int getStartXPos();
  int getStartYPos();
  int getEndXPos();
  int posToX(float _position);
  float xToPos(int _x);
  int getKeyIndexByX(int x);

  bool showKeyDialog(SimpleString &_text, float _position);
  void updateTooltipText(int x);

  TextGradient mKeys;
  WTooltip mTooltip;
  int changeKeyIndex, moveKeyIndex;
  bool isKeyMove, mSelected;
  int minCount, maxCount, oldXPos;
  SimpleString newValue;
  float curValue;
};
