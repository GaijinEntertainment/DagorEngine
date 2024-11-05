// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "init3d.h"
#include "wndManager.h"
#include "windows/wndVirtualWindow.h"
#include "windows/wndClientWindow.h"
#include "windows/wndMovableWindow.h"
#include <ioSys/dag_dataBlock.h>

#include <windows.h>

#include <osApiWrappers/dag_symHlp.h>
#include <debug/dag_debug.h>


IWndManager *IWndManager::createManager(IWndManagerEventHandler *event_handler)
{
  if (!::symhlp_load("daKernel-dev.dll"))
    DEBUG_CTX("can't load sym for: %s", "daKernel-dev.dll");

  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  return new WinManager(event_handler);
}

bool WinManager::init3d(const char *drv_name, const DataBlock *blkTexStreaming) { return tools3d::init(drv_name, blkTexStreaming); }


ClientWindow *WinManager::findWindowByHandle(void *handle)
{
  if (ClientWindow *cw = findLayoutWindowByHandle(handle))
    return cw;

  return findMovableWindowByHandle(handle);
}


ClientWindow *WinManager::findMovableWindowByHandle(void *handle)
{
  for (int i = 0; i < mMovableWindows.size(); ++i)
  {
    ClientWindow *wnd = mMovableWindows[i]->getClientWindow();
    if (wnd && wnd->getClientAreaHandle() == handle)
      return wnd;
  }

  return NULL;
}


ClientWindow *WinManager::findLayoutWindowByHandle(void *handle) { return treeWindowSearch(getRoot(), handle); }


ClientWindow *WinManager::treeWindowSearch(CascadeWindow *node, void *handle)
{
  if (!node)
    return NULL;

  VirtualWindow *vw = node->getVirtualWindow();

  if (!vw)
  {
    ClientWindow *cw = node->getClientWindow();

    if (cw && (cw->getClientAreaHandle() == handle))
      return cw;
  }
  else
  {
    ClientWindow *cw = NULL;

    if ((vw->getFirst()) && (cw = this->treeWindowSearch(vw->getFirst(), handle)))
      return cw;

    if ((vw->getSecond()) && (cw = this->treeWindowSearch(vw->getSecond(), handle)))
      return cw;
  }

  return NULL;
}


///////////////////////////////////////////////////////////////////////////////


void *WinManager::getFirstWindow() const
{
  if (ClientWindow *cw = getRoot()->getClientWindow())
    return cw->getClientAreaHandle();

  return 0;
}


void *WinManager::splitWindowF(void *old_handle, void *new_handle, float new_window_size, WindowAlign new_window_align)
{
  CascadeWindow *where = (old_handle) ? this->findLayoutWindowByHandle(old_handle) : mRootWindow;
  if (!where)
    return 0;

  ClientWindow *what = (new_handle) ? this->findMovableWindowByHandle(new_handle) : NULL;

  if (insertCustomToLayout(where, what, new_window_size, new_window_align))
    return what->getClientAreaHandle();

  return 0;
}


void *WinManager::splitNeighbourWindowF(void *old_handle, void *new_handle, float new_window_size, WindowAlign new_window_align)
{
  if (!old_handle)
    return 0;

  CascadeWindow *where = this->findLayoutWindowByHandle(old_handle);
  if (!where || where == mRootWindow)
    return 0;

  ClientWindow *what = (new_handle) ? this->findMovableWindowByHandle(new_handle) : NULL;

  CascadeWindow *neightbor = getNeighborWindow(where->getParent(), where);
  if (!neightbor)
    return 0;

  if (insertCustomToLayout(neightbor, what, new_window_size, new_window_align))
    return what->getClientAreaHandle();

  return 0;
}


bool WinManager::resizeWindowF(void *handle, float new_window_size)
{
  ClientWindow *cw = findLayoutWindowByHandle(handle);
  if (!cw)
    return false;

  G_ASSERT((new_window_size > 0) && "WinManager::resizeWindow(): new size <= 0");

  VirtualWindow *vparent = cw->getParent();

  if ((vparent) && (vparent->getSecond()) && (vparent->getFirst()))
  {
    CascadeWindow *nw = vparent->getFirst();

    bool is_right = true;
    if (nw->getClientWindow() == cw)
    {
      nw = vparent->getSecond();
      is_right = false;
    }

    if (new_window_size >= 1)
    {
      bool vertical_split = vparent->isVertical();

      IPoint2 sz11, sz12, sz21, sz22;

      cw->getPos(sz11, sz12);
      nw->getPos(sz21, sz22);

      unsigned all_size = (vertical_split) ? (sz12.x - sz11.x + sz22.x - sz21.x) : (sz12.y - sz11.y + sz22.y - sz21.y);

      new_window_size = ((all_size > new_window_size) && (all_size)) ? new_window_size / all_size : 0.5;
    }

    new_window_size = (is_right) ? 1 - new_window_size : new_window_size;
    vparent->setRatio(new_window_size);
    vparent->refreshPos();

    return true;
  }

  return false;
}


bool WinManager::fixWindow(void *handle, bool fix)
{
  ClientWindow *cw = findLayoutWindowByHandle(handle);
  if (!cw)
    return false;

  cw->setFixed(fix);
  return true;
}


bool WinManager::removeWindow(void *handle)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (!cw)
    return false;

  deleteWindow(cw);
  return true;
}


void WinManager::reset()
{
  if (getRoot())
  {
    deleteMovableWindows();
    deleteWindow(getRoot());
    setRoot(NULL);

    RECT rct;
    HWND mwh = (HWND)getMainWindow();
    GetClientRect(mwh, &rct);

    ClientWindow *newRoot = createClientWindow(NULL, 0, _pxS(MAIN_CLIENT_AREA_OFFSET), rct.right - rct.left, rct.bottom - rct.top);
    newRoot->setMinSize(_pxScaled(WINDOW_MIN_SIZE_W), _pxScaled(WINDOW_MIN_SIZE_H));

    setRoot(newRoot);
  }
}


void WinManager::show(WindowSizeInit size)
{
  switch (size)
  {
    case WSI_NORMAL: ShowWindow((HWND)getMainWindow(), SW_SHOW); break;
    case WSI_MINIMIZED: ShowWindow((HWND)getMainWindow(), WS_MINIMIZE); break;
    case WSI_MAXIMIZED:
    {
      int w = GetSystemMetrics(SM_CXFULLSCREEN);
      int h = GetSystemMetrics(SM_CYFULLSCREEN) + GetSystemMetrics(SM_CYCAPTION);

      SetWindowPos((HWND)getMainWindow(), 0, 0, 0, w, h, SWP_SHOWWINDOW);

      RECT rct;
      GetClientRect((HWND)getMainWindow(), &rct);
      mRootWindow->resize(IPoint2(0, _pxS(MAIN_CLIENT_AREA_OFFSET)), IPoint2(rct.right - rct.left, rct.bottom - rct.top));
    }
    break;
  }
}


bool WinManager::getMovableFlag(void *handle)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (!cw)
    return false;

  return cw->getMovable();
}


void WinManager::setWindowType(void *handle, int type)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
    cw->setType(type);
}


int WinManager::getWindowType(void *handle)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
    return cw->getType();

  return 0;
}


void WinManager::setHeader(void *handle, HeaderPos header_pos)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
    cw->setHeader(header_pos);
}


void WinManager::clientToScreen(int &x, int &y)
{
  POINT pos = {x, y};
  ClientToScreen((HWND)getMainWindow(), &pos);

  x = pos.x;
  y = pos.y;
}


void WinManager::getWindowClientSize(void *handle, unsigned &width, unsigned &height)
{
  RECT wrect;
  GetClientRect((HWND)handle, &wrect);

  width = wrect.right - wrect.left;
  height = wrect.bottom - wrect.top;
}


bool WinManager::getWindowPosSize(void *handle, int &x, int &y, unsigned &width, unsigned &height)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
  {
    IPoint2 p1, p2;
    cw->getPos(p1, p2);

    x = p1.x;
    y = p1.y;
    width = p2.x - p1.x;
    height = p2.y - p1.y;
    return true;
  }

  return false;
}

int WinManager::getSplitterControlSize() const { return _pxS(SPLITTER_SPACE) + _pxS(SPLITTER_THICKNESS) + _pxS(SPLITTER_SPACE); }

void WinManager::setCaption(void *handle, const char *caption)
{
  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
    cw->setCaption(caption);
}


void WinManager::setMinSize(void *handle, hdpi::Px width, hdpi::Px height)
{
  if (_px(width) < 0 || _px(height) < 0)
    return;

  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
  {
    IPoint2 p1, p2;
    cw->getPos(p1, p2);
    if (p2.x - p1.x < _px(width) || p2.y - p1.y < _px(height))
      return;

    cw->setMinSize(width, height);
  }
}


void WinManager::setMenuArea(void *handle, hdpi::Px width, hdpi::Px height)
{
  if (_px(width) < 0 || _px(height) < 0)
    return;

  ClientWindow *cw = findWindowByHandle(handle);
  if (cw)
    cw->setMenuArea(IPoint2(0, 0), IPoint2(_px(width), _px(height)));
}


IMenu *WinManager::getMainMenu() { return (IMenu *)mMainWindow.getMenu(); }


IMenu *WinManager::getMenu(void *handle)
{
  ClientWindow *cw = findWindowByHandle(handle);

  return (IMenu *)cw->getMenu();
}


void WinManager::setMainWindowCaption(const char *caption) { SetWindowText((HWND)getMainWindow(), caption); }
