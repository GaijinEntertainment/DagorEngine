// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "wndManager.h"
#include "windows/wndVirtualWindow.h"
#include "windows/wndSplitterWindow.h"
#include "windows/wndClientWindow.h"
#include "windows/wndMovableWindow.h"

#include <windows.h>

#include <debug/dag_debug.h>
#include <math/integer/dag_IBBox2.h>


extern const char *WNDCLASS_CLIENT_MOVABLE;


static void *set_parent(void *child, void *new_parent) { return SetParent((HWND)child, (HWND)new_parent); }


bool WinManager::calculateWinRects(CascadeWindow *where, float size_complex, WindowAlign align, IBBox2 &w1, IBBox2 &w2, IBBox2 &ws,
  float &ratio)
{
  bool isVertical = (align == WA_TOP) || (align == WA_BOTTOM);
  bool isTopLeft = (align == WA_TOP) || (align == WA_LEFT);

  int dir = isVertical ? 1 : 0;

  // size
  IPoint2 p1, p2;
  where->getPos(p1, p2);

  int size = (int)size_complex;
  if (size_complex < 1.f)
  {
    size = size_complex * ((p2[dir] - p1[dir]) - SPLITTER_SPACE * 2 - SPLITTER_THICKNESS);
  }

  // max size check
  IPoint2 minSize;
  where->getMinSize(minSize.x, minSize.y);

  int maxSize = (p2[dir] - p1[dir]) - minSize[dir] - SPLITTER_SPACE * 2 - SPLITTER_THICKNESS;

  if (size > maxSize)
    size = maxSize;

  if (maxSize <= 0)
    return false;

  // windows sizes
  if (isTopLeft)
  {
    w1[0] = p1;
    w1[1][dir] = p1[dir] + size;
    w1[1][1 - dir] = p2[1 - dir];

    ws[0][dir] = w1[1][dir] + SPLITTER_SPACE;
    ws[0][1 - dir] = p1[1 - dir];
    ws[1][dir] = ws[0][dir] + SPLITTER_THICKNESS;
    ws[1][1 - dir] = p2[1 - dir];

    w2[0][dir] = ws[1][dir] + SPLITTER_SPACE;
    w2[0][1 - dir] = p1[1 - dir];
    w2[1] = p2;

    ratio = float(w1[1][dir] - w1[0][dir]) / float(p2[dir] - p1[dir] - SPLITTER_SPACE * 2 - SPLITTER_THICKNESS);
  }
  else
  {
    w1[0][dir] = p2[dir] - size;
    w1[0][1 - dir] = p1[1 - dir];
    w1[1] = p2;

    ws[1][dir] = w1[0][dir] - SPLITTER_SPACE;
    ws[1][1 - dir] = p2[1 - dir];
    ws[0][dir] = ws[1][dir] - SPLITTER_THICKNESS;
    ws[0][1 - dir] = p1[1 - dir];

    w2[0] = p1;
    w2[1][dir] = ws[0][dir] - SPLITTER_SPACE;
    w2[1][1 - dir] = p2[1 - dir];

    ratio = float(w2[1][dir] - w2[0][dir]) / float(p2[dir] - p1[dir] - SPLITTER_SPACE * 2 - SPLITTER_THICKNESS);
  }

  return true;
}


bool WinManager::replaceCascadeWindow(VirtualWindow *parent, CascadeWindow *old_window, CascadeWindow *new_window)
{
  if (parent)
  {
    if (parent->getFirst() == old_window)
      parent->setFirst(new_window);
    else if (parent->getSecond() == old_window)
      parent->setSecond(new_window);
    else
      return false;
  }
  else
  {
    mRootWindow = new_window;
    new_window->setParent(NULL);
  }

  return true;
}


CascadeWindow *WinManager::getNeighborWindow(VirtualWindow *parent, CascadeWindow *window)
{
  G_ASSERT(parent && "WinManager::getNeighborWindow(): parent == NULL!");

  if (parent->getFirst() == window)
    return parent->getSecond();

  if (parent->getSecond() == window)
    return parent->getFirst();

  return NULL;
}


WindowAlign WinManager::getWindowAlign(CascadeWindow *window)
{
  G_ASSERT(window && "WinManager::getWindowAlign(): window == NULL!");

  VirtualWindow *vw = window->getParent();
  if (vw)
  {
    if (vw->getFirst() == window)
      return vw->isVertical() ? WA_LEFT : WA_TOP;

    if (vw->getSecond() == window)
      return vw->isVertical() ? WA_RIGHT : WA_BOTTOM;
  }

  return WA_NONE;
}


VirtualWindow *WinManager::insertCustomToLayout(CascadeWindow *where, ClientWindow *&new_window, float size, WindowAlign align)
{
  if (mRootWindow == new_window)
    return NULL;

  if (new_window && new_window->getMovable())
    return dropMovableToLayout(where, new_window, size, align);

  return insertToLayout(where, new_window, size, align);
}


VirtualWindow *WinManager::insertToLayout(CascadeWindow *where, ClientWindow *&new_window, float size, WindowAlign align)
{
  G_ASSERT((size > 0) && "WinManager::insertToLayout(): size <= 0!");

  // Do not insert if the destination window is fixed
  if (ClientWindow *newClient = where->getClientWindow())
  {
    if (newClient->getFixed())
      return NULL;
  }

  IBBox2 w1, w2, ws;
  float ratio;

  if (!calculateWinRects(where, size, align, w1, w2, ws, ratio))
    return NULL;

  IPoint2 p1, p2;
  where->getPos(p1, p2);

  VirtualWindow *parent = where->getParent();

  /////////////////////////

  VirtualWindow *winVirtual = new VirtualWindow(this);

  SplitterWindow *winSplitter = createSplitterWindow(winVirtual, ws[0].x, ws[0].y, ws[1].x - ws[0].x, ws[1].y - ws[0].y);

  where->resize(w2[0], w2[1]);

  ClientWindow *winNewWindow = new_window;
  if (!winNewWindow)
  {
    winNewWindow = createClientWindow(winVirtual, w1[0].x, w1[0].y, w1[1].x - w1[0].x, w1[1].y - w1[0].y);
  }
  else
  {
    winNewWindow->resize(w1[0], w1[1]);
  }

  /////////////////////////

  bool isTopLeft = (align == WA_TOP) || (align == WA_LEFT);

  if (isTopLeft)
    winVirtual->setWindows(winNewWindow, where, winSplitter, ratio, p1, p2);
  else
    winVirtual->setWindows(where, winNewWindow, winSplitter, ratio, p1, p2);

  /////////////////////////

  bool resReplace = replaceCascadeWindow(parent, where, winVirtual);
  G_ASSERT(resReplace && "WinManager::insertToLayout(): wrong parent window!");

  if (!new_window)
    new_window = winNewWindow;

  return winVirtual;
}


bool WinManager::deleteFromLayout(CascadeWindow *window)
{
  G_ASSERT(window && "WinManager::deleteFromLayout(): window == NULL!");

  VirtualWindow *parent = window->getParent();
  // If someone attemped to delete root window!
  if (!parent)
    return false;

  CascadeWindow *neighbor = getNeighborWindow(parent, window);
  G_ASSERT(neighbor && "WinManager::deleteFromLayout(): wrong parent window!");

  // Make neighbor unfixed
  if (ClientWindow *neighborClient = neighbor->getClientWindow())
  {
    neighborClient->setFixed(false);
  }

  bool resReplace = replaceCascadeWindow(parent->getParent(), parent, neighbor);
  G_ASSERT(resReplace && "WinManager::insertToLayout(): wrong parent window!");

  IPoint2 p1, p2;
  parent->getPos(p1, p2);
  neighbor->resize(p1, p2);

  // debug("w: x = %d, y = %d, w = %d, h = %d", p1.x, p1.y, p2.x - p1.x, p2.y - p1.y);

  parent->setFirst(NULL);
  parent->setSecond(NULL);
  delete parent;

  return true;
}


ClientWindow *WinManager::makeMovableWindow(ClientWindow *window, int x, int y, int w, int h, const char *caption)
{
  if (window && (mRootWindow == window || window->getMovable()))
    return NULL;

  if (window && !window->onMakingMovable(w, h))
    return NULL;

  G_ASSERT(w >= 0 && h >= 0);

  RECT windowRect = {x, y, x + w, y + h};
  AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, true, WS_EX_TOOLWINDOW);

  int windowW = windowRect.right - windowRect.left;
  int windowH = windowRect.bottom - windowRect.top;

  ///////////////////////////

  ClientWindow *cw = window;
  if (!cw)
    // Change parent - create on movable window right away
    cw = createClientWindow(NULL, 0, 0, w, h);

  MovableWindow *mw = new MovableWindow(this, cw);

  if (cw->getFixed())
  {
    WindowSizeLock sizeLock = cw->getParent()->isVertical() ? WSL_WIDTH : WSL_HEIGHT;
    mw->lockSize(sizeLock, windowW, windowH);
  }

  HWND hWndMovable = CreateWindowEx(WS_EX_TOOLWINDOW, WNDCLASS_CLIENT_MOVABLE, caption, WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y,
    windowW, windowH, (HWND)getMainWindow(), 0, NULL, static_cast<BaseWindow *>(mw));

  mw->setHandle(hWndMovable);

  cw->setMovable(mw);
  mMovableWindows.push_back(mw);

  // Malicious hack, while it's not possible to create client window on the required parent
  if (!window)
    ::set_parent(cw->getHandle(), hWndMovable);

  ///////////////////////////

  if (window)
  {
    saveOriginalWindowPlace(*window, *mw);

    deleteFromLayout(cw);

    ::set_parent(cw->getHandle(), hWndMovable);

    IPoint2 newP1(0, 0), newP2(w, h);
    cw->resize(newP1, newP2);
  }

  return cw;
}


void WinManager::saveOriginalWindowPlace(ClientWindow &original, MovableWindow &movable)
{
  VirtualWindow *parent = original.getParent();
  G_ASSERT(parent && "WinManager::saveOriginalWindowPlace(): parent window == NULL!");

  WindowAlign parentAlign = getWindowAlign(parent);
  bool parentIsFirst = (parentAlign == WA_LEFT) || (parentAlign == WA_TOP);

  WindowAlign windowAlign = getWindowAlign(&original);
  float size = 0.f;

  if (original.getFixed())
  {
    IPoint2 p1, p2;
    original.getPos(p1, p2);

    int dir = (windowAlign == WA_TOP) || (windowAlign == WA_BOTTOM) ? 1 : 0;
    size = p2[dir] - p1[dir];
  }
  else
  {
    size = parent->getRatio();
    if (windowAlign == WA_RIGHT || windowAlign == WA_BOTTOM)
      size = 1.f - size;
  }

  movable.setOriginalPlace(parent->getParent(), parentIsFirst, windowAlign, size);
}


VirtualWindow *WinManager::dropMovableToLayout(CascadeWindow *where, ClientWindow *movable, float size, WindowAlign align)
{
  MovableWindow *mw = movable->getMovable();
  if (!mw)
    return NULL;

  if (((align == WA_LEFT || align == WA_RIGHT) && !(mw->getSizeLock() & WSL_WIDTH)) ||
      ((align == WA_TOP || align == WA_BOTTOM) && !(mw->getSizeLock() & WSL_HEIGHT)))
    movable->setFixed(false);

  movable->setMovable(NULL);

  VirtualWindow *vw = insertToLayout(where, movable, size, align);
  if (!vw)
  {
    movable->setMovable(mw);
    return NULL;
  }

  mw->setClientWindow(NULL);

  ::set_parent(movable->getHandle(), getMainWindow());
  deleteMovableWindow(mw);

  return vw;
}


void WinManager::deleteMovableWindow(MovableWindow *window)
{
  int index = -1;
  for (int i = 0; i < mMovableWindows.size(); ++i)
  {
    if (mMovableWindows[i] == window)
    {
      index = i;
      break;
    }
  }

  G_ASSERT((index >= 0) && "WinManager::deleteMovableWindow(): MovableWindow isn't in Tab!");

  SetFocus((HWND)getMainWindow());

  delete mMovableWindows[index];
  erase_items(mMovableWindows, index, 1);
}


void WinManager::deleteWindow(CascadeWindow *window)
{
  if (ClientWindow *win = window->getClientWindow())
  {
    if (win->getMovable())
    {
      deleteMovableWindow(win->getMovable());
      return;
    }
    else
      win->notifyWmDestroyWindow();
  }

  deleteFromLayout(window);
  delete window;
}
