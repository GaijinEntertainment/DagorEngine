#pragma once
#ifndef _TARGET_PC_WIN
#error using windows specific implementation with wrong platform
#endif

#include <windows.h>
#include <3d/dag_drv3d.h>
#include "../drv3d_commonCode/drv_utils.h"

namespace drv3d_vulkan
{

struct WindowState
{
  RenderWindowSettings settings = {};
  RenderWindowParams params = {};
  bool ownsWindow = false;
  bool vsync = false;
  inline static main_wnd_f *mainCallback = nullptr;
  inline static WNDPROC originWndProc = nullptr;

  static LRESULT CALLBACK windowProcProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  WindowState() = default;
  ~WindowState() { closeWindow(); }
  WindowState(const WindowState &) = delete;
  WindowState &operator=(const WindowState &) = delete;

  void set(void *hinst, const char *name, int show, void *mainw, void *renderw, void *icon, const char *title, void *wnd_proc)
  {
    ownsWindow = renderw == nullptr;
    params.hinst = hinst;
    params.wcname = name;
    params.ncmdshow = show;
    params.hwnd = mainw;
    params.rwnd = renderw;
    params.icon = icon;
    params.title = title;
    params.mainProc = &windowProcProxy;
    mainCallback = (main_wnd_f *)wnd_proc;
    if (!ownsWindow && !originWndProc)
      originWndProc = (WNDPROC)SetWindowLongPtrW((HWND)params.rwnd, GWLP_WNDPROC, (LONG_PTR)params.mainProc);
  }

  inline void getRenderWindowSettings(Driver3dInitCallback *cb) { get_render_window_settings(settings, cb); }

  inline void getRenderWindowSettings() { get_render_window_settings(settings, nullptr); }

  inline bool setRenderWindowParams() { return set_render_window_params(params, settings); }

  inline void *getMainWindow() { return params.hwnd; }

  void closeWindow()
  {
    if (ownsWindow)
    {
      DestroyWindow((HWND)params.hwnd);
      ownsWindow = false;
    }
    else
    {
      SetWindowLongPtrW((HWND)params.rwnd, GWLP_WNDPROC, (LONG_PTR)originWndProc);
      originWndProc = nullptr;
    }
  }
};

} // namespace drv3d_vulkan