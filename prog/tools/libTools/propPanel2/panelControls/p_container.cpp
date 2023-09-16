// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_container.h"
#include "../c_constants.h"


CContainer::CContainer(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
  int interval) :

  PropertyContainerVert(id, event_handler, parent, x, y, w, h), mRect(this, parent->getWindow(), x, y, w, h), mWResize(false)
{
  __super::setVerticalInterval(interval);
}


PropertyContainerControlBase *CContainer::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  return parent->createContainer(id, new_line);
}


WindowBase *CContainer::getWindow() { return &mRect; }


void CContainer::setEnabled(bool enabled)
{
  mRect.setEnabled(enabled);

  for (int i = 0; i < mControlArray.size(); i++)
    mControlArray[i]->setEnabled(enabled);
}


void CContainer::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    mH = this->getNextControlY();
    mRect.resizeWindow(mW, mH);
  }
}


void CContainer::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mRect.moveWindow(x, y);
}


void CContainer::resizeControl(unsigned w, unsigned h)
{
  mH = h;
  mW = w;

  mRect.resizeWindow(mW, mH);
}


void CContainer::setWidth(unsigned w)
{
  mWResize = true;
  this->resizeControl(w, this->getNextControlY());

  PropertyContainerVert::setWidth(w);
  mWResize = false;
}


void CContainer::onChildResize(int id)
{
  if (!mWResize)
    __super::onChildResize(id);
}
