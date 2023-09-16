// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_spin_edit_int.h"
#include "../c_constants.h"


CSpinEditInt::CSpinEditInt(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT),
  mCaption(this, parent->getWindow(), x, y, w - DEFAULT_CONTROL_WIDTH - UP_DOWN_BUTTON_WIDTH - DEFAULT_CONTROLS_INTERVAL,
    DEFAULT_CONTROL_HEIGHT),
  mEditor(this, parent->getWindow(), x + mCaption.getWidth() + DEFAULT_CONTROLS_INTERVAL, y,
    DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH, DEFAULT_CONTROL_HEIGHT, 0, 0, 1, 1)
{
  mCaption.setTextValue(caption);
  mEditor.setValue(0);
  initTooltip(&mEditor);
}


PropertyContainerControlBase *CSpinEditInt::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createEditInt(id, caption, 0, true, new_line);
  return NULL;
}


void CSpinEditInt::setIntValue(int value) { mEditor.setValue(value); }

int CSpinEditInt::getIntValue() const { return floor(mEditor.getValue()); }


void CSpinEditInt::setMinMaxStepValue(float min, float max, float step)
{
  mEditor.setMinMaxStepValue(floor(min), ceil(max), ceil(step));
}


void CSpinEditInt::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CSpinEditInt::reset()
{
  this->setIntValue(floor(mEditor.getMinValue()));
  PropertyControlBase::reset();
}


void CSpinEditInt::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mEditor.moveWindow(x + mCaption.getWidth() + DEFAULT_CONTROLS_INTERVAL, y);
}


void CSpinEditInt::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEditor.setEnabled(enabled);
}


void CSpinEditInt::setWidth(unsigned w)
{
  int minw = DEFAULT_CONTROL_WIDTH + DEFAULT_CONTROLS_INTERVAL + UP_DOWN_BUTTON_WIDTH;
  w = (w < minw) ? minw : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(w - minw, mCaption.getHeight());
  mEditor.resizeWindow(DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH, mEditor.getHeight());

  this->moveTo(this->getX(), this->getY());
}
