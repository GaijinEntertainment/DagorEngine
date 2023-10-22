// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_combo_box.h"
#include "../c_constants.h"

CComboBox::CComboBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], const Tab<String> &vals, int index, bool sorted) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) * 2),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mComboBox(this, parent->getWindow(), x, y + mCaption.getHeight(), _px(w), _pxS(DEFAULT_CONTROL_HEIGHT), vals, index, sorted),
  mSorted(sorted)
{
  hasCaption = strlen(caption) > 0;

  if (hasCaption)
  {
    mCaption.setTextValue(caption);
    mH = _pxS(DEFAULT_CONTROL_HEIGHT) + mComboBox.getHeight();
  }
  else
  {
    mCaption.hide();
    mH = mComboBox.getHeight();
    this->moveTo(x, y);
  }
  initTooltip(&mComboBox);
}


PropertyContainerControlBase *CComboBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  Tab<String> vals(midmem);
  parent->createCombo(id, caption, vals, -1, true, new_line);
  return NULL;
}


int CComboBox::getIntValue() const
{
  if (mSorted)
    return mComboBox.getSelectedItemData();
  else
    return mComboBox.getSelectedIndex();
}


void CComboBox::setIntValue(int index)
{
  if (mSorted)
    mComboBox.setSelectedByItemData(index);
  else
    mComboBox.setSelectedIndex(index);
}


void CComboBox::setStringsValue(const Tab<String> &vals) { mComboBox.setStrings(vals); }


void CComboBox::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CComboBox::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mComboBox.setEnabled(enabled);
}


void CComboBox::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mComboBox.resizeWindow(_px(w), mComboBox.getHeight());

  this->moveTo(this->getX(), this->getY());
}


void CComboBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mComboBox.moveWindow(x, (hasCaption) ? y + mCaption.getHeight() : y);
}


void CComboBox::reset()
{
  this->setIntValue(-1);
  PropertyControlBase::reset();
}
