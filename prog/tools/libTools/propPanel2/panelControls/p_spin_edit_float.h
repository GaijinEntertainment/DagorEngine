#pragma once

#include "p_base.h"
#include "../windowControls/w_spin_edit.h"
#include <propPanel2/c_panel_base.h>

class CSpinEditFloat : public BasicPropertyControl
{
public:
  CSpinEditFloat(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
    const char caption[], int prec = 3);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const
  {
    return CONTROL_DATA_TYPE_FLOAT | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION | CONTROL_DATA_PREC;
  }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_FLOAT; }

  float getFloatValue() const;
  void setFloatValue(float value);
  void setMinMaxStepValue(float min, float max, float step);
  void setPrecValue(int prec);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

private:
  WStaticText mCaption;
  WFloatSpinEdit mEditor;
};
