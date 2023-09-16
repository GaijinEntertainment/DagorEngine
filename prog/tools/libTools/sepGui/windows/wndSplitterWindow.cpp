// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "../wndManager.h"
#include "../wndMenu.h"
#include "wndSplitterWindow.h"
#include "wndVirtualWindow.h"
#include "wndClientWindow.h"

#include <windows.h>

const COLORREF COLOR_BACK = RGB(255, 255, 255);
const COLORREF COLOR_FRONT = RGB(128, 128, 128);

enum
{
  FRONT_COLOR_OFFSET = 1,
};


HCURSOR spl_normal_cursor = LoadCursor(NULL, IDC_ARROW);


//=============================================================================
// This conversion is needed when DPI scaling is enabled but the application is
// not DPI aware (DAGOR_NO_DPI_AWARE is defined) because the desktop's DC
// apparently expects physical coordinates.
static void convert_from_logical_screen_to_physical_screen(int &x, int &y)
{
  POINT pt = {x, y};

  // It works with hwnd = 0 but not with hwnd = GetDesktopWindow.
  if (!LogicalToPhysicalPointForPerMonitorDPI(0, &pt))
    return;

  x = pt.x;
  y = pt.y;
}


//=============================================================================
static void draw_xor_lines(int x1, int y1, int x2, int y2, bool is_vertical, void *hdc)
{
  convert_from_logical_screen_to_physical_screen(x1, y1);
  convert_from_logical_screen_to_physical_screen(x2, y2);

  int modeOld = GetROP2((HDC)hdc);
  SetROP2((HDC)hdc, R2_NOTXORPEN);

  MoveToEx((HDC)hdc, x1, y1, 0);
  LineTo((HDC)hdc, x2, y2);

  if (is_vertical)
  {
    x1 += SPLITTER_THICKNESS + SPLITTER_SPACE * 2;
    x2 += SPLITTER_THICKNESS + SPLITTER_SPACE * 2;
  }
  else
  {
    y1 += SPLITTER_THICKNESS + SPLITTER_SPACE * 2;
    y2 += SPLITTER_THICKNESS + SPLITTER_SPACE * 2;
  }

  MoveToEx((HDC)hdc, x1, y1, 0);
  LineTo((HDC)hdc, x2, y2);

  SetROP2((HDC)hdc, modeOld);
}


//=============================================================================
SplitterWindow::SplitterWindow(VirtualWindow *parent, WinManager *owner, bool is_vertical, int x, int y, unsigned w, unsigned h)

  :
  CascadeWindow(owner, parent, x, y, w, h), mIsVertical(is_vertical), mMoving(false), mIsHold(false), mPopupMenuActions(NULL)
{
  mHdc = GetDC(nullptr);
  mOldDistance = 0;
}


//=============================================================================
SplitterWindow::~SplitterWindow()
{
  del_it(mPopupMenuActions);
  ReleaseDC(nullptr, (HDC)mHdc);
}


//=============================================================================
ClientWindow *SplitterWindow::getWindowOnPos(const IPoint2 &cur_point) { return NULL; }


//=============================================================================
bool SplitterWindow::checkFixed()
{
  VirtualWindow *vw = this->getParent();

  ClientWindow *lcw = (!vw->getFirst()->getVirtualWindow()) ? (ClientWindow *)vw->getFirst() : NULL;

  ClientWindow *rcw = (!vw->getSecond()->getVirtualWindow()) ? (ClientWindow *)vw->getSecond() : NULL;

  if (((lcw) && (lcw->getFixed())) || ((rcw) && (rcw->getFixed())))
  {
    return true;
  }

  if (mIsHold)
  {
    return true;
  }

  return false;
}


//=============================================================================
void SplitterWindow::setNormalCursor()
{
  if (GetCursor() != ::spl_normal_cursor)
    SetCursor(::spl_normal_cursor);
}


//=============================================================================
intptr_t SplitterWindow::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  switch (msg)
  {
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint((HWND)h_wnd, &ps);

      RECT rc;
      GetClientRect((HWND)h_wnd, &rc);

      HBRUSH hBrushBack = CreateSolidBrush(COLOR_BACK);
      FillRect(hdc, &rc, hBrushBack);
      DeleteObject(hBrushBack);

      if (mIsVertical)
      {
        rc.left += FRONT_COLOR_OFFSET;
        rc.right -= FRONT_COLOR_OFFSET;
      }
      else
      {
        rc.top += FRONT_COLOR_OFFSET;
        rc.bottom -= FRONT_COLOR_OFFSET;
      }

      HBRUSH hBrushFront = CreateSolidBrush(COLOR_FRONT);
      FillRect(hdc, &rc, hBrushFront);
      DeleteObject(hBrushFront);

      EndPaint((HWND)h_wnd, &ps);
      break;
    }

    case WM_CREATE:
    {
      mPopupMenuActions = new Menu(h_wnd, ROOT_MENU_ITEM, this, true);
      mPopupMenuActions->addItem(ROOT_MENU_ITEM, SPR_DELETE, "Delete splitter");
      mPopupMenuActions->addItem(ROOT_MENU_ITEM, SPR_HOLD, "Hold");

      return 0;
    }

    case WM_LBUTTONDOWN:
    {
      if (!this->checkFixed())
      {
        SetCapture((HWND)h_wnd);

        POINT pos;
        GetCursorPos(&pos);

        click(IPoint2(pos.x, pos.y));
        mMoving = true;
      }

      break;
    }

    case WM_MOUSEMOVE:
    {
      if (mMoving)
      {

        POINT pos;
        GetCursorPos(&pos);
        drawMovingLine(IPoint2(pos.x, pos.y));
      }

      break;
    }

    case WM_LBUTTONUP:
    {
      if (!mMoving)
        break;

      if (mOldDistance)
      {
        IPoint2 thisPos1, thisPos2;
        getPos(thisPos1, thisPos2);
        client_to_screen(mOwner->getMainWindow(), thisPos1);
        client_to_screen(mOwner->getMainWindow(), thisPos2);
        if (mIsVertical)
          ::draw_xor_lines(thisPos1.x + mOldDistance, thisPos1.y, thisPos1.x + mOldDistance, thisPos2.y, true, mHdc);
        else
          ::draw_xor_lines(thisPos1.x, thisPos1.y + mOldDistance, thisPos2.x, thisPos1.y + mOldDistance, false, mHdc);

        mOldDistance = 0;
      }

      POINT pos;
      GetCursorPos(&pos);
      moveTo(IPoint2(pos.x, pos.y));

      ReleaseCapture();
      mMoving = false;

      break;
    }

    case WM_RBUTTONUP:
    {
      if (mPopupMenuActions)
        mPopupMenuActions->showPopupMenu();
      break;
    }
  }


  if ((WM_LBUTTONDOWN == msg) | (WM_LBUTTONUP == msg) | (WM_RBUTTONDOWN == msg) | (WM_RBUTTONUP == msg) | (WM_MOUSEMOVE == msg))
  {
    if (this->checkFixed())
      this->setNormalCursor();
  }

  if ((mPopupMenuActions) && (!mPopupMenuActions->checkMenu(msg, w_param)))
    return 0;

  return BaseWindow::winProc(h_wnd, msg, w_param, l_param);
}


//=============================================================================
int SplitterWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case SPR_DELETE:
      if (!checkFixed())
        mParent->deleteSplitter();
      return 0;

    case SPR_HOLD:
      mIsHold = !mIsHold;

      if (mPopupMenuActions)
        mPopupMenuActions->setCheckById(id, mIsHold);
      return 0;
  }

  return 1;
}


//=============================================================================
VirtualWindow *SplitterWindow::getVirtualWindow() const { return NULL; }


//=============================================================================
ClientWindow *SplitterWindow::getClientWindow() const { return NULL; }


//=============================================================================
bool SplitterWindow::getIsVertical() const { return mIsVertical; }


//=============================================================================
void SplitterWindow::click(IPoint2 mouse_pos)
{
  screen_to_client(mOwner->getMainWindow(), mouse_pos); // pos in main window
  mMousePosOnClick = mouse_pos;
}


//=============================================================================
bool SplitterWindow::canMoveSplitter()
{
  CascadeWindow *leftWnd = mParent->getFirst();
  CascadeWindow *rightWnd = mParent->getSecond();
  if (mIsVertical)
  {
    if ((WSL_WIDTH & leftWnd->getSizeLock()) || (WSL_WIDTH & rightWnd->getSizeLock()))
      return false;
  }
  else
  {
    if ((WSL_HEIGHT & leftWnd->getSizeLock()) || (WSL_HEIGHT & rightWnd->getSizeLock()))
      return false;
  }

  return true;
}


//=============================================================================
void SplitterWindow::drawMovingLine(IPoint2 mouse_pos)
{
  if (mIsHold)
    return;

  IPoint2 thisPos1, thisPos2, parentPos1, parentPos2;
  getPos(thisPos1, thisPos2);
  client_to_screen(mOwner->getMainWindow(), thisPos1);
  client_to_screen(mOwner->getMainWindow(), thisPos2);
  getParent()->getPos(parentPos1, parentPos2);

  screen_to_client(mOwner->getMainWindow(), mouse_pos);

  int minSize[2];
  bool leftRes = false, rightRes = false;
  int i = mIsVertical ? 0 : 1;
  int d = mouse_pos[i] - mMousePosOnClick[i];

  rightRes = mParent->getSecond()->getMinSize(minSize[0], minSize[1]); // only when dx > 0
  if (d > 0)
    if (parentPos2[i] - mouse_pos[i] < minSize[i] + SPLITTER_THICKNESS + SPLITTER_SPACE * 2)
      d = parentPos2[i] - minSize[i] - mMousePosOnClick[i] - SPLITTER_THICKNESS - SPLITTER_SPACE * 2;

  leftRes = mParent->getFirst()->getMinSize(minSize[0], minSize[1]);
  if (d < 0)
    if (mouse_pos[i] - parentPos1[i] < minSize[i])
      d = parentPos1[i] + minSize[i] - mMousePosOnClick[i];

  if (leftRes || rightRes)
    if (!canMoveSplitter())
      d = 1;

  int dx = mIsVertical ? d : 0;
  int dy = mIsVertical ? 0 : d;
  int xDistance = mIsVertical ? mOldDistance : 0;
  int yDistance = mIsVertical ? 0 : mOldDistance;
  int x2 = mIsVertical ? thisPos1.x : thisPos2.x;
  int y2 = mIsVertical ? thisPos2.y : thisPos1.y;

  if (mOldDistance)
    ::draw_xor_lines(thisPos1.x + xDistance, thisPos1.y + yDistance, x2 + xDistance, y2 + yDistance, mIsVertical, mHdc);
  if (d)
    ::draw_xor_lines(thisPos1.x + dx, thisPos1.y + dy, x2 + dx, y2 + dy, mIsVertical, mHdc);

  mOldDistance = d;
}


//=============================================================================
void SplitterWindow::moveTo(IPoint2 mouse_pos)
{
  if (mIsHold)
    return;

  VirtualWindow *virtualParent = getParent();

  CascadeWindow *leftWnd = virtualParent->getFirst();
  CascadeWindow *rightWnd = virtualParent->getSecond();

  screen_to_client(mOwner->getMainWindow(), mouse_pos); // pos in main window

  IPoint2 parentPos1, parentPos2;
  getParent()->getPos(parentPos1, parentPos2);

  IPoint2 thisPos1, thisPos2;
  getPos(thisPos1, thisPos2);

  IPoint2 newPos1(thisPos1), newPos2(thisPos2);

  int i = mIsVertical ? 0 : 1;
  int minLeft[2], minRight[2];
  bool leftRes = leftWnd->getMinSize(minLeft[0], minLeft[1]);
  bool rightRes = rightWnd->getMinSize(minRight[0], minRight[1]);

  if (leftRes || rightRes)
    if (!canMoveSplitter())
      return;

  int d, size1, size2;

  IPoint2 leftWindowPos1, leftWindowPos2, rightWindowPos1, rightWindowPos2;
  leftWnd->getPos(leftWindowPos1, leftWindowPos2);
  rightWnd->getPos(rightWindowPos1, rightWindowPos2);

  minLeft[i] += 1;
  minRight[i] += 1;

  d = mouse_pos[i] - mMousePosOnClick[i];
  if (!d)
    return;

  mMousePosOnClick[i] = mouse_pos[i];

  newPos1[i] = thisPos1[i] + d;
  newPos2[i] = thisPos2[i] + d;

  if (newPos1[i] < parentPos1[i] + minLeft[i])
  {
    newPos1[i] = parentPos1[i] + minLeft[i];
    newPos2[i] = newPos1[i] + (thisPos2[i] - thisPos1[i]);
  }
  else if (newPos2[i] > parentPos2[i] - minRight[i])
  {
    newPos2[i] = parentPos2[i] - minRight[i];
    newPos1[i] = newPos2[i] - (thisPos2[i] - thisPos1[i]);
  }

  if ((newPos1[i] == thisPos1[i]) && (newPos2[i] == thisPos2[i]))
    return;

  d = newPos1[i] - thisPos1[i];

  leftWindowPos2[i] += d;
  size1 = leftWindowPos2[i] - leftWindowPos1[i];

  rightWindowPos1[i] += d;
  size2 = rightWindowPos2[i] - rightWindowPos1[i];

  if (d > 0)
  {
    rightWnd->resize(rightWindowPos1, rightWindowPos2);
    resize(newPos1, newPos2);
    leftWnd->resize(leftWindowPos1, leftWindowPos2);
  }
  else
  {
    leftWnd->resize(leftWindowPos1, leftWindowPos2);
    resize(newPos1, newPos2);
    rightWnd->resize(rightWindowPos1, rightWindowPos2);
  }

  virtualParent->setRatio((float)size1 / (size1 + size2));
}


//=============================================================================
WindowSizeLock SplitterWindow::getSizeLock() const { return WSL_NONE; }


//=============================================================================
bool SplitterWindow::getMinSize(int &min_w, int &min_h)
{
  min_w = SPLITTER_THICKNESS;
  min_h = SPLITTER_THICKNESS;

  return false;
}


//=============================================================================
void SplitterWindow::getPos(IPoint2 &left_top, IPoint2 &right_bottom) const
{
  left_top = mLeftTop;
  right_bottom = mRightBottom;
}


//=============================================================================
void SplitterWindow::resize(const IPoint2 &left_top, const IPoint2 &right_bottom)
{
  mLeftTop = left_top;
  mRightBottom = right_bottom;

  int w = right_bottom.x - left_top.x;
  int h = right_bottom.y - left_top.y;

  int x = left_top.x;
  int y = left_top.y;

  if (mIsVertical)
  {
    y -= 1;
    h += 2;
  }
  else
  {
    x -= 1;
    w += 2;
  }

  setWindowPos(x, y, w, h);
  updateWindow();
}
