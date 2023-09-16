#pragma once

#include "p_group_box.h"


class CTabPage : public CGroupBase
{
public:
  CTabPage(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION; }

  void setCaptionValue(const char value[]);

protected:
  virtual int getNextControlY(bool new_line = true);
};
