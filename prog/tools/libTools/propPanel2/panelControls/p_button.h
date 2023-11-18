#pragma once

#include "p_base.h"
#include "../windowControls/w_simple_controls.h"
#include <propPanel2/c_panel_base.h>


class CButton : public BasicPropertyControl
{
public:
  CButton(ControlEventHandler *event_h, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, const char caption[],
    bool text_align_left = false);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const { return 0; }

  void setCaptionValue(const char value[]);
  void setTextValue(const char value[]);

  void setEnabled(bool enabled);
  void setWidth(hdpi::Px w);
  void setFocus();
  void moveTo(int x, int y);

private:
  WButton mButton;
};
