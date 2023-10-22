// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_separator.h"
#include "../c_constants.h"

CSeparator::CSeparator(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w) :

  PropertyControlBase(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_SEPARATOR_HEIGHT)),
  mSeparator(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_SEPARATOR_HEIGHT))
{}


PropertyContainerControlBase *CSeparator::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  parent->createSeparator(id, new_line);
  return NULL;
}


void CSeparator::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);
  mSeparator.resizeWindow(_px(w), mSeparator.getHeight());
}


void CSeparator::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mSeparator.moveWindow(x, y);
}
