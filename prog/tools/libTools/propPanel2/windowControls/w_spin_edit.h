#pragma once

#include "w_simple_controls.h"
#include "w_updown.h"


class WSpinEdit : public WEdit, public WindowControlEventHandler
{
public:
  WSpinEdit(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, float min, float max, float step,
    int prec);

  float getValue() const { return mInternalVal; }
  float getMinValue() const { return mMin; }
  void setValue(float value);
  void setMinMaxStepValue(float min, float max, float step);
  void setPrecValue(int prec);

  virtual void setEnabled(bool enabled);
  virtual void resizeWindow(int w, int h);
  virtual void moveWindow(int w, int h);

  virtual intptr_t onKeyDown(unsigned v_key, int flags);
  virtual void onKillFocus() override;

  virtual void onWcChange(WindowBase *source);
  virtual void onWcClick(WindowBase *source);

  intptr_t onChar(unsigned code, int flags) override;

private:
  void setValueInternal(float value);

  float correctBounds(float value);
  void sendWcChangeIfVarChanged();

  WUpDown mUpDown;
  float mInternalVal, mMin, mMax, mStep;
  float mExternalVal; // The value, which is known externally (either sent through onWcChange() or set externally).
  int mPrec;
  bool mManualSet;
  WindowControlEventHandler *mExternalHandler; // only for editbox, because it work with itself
};

class WFloatSpinEdit : public WSpinEdit
{
public:
  WFloatSpinEdit(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, float min, float max,
    float step, int prec);
  intptr_t onChar(unsigned code, int flags) override;
};