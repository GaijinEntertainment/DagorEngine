// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "wndHeader.h"
#include "wndManager.h"
#include "windows/wndClientWindow.h"

#define _WIN32_WINNT 0x0600
#include <windows.h>

#include <debug/dag_debug.h>


extern const char *WNDCLASS_HEADER_WINDOW;

static HFONT header_font = 0;
static HFONT caption_font = 0;


LayoutWindowHeader::LayoutWindowHeader(ClientWindow *parent, WinManager *manager, bool is_top, int w, int h) :

  mParent(parent), mTopHeader(is_top), mManager(manager)
{
  if (!::header_font)
    ::header_font = CreateFont(-_pxS(CLOSE_BUTTON_FONT_HEIGHT), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
      CLEARTYPE_QUALITY, 0, "Webdings");
  if (!::caption_font)
    ::caption_font = CreateFont(-_pxS(CLOSE_BUTTON_FONT_HEIGHT), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
      CLEARTYPE_QUALITY, 0, "Tahoma");

  mHWnd = CreateWindowEx(0, WNDCLASS_HEADER_WINDOW, parent->getCaption(), WS_CHILD | WS_VISIBLE, 0, 0, w, h, (HWND)getParentHandle(),
    nullptr, nullptr, static_cast<BaseWindow *>(this));

  // controls

  if (mTopHeader)
  {
    int margin = (h - _pxS(CLOSE_BUTTON_SIZE)) / 2;

    mInternalHandles.mTopHeader.mCloseButtonHandle =
      CreateWindowEx(0, "Button", "r", WS_CHILD | WS_VISIBLE | BS_NOTIFY, w - _pxS(CLOSE_BUTTON_SIZE) - margin, margin + 1,
        _pxS(CLOSE_BUTTON_SIZE), _pxS(CLOSE_BUTTON_SIZE), (HWND)mHWnd, nullptr, nullptr, nullptr);

    SendMessage((HWND)mInternalHandles.mTopHeader.mCloseButtonHandle, WM_SETFONT, (WPARAM)::header_font, MAKELPARAM(true, 0));
  }
  else
  {
    unsigned hw = w / 2 + 1;

    mInternalHandles.mLeftHeader.mVerticalLineHandle1 =
      CreateWindowEx(0, "Static", "", WS_CHILD | WS_VISIBLE | WS_DLGFRAME, 0, 0, hw, h + 1, (HWND)mHWnd, nullptr, nullptr, nullptr);

    mInternalHandles.mLeftHeader.mVerticalLineHandle2 = CreateWindowEx(0, "Static", "", WS_CHILD | WS_VISIBLE | WS_DLGFRAME, hw - 1, 0,
      hw, h + 1, (HWND)mHWnd, nullptr, nullptr, nullptr);
  }
}


LayoutWindowHeader::~LayoutWindowHeader()
{
  if (mTopHeader)
  {
    DestroyWindow((HWND)mInternalHandles.mTopHeader.mCloseButtonHandle);
  }
  else
  {
    DestroyWindow((HWND)mInternalHandles.mLeftHeader.mVerticalLineHandle1);
    DestroyWindow((HWND)mInternalHandles.mLeftHeader.mVerticalLineHandle2);
  }

  DestroyWindow((HWND)mHWnd);
}


//------------------------------------------------------


const char *LayoutWindowHeader::getCaption() { return mParent->getCaption(); }


//------------------------------------------------------


void *LayoutWindowHeader::getParentHandle() { return mParent->getHandle(); }


//------------------------------------------------------


intptr_t LayoutWindowHeader::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  switch (msg)
  {
    case WM_COMMAND:
    {
      WORD wNotifyCode = HIWORD(w_param);
      HWND hwndCtl = (HWND)l_param;

      if (mTopHeader && (mInternalHandles.mTopHeader.mCloseButtonHandle == hwndCtl))
      {
        switch (wNotifyCode)
        {
          case BN_SETFOCUS: SendMessage((HWND)getHandle(), WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0); break;

          case BN_CLICKED: onCloseClick(); break;
        }
      }
    }
    break;

    case WM_RBUTTONUP:
    {
      HWND p_handle = (HWND)getParentHandle();

      if (p_handle)
        SendMessage(p_handle, msg, (WPARAM)w_param, (LPARAM)l_param);
    }
      return 0;

    case WM_PAINT:
      if (mTopHeader && (h_wnd == getHandle()))
      {
        intptr_t result = BaseWindow::winProc(h_wnd, msg, w_param, l_param);

        HDC dc = GetDC((HWND)h_wnd);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
        SelectObject(dc, caption_font);
        TextOut(dc, _pxS(CLOSE_CAPTION_X), _pxS(CLOSE_CAPTION_Y), getCaption(), i_strlen(getCaption()));
        ReleaseDC((HWND)h_wnd, dc);

        return result;
      }
      break;

    case WM_DESTROY: break;
  }

  return BaseWindow::winProc(h_wnd, msg, w_param, l_param);
}


//------------------------------------------------------


void LayoutWindowHeader::show() { ShowWindow((HWND)getHandle(), SW_SHOW); }


void LayoutWindowHeader::hide() { ShowWindow((HWND)getHandle(), SW_HIDE); }


//------------------------------------------------------


void LayoutWindowHeader::resize(int x, int y, int width, int height)
{
  setWindowPos(x, y, width, height);
  onResize(width, height);
}


//------------------------------------------------------


void LayoutWindowHeader::onResize(int w, int h)
{
  if (mTopHeader)
  {
    HWND handle = (HWND)mInternalHandles.mTopHeader.mCloseButtonHandle;
    int margin = (h - _pxS(CLOSE_BUTTON_SIZE)) / 2;
    SetWindowPos(handle, 0, w - _pxS(CLOSE_BUTTON_SIZE) - margin, margin + 1, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  }
  else
  {
    int hw = w / 2 + 1;

    HWND handle = (HWND)mInternalHandles.mLeftHeader.mVerticalLineHandle1;
    SetWindowPos(handle, 0, 0, 0, hw, h + 1, SWP_NOMOVE | SWP_NOZORDER);

    handle = (HWND)mInternalHandles.mLeftHeader.mVerticalLineHandle2;
    SetWindowPos(handle, 0, hw - 1, 0, hw, h + 1, SWP_NOZORDER);
  }
}


void LayoutWindowHeader::onCloseClick() { mManager->removeWindow(mParent->getClientAreaHandle()); }
