#pragma once

#include "p_group.h"
#include "../windowControls/w_ext_buttons.h"

class CExtGroup : public CGroup
{
public:
  CExtGroup(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION | CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }

  int getIntValue() const;
  void setIntValue(int value);

  void setWidth(unsigned w);

protected:
  virtual void onWcClick(WindowBase *source);
  virtual void onWcRefresh(WindowBase *source);

  WExtButtons mQuartButtons;
  mutable int mButtonStatus;
};