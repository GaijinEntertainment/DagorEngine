// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_list_box.h"
#include "../c_constants.h"

CListBox::CListBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x, int y, hdpi::Px w,
  const char caption[], const Tab<String> &vals, int index) :

  BasicPropertyControl(id, event_handler, parent, x, y, w, _pxScaled(DEFAULT_CONTROL_HEIGHT) + _pxScaled(DEFAULT_LISTBOX_HEIGHT)),
  mCaption(this, parent->getWindow(), x, y, _px(w), _pxS(DEFAULT_CONTROL_HEIGHT)),
  mListBox(this, parent->getWindow(), x, y + mCaption.getHeight(), _px(w), _pxS(DEFAULT_LISTBOX_HEIGHT), vals, index)
{
  mCaption.setTextValue(caption);
  initTooltip(&mListBox);
}


PropertyContainerControlBase *CListBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  Tab<String> vals(midmem);
  parent->createList(id, caption, vals, -1, true, new_line);
  return NULL;
}


int CListBox::getIntValue() const { return mListBox.getSelectedIndex(); }


void CListBox::setIntValue(int index) { mListBox.setSelectedIndex(index); }


void CListBox::setStringsValue(const Tab<String> &vals) { mListBox.setStrings(vals); }


void CListBox::setCaptionValue(const char value[]) { mCaption.setTextValue(value); }


void CListBox::setTextValue(const char value[]) { mListBox.renameSelected(value); }


int CListBox::addStringValue(const char *value) { return mListBox.addString(value); }


void CListBox::removeStringValue(int idx) { mListBox.removeString(idx); }


void CListBox::setEnabled(bool enabled)
{
  mCaption.setEnabled(enabled);
  mListBox.setEnabled(enabled);
}


void CListBox::setWidth(hdpi::Px w)
{
  PropertyControlBase::setWidth(w);

  mCaption.resizeWindow(_px(w), mCaption.getHeight());
  mListBox.resizeWindow(_px(w), mListBox.getHeight());
}


void CListBox::setHeight(hdpi::Px h)
{
  PropertyControlBase::setHeight(h);

  mListBox.resizeWindow(mListBox.getWidth(), _px(h) - mCaption.getHeight());
}


void CListBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);

  mCaption.moveWindow(x, y);
  mListBox.moveWindow(x, y + mCaption.getHeight());
}


void CListBox::reset()
{
  this->setIntValue(-1);
  PropertyControlBase::reset();
}
