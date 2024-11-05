// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "wndManager.h"
#include "wndDragPlacesShower.h"
#include "windows/wndVirtualWindow.h"
#include "windows/wndSplitterWindow.h"
#include "windows/wndClientWindow.h"
#include "windows/wndMovableWindow.h"

#include <workCycle/dag_workCycle.h>
#include <drv/3d/dag_driver.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>


const char *WNDCLASS_EDITOR = "EDITOR_LAYOUT_DE_WINDOW";
const char *WNDCLASS_DRAG_PLACE = "EDITOR_LAYOUT_DE_DRAG_PLACE";
const char *WNDCLASS_DRAG_ROOT_PLACE = "EDITOR_LAYOUT_DE_DRAG_ROOT_PLACE";
const char *WNDCLASS_CLIENT = "EDITOR_LAYOUT_DE_CLIENT";
const char *WNDCLASS_CLIENT_MOVABLE = "EDITOR_LAYOUT_DE_CLIENT_MOVABLE";
const char *WNDCLASS_VSPLITTER = "EDITOR_LAYOUT_DE_VSPLITTER";
const char *WNDCLASS_HSPLITTER = "EDITOR_LAYOUT_DE_HSPLITTER";
const char *WNDCLASS_HEADER_WINDOW = "EDITOR_LAYOUT_DE_HEADER_WINDOW";
const char *WNDCLASS_CLIENT_AREA = "EDITOR_LAYOUT_DE_CLIENT_AREA";

LRESULT CALLBACK sepgui_window_proc(HWND h_wnd, unsigned msg, WPARAM w_param, LPARAM l_param);

//=============================================================================
WinManager::WinManager(IWndManagerEventHandler *event_handler) :
  mMainWindow(this), mEventHandler(event_handler), mRootWindow(NULL), mLayoutSaver(this), mMovableWindows(midmem), mAccels(this)
{}


//=============================================================================
WinManager::~WinManager()
{
  // all moved to onClose()
}


//=============================================================================
void WinManager::close()
{
  if (mEventHandler->onClose())
  {
    PostMessage((HWND)mMainWindow.getHandle(), WM_DESTROY, 0, 0);
  }
}


//=============================================================================
void WinManager::onClose()
{
  del_it(mRootWindow);
  deleteMovableWindows();
  del_it(mDragPlacesShower);

  if (mEventHandler)
    mEventHandler->onDestroy();

  PostQuitMessage(0);
}


//=============================================================================
void WinManager::deleteMovableWindows()
{
  for (int i = 0; i < mMovableWindows.size(); i++)
    delete mMovableWindows[i];

  clear_and_shrink(mMovableWindows);
}


//=============================================================================
void WinManager::setUnMovable(ClientWindow *movable_wnd, IPoint2 *mouse_pos)
{
  WindowAlign align = WA_NONE;

  CascadeWindow *window = getWindowAlign(*mouse_pos, align);

  if (!window)
    return;

  G_ASSERT(movable_wnd);
  IPoint2 leftTop, rightBottom;
  movable_wnd->getPos(leftTop, rightBottom);
  int size = align & WA_IS_VERTICAL ? rightBottom.x - leftTop.x : rightBottom.y - leftTop.y;
  dropMovableToLayout(window, movable_wnd, size, align);

  SetFocus((HWND)getMainWindow());
}


//=============================================================================
ClientWindow *WinManager::setMovable(ClientWindow *make_movable_wnd)
{
  IPoint2 p1, p2;
  make_movable_wnd->getPos(p1, p2);
  IPoint2 ps = p1;
  client_to_screen(mMainWindow.getHandle(), ps);

  G_ASSERT(make_movable_wnd);
  makeMovableWindow(make_movable_wnd, ps.x, ps.y, p2.x - p1.x, p2.y - p1.y, make_movable_wnd->getCaption());

  return make_movable_wnd;
}


//=============================================================================
VirtualWindow *WinManager::addSplitter(CascadeWindow *splitted_wnd, IPoint2 mouse_pos, bool is_vertical, bool original_to_left)
{
  screen_to_client(mMainWindow.getHandle(), mouse_pos); // pos in main window

  IPoint2 p1, p2;
  splitted_wnd->getPos(p1, p2);

  int size = 0;
  WindowAlign align;

  if (is_vertical)
  {
    if (original_to_left)
    {
      align = WA_RIGHT;
      size = p2.x - mouse_pos.x - _pxS(SPLITTER_SPACE) - _pxS(SPLITTER_THICKNESS);
    }
    else
    {
      align = WA_LEFT;
      size = mouse_pos.x - p1.x - _pxS(SPLITTER_SPACE);
    }
  }
  else
  {
    if (original_to_left)
    {
      align = WA_BOTTOM;
      size = p2.y - mouse_pos.y - _pxS(SPLITTER_SPACE) - _pxS(SPLITTER_THICKNESS);
    }
    else
    {
      align = WA_TOP;
      size = mouse_pos.y - p1.y - _pxS(SPLITTER_SPACE);
    }
  }

  ClientWindow *newClient = NULL;
  VirtualWindow *win = insertToLayout(splitted_wnd, newClient, size, align);

  if (newClient)
  {
    newClient->setMinSize(_pxScaled(WINDOW_MIN_SIZE_W), _pxScaled(WINDOW_MIN_SIZE_H));

    if (ClientWindow *client = splitted_wnd->getClientWindow())
      newClient->setType(client->getType());
  }

  return win;
}


//=============================================================================
ClientWindow *WinManager::createClientWindow(VirtualWindow *parent, int x, int y, int w, int h)
{
  ClientWindow *cw = new ClientWindow(parent, this, "", x, y, w, h);

  HWND hWnd = CreateWindowEx(0, WNDCLASS_CLIENT, NULL, WS_CHILD | WS_VISIBLE, x, y, w, h, (HWND)mMainWindow.getHandle(), NULL, NULL,
    static_cast<BaseWindow *>(cw));

  cw->setHandle(hWnd);
  return cw;
}


//=============================================================================
SplitterWindow *WinManager::createSplitterWindow(VirtualWindow *parent, int x, int y, int w, int h)
{
  SplitterWindow *sw = NULL;
  LPCSTR className = NULL;

  if (w == _pxS(SPLITTER_THICKNESS))
  {
    sw = new SplitterWindow(parent, this, true, x, y, w, h);
    className = WNDCLASS_VSPLITTER;
    y -= 1;
    h += 2;
  }
  else
  {
    sw = new SplitterWindow(parent, this, false, x, y, w, h);
    className = WNDCLASS_HSPLITTER;
    x -= 1;
    w += 2;
  }

  HWND hWnd = CreateWindowEx(0, className, NULL, WS_CHILD | WS_VISIBLE, x, y, w, h, (HWND)mMainWindow.getHandle(), NULL, NULL,
    static_cast<BaseWindow *>(sw));

  sw->setHandle(hWnd);

  return sw;
}


//=============================================================================
void WinManager::UpdatePlacesShower(const IPoint2 &mouse_pos, void *moving_handle)
{
  mDragPlacesShower->Update(mouse_pos, moving_handle);
}


//=============================================================================
void WinManager::StopPlacesShower() { mDragPlacesShower->Stop(); }


//=============================================================================
CascadeWindow *WinManager::getWindowAlign(const IPoint2 &mouse_pos, WindowAlign &align)
{
  return mDragPlacesShower->getWindowAlign(mouse_pos, align);
}


//=============================================================================
void *WinManager::getMainWindow() const { return mMainWindow.getHandle(); }


//=============================================================================
void WinManager::setRoot(CascadeWindow *root_window) { mRootWindow = root_window; }


//=============================================================================
CascadeWindow *WinManager::getRoot() const { return mRootWindow; }


//=============================================================================
IWndManagerEventHandler *WinManager::getEventHandler() const { return mEventHandler; }


//=============================================================================
dag::ConstSpan<MovableWindow *> WinManager::getMovableWindows() const { return mMovableWindows; }


//=============================================================================
bool WinManager::loadLayout(const char *filename) { return mLayoutSaver.loadLayout(filename); }


//=============================================================================
void WinManager::saveLayout(const char *filename) { mLayoutSaver.saveLayout(filename); }


//=============================================================================
bool WinManager::loadLayoutFromDataBlock(const DataBlock &data_block) { return mLayoutSaver.loadLayoutFromDataBlock(data_block); }


//=============================================================================
void WinManager::saveLayoutToDataBlock(DataBlock &data_block) { mLayoutSaver.saveLayoutToDataBlock(data_block); }

//=============================================================================
int WinManager::run(int width, int height, const char *caption, const char *icon, WindowSizeInit size)
{
  HINSTANCE hInstance = GetModuleHandle(NULL);

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpszClassName = WNDCLASS_EDITOR;
  wc.hInstance = hInstance;
  wc.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
  wc.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  wc.hCursor = LoadCursor(0, IDC_ARROW);
  wc.hIcon = LoadIcon(hInstance, icon);
  RegisterClassEx(&wc);

  int x = CW_USEDEFAULT;
  int y = CW_USEDEFAULT;
  int style = 0;

  if (size == WSI_MAXIMIZED)
  {
    x = y = 0;
    width = GetSystemMetrics(SM_CXFULLSCREEN);
    height = GetSystemMetrics(SM_CYFULLSCREEN) + GetSystemMetrics(SM_CYCAPTION);
    style = WS_MAXIMIZE;
  }

  HWND handle = CreateWindowEx(0, wc.lpszClassName, caption, WS_OVERLAPPEDWINDOW | WS_VISIBLE | style, x, y, width, height, 0, 0, 0,
    static_cast<BaseWindow *>(&mMainWindow));

  if (size == WSI_MINIMIZED)
    ShowWindow(handle, WS_MINIMIZE);

  ::dagor_reset_spent_work_time();
  for (;;) // infinite loop!
    if (d3d::is_inited())
      ::dagor_work_cycle();
    else
      ::dagor_idle_cycle();
  return 0;
}


//=============================================================================
void WinManager::onMainWindowCreated()
{
  registerWinAPIWindowClasses();

  WINDOWINFO minInfo;
  GetWindowInfo((HWND)mMainWindow.getHandle(), &minInfo);

  int clientW = minInfo.rcClient.right - minInfo.rcClient.left;
  int clientH = minInfo.rcClient.bottom - minInfo.rcClient.top - _pxS(MAIN_CLIENT_AREA_OFFSET);

  ClientWindow *rootWindow = createClientWindow(NULL, 0, _pxS(MAIN_CLIENT_AREA_OFFSET), clientW, clientH);

  rootWindow->setMinSize(_pxScaled(WINDOW_MIN_SIZE_W), _pxScaled(WINDOW_MIN_SIZE_H));

  mRootWindow = rootWindow;

  mDragPlacesShower = new DragPlacesShower(this);

  // ShowWindow((HWND) getMainWindow(), true);

  if (mEventHandler)
    mEventHandler->onInit(this);
}


//=============================================================================
void WinManager::onVirtualWindowDeleted(VirtualWindow *window)
{
  for (int i = 0; i < mMovableWindows.size(); i++)
  {
    mMovableWindows[i]->onVirtualWindowDeleted(window);
  }
}


//=============================================================================
LRESULT CALLBACK sepgui_window_proc(HWND h_wnd, unsigned msg, WPARAM w_param, LPARAM l_param)
{
  if (msg == WM_NCCREATE)
  {
    CREATESTRUCT *cs = (CREATESTRUCT *)l_param;
    SetWindowLongPtr(h_wnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);

    return DefWindowProc(h_wnd, msg, w_param, l_param);
  }

  BaseWindow *w = (BaseWindow *)GetWindowLongPtr(h_wnd, GWLP_USERDATA);
  if (w)
    return w->winProc(h_wnd, msg, (TSgWParam)w_param, (TSgLParam)l_param);

  return DefWindowProc(h_wnd, msg, w_param, l_param);
}


//=============================================================================
void WinManager::registerWinAPIWindowClasses()
{
  WNDCLASSEX cw = {0};
  cw.cbSize = sizeof(WNDCLASSEX);
  cw.lpszClassName = WNDCLASS_CLIENT;
  cw.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
  cw.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  cw.hCursor = LoadCursor(0, IDC_ARROW);
  RegisterClassEx(&cw);

  WNDCLASSEX mcw = {0};
  mcw.cbSize = sizeof(WNDCLASSEX);
  mcw.lpszClassName = WNDCLASS_CLIENT_MOVABLE;
  mcw.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  mcw.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  mcw.hCursor = LoadCursor(0, IDC_ARROW);
  RegisterClassEx(&mcw);

  HBRUSH hbrush = CreateSolidBrush(RGB(212, 212, 212));

  WNDCLASSEX vsw = {0};
  vsw.cbSize = sizeof(WNDCLASSEX);
  vsw.lpszClassName = WNDCLASS_VSPLITTER;
  vsw.hbrBackground = hbrush;
  vsw.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  vsw.hCursor = LoadCursor(0, IDC_SIZEWE);
  RegisterClassEx(&vsw);

  WNDCLASSEX hsw = {0};
  hsw.cbSize = sizeof(WNDCLASSEX);
  hsw.lpszClassName = WNDCLASS_HSPLITTER;
  hsw.hbrBackground = hbrush;
  hsw.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  hsw.hCursor = LoadCursor(0, IDC_SIZENS);
  RegisterClassEx(&hsw);


  WNDCLASSEX hdw = {0};
  hdw.style = 0;
  hdw.cbSize = sizeof(WNDCLASSEX);
  hdw.lpszClassName = WNDCLASS_DRAG_PLACE;
  hdw.hbrBackground = CreateSolidBrush(RGB(222, 233, 245));
  hdw.lpfnWndProc = (WNDPROC)DefWindowProc;
  hdw.hCursor = LoadCursor(0, IDC_ARROW);
  RegisterClassEx(&hdw);

  WNDCLASSEX hrdw = {0};
  hrdw.style = 0;
  hrdw.cbSize = sizeof(WNDCLASSEX);
  hrdw.lpszClassName = WNDCLASS_DRAG_ROOT_PLACE;
  hrdw.hbrBackground = CreateSolidBrush(RGB(222, 0, 0));
  hrdw.lpfnWndProc = (WNDPROC)DefWindowProc;
  hrdw.hCursor = LoadCursor(0, IDC_ARROW);
  RegisterClassEx(&hrdw);


  WNDCLASSEX head = {0};
  head.cbSize = sizeof(WNDCLASSEX);
  head.hCursor = LoadCursor(NULL, IDC_ARROW);
  head.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  head.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
  head.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  head.hbrBackground = CreateSolidBrush(RGB(0, 80, 150));
  head.hInstance = (HMODULE)GetModuleHandle(NULL);
  head.lpszClassName = WNDCLASS_HEADER_WINDOW;
  RegisterClassEx(&head);

  WNDCLASSEX area = {0};
  area.cbSize = sizeof(WNDCLASSEX);
  area.hCursor = LoadCursor(NULL, IDC_ARROW);
  area.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  area.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
  area.lpfnWndProc = (WNDPROC)::sepgui_window_proc;
  area.hbrBackground = CreateSolidBrush(RGB(0, 0, 50));
  area.hInstance = (HMODULE)GetModuleHandle(NULL);
  area.lpszClassName = WNDCLASS_CLIENT_AREA;
  RegisterClassEx(&area);
}
