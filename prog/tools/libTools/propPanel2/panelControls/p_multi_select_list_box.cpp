// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "p_multi_select_list_box.h"
#include "../c_constants.h"


CMultiSelectListBox::CMultiSelectListBox(ControlEventHandler *event_handler, PropertyContainerControlBase *parent, int id, int x,
  int y, int w, int h, const Tab<String> &vals) :
  BasicPropertyControl(id, event_handler, parent, x, y, w, h), mListBox(this, parent->getWindow(), x, y, w, h, vals)
{
  initTooltip(&mListBox);
}


PropertyContainerControlBase *CMultiSelectListBox::createDefault(int id, PropertyContainerControlBase *parent, const char caption[],
  bool new_line)
{
  Tab<String> vals(midmem);
  parent->createMultiSelectList(id, vals, DEFAULT_LISTBOX_HEIGHT, true, true);
  return NULL;
}


void CMultiSelectListBox::setTextValue(const char value[]) { mListBox.setFilter(value); }


void CMultiSelectListBox::setStringsValue(const Tab<String> &vals) { mListBox.setStrings(vals); }


void CMultiSelectListBox::setSelectionValue(const Tab<int> &sels) { mListBox.setSelection(sels); }


int CMultiSelectListBox::getStringsValue(Tab<String> &vals) { return mListBox.getStrings(vals); }


int CMultiSelectListBox::getIntValue() const { return mListBox.getCount(); }


int CMultiSelectListBox::getSelectionValue(Tab<int> &sels) { return mListBox.getSelection(sels); }


void CMultiSelectListBox::reset()
{
  Tab<int> vals(midmem);
  this->setSelectionValue(vals);
  PropertyControlBase::reset();
}


void CMultiSelectListBox::setEnabled(bool enabled) { mListBox.setEnabled(enabled); }


void CMultiSelectListBox::setWidth(unsigned w) { mListBox.resizeWindow(w, mListBox.getHeight()); }


void CMultiSelectListBox::moveTo(int x, int y)
{
  PropertyControlBase::moveTo(x, y);
  mListBox.moveWindow(x, y);
}
