#pragma once

#include "p_base.h"
#include "../windowControls/w_track.h"
#include "../windowControls/w_spin_edit.h"
#include <propPanel2/c_panel_base.h>


class CTrackBarFloat : public BasicPropertyControl
{
public:
  CTrackBarFloat(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
    const char caption[], float min, float max, float step, float power = TRACKBAR_DEFAULT_POWER);

  static PropertyContainerControlBase *createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
    bool new_line = true);

  unsigned getTypeMaskForSet() const { return CONTROL_DATA_TYPE_FLOAT | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const { return CONTROL_DATA_TYPE_FLOAT; }

  float getFloatValue() const;
  void setFloatValue(float value);
  void setMinMaxStepValue(float min, float max, float step);
  void setCaptionValue(const char value[]);

  void reset();

  void setEnabled(bool enabled);
  void setWidth(unsigned w);
  void moveTo(int x, int y);

protected:
  virtual void onWcChange(WindowBase *source);

private:
  WStaticText mCaption;
  WTrackBar mTrackBar;
  WFloatSpinEdit mEditor;
  float mValue;
};
