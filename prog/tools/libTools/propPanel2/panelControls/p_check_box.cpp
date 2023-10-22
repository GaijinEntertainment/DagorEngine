// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_check_box.h"
#include "../c_constants.h"

CCheckBox::CCheckBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT)),
  mCheckBox(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT))
{
  mCheckBox.setTextValue(caption);
  initTooltip(&mCheckBox);
}


PropertyContainerControlBase *CCheckBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createCheckBox(id, caption, false, true, new_line);
  return NULL;
}


void CCheckBox::setBoolValue(bool value) { mCheckBox.setState(value); }


void CCheckBox::setCaptionValue(const char value[]) { mCheckBox.setTextValue(value); }


void CCheckBox::reset()
{
  this->setBoolValue(false);
  PropertyControlBase::reset();
}


bool CCheckBox::getBoolValue() const { return mCheckBox.checked(); }


void CCheckBox::setEnabled(bool enabled) { mCheckBox.setEnabled(enabled); }


void CCheckBox::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);
  mCheckBox.resizeWindow(_px(w), mCheckBox.getHeight());
}


void CCheckBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mCheckBox.moveWindow(x, y);
}
