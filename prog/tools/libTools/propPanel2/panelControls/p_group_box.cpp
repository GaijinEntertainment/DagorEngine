// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_group_box.h"
#include "../c_constants.h"

#include <util/dag_globDef.h>

// ------------------- Group Box -----------------


CGroupBox::CGroupBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  hdpi::Px h, const char caption[])

  :
  CGroupBase(event_handler, parent, new WGroup(this, parent->getWindow(), x, y, _px(w), _px(h)), id, x, y, w, h, caption)
{}


PropertyContainerControlBase *CGroupBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  return parent->createGroupBox(id, caption);
}


//-----------------------------------------------------------------------------


CGroupBase::CGroupBase(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, WindowControlBase *wc_rect, int id,
  int x, int y, hdpi::Px w, hdpi::Px h, const char caption[])

  :
  PropertyContainerVert(id, event_handler, parent, x, y, w, h), mRect(wc_rect)
{
  this->setCaptionValue(caption);
}


CGroupBase::~CGroupBase() { delete mRect; }


WindowBase *CGroupBase::getWindow() { return mRect; }


void CGroupBase::setCaptionValue(const char value[]) { mRect->setTextValue(value); }


void CGroupBase::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    mH = this->getNextControlY();
    mRect->resizeWindow(mW, mH);
  }
}


void CGroupBase::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mRect->moveWindow(x, y);
}


void CGroupBase::setEnabled(bool enabled) { mRect->setEnabled(enabled); }


void CGroupBase::resizeControl(unsigned w, unsigned h)
{
  mW = w;
  mH = h;

  mRect->resizeWindow(mW, mH);
}


void CGroupBase::setWidth(hdpi::Px w)
{
  this->resizeControl(_px(w), this->getNextControlY());

  PropertyContainerVert::setWidth(w);
}


int CGroupBase::getNextControlY(bool new_line)
{
  if (!(this->getChildCount()))
  {
    return PropertyContainerVert::getNextControlY(new_line) + _pxS(DEFAULT_CONTROL_HEIGHT);
  }
  else
  {
    return PropertyContainerVert::getNextControlY(new_line);
  }
}
