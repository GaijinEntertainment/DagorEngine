#pragma once

#include "w_simple_controls.h"

class WColorButton : public WindowControlBase
{
public:
  WColorButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onButtonStaticDraw(void *hdc);
  virtual intptr_t onButtonDraw(void *hdc);

  virtual void setTextValue(const char value[]);
  virtual int getTextValue(char *buffer, int buflen) const;
  virtual void setColorValue(E3DCOLOR color);
  virtual E3DCOLOR getColorValue() const { return mColor; }

  virtual void resizeWindow(int w, int h);

  ~WColorButton();

private:
  void handleClick();

  WindowControlBase mStatic;
  char mCaption[CAPTION_SIZE];
  E3DCOLOR mColor;
  void *mHBrush;
  bool mPushed;
  static E3DCOLOR colorChangeBuffer;
};


class WTwoColorArea : public WindowControlBase
{
public:
  WTwoColorArea(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  ~WTwoColorArea();

  virtual void setColorFValue(Point3 value);
  virtual void setColorValue(E3DCOLOR value);

  // standard w_control routines
  void moveWindow(int x, int y);
  void resizeWindow(int w, int h);
  void reset();

  // events
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onControlDrawItem(void *info);

private:
  E3DCOLOR mColorOld, mColorNew;
  void *mHBrushOld, *mHBrushNew;
};


class WPaletteCell : public WindowControlBase
{
public:
  WPaletteCell(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);

  ~WPaletteCell();

  virtual void setColorValue(E3DCOLOR value);
  virtual void setTextValue(const char value[]);
  virtual void setSelection(bool value) { mSelected = value; }

  // standard w_control routines
  void moveWindow(int x, int y);
  void resizeWindow(int w, int h);
  void reset();

  // events
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onControlDrawItem(void *info);

private:
  E3DCOLOR mColor;
  void *mHBrush;
  WTooltip mTooltip;
  bool mSelected;
};
