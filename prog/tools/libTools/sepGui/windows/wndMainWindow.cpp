// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../wndManager.h"
#include "../wndMenu.h"
#include "wndMainWindow.h"

#include <windows.h>
#include <commctrl.h>

#include <startup/dag_globalSettings.h>
namespace workcycle_internal
{
extern bool enable_idle_priority;
void set_priority(bool foreground);
} // namespace workcycle_internal

MainWindow::MainWindow(WinManager *cur_owner) : mOwner(cur_owner), mMenu(NULL), mWasMaximized(true) {}


MainWindow::~MainWindow()
{
  // may be onDestroy should be here

  del_it(mMenu);
}


void MainWindow::resizeRoot()
{
  CascadeWindow *root = mOwner->getRoot();
  if (root)
    root->resize(mLeftTop, mRightBottom);
}


intptr_t MainWindow::winProc(void *h_wnd, unsigned msg, TSgWParam w_param, TSgLParam l_param)
{
  switch (msg)
  {
    case WM_PAINT:
    {
      HDC hdc = GetDC((HWND)h_wnd);

      RECT rc;
      GetClientRect((HWND)h_wnd, &rc);
      int width = rc.right - rc.left;

      HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
      SelectObject(hdc, hPen);

      MoveToEx(hdc, 0, 0, 0);
      LineTo(hdc, width, 0);
      DeleteObject(hPen);

      hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
      SelectObject(hdc, hPen);

      MoveToEx(hdc, 0, 1, 0);
      LineTo(hdc, width, 1);
      DeleteObject(hPen);

      break;
    }

    case WM_CREATE:
    {
      setHandle(h_wnd);
      mMenu = new Menu(h_wnd, (unsigned)ROOT_MENU_ITEM);
      mOwner->onMainWindowCreated();
      return 0;
    }

    case WM_CAPTURECHANGED:
    {
      resizeRoot();
      break;
    }

    case WM_SIZE:
    {
      if ((intptr_t)w_param == SIZE_MINIMIZED)
        break;

      mLeftTop = IPoint2(0, _pxS(MAIN_CLIENT_AREA_OFFSET));
      mRightBottom = IPoint2(LOWORD(l_param), HIWORD(l_param));

      if (((intptr_t)w_param == SIZE_MAXIMIZED) || (((intptr_t)w_param == SIZE_RESTORED) && (mWasMaximized)))
      {
        mWasMaximized = ((intptr_t)w_param == SIZE_MAXIMIZED);
        resizeRoot();
      }

      break;
    }

    case WM_GETMINMAXINFO:
    {
      CascadeWindow *root = mOwner->getRoot();
      if (root)
      {
        hdpi::Px minW, minH;
        root->getMinSize(minW, minH);
        WindowSizeLock lock = root->getSizeLock();

        RECT rect = {0, 0, _px(minW), _px(minH)};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, true);

        MINMAXINFO *pInfo = (MINMAXINFO *)l_param;
        POINT min = {rect.right - rect.left, rect.bottom - rect.top};
        pInfo->ptMinTrackSize = min;

        if (lock != WSL_NONE)
        {
          pInfo->ptMaxTrackSize.x = (lock & WSL_WIDTH) ? rect.right - rect.left : pInfo->ptMaxTrackSize.x;
          pInfo->ptMaxTrackSize.y = (lock & WSL_HEIGHT) ? rect.bottom - rect.top : pInfo->ptMaxTrackSize.y;
        }

        return 0;
      }

      break;
    }

    case WM_DESTROY: mOwner->onClose(); break;

    case WM_CLOSE: mOwner->close(); return 0;

    case WM_ACTIVATEAPP:
    {
      using namespace workcycle_internal;
      BOOL n_app_active = LOWORD(w_param);
      if (n_app_active && !::dgs_app_active)
      {
        set_priority(true);
        SetCursor(NULL);
        debug("---- APP  Active  --->");
        dgs_app_active = true;
        _fpreset();
      }
      else if (!n_app_active && ::dgs_app_active)
      {
        if (workcycle_internal::enable_idle_priority)
          set_priority(false);
        debug("<--- APP inactive ----");
        dgs_app_active = false;
      }
    }
    break;
  }


  if ((mMenu) && (!mMenu->checkMenu(msg, w_param)))
    return 0;


  return BaseWindow::winProc(h_wnd, msg, w_param, l_param);
}
