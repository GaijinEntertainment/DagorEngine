#pragma once

#include "../c_window_controls.h"

class WWindow : public WindowControlBase
{
public:
  WWindow(WindowControlEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[],
    bool user_color = false);

  // vertical scroll

  void addVScroll();
  void vScrollUpdate(int size, int pagesize);
  int getVScrollPos();
  void setVScrollPos(int pos);
  void removeVScroll();
  bool hasVScroll();
  intptr_t onVScroll(int dy, bool is_wheel);

  // horizontal scroll

  void addHScroll();
  void hScrollUpdate(int size, int pagesize);
  int getHScrollPos();
  void removeHScroll();
  bool hasHScroll();
  intptr_t onHScroll(int dx);

  // tab processing for scroll

  virtual void onTabFocusChange();
  virtual bool onClose();
  virtual void onPostEvent(unsigned id);

private:
  bool flagHasVScroll, flagHasHScroll;
  int mVScrollPosition, mVPageSize, mVSize;
  int mHScrollPosition, mHPageSize, mHSize;
};
