// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_target_button.h"
#include "../c_constants.h"


CTargetButton::CTargetButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y,
  hdpi::Px w, const char caption[]) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT)), // _pxScaled(DEFAULT_BUTTON_HEIGHT)
  mCaption(this, parent->getWindow(), x, y, _px(w) / 2, _pxS(DEFAULT_CONTROL_HEIGHT)),
  mTargetButton(this, parent->getWindow(), x + _px(w) / 2, y, _px(w) / 2, _pxS(DEFAULT_CONTROL_HEIGHT)),
  mValue("")
{
  mCaption.setTextValue(caption);
  mTargetButton.setTextValue("<none>");
  initTooltip(&mTargetButton);
}


PropertyContainerControlBase *CTargetButton::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createTargetButton(id, caption, "", true, new_line);
  return NULL;
}


void CTargetButton::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mTargetButton.setEnabled(enabled);
}


void CTargetButton::reset()
{
  this->setTextValue("");
  PropertyControlBase::reset();
}


void CTargetButton::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w) / 2, mCaption.getHeight());
  mTargetButton.resizeWindow(_px(w) / 2, mTargetButton.getHeight());

  this->moveTo(this->getX(), this->getY());
}


void CTargetButton::setTextValue(const char value[])
{
  mValue = value;
  if (mValue != "")
    mTargetButton.setTextValue(value);
  else
    mTargetButton.setTextValue("<none>");
}


void CTargetButton::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


int CTargetButton::getTextValue(char *buffer, int buflen) const
{
  if (mValue.size() < buflen)
  {
    strcpy(buffer, mValue);
    return i_strlen(buffer);
  }

  return 0;
}


void CTargetButton::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mTargetButton.moveWindow(x + mCaption.getWidth(), y);
}


void CTargetButton::onWcClick(WindowBase *source)
{
  if (source == &mTargetButton)
    PropertyControlBase::onWcChange(source);
}
