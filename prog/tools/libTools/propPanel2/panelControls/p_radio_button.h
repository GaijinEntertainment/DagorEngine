#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>


class CRadioButton : public BasicPropertyControl
{
public:
  CRadioButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int index);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_BOOL | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_BOOL | CONTROL_DATA_TYPE_INT; }

  int getIntValue() const;
  bool getBoolValue() const;
  void setBoolValue(bool value);
  void setCaptionValue(const char value[]);

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  WRadioButton mButton;
};
