// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_group.h"
#include "../c_constants.h"

#include <ioSys/dag_dataBlock.h>

CGroup::CGroup(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w, hdpi::Px h,
  const char caption[], HorzFlow horzFlow) :

  PropertyContainerVert(id, event_handler, parent, x, y, w, h, horzFlow),
  mRect(this, parent->getWindow(), x, y, _px(w), _px(h)),
  mMaxButton(this, parent->getWindow(), x + _pxS(GROUP_MINIMIZE_BUTTON_X), y + _pxS(GROUP_MINIMIZE_BUTTON_Y),
    _px(w) - 2 * _pxS(GROUP_MINIMIZE_BUTTON_X), _pxS(GROUP_MINIMIZE_BUTTON_SIZE))
{
  mMinimized = false;
  mMaximizedSize = _px(h);
  mMaxButton.setTextValue(caption);
}


WindowBase *CGroup::getWindow() { return &mRect; }


PropertyContainerControlBase *CGroup::createDefault(int id, PropertyContainerControlBase *parent, const char caption[], bool new_line)
{
  return parent->createGroup(id, caption);
}


void CGroup::setCaptionValue(const char value[])
{
  mMaxButton.setTextValue(value);

  // Explicit refresh call is needed for the case when only the caption is updated and there are no children changes otherwise.
  mMaxButton.refresh();
}

void CGroup::setEnabled(bool enabled)
{
  for (int i = 0; i < mControlArray.size(); ++i)
    mControlArray[i]->setEnabled(enabled);

  // mRect.setEnabled(enabled);
}


void CGroup::setBoolValue(bool value) { value ? minimize() : restore(); }


void CGroup::minimize()
{
  mH = _pxS(DEFAULT_GROUP_HEIGHT);
  mRect.resizeWindow(mW, mH);
  mMinimized = true;
  mMaxButton.setOpened(!mMinimized);
}


void CGroup::restore()
{
  mH = mMaximizedSize;
  mRect.resizeWindow(mW, mH);
  mMinimized = false;
  mMaxButton.setOpened(!mMinimized);
}


void CGroup::onWcClick(WindowBase *source)
{
  if (source == &mMaxButton)
  {
    if (mMinimized)
      restore();
    else
      minimize();
  }
}


void CGroup::onWcRefresh(WindowBase *source)
{
  if (source == &mRect)
    mMaxButton.refresh(true);
}


void CGroup::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    mMaximizedSize = mH = this->getNextControlY();
    mRect.resizeWindow(mW, mH);
  }
}


void CGroup::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mRect.moveWindow(x, y);
  mMaxButton.moveWindow(x + _pxS(GROUP_MINIMIZE_BUTTON_X), y + _pxS(GROUP_MINIMIZE_BUTTON_Y));

  mRect.hide();
  mRect.show();
}


void CGroup::resizeControl(unsigned w, unsigned h)
{
  mMaximizedSize = h;
  mW = w;

  if (!mMinimized)
  {
    mH = h;
  }

  mRect.resizeWindow(mW, mH);
}


void CGroup::setWidth(hdpi::Px w)
{
  unsigned int oldH = mH;
  unsigned int newH = this->getNextControlY();
  PropPanel2 *pc = getParent();
  if (horzFlow == HorzFlow::Enabled && pc && oldH != newH)
    pc->enableResizeCallback();
  this->resizeControl(_px(w), newH);
  mMaxButton.resizeWindow(_px(w) - 2 * _pxS(GROUP_MINIMIZE_BUTTON_X), _pxS(GROUP_MINIMIZE_BUTTON_SIZE));

  PropertyContainerVert::setWidth(w);
  if (horzFlow == HorzFlow::Enabled && pc && oldH != newH)
    pc->disableResizeCallback();
}


int CGroup::getNextControlY(bool new_line)
{
  if (!(this->getChildCount()))
  {
    return PropertyContainerVert::getNextControlY(new_line) + _pxS(DEFAULT_GROUP_FIRST_LINE);
  }
  else
  {
    return PropertyContainerVert::getNextControlY(new_line);
  }
}

int CGroup::getCaptionValue(char *buffer, int buflen) const { return mMaxButton.getTextValue(buffer, buflen); }


void CGroup::clear()
{
  __super::clear();
  restore();
}


// ----------------------------------------------------------------------------

int CGroup::saveState(DataBlock &datablk)
{
  DataBlock *_block = datablk.addNewBlock(String(64, "group_%d", this->getID()).str());
  _block->addBool("minimize", this->getBoolValue());

  __super::saveState(datablk);
  return 0;
}


int CGroup::loadState(DataBlock &datablk)
{
  DataBlock *_block = datablk.getBlockByName(String(64, "group_%d", this->getID()).str());
  if (_block)
    this->setBoolValue(_block->getBool("minimize", true));

  __super::loadState(datablk);
  return 0;
}
