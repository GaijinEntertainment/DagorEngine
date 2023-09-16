// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_simple_controls.h"

#include <windows.h>
#include <commctrl.h>

// ------------------- Simple controls ---------------------------


// -------------- Container --------------

WContainer::WContainer(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent,
    parent->getUserColorsFlag() ? USER_CONTAINER_WINDOW_CLASS_NAME : CONTAINER_WINDOW_CLASS_NAME, WS_EX_CONTROLPARENT,
    WS_CHILD | WS_VISIBLE, "", x, y, w, h)
{}

void WContainer::onPostEvent(unsigned id) { mEventHandler->onWcPostEvent(id); }

intptr_t WContainer::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (mEventHandler)
    mEventHandler->onWcCommand(this, notify_code, elem_id);
  return 0;
}


// -------------- Group --------------

WGroup::WGroup(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", WS_EX_CONTROLPARENT, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, "", x, y, w, h)
{}


void WGroup::onPaint() { mEventHandler->onWcRefresh(this); }


void WGroup::onPostEvent(unsigned id) { mEventHandler->onWcPostEvent(id); }

intptr_t WGroup::onRButtonUp(long x, long y)
{
  if (mParent)
    mParent->onRButtonUp(x, y);
  return 0;
}


// -------------- Static --------------

WStaticText::WStaticText(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "STATIC", WS_EX_CONTROLPARENT, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_WORDELLIPSIS, "", x, y,
    w, h)
{}


intptr_t WStaticText::onResize(unsigned new_width, unsigned new_height)
{
  refresh();
  return WindowControlBase::onResize(new_width, new_height);
}

// -------------- Edit --------------


WEdit::WEdit(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, bool multiline) :
  WindowControlBase(event_handler, parent, "EDIT", WS_EX_STATICEDGE,
    WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | (multiline ? (ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL) : WS_TABSTOP), "",
    x, y, w, h),
  mNeedColorIndicate(false)
{}

intptr_t WEdit::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case EN_CHANGE: mEventHandler->onWcChange(this); break;

    default: break;
  }

  return 0;
}


intptr_t WEdit::onKeyDown(unsigned v_key, int flags)
{
  if (!mEventHandler)
    return 0;

  return mEventHandler->onWcKeyDown(this, v_key);
}


void WEdit::setColorIndication(bool value)
{
  if (mNeedColorIndicate != value)
  {
    mNeedColorIndicate = value;
    refresh(true);
  }
}

static void *edit_brush = CreateSolidBrush(RGB(173, 218, 165));

intptr_t WEdit::onDrawEdit(void *hdc)
{
  if (!mNeedColorIndicate || !edit_brush)
    return 0;

  SetBkMode((HDC)hdc, TRANSPARENT);
  return (intptr_t)edit_brush;
}

// -------------- Button --------------

WButton::WButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, bool show_sel) :

  WindowControlBase(event_handler, parent, "BUTTON", 0,
    WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER | WS_TABSTOP | (show_sel ? 0 : BS_NOTIFY), "", x, y, w, h)
{}

intptr_t WButton::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED: mEventHandler->onWcClick(this); break;

    case BN_SETFOCUS: SendMessage((HWND)getHandle(), WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0); break;

    default: break;
  }

  return 0;
}


// -------------- CheckBox --------------

bool WCheckBox::dragChecking = false;
bool WCheckBox::dragCheckingNewValue = false;

WCheckBox::WCheckBox(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, "", x, y, w, h)
{}

WCheckBox::~WCheckBox() { stopDragChecking(); }

intptr_t WCheckBox::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED:

      mEventHandler->onWcClick(this);
      mEventHandler->onWcChange(this);
      break;

    default: break;
  }

  return 0;
}


bool WCheckBox::checked() const { return (bool)SendMessage((HWND)this->getHandle(), BM_GETCHECK, 0, 0); }


void WCheckBox::setState(bool value) { SendMessage((HWND)this->getHandle(), BM_SETCHECK, (int)value, 0); }


intptr_t WCheckBox::onMouseMove(int x, int y)
{
  if (!dragChecking)
    return 0;

  // We are only interested in sibling check boxes.
  POINT pt = {x, y};
  ClientToScreen((HWND)getHandle(), &pt);
  const HWND parent = (HWND)getParentHandle();
  ScreenToClient(parent, &pt);
  const HWND sibling = ChildWindowFromPoint(parent, pt);
  char className[32];
  if (!sibling || GetClassName(sibling, className, sizeof(className)) <= 0 || stricmp(className, "button") != 0 ||
      (GetWindowLong(sibling, GWL_STYLE) & BS_TYPEMASK) != BS_AUTOCHECKBOX)
    return 0;

  const bool siblingChecked = SendMessage(sibling, BM_GETCHECK, 0, 0) == BST_CHECKED;
  if (siblingChecked == dragCheckingNewValue)
    return 0;

  // Sending BM_CLICK did not work.
  SendMessage(sibling, BM_SETCHECK, dragCheckingNewValue ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessage(parent, WM_COMMAND, MAKEWPARAM(getID(), BN_CLICKED), (LPARAM)sibling);

  return 1;
}

intptr_t WCheckBox::onRButtonUp(long x, long y)
{
  if (!dragChecking)
    return 0;

  stopDragChecking();
  return 1;
}

intptr_t WCheckBox::onRButtonDown(long x, long y)
{
  if (dragChecking)
    return 0;

  dragChecking = true;
  dragCheckingNewValue = !checked();
  SetCapture((HWND)getHandle());
  return 1;
}

intptr_t WCheckBox::onCaptureChanged(void *window_gaining_capture)
{
  if (dragChecking)
  {
    stopDragChecking();
    return 1;
  }

  return 0;
}

void WCheckBox::stopDragChecking()
{
  if (!dragChecking)
    return;

  dragChecking = false;
  ReleaseCapture();
}

// -------------- Radio Button --------------


WRadioButton::WRadioButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h, int index) :

  WindowControlBase(event_handler, parent, "BUTTON", 0,
    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | (index ? 0 : WS_GROUP) | WS_TABSTOP, "", x, y, w, h),

  mGroupIndex(index)
{}


void WRadioButton::setCheck(bool value) const
{
  long state = (value) ? BST_CHECKED : BST_UNCHECKED;
  SendMessage((HWND)this->getHandle(), BM_SETCHECK, state, 0);
}


bool WRadioButton::getCheck() const { return (BST_CHECKED == SendMessage((HWND)this->getHandle(), BM_GETCHECK, 0, 0)); }


intptr_t WRadioButton::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
    case BN_CLICKED: mEventHandler->onWcChange(this); break;

    default: break;
  }

  return 0;
}


intptr_t WRadioButton::onKeyDown(unsigned v_key, int flags)
{
  switch (v_key)
  {
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    {
      MSG _message = {0};
      POINT _pos;
      GetCursorPos(&_pos);

      _message.hwnd = (HWND)getHandle();
      _message.message = WM_KEYDOWN;
      _message.wParam = v_key;
      _message.lParam = flags;
      _message.time = GetTickCount();
      _message.pt = _pos;

      if (IsDialogMessage((HWND)getParentHandle(), &_message))
        return 1;
    }
  }

  return 0;
}


intptr_t WRadioButton::onLButtonDClick(long x, long y)
{
  mEventHandler->onWcDoubleClick(this);
  return 0;
}


// -------------- Separator --------------


WSeparator::WSeparator(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "STATIC", 0, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ETCHEDHORZ, "", x, y, w, h)
{}


// ----------------- Tooltip --------------------------


WindowBase *WTooltip::tooltipWnd = nullptr;

WTooltip::WTooltip(WindowControlEventHandler *event_handler, WindowBase *parent) : cHandle((intptr_t)(void *)parent->getHandle())
{
  if (!tooltipWnd)
    tooltipWnd = new WindowBase(nullptr, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP, nullptr, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT);
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  ti.hwnd = (HWND)(void *)cHandle;
  ti.hinst = NULL;
  ti.uId = cHandle;
  ti.lpszText = (LPSTR)tText.str();

  SendMessage((HWND)tooltipWnd->getHandle(), TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
  EnableWindow((HWND)tooltipWnd->getHandle(), false);
}


WTooltip::~WTooltip()
{
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd = (HWND)(void *)cHandle;
  ti.hinst = NULL;
  ti.uId = cHandle;
  SendMessage((HWND)tooltipWnd->getHandle(), TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
}

void WTooltip::setText(const char text[])
{
  SendMessage((HWND)tooltipWnd->getHandle(), TTM_ACTIVATE, (WPARAM)(strlen(text) > 0), 0);

  if (strcmp(tText.str(), text) == 0)
    return;

  tText = text;
  TOOLINFO ti = {0};
  ti.cbSize = sizeof(TOOLINFO);
  ti.hwnd = (HWND)(void *)cHandle;
  ti.hinst = NULL;
  ti.uId = cHandle;
  ti.lpszText = (LPSTR)tText.str();

  SendMessage((HWND)tooltipWnd->getHandle(), TTM_UPDATETIPTEXT, 0, (LPARAM)(LPTOOLINFO)&ti);
}
