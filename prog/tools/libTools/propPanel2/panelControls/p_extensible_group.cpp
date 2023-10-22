// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_extensible_group.h"

CExtGroup::CExtGroup(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  hdpi::Px h, const char caption[]) :

  CGroup(event_handler, parent, id, x, y, w, h, caption),
  mQuartButtons(this, &mMaxButton, mMaxButton.getWidth() - _pxS(DEFAULT_CONTROL_HEIGHT) - 1, 1, _pxS(DEFAULT_CONTROL_HEIGHT),
    _pxS(DEFAULT_CONTROL_HEIGHT)),
  mButtonStatus(EXT_BUTTON_NONE)
{}

PropertyContainerControlBase *CExtGroup::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  return parent->createExtGroup(id, caption);
}


int CExtGroup::getIntValue() const
{
  int result = mButtonStatus;
  mButtonStatus = EXT_BUTTON_NONE;
  return result;
}


void CExtGroup::setIntValue(int value) { mQuartButtons.setItemFlags(value); }


void CExtGroup::onWcClick(WindowBase *source)
{
  if (source == &mQuartButtons)
  {
    mButtonStatus = mQuartButtons.getValue();
    PropertyControlBase::onWcClick(source);
  }

  CGroup::onWcClick(source);
}


void CExtGroup::onWcRefresh(WindowBase *source)
{
  if (source == &mMaxButton)
    mQuartButtons.refresh(true);

  CGroup::onWcRefresh(source);
}


void CExtGroup::setWidth(hdpi::Px w)
{
  CGroup::setWidth(w);
  mQuartButtons.moveWindow(mMaxButton.getWidth() - _pxS(DEFAULT_CONTROL_HEIGHT) - 1, 1);
}
