#pragma once

#include "../c_window_controls.h"


class WTrackBar : public WindowControlBase
{
public:
  WTrackBar(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, float min, float max, float step,
    float power = TRACKBAR_DEFAULT_POWER);

  float getValue() { return mVal; }
  void setValue(float value);
  void setMinMaxStepValue(float min, float max, float step);
  void setPower(float power);

  void setRunnerPos(int x);

  virtual intptr_t onControlCommand(unsigned notify_code, unsigned elem_id);
  virtual intptr_t onDrag(int new_x, int new_y);

  virtual void setEnabled(bool enabled);
  virtual void resizeWindow(int w, int h);

private:
  float calculateXScale(float value_scale) const;
  float calculateValueScale(float x_scale) const;

  WindowControlBase mRunner, mLeader;
  float mVal, mMin, mMax, mStep;
  float mPower = 1.0f;
};
