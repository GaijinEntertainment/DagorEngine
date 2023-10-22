// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_spin_edit_int.h"
#include "../c_constants.h"


CSpinEditInt::CSpinEditInt(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT)),
  mCaption(this, parent->getWindow(), x, y,
    _px(w) - _pxS(DEFAULT_CONTROL_WIDTH) - _pxS(UP_DOWN_BUTTON_WIDTH) - _pxS(DEFAULT_CONTROLS_INTERVAL), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mEditor(this, parent->getWindow(), x + mCaption.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), y,
    _pxS(DEFAULT_CONTROL_WIDTH) + _pxS(UP_DOWN_BUTTON_WIDTH), _pxS(DEFAULT_CONTROL_HEIGHT), 0, 0, 1, 1)
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
  mEditor.moveWindow(x + mCaption.getWidth() + _pxS(DEFAULT_CONTROLS_INTERVAL), y);
}


void CSpinEditInt::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mEditor.setEnabled(enabled);
}


void CSpinEditInt::setWidth(hdpi::Px w)
{
  int minw = _pxS(DEFAULT_CONTROL_WIDTH) + _pxS(DEFAULT_CONTROLS_INTERVAL) + _pxS(UP_DOWN_BUTTON_WIDTH);
  w = (_px(w) < minw) ? _pxActual(minw) : w;

  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w) - minw, mCaption.getHeight());
  mEditor.resizeWindow(_pxS(DEFAULT_CONTROL_WIDTH) + _pxS(UP_DOWN_BUTTON_WIDTH), mEditor.getHeight());

  this->moveTo(this->getX(), this->getY());
}
