// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../wndManager.h"
#include "../wndMenu.h"
#include "wndMovableWindow.h"
#include "wndClientWindow.h"
#include "wndVirtualWindow.h"

#include <windows.h>


enum
{
  ID_UNMOVABLE = 1,
};


//=============================================================================
MovableWindow::MovableWindow(WinManager *win_manager, ClientWindow *client_window) :
  mClientWindow(client_window),
  mOwner(win_manager),
  mDragMovable(false),
  mSizeLock(WSL_NONE),
  mLockW(0),
  mLockH(0),
  mMenuActions(NULL),
  mOriginalPosInfo(NULL),
  mWasMaximized(false)
{}


//=============================================================================
MovableWindow::~MovableWindow()
{
  del_it(mMenuActions);

  del_it(mClientWindow);
  del_it(mOriginalPosInfo);
}


//=============================================================================
intptr_t MovableWindow::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  switch (msg)
  {
    case WM_CREATE:
    {
      mMenuActions = new Menu(h_wnd, ROOT_MENU_ITEM, this);
      mMenuActions->addItem(ROOT_MENU_ITEM, ID_UNMOVABLE, "Make unmovable");
      return 0;
    }

    case WM_CLOSE:
    {
      G_ASSERT(mClientWindow && "MovableWindow WM_CLOSE: ClientWindow was not set!");
      mOwner->deleteWindow(mClientWindow);
      return 0;
    }

    case WM_MOVING:
    {
      POINT mouse_pos;
      GetCursorPos(&mouse_pos);

      mDragMovable = true;
      mOwner->UpdatePlacesShower(IPoint2(mouse_pos.x, mouse_pos.y), h_wnd);
      break;
    }

    case WM_SIZE:
    {
      mLeftTop = IPoint2(0, 0);
      mRightBottom = IPoint2((int)LOWORD(l_param), (int)HIWORD(l_param));

      if (((intptr_t)w_param == SIZE_MAXIMIZED) || (((intptr_t)w_param == SIZE_RESTORED) && (mWasMaximized)))
      {
        mWasMaximized = ((intptr_t)w_param == SIZE_MAXIMIZED);
        mClientWindow->resize(mLeftTop, mRightBottom);
      }

      break;
    }

    case WM_CAPTURECHANGED:
    {
      mOwner->StopPlacesShower();

      if (mDragMovable)
      {
        mDragMovable = false;

        POINT mouse_pos;
        GetCursorPos(&mouse_pos);

        IPoint2 mousePos(mouse_pos.x, mouse_pos.y);
        mOwner->setUnMovable(mClientWindow, &mousePos);
        return 0;
      }
      else
        mClientWindow->resize(mLeftTop, mRightBottom);

      break;
    }

    case WM_GETMINMAXINFO:
    {
      MINMAXINFO *pInfo = (MINMAXINFO *)l_param;

      if (mSizeLock & WSL_WIDTH)
      {
        pInfo->ptMinTrackSize.x = mLockW;
        pInfo->ptMaxTrackSize.x = mLockW;
      }

      if (mSizeLock & WSL_HEIGHT)
      {
        pInfo->ptMinTrackSize.y = mLockH;
        pInfo->ptMaxTrackSize.y = mLockH;
      }

      return 0;
    }
  }

  if ((mMenuActions) && (!mMenuActions->checkMenu(msg, w_param)))
    return 0;

  return BaseWindow::winProc(h_wnd, msg, w_param, l_param);
}


//=============================================================================
int MovableWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ID_UNMOVABLE:

      if (mOriginalPosInfo)
      {
        MovableOriginalPos &opi = *mOriginalPosInfo;
        CascadeWindow *where = mOwner->getRoot();

        if (opi.mGrandParent)
          where = opi.mIsParentFirst ? opi.mGrandParent->getFirst() : opi.mGrandParent->getSecond();

        mOwner->dropMovableToLayout(where, getClientWindow(), opi.mSize, opi.mWindowAlign);
      }

      return 0;
  }

  return 1;
}


//=============================================================================
ClientWindow *MovableWindow::getClientWindow() { return mClientWindow; }


//=============================================================================
void MovableWindow::setClientWindow(ClientWindow *client_window) { mClientWindow = client_window; }


//=============================================================================
void MovableWindow::lockSize(WindowSizeLock size_lock, int w, int h)
{
  mSizeLock = size_lock;
  mLockW = w;
  mLockH = h;
}


//=============================================================================
WindowSizeLock MovableWindow::getSizeLock() const { return mSizeLock; }


//=============================================================================
void MovableWindow::setOriginalPlace(VirtualWindow *grand_parent, bool is_parent_first, WindowAlign align, float size)
{
  if (!mOriginalPosInfo)
    mOriginalPosInfo = new MovableOriginalPos();

  MovableOriginalPos &opi = *mOriginalPosInfo;

  opi.mGrandParent = grand_parent;
  opi.mIsParentFirst = is_parent_first;
  opi.mWindowAlign = align;
  opi.mSize = size;
}

//=============================================================================
void MovableWindow::onVirtualWindowDeleted(VirtualWindow *window)
{
  if (mOriginalPosInfo && mOriginalPosInfo->mGrandParent == window)
  {
    delete mOriginalPosInfo;
    mOriginalPosInfo = NULL;

    mMenuActions->setEnabledById(ID_UNMOVABLE, false);
  }
}
