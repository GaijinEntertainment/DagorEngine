// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_button.h"
#include "../c_constants.h"

CButton::CButton(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], const bool text_align_left) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_BUTTON_HEIGHT)),
  mButton(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_BUTTON_HEIGHT), true, text_align_left)
{
  mButton.setTextValue(caption);
  initTooltip(&mButton);
}


PropertyContainerControlBase *CButton::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  parent->createButton(id, caption, true, new_line);
  return NULL;
}


void CButton::setCaptionValue(const char value[]) { mButton.setTextValue(value); }


void CButton::setTextValue(const char value[]) { setCaptionValue(value); }


void CButton::setEnabled(bool enabled) { mButton.setEnabled(enabled); }


void CButton::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);
  mButton.resizeWindow(_px(w), mButton.getHeight());
}

void CButton::setFocus() { mButton.setFocus(); }

void CButton::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mButton.moveWindow(x, y);
}
