// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_radio_button.h"
#include "../c_constants.h"

CRadioButton::CRadioButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w,
  const char caption[], int index) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, DEFAULT_CONTROL_HEIGHT),
  mButton(this, parent->getWindow(), x, y, w, DEFAULT_CONTROL_HEIGHT, index)
{
  mButton.setTextValue(caption);
  initTooltip(&mButton);
}


PropertyContainerControlBase *CRadioButton::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createRadio(id, caption, true, new_line);
  return NULL;
}


int CRadioButton::getIntValue() const { return mButton.getIndex(); }


bool CRadioButton::getBoolValue() const { return mButton.getCheck(); }


void CRadioButton::setBoolValue(bool value) { mButton.setCheck(value); }


void CRadioButton::setCaptionValue(const char value[]) { mButton.setTextValue(value); }


void CRadioButton::onWcChange(WindowBase *source)
{
  mParent->onWcChange(source);
  // PropertyControlBase::onWcChange(source);
}


void CRadioButton::setEnabled(bool enabled) { mButton.setEnabled(enabled); }


void CRadioButton::setWidth(unsigned w)
{
  PropertyControlBase::setWidth(w);
  mButton.resizeWindow(w, mButton.getHeight());
}


void CRadioButton::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mButton.moveWindow(x, y);
}
