// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "c_window_base.h"
#include "c_window_constants.h"
#include "c_constants.h"

#include <propPanel2/c_util.h>

#include <stdio.h>
#include <windowsx.h>
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>
#include <debug/dag_debug.h>
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <libTools/util/blkUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_input.h>


enum
{
  FONT_HT = 14,
};

static void *wnd_brush = CreateSolidBrush(USER_GUI_COLOR);
HGLOBAL gclipbuf_handle = NULL;

void debug_int(int value)
{
  char buf[100];
  sprintf(buf, "%d", value);
  MessageBox(0, buf, "Int value debug", MB_OK);
}

void debug_string(const char value[]) { MessageBox(0, value, "String value debug", MB_OK); }


///////////////////////////////////////////////////////////////////////////////

class WindowBaseHandler
{
public:
  static LRESULT __stdcall windowProc(HWND wnd, unsigned msg, WPARAM w_param, LPARAM l_param);
  static LRESULT __stdcall containerProc(HWND wnd, unsigned msg, WPARAM w_param, LPARAM l_param);
  static LRESULT __stdcall controlProc(HWND wnd, unsigned msg, WPARAM w_param, LPARAM l_param);

private:
  static bool processUserMessages(WindowBase *lwbp, unsigned msg, WPARAM w_param, LPARAM l_param, LRESULT &res);
  static int processDialogMessages(WindowBase *lwbp, unsigned msg, WPARAM w_param, LPARAM l_param);
};


int WindowBaseHandler::processDialogMessages(WindowBase *lwbp, unsigned msg, WPARAM w_param, LPARAM l_param)
{
  G_ASSERT(lwbp && "WindowBase::processDialogMessages(): WindowBase is NULL!");

  if (msg == WM_KEYDOWN && w_param == VK_TAB)
  {
    void *_rootHandle = lwbp->getRootWindowHandle();
    if (!lwbp->skipDialogProc())
    {
      MSG _message;
      POINT _pos;
      GetCursorPos(&_pos);

      _message.hwnd = (HWND)lwbp->getHandle();
      _message.message = msg;
      _message.wParam = w_param;
      _message.lParam = l_param;
      _message.time = GetTickCount();
      _message.pt = _pos;
      if (!IsDialogMessage((HWND)_rootHandle, &_message))
        return 0;
    }

    WindowBase *_lwroot = (WindowBase *)GetWindowLongPtr((HWND)_rootHandle, GWLP_USERDATA);

    if (_lwroot)
      _lwroot->onTabFocusChange();

    return 1;
  }

  return 0;
}


// we may want to divide scroll input by some SCROLL_WHEEL_SMOOTHNESS
// but we cant just do that because multiple small scrolls (e.g. trackpad)
// will not be able to scroll at all
int calc_smooth_scroll(short scroll_input)
{
  static int scrollAccum = 0;

  if (scroll_input * scrollAccum < 0)
    scrollAccum = 0;

  scrollAccum += scroll_input % SCROLL_WHEEL_SMOOTHNESS;
  const int totalScroll = scroll_input + scrollAccum;
  scrollAccum %= SCROLL_WHEEL_SMOOTHNESS;

  return totalScroll / SCROLL_WHEEL_SMOOTHNESS;
}


bool WindowBaseHandler::processUserMessages(WindowBase *lwbp, unsigned msg, WPARAM w_param, LPARAM l_param, LRESULT &res)
{
  G_ASSERT(lwbp && "WindowBase::processUserMessages(): WindowBase is NULL!");

  switch (msg)
  {
    case WM_COMMAND:
      // If an application processes this message, it should return zero
      {
        WORD wNotifyCode = HIWORD(w_param);
        WORD wID = LOWORD(w_param);
        HWND hwndCtl = (HWND)l_param;

        WindowBase *lWCp = lwbp;
        if (hwndCtl)
          lWCp = (WindowBase *)GetWindowLongPtr(hwndCtl, GWLP_USERDATA);

        if (lWCp && lWCp->onControlCommand(wNotifyCode, wID))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_NOTIFY:
      // If an application processes this message, it should return result
      {
        LPNMHDR info = (LPNMHDR)l_param;
        WindowBase *lWCp = (WindowBase *)GetWindowLongPtr(info->hwndFrom, GWLP_USERDATA);
        if (lWCp)
        {
          res = lWCp->onControlNotify((void *)info);
          if (res)
            return 1;
        }
      }
      break;

    case WM_DRAWITEM:
      // If an application processes this message, it should return TRUE
      {
        LPDRAWITEMSTRUCT info = (LPDRAWITEMSTRUCT)l_param;
        WindowBase *lWCp = (WindowBase *)GetWindowLongPtr(info->hwndItem, GWLP_USERDATA);

        if (lWCp)
        {
          res = lWCp->onControlDrawItem((void *)info);
          if (res)
            return 1;
        }
      }
      break;

    case WM_CTLCOLORBTN:
      // If an application processes this message, it must return a handle to a brush
      {
        HWND hwndCtl = (HWND)l_param;
        WindowBase *lWCp = (WindowBase *)GetWindowLongPtr(hwndCtl, GWLP_USERDATA);

        if (lWCp)
        {
          res = lWCp->onButtonDraw((HDC)w_param);
          if (res)
            return 1;
        }

        if (lwbp->getUserColorsFlag())
        {
          SetBkMode((HDC)w_param, TRANSPARENT);
          res = (LRESULT)wnd_brush;
          return 1;
        }
      }
      break;

    case WM_CTLCOLORSTATIC:
      // If an application processes this message, it must return a handle to a brush
      {
        res = lwbp->onButtonStaticDraw((HDC)w_param);
        if (res)
          return 1;

        if (lwbp->getUserColorsFlag())
        {
          SetBkMode((HDC)w_param, TRANSPARENT);
          res = (LRESULT)wnd_brush;
          return 1;
        }
      }
      break;

    case WM_CTLCOLOREDIT:
    {
      HWND hwndCtl = (HWND)l_param;
      WindowBase *lWCp = (WindowBase *)GetWindowLongPtr(hwndCtl, GWLP_USERDATA);

      if (lWCp && (res = lWCp->onDrawEdit((HDC)w_param)))
        return 1;
    }
    break;

    case WM_SIZE:
      // If an application processes this message, it should return zero
      {
        RECT wRect;
        GetWindowRect((HWND)lwbp->getHandle(), &wRect);

        if (lwbp->onResize(wRect.right - wRect.left, wRect.bottom - wRect.top))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_MOUSEMOVE:
      // If an application processes this message, it should return zero
      {
        if (w_param & MK_LBUTTON)
        {
          if (lwbp->onDrag(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
          {
            res = 0;
            return 1;
          }
        }
        else if (lwbp->onMouseMove(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      // If an application processes this message, it should return zero
      {
        HWND whdl = (HWND)lwbp->getParentHandle();
        if (whdl)
          PostMessage(whdl, msg, w_param, l_param);
      }
      break;

    case WM_RBUTTONUP:
      // If an application processes this message, it should return zero
      {
        if (lwbp->onRButtonUp(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_LBUTTONUP:
      // If an application processes this message, it should return zero
      {
        if (lwbp->onLButtonUp(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_LBUTTONDOWN:
      // If an application processes this message, it should return zero
      {
        if (GetFocus() != (HWND)lwbp->getHandle())
        {
          SetFocus((HWND)lwbp->getHandle());
        }

        if (lwbp->onLButtonDown(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_RBUTTONDOWN:
      // If an application processes this message, it should return zero
      {
        if (lwbp->onRButtonDown(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_LBUTTONDBLCLK:

    {
      if (lwbp->onLButtonDClick(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)))
      {
        res = 0;
        return 1;
      }
    }
    break;

    case WM_CHAR:
      if (lwbp->onChar((unsigned)w_param, (int)l_param))
      {
        res = 0;
        return 1;
      }
      break;

    case WM_KEYDOWN:
      // If an application processes this message, it should return zero
      {
        if (wingw::is_key_pressed(VK_CONTROL))
        {
          if ((w_param == 0x43 && lwbp->copyToClipboard()) || (w_param == 0x56 && lwbp->pasteFromClipboard()))
          {
            res = 0;
            return 1;
          }
        }

        if (lwbp->onKeyDown((unsigned)w_param, (int)l_param))
        {
          res = 0;
          return 1;
        }
      }
      break;

    case WM_MOUSEWHEEL:
      // If an application processes this message, it should return zero
      if (lwbp->scrollParentFirst)
      {
        if (GetFocus() == (HWND)lwbp->getHandle())
        {
          res = 0;
          return lwbp->onVScroll(calc_smooth_scroll((short)HIWORD(w_param)), true);
        }
      }
      else
      {
        if (GetFocus() == (HWND)lwbp->getHandle())
        {
          if (lwbp->onVScroll(calc_smooth_scroll((short)HIWORD(w_param)), true))
          {
            res = 0;
            return 1;
          }
        }
        break;
      }
      // we are not processing this message so propagate it to parent
      res = DefWindowProc((HWND)lwbp->getHandle(), msg, w_param, l_param);
      return 1;

    case WM_PAINT: lwbp->onPaint(); break;

    case WM_KILLFOCUS: lwbp->onKillFocus(); break;

    case WM_POST_MESSAGE: lwbp->onPostEvent(w_param); return 0;

    case WM_DESTROYCLIPBOARD:
      if (gclipbuf_handle)
      {
        GlobalFree(gclipbuf_handle);
        gclipbuf_handle = NULL;
        res = 0;
        return 1;
      }
      break;

    case WM_CAPTURECHANGED:
      if (lwbp->onCaptureChanged((void *)l_param))
      {
        res = 0;
        return 1;
      }
      break;
  }

  return 0;
}


LRESULT CALLBACK WindowBaseHandler::controlProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  WindowBase *lWBp = (WindowBase *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  if (lWBp)
  {
    // dialog navigation processing

    if (processDialogMessages(lWBp, message, wparam, lparam))
      return 0;

    // user messages processing

    LRESULT res = 0;
    if (processUserMessages(lWBp, message, wparam, lparam, res))
      return res;

    return CallWindowProc((WNDPROC)lWBp->mWndProc, hwnd, message, wparam, lparam);
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}


LRESULT CALLBACK WindowBaseHandler::containerProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  WindowBase *lWBp = (WindowBase *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

  if (lWBp)
  {
    LRESULT res = 0;
    if (processUserMessages(lWBp, message, wparam, lparam, res))
      return res;
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}


LRESULT CALLBACK WindowBaseHandler::windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  if (message == WM_NCCREATE)
  {
    CREATESTRUCT *cs = (CREATESTRUCT *)lparam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);

    return DefWindowProc(hwnd, message, wparam, lparam);
  }

  // searching window

  WindowBase *lWBp = (WindowBase *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!lWBp)
    return DefWindowProc(hwnd, message, wparam, lparam);

  G_ASSERT(lWBp && "WindowBase::windowProc(): WindowBase is NULL!");

  if (!lWBp->getHandle())
  {
    lWBp->mHandle = hwnd;
  }

  // system messages

  switch (message)
  {
    case WM_SIZE:
    {
      RECT wRect;
      GetWindowRect(hwnd, &wRect);
      lWBp->onResize(wRect.right - wRect.left, wRect.bottom - wRect.top);
    }
      return 0;

    case WM_NOTIFY:
    {
      // parent receives notify control messages and call control event handler
      LPNMHDR info = (LPNMHDR)lparam;
      WindowBase *lWCp = (WindowBase *)GetWindowLongPtr(info->hwndFrom, GWLP_USERDATA);
      if (lWCp)
        return lWCp->onControlNotify((void *)info);
    }
      return 0;

    case WM_MOUSEWHEEL: lWBp->onVScroll(calc_smooth_scroll((short)HIWORD(wparam)), true); return 0;

    case WM_VSCROLL:
    {
      int currentVScrollPos = -GetScrollPos(hwnd, SB_VERT);
      int newVScrollPos = currentVScrollPos;
      switch (LOWORD(wparam))
      {
        // User clicked the scroll bar shaft above the scroll box.
        case SB_PAGEUP: newVScrollPos = currentVScrollPos + 50; break;

        // User clicked the scroll bar shaft below the scroll box.
        case SB_PAGEDOWN: newVScrollPos = currentVScrollPos - 50; break;

        // User clicked the top arrow.
        case SB_LINEUP: newVScrollPos = currentVScrollPos + 5; break;

        // User clicked the bottom arrow.
        case SB_LINEDOWN: newVScrollPos = currentVScrollPos - 5; break;

        // User dragged the scroll box.
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK: newVScrollPos = -HIWORD(wparam); break;
      }
      lWBp->onVScroll(newVScrollPos - currentVScrollPos, false);
      return 0;
    }

    case WM_MBUTTONDOWN:
    {
      POINT mouse_pos;
      GetCursorPos(&mouse_pos);
      lWBp->mScrollVPos = mouse_pos.y;
      lWBp->mScrollHPos = mouse_pos.x;

      SetCapture((HWND)lWBp->getHandle());
    }
      return 0;

    case WM_MBUTTONUP: ReleaseCapture(); return 0;

    case WM_MOUSEMOVE:
    {
      POINT mouse_pos;
      GetCursorPos(&mouse_pos);

      if (wparam & MK_MBUTTON)
      {
        int dy = mouse_pos.y - lWBp->mScrollVPos;
        int dx = mouse_pos.x - lWBp->mScrollHPos;
        lWBp->onVScroll(dy, false);
        lWBp->onHScroll(dx);

        // ------

        bool changes = false;

        if (mouse_pos.x == GetSystemMetrics(SM_CXSCREEN) - 1)
        {
          mouse_pos.x -= _pxS(SCROLL_MOUSE_JUMP);
          changes = true;
        }

        if (mouse_pos.x == 0)
        {
          mouse_pos.x += _pxS(SCROLL_MOUSE_JUMP);
          changes = true;
        }

        if (mouse_pos.y == GetSystemMetrics(SM_CYSCREEN) - 1)
        {
          mouse_pos.y -= _pxS(SCROLL_MOUSE_JUMP);
          changes = true;
        }

        if (mouse_pos.y == 0)
        {
          mouse_pos.y += _pxS(SCROLL_MOUSE_JUMP);
          changes = true;
        }

        if (changes)
          SetCursorPos(mouse_pos.x, mouse_pos.y);

        // ------

        lWBp->mScrollVPos = mouse_pos.y;
        lWBp->mScrollHPos = mouse_pos.x;
      }
    }
      return 0;

    case WM_CLOSE:
      SetFocus(hwnd);
      if (lWBp->onClose())
        DestroyWindow(hwnd);
      return 0;

    case WM_DESTROY:
      // PostQuitMessage(0);
      // debug("WM_DESTROY");
      break;

    case WM_POST_MESSAGE: lWBp->onPostEvent(wparam); return 0;

    case WM_DROPFILES:
    {
      HDROP hdrop = (HDROP)wparam;
      const int fileCount = DragQueryFileA(hdrop, -1, nullptr, 0);
      if (fileCount > 0)
      {
        dag::Vector<String> files;
        files.set_capacity(fileCount);

        for (int i = 0; i < fileCount; ++i)
        {
          const int lengthWithoutNullTerminator = DragQueryFileA(hdrop, 0, nullptr, 0);
          if (lengthWithoutNullTerminator > 0)
          {
            String path;
            path.resize(lengthWithoutNullTerminator + 1);
            if (DragQueryFileA(hdrop, 0, path.begin(), lengthWithoutNullTerminator + 1))
              files.emplace_back(path);
          }
        }

        if (lWBp->onDropFiles(files))
          return 0;
      }
    }
    break;
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}


///////////////////////////////////////////////////////////////////////////////


void *WindowBase::getHandle() const { return mHandle; }

void *WindowBase::getParentHandle() const
{
  if (mParent)
    return mParent->getHandle();

  return GetParent((HWND)this->getHandle());
}

TInstHandle WindowBase::getModule() const { return mHInstance; }

unsigned WindowBase::getID() const { return mID; }

unsigned WindowBase::getNextChildID() { return ++mIDCounter; }


static bool wnd_classes_initialized = false;
static void init_window_classes()
{
  if (wnd_classes_initialized)
    return;

  // init common controls

  INITCOMMONCONTROLSEX comCtrl;
  comCtrl.dwSize = sizeof(INITCOMMONCONTROLSEX);
  comCtrl.dwICC = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS | ICC_TAB_CLASSES;
  if (!InitCommonControlsEx(&comCtrl))
    G_ASSERT(false && "pp2: Common controls not initialized!");

  // init COM

  if (CoInitialize(NULL) != S_OK)
    G_ASSERT(false && "pp2: COM not initialized!");

  // register property window class

  WNDCLASSEX propWndCls = {0};

  propWndCls.cbSize = sizeof(WNDCLASSEX);
  propWndCls.hCursor = LoadCursor(NULL, IDC_ARROW);
  propWndCls.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  propWndCls.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
  propWndCls.lpfnWndProc = (WNDPROC)WindowBaseHandler::windowProc;
  propWndCls.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
  propWndCls.hInstance = (HMODULE)GetModuleHandle(NULL);
  propWndCls.lpszClassName = TEXT(PROPERTY_WINDOW_CLASS_NAME);
  propWndCls.style = 0; // CS_HREDRAW | CS_VREDRAW;
  propWndCls.cbClsExtra = 0;
  propWndCls.cbWndExtra = 0;
  propWndCls.lpszMenuName = NULL;

  if (!RegisterClassEx(&propWndCls))
    G_ASSERT(false && "pp2: Window Class not registered!");

  // user colors

  propWndCls.hbrBackground = (HBRUSH)wnd_brush;
  propWndCls.lpszClassName = TEXT(USER_PROPERTY_WINDOW_CLASS_NAME);
  if (!RegisterClassEx(&propWndCls))
    G_ASSERT(false && "pp2: Window Class not registered!");

  // register container window class

  WNDCLASSEX contWndCls = {0};

  contWndCls.cbSize = sizeof(WNDCLASSEX);
  contWndCls.hCursor = LoadCursor(NULL, IDC_ARROW);
  contWndCls.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  contWndCls.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
  contWndCls.lpfnWndProc = (WNDPROC)WindowBaseHandler::containerProc;
  contWndCls.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
  contWndCls.hInstance = (HMODULE)GetModuleHandle(NULL);
  contWndCls.lpszClassName = TEXT(CONTAINER_WINDOW_CLASS_NAME);
  contWndCls.style = 0;
  contWndCls.cbClsExtra = 0;
  contWndCls.cbWndExtra = 0;
  contWndCls.lpszMenuName = NULL;

  if (!RegisterClassEx(&contWndCls))
    G_ASSERT(false && "pp2: Window Class not registered!");

  // user colors

  contWndCls.hbrBackground = (HBRUSH)wnd_brush;
  contWndCls.lpszClassName = TEXT(USER_CONTAINER_WINDOW_CLASS_NAME);
  if (!RegisterClassEx(&contWndCls))
    G_ASSERT(false && "pp2: Window Class not registered!");

  wnd_classes_initialized = true;
}


WindowBase::WindowBase(WindowBase *parent, const char class_name[], unsigned long ex_style, unsigned long style, const char caption[],
  int x, int y, int w, int h) :

  mWidth(w),
  mHeight(h),
  mX(x),
  mY(y),
  mScrollVPos(0),
  mScrollHPos(0),
  mHandle(0),
  mID(0),
  mRootWindowHandle(0),
  mUserGuiColor(false),
  noDialogProc(false)
{
  scrollParentFirst = true;
  mParent = parent;
  HWND lParentHWnd = 0;
  mIDCounter = 0; // for children

  ::init_window_classes();

  if (!parent)
  {
    mHInstance = GetModuleHandle(NULL);
    G_ASSERT(mHInstance && "pp2: Can't get handle of the module!");

    lParentHWnd = (HWND)p2util::get_main_parent_handle();
    G_ASSERT(lParentHWnd && "pp2: Parent handle = NULL! Use p2util::set_main_parent_handle()");
    mID = 0;
  }
  else
  {
    mHInstance = parent->getModule();
    lParentHWnd = (HWND)parent->getHandle();
    mID = (strcmp(class_name, TOOLTIPS_CLASS) != 0) ? parent->getNextChildID() : 0;
    mUserGuiColor = parent->getUserColorsFlag();
  }

  // create window

  HWND lHWnd =
    CreateWindowEx(ex_style, class_name, caption, style, x, y, w, h, lParentHWnd, (HMENU)(uintptr_t)mID, (HMODULE)mHInstance, this);

  G_ASSERT(lHWnd && "pp2: Window not created!");

  mHandle = lHWnd;
  SetWindowLongPtr(lHWnd, GWLP_USERDATA, (LONG_PTR)this);
  mWndProc = 0;

  if ((parent) && (strcmp(class_name, CONTAINER_WINDOW_CLASS_NAME) && strcmp(class_name, USER_CONTAINER_WINDOW_CLASS_NAME)))
  {
    mWndProc = GetWindowLongPtr(lHWnd, GWLP_WNDPROC);
    SetWindowLongPtr(lHWnd, GWLP_WNDPROC, (LONG_PTR)&WindowBaseHandler::controlProc);
    // ShowWindow(lHWnd, SW_SHOWDEFAULT);
  }

  mRootWindowHandle = (parent) ? parent->getRootWindowHandle() : mHandle;

  UpdateWindow(lHWnd);

  if (strcmp(class_name, TOOLTIPS_CLASS))
  {
    setFont(this->getNormalFont());
  }

  if (!strcmp(class_name, "EDIT") && (style & ES_MULTILINE))
  {
    noDialogProc = true;
  }
}


WindowBase::~WindowBase() { DestroyWindow((HWND)mHandle); }

// gui methods

void WindowBase::show() { ShowWindow((HWND)mHandle, SW_SHOW); }


void WindowBase::hide() { ShowWindow((HWND)mHandle, SW_HIDE); }


void WindowBase::setEnabled(bool enabled) { EnableWindow((HWND)mHandle, enabled); }

void WindowBase::setRedraw(bool redraw) { SendMessage((HWND)mHandle, WM_SETREDRAW, redraw, 0); }

void WindowBase::refresh(bool all) { InvalidateRect(HWND(mHandle), NULL, all); }

void WindowBase::resizeWindow(unsigned w, unsigned h)
{
  mWidth = w;
  mHeight = h;

  SetWindowPos((HWND)mHandle, 0, mX, mY, mWidth, mHeight, SWP_NOMOVE + SWP_NOZORDER);
}

void WindowBase::moveWindow(int x, int y)
{
  mX = x;
  mY = y;

  SetWindowPos((HWND)mHandle, 0, mX, mY, mWidth, mHeight, SWP_NOZORDER + SWP_NOSIZE);
}

void WindowBase::updateWindow() { UpdateWindow((HWND)mHandle); }

void WindowBase::getClientSize(int &cw, int &ch)
{
  RECT rc;
  GetClientRect((HWND)mHandle, &rc);
  cw = rc.right - rc.left;
  ch = rc.bottom - rc.top;
}

void WindowBase::setZAfter(void *handle)
{
  SetWindowPos((HWND)mHandle, (HWND)handle, mX, mY, mWidth, mHeight, SWP_NOMOVE + SWP_NOSIZE);
}

void WindowBase::setFont(TFontHandle font) { SendMessage((HWND)mHandle, WM_SETFONT, (WPARAM)font, MAKELPARAM(true, 0)); }

bool WindowBase::isActive() { return GetFocus() == (HWND)mHandle; }

void WindowBase::setFocus() { SetFocus((HWND)mHandle); }

void WindowBase::setDragAcceptFiles(bool accept) { DragAcceptFiles((HWND)mHandle, accept); }

void WindowBase::setTextValue(const char value[]) { SetWindowText((HWND)mHandle, value); }

int WindowBase::getTextValue(char *buffer, int buflen) const { return GetWindowText((HWND)mHandle, buffer, buflen); }


TFontHandle WindowBase::mFNH = 0;
TFontHandle WindowBase::mFBH = 0;
TFontHandle WindowBase::mFSPH = 0;
TFontHandle WindowBase::mFCH = 0;
TFontHandle WindowBase::mFSBH = 0;
TFontHandle WindowBase::mFSPlH = 0;


TFontHandle WindowBase::getNormalFont()
{
  if (!mFNH)
  {
    mFNH = (TFontHandle)CreateFont(-_pxS(FONT_HT), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY, 0,
      "Tahoma");
  }

  return mFNH;
}

TFontHandle WindowBase::getBoldFont()
{
  if (!mFBH)
  {
    mFBH = (TFontHandle)CreateFont(-_pxS(FONT_HT), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY, 0,
      "Tahoma");
  }

  return mFBH;
}

TFontHandle WindowBase::getSmallPrettyFont()
{
  if (!mFSPH)
  {
    mFSPH = (TFontHandle)CreateFont(-_pxS(FONT_HT - 3), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
      0, "Tahoma");
  }

  return mFSPH;
}

TFontHandle WindowBase::getComboFont()
{
  if (!mFCH)
  {
    mFCH = (TFontHandle)CreateFont(-_pxS(FONT_HT - 2), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
      0, "Tahoma");
  }

  return mFCH;
}


TFontHandle WindowBase::getSmallButtonFont()
{
  if (!mFSBH)
  {
    mFSBH = (TFontHandle)CreateFont(-_pxS(FONT_HT - 6), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
      0, "System");
  }

  return mFSBH;
}


TFontHandle WindowBase::getSmallPlotFont()
{
  if (!mFSPlH)
  {
    mFSPlH = (TFontHandle)CreateFont(-_pxS(FONT_HT - 8), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY,
      0, "Terminal");
  }

  return mFSPlH;
}

// clipboard routines

long WindowBase::copyToClipboard()
{
  DataBlock blk;
  if (!onClipboardCopy(blk))
    return 0;

  if (!OpenClipboard((HWND)getHandle()))
    return 0;
  EmptyClipboard();

  SimpleString str_buffer;
  str_buffer = blk_util::blkTextData(blk);
  if (str_buffer.empty())
    return 0;

  int len = str_buffer.length();
  if (gclipbuf_handle)
    GlobalFree(gclipbuf_handle);
  gclipbuf_handle = GlobalAlloc(GMEM_MOVEABLE, len + 1);
  if (gclipbuf_handle == NULL)
  {
    CloseClipboard();
    return 0;
  }

  LPTSTR lptstrCopy = (LPTSTR)GlobalLock(gclipbuf_handle);
  memcpy(lptstrCopy, str_buffer.str(), len);
  lptstrCopy[len] = (TCHAR)0;
  GlobalUnlock(gclipbuf_handle);
  SetClipboardData(CF_TEXT, gclipbuf_handle);
  CloseClipboard();
  return 1;
}


long WindowBase::pasteFromClipboard()
{
  if (!IsClipboardFormatAvailable(CF_TEXT))
    return 0;
  if (!OpenClipboard((HWND)getHandle()))
    return 0;

  HGLOBAL hglb = GetClipboardData(CF_TEXT);
  DataBlock blk;
  if (hglb != NULL)
  {
    LPSTR lptstr = (LPSTR)GlobalLock(hglb);
    if (lptstr != NULL)
    {
      dblk::load_text(blk, make_span(lptstr, i_strlen(lptstr)), dblk::ReadFlag::ROBUST | dblk::ReadFlag::RESTORE_FLAGS);
      GlobalUnlock(hglb);
    }
  }
  CloseClipboard();
  if (blk.isValid())
    return onClipboardPaste(blk);
  return 0;
}

// event routines

intptr_t WindowBase::onControlNotify(void *info) { return 0; }

intptr_t WindowBase::onControlDrawItem(void *info) { return 0; }

intptr_t WindowBase::onControlCommand(unsigned notify_code, unsigned elem_id) { return 0; }

intptr_t WindowBase::onResize(unsigned new_width, unsigned new_height)
{
  mWidth = new_width;
  mHeight = new_height;

  return 0;
}


intptr_t WindowBase::onDrag(int new_x, int new_y) { return 0; }

intptr_t WindowBase::onMouseMove(int x, int y) { return 0; }

intptr_t WindowBase::onRButtonUp(long x, long y) { return 0; }

intptr_t WindowBase::onLButtonUp(long x, long y) { return 0; }

intptr_t WindowBase::onRButtonDown(long x, long y) { return 0; }

intptr_t WindowBase::onLButtonDown(long x, long y) { return 0; }

intptr_t WindowBase::onLButtonDClick(long x, long y) { return 0; }

intptr_t WindowBase::onButtonDraw(void *hdc) { return 0; }

intptr_t WindowBase::onButtonStaticDraw(void *hdc) { return 0; }

intptr_t WindowBase::onDrawEdit(void *hdc) { return 0; }


intptr_t WindowBase::onKeyDown(unsigned v_key, int flags) { return 0; }

intptr_t WindowBase::onChar(unsigned code, int flags) { return 0; }

intptr_t WindowBase::onVScroll(int dy, bool is_wheel) { return 0; }

intptr_t WindowBase::onHScroll(int dx) { return 0; }

intptr_t WindowBase::onCaptureChanged(void *window_gaining_capture) { return 0; }

bool WindowBase::onClose() { return true; }

intptr_t WindowBase::onClipboardCopy(DataBlock &blk) { return 0; }

intptr_t WindowBase::onClipboardPaste(const DataBlock &blk) { return 0; }

intptr_t WindowBase::onDropFiles(const dag::Vector<String> &files) { return 0; }