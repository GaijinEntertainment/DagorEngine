// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "wndDragPlacesShower.h"
#include "wndBase.h"
#include "wndManager.h"
#include "windows/wndClientWindow.h"

#include <windows.h>
#include <tchar.h>


extern const char *WNDCLASS_DRAG_PLACE;
extern const char *WNDCLASS_DRAG_ROOT_PLACE;

//=============================================================================
DragPlacesShower::DragPlacesShower(WinManager *owner) : mOwner(owner), mCurWindow(NULL), mAlign(WA_NONE), mMoving(false)
{
  mLeft = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, WNDCLASS_DRAG_ROOT_PLACE, NULL, WS_POPUP, 0, 0, 0, 0,
    (HWND)mOwner->getMainWindow(), NULL, NULL, NULL);

  mRight = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, WNDCLASS_DRAG_ROOT_PLACE, NULL, WS_POPUP, 0, 0, 0, 0,
    (HWND)mOwner->getMainWindow(), NULL, NULL, NULL);
  mTop = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, WNDCLASS_DRAG_ROOT_PLACE, NULL, WS_POPUP, 0, 0, 0, 0,
    (HWND)mOwner->getMainWindow(), NULL, NULL, NULL);
  mBottom = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, WNDCLASS_DRAG_ROOT_PLACE, NULL, WS_POPUP, 0, 0, 0, 0,
    (HWND)mOwner->getMainWindow(), NULL, NULL, NULL);

  mDragPlaceWindowHwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, WNDCLASS_DRAG_PLACE, NULL, WS_POPUP, 0, 0, 0, 0,
    (HWND)mOwner->getMainWindow(), NULL, NULL, NULL);
  // --- setting alpha ---

  setDefaultAlpha();
}


//=============================================================================
void DragPlacesShower::setDefaultAlpha()
{
  SetLayeredWindowAttributes((HWND)mLeft, 0, ALIGN_ROOT_WINDOWS_DEFAULT_ALPHA, LWA_ALPHA);
  SetLayeredWindowAttributes((HWND)mRight, 0, ALIGN_ROOT_WINDOWS_DEFAULT_ALPHA, LWA_ALPHA);
  SetLayeredWindowAttributes((HWND)mTop, 0, ALIGN_ROOT_WINDOWS_DEFAULT_ALPHA, LWA_ALPHA);
  SetLayeredWindowAttributes((HWND)mBottom, 0, ALIGN_ROOT_WINDOWS_DEFAULT_ALPHA, LWA_ALPHA);

  SetLayeredWindowAttributes((HWND)mDragPlaceWindowHwnd, 0, ALIGN_WINDOWS_ALPHA, LWA_ALPHA);
}


//=============================================================================
bool DragPlacesShower::isInside(void *h_wnd, const IPoint2 &mouse_pos)
{
  RECT rect;
  GetWindowRect((HWND)h_wnd, &rect);

  if (mouse_pos.x >= rect.left && mouse_pos.x <= rect.right && mouse_pos.y >= rect.top && mouse_pos.y <= rect.bottom)
  {
    mCurAlignWindow = h_wnd;
    SetLayeredWindowAttributes((HWND)mCurAlignWindow, 0, ALIGN_ROOT_WINDOWS_FOCUS_ALPHA, LWA_ALPHA);
    return true;
  }
  return false;
}


//=============================================================================
WindowAlign DragPlacesShower::getRootAlign(const IPoint2 &mouse_pos)
{
  if (isInside(mLeft, mouse_pos))
    return WA_LEFT;
  if (isInside(mRight, mouse_pos))
    return WA_RIGHT;
  if (isInside(mTop, mouse_pos))
    return WA_TOP;
  if (isInside(mBottom, mouse_pos))
    return WA_BOTTOM;

  if (mCurAlignWindow != NULL)
  {
    mCurAlignWindow = NULL;
    setDefaultAlpha();
  }

  return WA_NONE;
}


//=============================================================================
void DragPlacesShower::ReDrawRootPlaces(bool show)
{
  if (!show)
  {
    ShowWindow((HWND)mLeft, SW_HIDE);
    ShowWindow((HWND)mRight, SW_HIDE);
    ShowWindow((HWND)mTop, SW_HIDE);
    ShowWindow((HWND)mBottom, SW_HIDE);

    return;
  }

  int size = WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE_TO_ROOT; // to long constant name =)

  IPoint2 leftTop, rightBottom;
  mOwner->getRoot()->getPos(leftTop, rightBottom);

  client_to_screen(mOwner->getMainWindow(), leftTop);
  client_to_screen(mOwner->getMainWindow(), rightBottom);

  int xCenter = leftTop.x + (rightBottom.x - leftTop.x) / 2 - size / 2;
  int xRight = rightBottom.x - size;
  int yCenter = leftTop.y + (rightBottom.y - leftTop.y) / 2 - size / 2;
  int yBottom = rightBottom.y - size;

  SetWindowPos((HWND)mLeft, 0, leftTop.x, yCenter, size, size, SWP_NOOWNERZORDER);
  ShowWindow((HWND)mLeft, SW_SHOWNOACTIVATE);
  SetWindowPos((HWND)mRight, 0, xRight, yCenter, size, size, SWP_NOOWNERZORDER);
  ShowWindow((HWND)mRight, SW_SHOWNOACTIVATE);
  SetWindowPos((HWND)mTop, 0, xCenter, leftTop.y, size, size, SWP_NOOWNERZORDER);
  ShowWindow((HWND)mTop, SW_SHOWNOACTIVATE);
  SetWindowPos((HWND)mBottom, 0, xCenter, yBottom, size, size, SWP_NOOWNERZORDER);
  ShowWindow((HWND)mBottom, SW_SHOWNOACTIVATE);
}


//=============================================================================
CascadeWindow *DragPlacesShower::getWindowAlign(const IPoint2 &mouse_pos, WindowAlign &align)
{
  align = getRootAlign(mouse_pos);
  if (align != WA_NONE)
    return mOwner->getRoot();

  IPoint2 mousePos(mouse_pos);
  screen_to_client(mOwner->getMainWindow(), mousePos); // pos in main window
  ClientWindow *atPosWindow = mOwner->getRoot()->getWindowOnPos(mousePos);

  if (!atPosWindow) // miss window
    return NULL;

  IPoint2 leftTop, rightBottom;
  atPosWindow->getPos(leftTop, rightBottom);
  int w = rightBottom.x - leftTop.x;
  int h = rightBottom.y - leftTop.y;
  if (w < WINDOW_MIN_SIZE_W * 2 || h < WINDOW_MIN_SIZE_H * 2) // window too small
    return NULL;

  if (mousePos.x < leftTop.x + WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE &&           // left border
        (mousePos.y < leftTop.y + WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE ||        // left-top corner
          mousePos.y > rightBottom.y - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE)     // left-bottom corner
      || mousePos.x > rightBottom.x - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE &&    // right border
           (mousePos.y < leftTop.y + WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE ||     // right-top corner
             mousePos.y > rightBottom.y - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE)) // right-bottom corner
    return NULL;

  if (mousePos.x < leftTop.x + WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE)
    align = WA_LEFT;
  else if (mousePos.x > rightBottom.x - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE)
    align = WA_RIGHT;
  else if (mousePos.y < leftTop.y + WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE)
    align = WA_TOP;
  else if (mousePos.y > rightBottom.y - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE)
    align = WA_BOTTOM;
  else
    return NULL;

  return atPosWindow;
}


//=============================================================================
void DragPlacesShower::drawAlignWindows(CascadeWindow *window, WindowAlign align, void *moving_handle)
{
  bool rootClient = (bool)mOwner->getRoot()->getClientWindow();
  if (!rootClient && (mCurWindow == NULL && window == mOwner->getRoot() || mCurWindow == mOwner->getRoot() && window == NULL))
    return;

  if (!window || align == WA_NONE || (window == mOwner->getRoot() && !rootClient))
  {
    ShowWindow((HWND)mDragPlaceWindowHwnd, SW_HIDE);
    if (!rootClient)
      ReDrawRootPlaces(true);
  }
  else
  {
    ReDrawRootPlaces(false);

    IPoint2 leftTop, rightBottom;
    window->getPos(leftTop, rightBottom);

    int w = rightBottom.x - leftTop.x;
    int h = rightBottom.y - leftTop.y;

    int x, y, w1, h1;
    if (align == WA_LEFT || align == WA_RIGHT)
    {
      w1 = WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE;
      h1 = h;
      y = 0;
      x = (align == WA_LEFT) ? 0 : w - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE;
    }

    if (align == WA_TOP || align == WA_BOTTOM)
    {
      w1 = w;
      h1 = WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE;
      x = 0;
      y = (align == WA_TOP) ? 0 : h - WINDOW_AREA_SIZE_TO_MAKE_UNMOVABLE;
    }

    // offset for current child window
    x += leftTop.x;
    y += leftTop.y;

    POINT pos;
    pos.x = x;
    pos.y = y;
    ClientToScreen((HWND)mOwner->getMainWindow(), &pos);

    SetWindowPos((HWND)mDragPlaceWindowHwnd, 0, pos.x, pos.y, w1, h1, SWP_NOOWNERZORDER);
    ShowWindow((HWND)mDragPlaceWindowHwnd, SW_SHOWNOACTIVATE);
  }
  SetFocus((HWND)moving_handle);
}


//=============================================================================
void DragPlacesShower::Update(const IPoint2 &mouse_pos, void *moving_handle)
{
  WindowAlign align;
  CascadeWindow *window = getWindowAlign(mouse_pos, align);

  if (window != mCurWindow || align != mAlign || !mMoving)
  {
    if (window == NULL)
      mAlign = WA_NONE;
    else
      mAlign = align;

    drawAlignWindows(window, mAlign, moving_handle);
    mCurWindow = window;
    mMoving = true;
  }
}


//=============================================================================
void DragPlacesShower::Stop()
{
  ShowWindow((HWND)mDragPlaceWindowHwnd, SW_HIDE);
  ReDrawRootPlaces(false);
  mMoving = false;
}
