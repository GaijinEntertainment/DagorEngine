// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_window.h>
#include <EditorCore/ec_rect.h>
#include <windows.h>


const char *WIN_CLASS_NAME = "EC_WINDOW_CLASS_NAME";


LRESULT __stdcall window_proc(HWND h_wnd, unsigned msg, WPARAM w_param, LPARAM l_param)
{
  if (msg == WM_NCCREATE)
  {
    CREATESTRUCT *cs = (CREATESTRUCT *)l_param;
    SetWindowLongPtr(h_wnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);

    return DefWindowProc(h_wnd, msg, w_param, l_param);
  }

  EcWindow *w = (EcWindow *)GetWindowLongPtr(h_wnd, GWLP_USERDATA);
  if (w)
    return w->windowProc((TEcHandle)h_wnd, msg, (TEcWParam)w_param, (TEcLParam)l_param);

  return DefWindowProc(h_wnd, msg, w_param, l_param);
}


static ATOM window_class_id = 0;
void init_window_class()
{
  if (::window_class_id)
    return;

  WNDCLASSEX wndc = {0};

  wndc.cbSize = sizeof(WNDCLASSEX);
  wndc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndc.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  wndc.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
  wndc.lpfnWndProc = (WNDPROC)::window_proc;
  wndc.hbrBackground = CreateSolidBrush(RGB(0, 0, 50));
  wndc.hInstance = (HMODULE)0;
  wndc.lpszClassName = TEXT(WIN_CLASS_NAME);

  ::window_class_id = RegisterClassEx(&wndc);
}


///////////////////////////////////////////////////////////////////////////////


EcWindow::EcWindow(TEcHandle parent, int x, int y, int w, int h)
{
  unsigned ex_style = 0;
  unsigned style = WS_CHILD | WS_VISIBLE;
  const char *caption = "";

  ::init_window_class();

  mHWnd = (TEcHandle)CreateWindowEx(ex_style, WIN_CLASS_NAME, caption, style, x, y, w, h, (HWND)parent, 0, 0, this);
}


EcWindow::~EcWindow()
{
  if (mHWnd)
  {
    DestroyWindow((HWND)mHWnd);
    mHWnd = 0;
  }
}


int EcWindow::windowProc(TEcHandle h_wnd, unsigned msg, TEcWParam w_param, TEcLParam l_param)
{
  return DefWindowProc((HWND)h_wnd, msg, (WPARAM)w_param, (LPARAM)l_param);
}


TEcHandle EcWindow::getParentHandle() const { return (TEcHandle)GetParent((HWND)mHWnd); }


int EcWindow::getW() const
{
  RECT rect;
  GetClientRect((HWND)mHWnd, &rect);

  return rect.right - rect.left;
}


int EcWindow::getH() const
{
  RECT rect;
  GetClientRect((HWND)mHWnd, &rect);

  return rect.bottom - rect.top;
}


void EcWindow::getClientRect(EcRect &clientRect) const
{
  RECT rect;
  GetClientRect((HWND)mHWnd, &rect);

  clientRect.l = rect.left;
  clientRect.t = rect.top;
  clientRect.r = rect.right;
  clientRect.b = rect.bottom;
}


void EcWindow::moveWindow(int left, int top, bool repaint)
{
  SetWindowPos((HWND)mHWnd, 0, left, top, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}


void EcWindow::resizeWindow(int w, int h, bool repaint)
{
  SetWindowPos((HWND)mHWnd, 0, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}


void EcWindow::moveResizeWindow(int left, int top, int w, int h, bool repaint)
{
  SetWindowPos((HWND)mHWnd, 0, left, top, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}


void EcWindow::activate(bool /*repaint*/) { SetFocus((HWND)mHWnd); }


bool EcWindow::isActive() const { return GetFocus() == (HWND)mHWnd; }


void EcWindow::translateToScreen(int &x, int &y)
{
  POINT point = {x, y};
  ClientToScreen((HWND)mHWnd, &point);

  x = point.x;
  y = point.y;
}


void EcWindow::translateToClient(int &x, int &y)
{
  POINT point = {x, y};
  ScreenToClient((HWND)mHWnd, &point);

  x = point.x;
  y = point.y;
}


void EcWindow::invalidateClientRect() { InvalidateRect((HWND)mHWnd, NULL, true); }


void EcWindow::captureMouse() { SetCapture((HWND)mHWnd); }


void EcWindow::releaseMouse() { ReleaseCapture(); }
