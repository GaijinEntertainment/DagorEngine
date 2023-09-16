#pragma once

#include "../c_window_controls.h"


class WMaxGroupButton : public WindowControlBase
{
public:
  WMaxGroupButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);
  virtual intptr_t onControlDrawItem(void *info);
  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onLButtonDown(long x, long y);
  virtual intptr_t onRButtonUp(long x, long y);

  virtual void setTextValue(const char value[]);
  virtual int getTextValue(char *buffer, int buflen) const;
  virtual void setOpened(bool value) { mOpened = value; }

private:
  bool mOpened;
  char mCaption[CAPTION_SIZE];
};
