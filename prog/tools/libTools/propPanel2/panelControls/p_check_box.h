#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>


class CCheckBox : public BasicPropertyControl
{
public:
  CCheckBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
    const char caption[]);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_BOOL | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_BOOL; }

  void setCaptionValue(const char value[]);
  void setBoolValue(bool value);
  bool getBoolValue() const;

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

private:
  WCheckBox mCheckBox;
};
