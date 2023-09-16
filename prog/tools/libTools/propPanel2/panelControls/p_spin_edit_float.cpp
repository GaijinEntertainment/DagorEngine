// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_spin_edit_float.h"
#include "../c_constants.h"

CSpinEditFloat::CSpinEditFloat(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[], int prec) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT),
  mCaption(this, parent->getWindow(), x, y, w - DEFAULT_CONTROL_WIDTH - UP_DOWN_BUTTON_WIDTH - DEFAULT_CONTROLS_INTERVAL,
    DEFAULT_CONTROL_HEIGHT),
  mEditor(this, parent->getWindow(), x + mCaption.getWidth() + DEFAULT_CONTROLS_INTERVAL, y,
    DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH, DEFAULT_CONTROL_HEIGHT, 0, 0, 0, prec)
{
  mCaption.setTextValue(caption);
  mEditor.setValue(0);
  initTooltip(&mEditor);
}

PropertyContainerControlBase *CSpinEditFloat::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createEditFloat(id, caption, 0, 2, true, new_line);
  return NULL;
}

void CSpinEditFloat::setFloatValue(float value) { mEditor.setValue(value); }

float CSpinEditFloat::getFloatValue() const { return mEditor.getValue(); }


void CSpinEditFloat::setMinMaxStepValue(float min, float max, float step) { mEditor.setMinMaxStepValue(min, max, step); }


void CSpinEditFloat::setPrecValue(int prec) { mEditor.setPrecValue(prec); }


void CSpinEditFloat::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CSpinEditFloat::reset()
{
  this->setFloatValue(mEditor.getMinValue());
  PropertyControlBase::reset();
}


void CSpinEditFloat::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mEditor.moveWindow(x + mCaption.getWidth() + DEFAULT_CONTROLS_INTERVAL, y);
}


void CSpinEditFloat::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEditor.setEnabled(enabled);
}


void CSpinEditFloat::setWidth(unsigned w)
{
  int minw = DEFAULT_CONTROL_WIDTH + DEFAULT_CONTROLS_INTERVAL + UP_DOWN_BUTTON_WIDTH;
  w = (w < minw) ? minw : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(w - minw, mCaption.getHeight());
  mEditor.resizeWindow(DEFAULT_CONTROL_WIDTH + UP_DOWN_BUTTON_WIDTH, mEditor.getHeight());

  this->moveTo(this->getX(), this->getY());
}
