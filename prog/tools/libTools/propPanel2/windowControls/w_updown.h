#pragma once

#include "../c_window_controls.h"


class WUpDown : public WindowControlBase
{
public:
  WUpDown(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h);
  virtual intptr_t onControlNotify(void *info);
  virtual intptr_t onDrag(int new_x, int new_y);

  bool lastUpClicked();

private:
  bool mUpClicked;
  int mPriorY;
};
