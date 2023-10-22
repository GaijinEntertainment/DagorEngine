// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <sepGui/wndEmbeddedWindow.h>

#include "../wndManager.h"
#include "../wndHeader.h"
#include "../wndMenu.h"
#include "wndClientWindow.h"
#include "wndVirtualWindow.h"

#include <windows.h>


enum
{
  MENU_SPLITTER_VERTICAL = 0xFFFF - 10, // not to conflict with user defined menu ids
  MENU_SPLITTER_HORIZONTAL,
  MENU_WINDOW_MOVABLE,
  MENU_WINDOW_FIX,
  MENU_WINDOW_CLOSE,
};


const char *MENU_SPLITTER_VERTICAL_NAME = "Split vertically";
const char *MENU_SPLITTER_HORIZONTAL_NAME = "Split horizontally";
const char *MENU_WINDOW_MOVABLE_NAME = "Make movable";
const char *MENU_WINDOW_FIX_NAME = "Lock";
const char *MENU_WINDOW_CLOSE_NAME = "Close";


//=============================================================================
ClientWindow::ClientWindow(VirtualWindow *parent, WinManager *owner, const char *caption, int x, int y, unsigned w, unsigned h)

  :
  CascadeWindow(owner, parent, x, y, w, h),
  mIsFixed(false),
  mType(CLIENT_WINDOW_TYPE_NONE),
  mPopupMenuActions(NULL),
  mPopupMenuUser(NULL),
  mPHeader(NULL),
  mPClientArea(NULL),
  mHeaderPos(HEADER_NONE),
  mMinWidth(_pxScaled(WINDOW_TEMP_MIN_SIZE)),
  mMinHeight(_pxScaled(WINDOW_TEMP_MIN_SIZE)),
  mMovableWindow(NULL),
  mCaption(caption),
  mEmbeddedWindow(NULL)
{
  mMenuAreaPos = IPoint2(_pxS(WINDOW_MENU_AREA_X), _pxS(WINDOW_MENU_AREA_Y));
  mMenuAreaSize = IPoint2(_pxS(WINDOW_MENU_AREA_W), _pxS(WINDOW_MENU_AREA_H));
}


//=============================================================================
ClientWindow::~ClientWindow()
{
  notifyWmDestroyWindow();

  del_it(mPHeader);
  del_it(mPClientArea);

  del_it(mPopupMenuActions);
  del_it(mPopupMenuUser);
}


//=============================================================================
void ClientWindow::notifyWmDestroyWindow()
{
  if (mEmbeddedWindow)
  {
    mEmbeddedWindow = NULL;
    G_ASSERT(mOwner);
    mOwner->onWmDestroyWindow(getClientAreaHandle());
  }
}


//=============================================================================
ClientWindow *ClientWindow::getWindowOnPos(const IPoint2 &cur_point)
{
  IPoint2 mLeftTop, mRightBottom;
  getPos(mLeftTop, mRightBottom);
  if (cur_point.x >= mLeftTop.x && cur_point.y >= mLeftTop.y && cur_point.x <= mRightBottom.x && cur_point.y <= mRightBottom.y)
    return this;

  return NULL;
}


//=============================================================================
int ClientWindow::getType() const { return mType; }


//=============================================================================
void ClientWindow::setType(int type)
{
  bool isNewType = type != mType;

  if (isNewType)
    notifyWmDestroyWindow();

  mType = type;

  if (isNewType)
    mEmbeddedWindow = mOwner->onWmCreateWindow(getClientAreaHandle(), type);
}


//=============================================================================
void ClientWindow::setCaption(const char *caption)
{
  mCaption = caption;
  if (mPHeader)
    mPHeader->updateWindow();
}


//=============================================================================
const char *ClientWindow::getCaption() const { return mCaption.str(); }


//=============================================================================
void ClientWindow::setMovable(MovableWindow *movable) { mMovableWindow = movable; }


//=============================================================================
MovableWindow *ClientWindow::getMovable() { return mMovableWindow; }

//=============================================================================
bool ClientWindow::onMakingMovable(int &width, int &height)
{
  if (mEmbeddedWindow)
    return mEmbeddedWindow->onWmEmbeddedMakingMovable(width, height);

  return true;
}


//=============================================================================
void ClientWindow::setMenuArea(IPoint2 pos, IPoint2 size)
{
  mMenuAreaPos = pos;
  mMenuAreaSize = size;
}


//=============================================================================
WindowSizeLock ClientWindow::getSizeLock() const
{
  if (mIsFixed)
  {
    if (mParent)
    {
      if (mParent->isVertical())
        return WSL_WIDTH;
      else
        return WSL_HEIGHT;
    }
    else
      return WSL_BOTH;
  }
  return WSL_NONE;
}


//=============================================================================
bool ClientWindow::getMinSize(hdpi::Px &min_w, hdpi::Px &min_h) const
{
  min_w = mMinWidth;
  min_h = mMinHeight;

  if (mIsFixed)
  {
    IPoint2 leftTop, rightBottom;
    getPos(leftTop, rightBottom);

    if (mParent)
    {
      if (mParent->isVertical())
        min_w = _pxActual(rightBottom.x - leftTop.x);
      else
        min_h = _pxActual(rightBottom.y - leftTop.y);
    }
    else
    {
      min_w = _pxActual(rightBottom.x - leftTop.x);
      min_h = _pxActual(rightBottom.y - leftTop.y);
    }
    return true;
  }
  return false;
}


//=============================================================================
void ClientWindow::setMinSize(hdpi::Px min_w, hdpi::Px min_h)
{
  mMinWidth = min_w;
  mMinHeight = min_h;
}


//=============================================================================
void ClientWindow::setFixed(bool is_fixed)
{
  mIsFixed = is_fixed;

  if (mPopupMenuActions)
    mPopupMenuActions->setCheckById(MENU_WINDOW_FIX, mIsFixed);
}


//=============================================================================
bool ClientWindow::getFixed() const { return mIsFixed; }


//=============================================================================
int ClientWindow::calcMousePos(int left, int right, int min_old, int size_new, bool to_left, bool to_root)
{
  int offset = _pxS(SPLITTER_THICKNESS) + _pxS(SPLITTER_SPACE) * 2;
  if (right - left > min_old + offset + size_new)
  {
    if (to_left)
      return left + size_new + _pxS(SPLITTER_SPACE);
    else
      return right - offset - size_new;
  }
  else
  {
    int half = left + (right - left) / 2;
    if (to_left)
      return to_root ? left + _pxS(WINDOW_MIN_SIZE_W) + _pxS(SPLITTER_SPACE) : half;
    else
      return to_root ? right - _pxS(WINDOW_MIN_SIZE_W) - offset : half;
  }
}


//=============================================================================
void ClientWindow::resize(const IPoint2 &left_top, const IPoint2 &right_bottom)
{
  if (mLeftTop == left_top && mRightBottom == right_bottom)
    return;

  mLeftTop = left_top;
  mRightBottom = right_bottom;

  int w = right_bottom.x - left_top.x;
  int h = right_bottom.y - left_top.y;

  setWindowPos(left_top.x, left_top.y, w, h);

  headerUpdate();

  if (!getMovable())
    updateWindow();
}


//=============================================================================
void ClientWindow::headerUpdate()
{
  int width = mRightBottom.x - mLeftTop.x;
  int height = mRightBottom.y - mLeftTop.y;

  HeaderPos headerCurPos = mHeaderPos;

  if (getMovable() && (HEADER_TOP == headerCurPos))
  {
    if (mPHeader)
      mPHeader->hide();

    headerCurPos = HEADER_NONE;
  }
  else
  {
    if (mPHeader)
      mPHeader->show();
  }

  int areaLeft = 0;
  int areaTop = 0;
  int areaWidth = width;
  int areaHeight = height;

  switch (headerCurPos)
  {
    case HEADER_NONE: break;

    case HEADER_TOP:
      areaTop = _pxS(TOP_HEADER_HEIGHT);
      areaHeight -= _pxS(TOP_HEADER_HEIGHT);

      G_ASSERT(mPHeader && "ClientWindow::headerUpdate(): header client window == NULL!");

      mPHeader->resize(0, 0, width, _pxS(TOP_HEADER_HEIGHT));
      break;

    case HEADER_LEFT:
      areaLeft = _pxS(LEFT_HEADER_WIDTH);
      areaWidth -= _pxS(LEFT_HEADER_WIDTH);

      G_ASSERT(mPHeader && "ClientWindow::headerUpdate(): header client window == NULL!");

      mPHeader->resize(0, 0, _pxS(LEFT_HEADER_WIDTH), height);
      break;
  }

  if (areaWidth < 0)
    areaWidth = 0;
  if (areaHeight < 0)
    areaHeight = 0;

  mPClientArea->resize(areaLeft, areaTop, areaWidth, areaHeight);

  if (mEmbeddedWindow)
    mEmbeddedWindow->onWmEmbeddedResize(areaWidth, areaHeight);
}


//=============================================================================
void ClientWindow::setHeader(HeaderPos header_pos)
{
  if (mPHeader)
  {
    delete mPHeader;
    mPHeader = NULL;
  }

  mHeaderPos = header_pos;

  switch (mHeaderPos)
  {
    case HEADER_NONE: break;

    case HEADER_TOP:
    {
      mPHeader = new LayoutWindowHeader(this, getOwner(), true, 0, 0);
    }
    break;

    case HEADER_LEFT:
    {
      setMenuArea(IPoint2(0, 0), IPoint2(0, 0));
      mPHeader = new LayoutWindowHeader(this, getOwner(), false, 10, 10);
    }
    break;
  }

  headerUpdate();
}


//=============================================================================
intptr_t ClientWindow::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  switch (msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      GetClientRect((HWND)h_wnd, &rc);

      mPClientArea = new ClientWindowArea(h_wnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top);

      // system menu
      mPopupMenuActions = new Menu(h_wnd, ROOT_MENU_ITEM, this, true);
      mPopupMenuActions->addItem(ROOT_MENU_ITEM, MENU_SPLITTER_VERTICAL, MENU_SPLITTER_VERTICAL_NAME);
      mPopupMenuActions->addItem(ROOT_MENU_ITEM, MENU_SPLITTER_HORIZONTAL, MENU_SPLITTER_HORIZONTAL_NAME);
      mPopupMenuActions->addItem(ROOT_MENU_ITEM, MENU_WINDOW_MOVABLE, MENU_WINDOW_MOVABLE_NAME);
      mPopupMenuActions->addItem(ROOT_MENU_ITEM, MENU_WINDOW_FIX, MENU_WINDOW_FIX_NAME);

      mPopupMenuActions->addSeparator(ROOT_MENU_ITEM);

      mPopupMenuActions->addItem(ROOT_MENU_ITEM, MENU_WINDOW_CLOSE, MENU_WINDOW_CLOSE_NAME);


      // user defined menu
      mPopupMenuUser = new Menu(h_wnd, ROOT_MENU_ITEM, NULL, true);

      return 0;
    }

    case WM_RBUTTONUP:
    {
      POINT mousePos; // = {mMouseClickPos.x, mMouseClickPos.y};
      GetCursorPos(&mousePos);
      mMouseClickPos = IPoint2(mousePos.x, mousePos.y);

      RECT rect;
      GetWindowRect((HWND)h_wnd, &rect);

      IPoint2 posInWnd(mMouseClickPos);
      screen_to_client(h_wnd, posInWnd);
      if (posInWnd.x >= mMenuAreaPos.x && posInWnd.x <= mMenuAreaPos.x + mMenuAreaSize.x && posInWnd.y >= mMenuAreaPos.y &&
          posInWnd.y <= mMenuAreaPos.y + mMenuAreaSize.y)
      {
        if (mPopupMenuUser)
          mPopupMenuUser->showPopupMenu();
      }
      else
      {
        if ((mPopupMenuActions) && (!getMovable()))
          mPopupMenuActions->showPopupMenu();
      }

      return 0;
    }

    case WM_ERASEBKGND: return 1;
  }

  // popup menu check

  if ((mPopupMenuActions) && (!mPopupMenuActions->checkMenu(msg, w_param)))
    return 0;

  if ((mPopupMenuUser) && (!mPopupMenuUser->checkMenu(msg, w_param)))
    return 0;

  return BaseWindow::winProc(h_wnd, msg, w_param, l_param);
}


//=============================================================================

int ClientWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case MENU_SPLITTER_VERTICAL: mOwner->addSplitter(this, IPoint2(mMouseClickPos.x, mMouseClickPos.y), true); return 0;

    case MENU_SPLITTER_HORIZONTAL: mOwner->addSplitter(this, IPoint2(mMouseClickPos.x, mMouseClickPos.y), false); return 0;

    case MENU_WINDOW_MOVABLE: mOwner->setMovable(this); return 0;

    case MENU_WINDOW_FIX: setFixed(!getFixed()); return 0;

    case MENU_WINDOW_CLOSE:
      CascadeWindow *thisWindow = static_cast<CascadeWindow *>(this);
      if (thisWindow != mOwner->getRoot())
        mOwner->deleteWindow(this);
      return 0;
  }

  return 1;
}


//=============================================================================
void ClientWindow::getPos(IPoint2 &left_top, IPoint2 &right_bottom) const
{
  left_top = mLeftTop;
  right_bottom = mRightBottom;
}


//=============================================================================
VirtualWindow *ClientWindow::getVirtualWindow() const { return NULL; }


//=============================================================================
ClientWindow *ClientWindow::getClientWindow() const { return (ClientWindow *)this; }
