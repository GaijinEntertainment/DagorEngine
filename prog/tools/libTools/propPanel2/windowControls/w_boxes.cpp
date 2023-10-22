// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_boxes.h"
#include "../c_constants.h"
#include <sepGui/wndMenuInterface.h>

#include <windows.h>
#include <windowsx.h>


// -------------- Combo Box --------------


WComboBox::WComboBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const Tab<String> &vals,
  int index, bool sorted) :

  WindowControlBase(event_handler, parent, "COMBOBOX", 0,
    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_HASSTRINGS | WS_TABSTOP | WS_VSCROLL | (sorted ? CBS_SORT : 0),
    "", x, y, w, h * _pxS(COMBO_HEIGHT_MUL)),

  mSelectIndex(-1)
{
  setFont(WindowBase::getComboFont());
  UpdateWindow((HWND)this->getHandle());
  this->setStrings(vals, sorted);
  this->setSelectedIndex(index);
}


int WComboBox::getSelectedText(char *buffer, int buflen) const
{
  HWND chandle = (HWND)this->getHandle();
  strcpy(buffer, "\0");

  int textlen = SendMessage(chandle, CB_GETLBTEXTLEN, (WPARAM)getSelectedIndex(), 0);

  if ((CB_ERR == textlen) || (buflen < textlen))
    return 0;

  int result = SendMessage(chandle, CB_GETLBTEXT, (WPARAM)getSelectedIndex(), (LPARAM)buffer);

  return (CB_ERR == result) ? 0 : result;
}


int WComboBox::getSelectedItemData() const
{
  HWND chandle = (HWND)this->getHandle();
  return SendMessage(chandle, CB_GETITEMDATA, (WPARAM)getSelectedIndex(), 0);
}


void WComboBox::setStrings(const Tab<String> &vals, bool set_item_data)
{
  HWND chandle = (HWND)this->getHandle();
  SendMessage(chandle, CB_RESETCONTENT, 0, 0);
  mSelectIndex = -1;

  for (int i = 0; i < vals.size(); i++)
  {
    int itemIndex = SendMessage(chandle, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)vals[i].str());
    if (set_item_data)
      SendMessage(chandle, CB_SETITEMDATA, itemIndex, i);
  }
}


void WComboBox::setSelectedIndex(int index)
{
  HWND chandle = (HWND)this->getHandle();

  if ((index >= -1) && (index < SendMessage(chandle, CB_GETCOUNT, 0, 0)))
  {
    mSelectIndex = index;
    SendMessage(chandle, CB_SETCURSEL, (WPARAM)index, 0);
  }
}


void WComboBox::setSelectedText(const char buffer[])
{
  HWND chandle = (HWND)this->getHandle();
  int sel = SendMessage(chandle, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)buffer);
  if (sel == CB_ERR)
    sel = -1;
  SendMessage(chandle, CB_SETCURSEL, (WPARAM)sel, 0);
  mSelectIndex = SendMessage(chandle, CB_GETCURSEL, 0, 0);
}


void WComboBox::setSelectedByItemData(int data)
{
  HWND chandle = (HWND)this->getHandle();
  int count = SendMessage(chandle, CB_GETCOUNT, 0, 0);
  int sel = -1;
  for (int i = 0; i < count; ++i)
  {
    if (SendMessage(chandle, CB_GETITEMDATA, i, 0) == data)
    {
      sel = i;
      break;
    }
  }

  SendMessage(chandle, CB_SETCURSEL, (WPARAM)sel, 0);
  mSelectIndex = SendMessage(chandle, CB_GETCURSEL, 0, 0);
}


intptr_t WComboBox::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  HWND chandle = (HWND)this->getHandle();

  switch (notify_code)
  {
    case CBN_SELCHANGE:

      mSelectIndex = SendMessage(chandle, CB_GETCURSEL, 0, 0);
      mEventHandler->onWcChange(this);
      break;

    default: break;
  }

  return 0;
}


unsigned WComboBox::getHeight()
{
  RECT rc;
  GetWindowRect((HWND)getHandle(), &rc);
  return rc.bottom - rc.top;
}


// -------------- List Box --------------


WListBox::WListBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, const Tab<String> &vals,
  int index, bool allow_popup_menu) :

  WindowControlBase(event_handler, parent, "LISTBOX", WS_EX_CLIENTEDGE,
    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_HASSTRINGS | LBS_NOTIFY | WS_TABSTOP, "", x, y, w, h),

  mSelectIndex(-1),
  dcEndWait(false)
{
  // SendMessage((HWND) this->getHandle(), LB_SETHORIZONTALEXTENT,
  //   (WPARAM) 5000, 0);

  showPopupMenu = false;
  menu = allow_popup_menu ? IMenu::createPopupMenu(mHandle) : nullptr;

  UpdateWindow((HWND)this->getHandle());
  this->setFont(WindowBase::getSmallPrettyFont());
  this->setStrings(vals);
  this->setSelectedIndex(index);
  this->resizeWindow(w, h);
}


WListBox::~WListBox() { del_it(menu); }


int WListBox::getSelectedText(char *buffer, int buflen) const
{
  HWND chandle = (HWND)this->getHandle();
  strcpy(buffer, "\0");

  int textlen = SendMessage(chandle, LB_GETTEXTLEN, (WPARAM)getSelectedIndex(), 0);

  if ((LB_ERR == textlen) || (buflen < textlen))
    return 0;

  int result = SendMessage(chandle, LB_GETTEXT, (WPARAM)getSelectedIndex(), (LPARAM)buffer);

  return (LB_ERR == result) ? 0 : result;
}


void WListBox::setStrings(const Tab<String> &vals)
{
  HWND chandle = (HWND)this->getHandle();
  SendMessage(chandle, LB_RESETCONTENT, 0, 0);
  mSelectIndex = -1;

  for (int i = 0; i < vals.size(); i++)
  {
    SendMessage(chandle, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)vals[i].str());
  }
}


void WListBox::renameSelected(const char *name)
{
  if (mSelectIndex > -1)
  {
    HWND chandle = (HWND)this->getHandle();
    SendMessage(chandle, LB_DELETESTRING, (WPARAM)mSelectIndex, 0);
    SendMessage(chandle, LB_INSERTSTRING, (WPARAM)mSelectIndex, (LPARAM)(LPCTSTR)name);
    SendMessage(chandle, LB_SETCURSEL, (WPARAM)mSelectIndex, 0);
  }
}


int WListBox::addString(const char *value)
{
  HWND chandle = (HWND)this->getHandle();
  int idx = SendMessage(chandle, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)value);
  return idx;
}


void WListBox::removeString(int idx)
{
  HWND chandle = (HWND)this->getHandle();
  int count = SendMessage(chandle, LB_DELETESTRING, (WPARAM)idx, 0);
  if (count == 0)
    mSelectIndex = -1;
  else if (count == idx) // if we delete last item
    --mSelectIndex;

  SendMessage(chandle, LB_SETCURSEL, (WPARAM)mSelectIndex, 0);
}


void WListBox::setSelectedIndex(int index)
{
  HWND chandle = (HWND)this->getHandle();

  if ((index >= -1) && (index < SendMessage(chandle, LB_GETCOUNT, 0, 0)))
  {
    mSelectIndex = index;
    SendMessage(chandle, LB_SETCURSEL, (WPARAM)index, 0);
  }
}


intptr_t WListBox::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  HWND chandle = (HWND)this->getHandle();

  switch (notify_code)
  {
    case LBN_SELCHANGE:

      mSelectIndex = SendMessage(chandle, LB_GETCURSEL, 0, 0);
      mEventHandler->onWcChange(this);
      break;

    case 0:
      if (menu)
        menu->click(elem_id);
      break;

    default: break;
  }

  return 0;
}


intptr_t WListBox::onLButtonDClick(long x, long y)
{
  dcEndWait = true;
  return 0;
}

intptr_t WListBox::onLButtonUp(long x, long y)
{
  if (dcEndWait)
  {
    mEventHandler->onWcDoubleClick(this);
    dcEndWait = false;
  }
  return 0;
}

intptr_t WListBox::onRButtonUp(long x, long y)
{
  if (!menu)
    return 0;

  const int itemCount = ListBox_GetCount((HWND)mHandle);
  for (int i = 0; i < itemCount; ++i)
  {
    RECT rc;
    ListBox_GetItemRect((HWND)mHandle, i, &rc);
    if (x >= rc.left && x < rc.right && y >= rc.top && y < rc.bottom)
    {
      setSelectedIndex(i);
      mEventHandler->onWcChange(this); // To behave the same way as a click (that sends a LBN_SELCHANGE notification).
      mEventHandler->onWcRightClick(this);
      if (showPopupMenu)
        menu->showPopupMenu();

      break;
    }
  }

  return 0;
}


// -------------- Multi Select List Box --------------


WMultiSelListBox::WMultiSelListBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h,
  const Tab<String> &vals) :

  WindowControlBase(event_handler, parent, "LISTBOX", WS_EX_CLIENTEDGE,
    WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOTIFY | WS_TABSTOP | LBS_MULTIPLESEL | LBS_EXTENDEDSEL, "", x, y, w, h),

  mSelIndexes(midmem),
  dcEndWait(false),

  mLines(midmem)
{
  UpdateWindow((HWND)this->getHandle());
  this->setFont(WindowBase::getSmallPrettyFont());
  this->setStrings(vals);
}


int WMultiSelListBox::getStrings(Tab<String> &vals)
{
  vals = mLines;
  return vals.size();
}


int WMultiSelListBox::getCount() const { return mLines.size(); }


void WMultiSelListBox::setFilter(const char value[])
{
  Tab<int> _filtered(tmpmem);

  if (strlen(value))
  {
    String filter(value);
    filter = filter.toLower();

    for (int i = 0; i < mLines.size(); ++i)
      if (strstr(mLines[i].toLower().str(), filter.str()))
        _filtered.push_back(i);
  }

  this->setSelection(_filtered);
}


void WMultiSelListBox::setStrings(const Tab<String> &vals)
{
  HWND chandle = (HWND)this->getHandle();
  SendMessage(chandle, LB_RESETCONTENT, 0, 0);
  clear_and_shrink(mSelIndexes);

  for (int i = 0; i < vals.size(); i++)
  {
    SendMessage(chandle, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)vals[i].str());
  }

  mLines = vals;
}


int WMultiSelListBox::updateSelection()
{
  HWND chandle = (HWND)this->getHandle();

  int _sel_count = SendMessage(chandle, LB_GETSELCOUNT, 0, 0);
  if (_sel_count)
  {
    int *sel_buffer = new int[_sel_count];
    _sel_count = SendMessage(chandle, LB_GETSELITEMS, _sel_count, (LPARAM)sel_buffer);

    if (_sel_count)
    {
      clear_and_shrink(mSelIndexes);
      for (int i = 0; i < _sel_count; ++i)
        mSelIndexes.push_back(sel_buffer[i]);
    }

    delete[] sel_buffer;
  }

  return _sel_count;
}


int WMultiSelListBox::getSelection(Tab<int> &sels)
{
  if (this->updateSelection())
    sels = mSelIndexes;

  return 0;
}


void WMultiSelListBox::setSelection(const Tab<int> &sels)
{
  HWND chandle = (HWND)this->getHandle();

  SendMessage(chandle, LB_SETSEL, (WPARAM) false, -1); // clear selection

  if (!sels.empty())
  {
    int beg = sels[0];
    for (int i = 1; i < sels.size(); ++i)
    {
      if (sels[i] == sels[i - 1] + 1)
        continue;
      else
      {
        SendMessage(chandle, LB_SELITEMRANGE, (WPARAM) true, beg + (sels[i - 1] << 16));
        beg = sels[i];
      }
    }

    SendMessage(chandle, LB_SELITEMRANGE, (WPARAM) true, beg + (sels.back() << 16));
    SendMessage(chandle, LB_SETCARETINDEX, sels[0], (LPARAM) false);
    SendMessage(chandle, LB_SETTOPINDEX, sels[0], 0);
  }

  this->updateSelection();
}


intptr_t WMultiSelListBox::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case LBN_SELCHANGE:
    {
      this->updateSelection();
      mEventHandler->onWcChange(this);
      break;
    }

    default: break;
  }

  return 0;
}


intptr_t WMultiSelListBox::onLButtonDClick(long x, long y)
{
  dcEndWait = true;
  return 0;
}


intptr_t WMultiSelListBox::onLButtonUp(long x, long y)
{
  if (dcEndWait)
  {
    mEventHandler->onWcDoubleClick(this);
    dcEndWait = false;
  }
  return 0;
}
