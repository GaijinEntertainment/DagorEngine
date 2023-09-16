// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_radio_group.h"
#include "p_radio_button.h"
#include "../c_constants.h"

CRadioGroup::CRadioGroup(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, int w, int h,
  const char caption[]) :

  PropertyContainerVert(id, event_handler, parent, x, y, w, h),
  mRect(this, parent->getWindow(), x, y, w, h),
  mWResize(false),
  mSelectedIndex(RADIO_SELECT_NONE)
{
  hasCaption = (caption && strlen(caption) > 0);
  mRect.setTextValue(caption);
}


PropertyContainerControlBase *CRadioGroup::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  return parent->createRadioGroup(id, caption);
}


WindowBase *CRadioGroup::getWindow() { return &mRect; }


void CRadioGroup::setEnabled(bool enabled)
{
  mRect.setEnabled(enabled);

  for (int i = 0; i < mControlArray.size(); i++)
    mControlArray[i]->setEnabled(enabled);
}


void CRadioGroup::setCaptionValue(const char value[])
{
  hasCaption = (value && strlen(value) > 0);
  mRect.setTextValue(value);
}


void CRadioGroup::setIntValue(int value)
{
  if (value == mSelectedIndex)
    return;

  mSelectedIndex = RADIO_SELECT_NONE;

  for (int i = 0; i < mControlArray.size(); i++)
  {
    if (mControlArray[i]->getID() != this->getID())
      continue;

    if (mControlArray[i]->getIntValue() == value)
    {
      mControlArray[i]->setBoolValue(true);
      mSelectedIndex = value;
    }
    else
    {
      mControlArray[i]->setBoolValue(false);
    }
  }
}

int CRadioGroup::getIntValue() const { return mSelectedIndex; }


void CRadioGroup::reset()
{
  this->setIntValue(RADIO_SELECT_NONE);
  PropertyControlBase::reset();
}


void CRadioGroup::onWcChange(WindowBase *source)
{
  for (int i = 0; i < mControlArray.size(); i++)
  {
    if (mControlArray[i]->getID() != this->getID())
      continue;

    if (mControlArray[i]->getBoolValue())
    {
      mSelectedIndex = mControlArray[i]->getIntValue();
      break;
    }
  }

  PropertyControlBase::onWcChange(source);
}


void CRadioGroup::clear()
{
  mSelectedIndex = RADIO_SELECT_NONE;
  __super::clear();
}


void CRadioGroup::onControlAdd(PropertyControlBase *control)
{
  if (control)
  {
    mH = this->getNextControlY();
    mRect.resizeWindow(mW, mH);
  }
}


void CRadioGroup::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mRect.moveWindow(x, y);
}


void CRadioGroup::resizeControl(unsigned w, unsigned h)
{
  mH = h;
  mW = w;

  mRect.resizeWindow(mW, mH);
}


void CRadioGroup::setWidth(unsigned w)
{
  mWResize = true;
  this->resizeControl(w, this->getNextControlY());

  PropertyContainerVert::setWidth(w);
  mWResize = false;
}


void CRadioGroup::onChildResize(int id)
{
  if (!mWResize)
    __super::onChildResize(id);
}


int CRadioGroup::getVerticalInterval(int line_number, bool new_line)
{
  int def_val = PropertyContainerVert::getVerticalInterval(line_number, new_line);
  if (!hasCaption)
    return def_val;

  if (line_number == -1)
    def_val += DEFAULT_CONTROL_HEIGHT;
  else if (new_line)
    def_val -= DEFAULT_CONTROLS_INTERVAL;

  return def_val;
}
