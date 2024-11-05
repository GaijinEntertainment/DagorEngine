// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#include "dag_winSplashScreen.h"

#if !_TARGET_PC_WIN || defined(NOGDI)

void win_show_splash_screen() {}
void win_hide_splash_screen() {}

#else

#include <util/dag_globDef.h>

#include <windows.h>
#include <objidl.h>

// GDI+ for PNG support
#pragma comment(lib, "gdiplus.lib")
#pragma warning(push, 0) // gdiplus.h is full of warnings :(
#include <gdiplus.h>
#pragma warning(pop)

#include <osApiWrappers/dag_progGlobals.h>

#ifdef UNICODE
inline constexpr const wchar_t *splash_window_class = L"SplashWindow";
inline constexpr const wchar_t *splash_window_icon = L"WindowIcon";
#else
inline constexpr const char *splash_window_class = "SplashWindow";
inline constexpr const char *splash_window_icon = "WindowIcon";
#endif

static HBITMAP load_splash_bitmap(const wchar_t *path)
{
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  Gdiplus::Status gdiplusStartupStatus = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  if (gdiplusStartupStatus != Gdiplus::Status::Ok)
  {
    debug("WinSplash: Failed to startup GDI+, Status: %d", gdiplusStartupStatus);
    return NULL;
  }

  Gdiplus::Bitmap *splashImageGdi = Gdiplus::Bitmap::FromFile(path);
  if (splashImageGdi == nullptr)
  {
    debug("WinSplash: Failed to load %s", path);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return NULL;
  }

  HBITMAP result;
  Gdiplus::Status getHBITMAPStatus = splashImageGdi->GetHBITMAP(0, &result);

  delete splashImageGdi;
  Gdiplus::GdiplusShutdown(gdiplusToken);

  if (getHBITMAPStatus != Gdiplus::Status::Ok)
  {
    debug("WinSplash: Failed to get splash HBITMAP from Gdiplus, Status: %d", getHBITMAPStatus);
    return NULL;
  }

  return result;
}

static void register_splash_window_class()
{
  WNDCLASS wc = {0};
  wc.lpfnWndProc = DefWindowProc;
  wc.hInstance = (HINSTANCE)win32_get_instance();
  wc.hIcon = LoadIcon((HINSTANCE)win32_get_instance(), splash_window_icon);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = splash_window_class;
  RegisterClass(&wc);
}

static HWND create_splash_window()
{
  // When creating the windows, I use an old trick (a hidden owner window) to make the splash screen
  // appear in the Alt+Tab list, but not in the taskbar. If you wanted the splash screen to also appear
  // in the taskbar, you could drop the hidden owner window. If you didn’t want it to appear in the
  // Alt+Tab list or the taskbar, you could use the WS_EX_TOOLWINDOW extended window style (and omit
  // the owner window).
  // Source: https://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_ii/

  HWND hwndOwner = CreateWindow(splash_window_class, NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, (HINSTANCE)win32_get_instance(), NULL);
  return CreateWindowEx(WS_EX_LAYERED, splash_window_class, NULL, WS_POPUP | WS_VISIBLE, 0, 0, 0, 0, hwndOwner, NULL,
    (HINSTANCE)win32_get_instance(), NULL);
}

static void set_splash_image(HWND splash_window, HBITMAP splash_bitmap)
{
  // get the size of the bitmap
  BITMAP bm;
  GetObject(splash_bitmap, sizeof(bm), &bm);
  SIZE sizeSplash = {bm.bmWidth, bm.bmHeight};

  // get the primary monitor's info
  POINT ptZero = {0};
  HMONITOR hmonPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitorinfo = {0};
  monitorinfo.cbSize = sizeof(monitorinfo);
  GetMonitorInfo(hmonPrimary, &monitorinfo);

  // center the splash screen in the middle of the primary work area
  const RECT &rcWork = monitorinfo.rcWork;
  POINT ptOrigin;
  ptOrigin.x = rcWork.left + (rcWork.right - rcWork.left - sizeSplash.cx) / 2;
  ptOrigin.y = rcWork.top + (rcWork.bottom - rcWork.top - sizeSplash.cy) / 2;

  // create a memory DC holding the splash bitmap
  HDC hdcScreen = GetDC(NULL);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);
  HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, splash_bitmap);

  // use the source image's alpha channel for blending
  BLENDFUNCTION blend = {0};
  blend.BlendOp = AC_SRC_OVER; // -V1048
  blend.SourceConstantAlpha = 255;
  blend.AlphaFormat = AC_SRC_ALPHA;

  // paint the window (in the right location) with the alpha-blended bitmap
  UpdateLayeredWindow(splash_window, hdcScreen, &ptOrigin, &sizeSplash, hdcMem, &ptZero, RGB(0, 0, 0), &blend, ULW_ALPHA);

  // delete temporary objects
  SelectObject(hdcMem, hbmpOld);
  DeleteDC(hdcMem);
  ReleaseDC(NULL, hdcScreen);
}

class SplashWindow final : public DaThread
{
public:
  SplashWindow(const wchar_t *splash_image_path) : splashImagePath{splash_image_path} {}

  void execute() override
  {
    debug("WinSplash: win_show_splash_screen");

    HBITMAP splash_image_bitmap = load_splash_bitmap(splashImagePath.c_str());
    if (splash_image_bitmap == NULL)
    {
      debug("WinSplash: missing image");
      return;
    }

    register_splash_window_class();
    HWND splash_hwnd = create_splash_window();
    set_splash_image(splash_hwnd, splash_image_bitmap);

    while (!isThreadTerminating())
    {
      Sleep(1);
    }

    debug("WinSplash: win_hide_splash_screen");

    DestroyWindow(splash_hwnd);
    DeleteObject(splash_image_bitmap);
  }

  eastl::wstring splashImagePath;
};

static InitOnDemand<SplashWindow> splash_window_thread;

void win_show_splash_screen(const wchar_t *splash_image_path)
{
  if (!splash_window_thread)
  {
    splash_window_thread.demandInit(splash_image_path);
    splash_window_thread->start();
  }
}

void win_hide_splash_screen()
{
  if (splash_window_thread)
    splash_window_thread->terminate(false);
}

#endif