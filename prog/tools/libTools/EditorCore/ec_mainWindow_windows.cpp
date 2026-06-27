// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_mainWindow.h>

#include "ec_imguiWndManagerBase.h"
#include "ec_init3d.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/hid/dag_hiPointing.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_rect.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <winGuiWrapper/wgw_input.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_genGuiMgr.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_string.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>

namespace workcycle_internal
{
extern bool window_initing;
} // namespace workcycle_internal

static IWndManagerEventHandler *editor_main_window_event_handler = nullptr;
static void (*old_shutdown_handler)() = nullptr;
static ImguiWndManagerBase::WindowPositionAndSize last_windw_position_and_size;

class WindowsDpiHelper
{
public:
  WindowsDpiHelper()
  {
    shcoreDll = LoadLibraryA("Shcore.dll");
    user32Dll = LoadLibraryA("User32.dll");

    // requires Windows 8.1
    getDpiForMonitorPointer = shcoreDll ? (GetDpiForMonitorType)GetProcAddress(shcoreDll, "GetDpiForMonitor") : nullptr;

    // requires Windows 10, version 1607
    getSystemMetricsForDpiPointer =
      user32Dll ? (GetSystemMetricsForDpiType)GetProcAddress(user32Dll, "GetSystemMetricsForDpi") : nullptr;
  }

  ~WindowsDpiHelper()
  {
    FreeLibrary(shcoreDll);
    FreeLibrary(user32Dll);
  }

  UINT getDpiForMonitor(HMONITOR monitor) const
  {
    UINT dpi = 96;
    if (getDpiForMonitorPointer && SUCCEEDED(getDpiForMonitorPointer((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &dpi, &dpi)))
      return dpi;
    return dpi;
  }

  int getSystemMetricsForDpi(int index, UINT dpi) const
  {
    if (getSystemMetricsForDpiPointer)
    {
      const int result = getSystemMetricsForDpiPointer(index, dpi);
      if (result != 0)
        return result;
    }

    return GetSystemMetrics(index);
  }

  typedef HRESULT(WINAPI *GetDpiForMonitorType)(HMONITOR, MONITOR_DPI_TYPE, UINT *, UINT *);
  typedef HRESULT(WINAPI *GetSystemMetricsForDpiType)(int, UINT);

  HINSTANCE shcoreDll;
  HINSTANCE user32Dll;
  GetSystemMetricsForDpiType getSystemMetricsForDpiPointer;
  GetDpiForMonitorType getDpiForMonitorPointer;
};

class EditorCoreGeneralGuiManager : public IGeneralGuiManager
{
public:
  void beforeRender(int) override {}

  bool canCloseNow() override { return !editor_main_window_event_handler || editor_main_window_event_handler->onClose(); }

  void drawFps(float, float, float) override {}
};

static EditorCoreGeneralGuiManager general_gui_manager;

class EditorCoreWndProcComponent : public IWndProcComponent
{
public:
  RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override
  {
    switch (msg)
    {
      case WM_MOVE:
        if (ImGui::GetCurrentContext())
          if (ImGuiViewport *imguiViewport = getMainImguiViewport())
            imguiViewport->PlatformRequestMove = true;

        if (!IsIconic((HWND)hwnd))
          updateLastWindowPositionAndSize(hwnd);
        break;

      case WM_SIZE:
        if (ImGui::GetCurrentContext())
          if (ImGuiViewport *imguiViewport = getMainImguiViewport())
            imguiViewport->PlatformRequestResize = true;

        if (!IsIconic((HWND)hwnd))
          updateLastWindowPositionAndSize(hwnd);
        break;

      case WM_DROPFILES:
        if (fileDropHandler)
        {
          HDROP hdrop = (HDROP)wParam;
          const int fileCount = DragQueryFileA(hdrop, -1, nullptr, 0);
          if (fileCount > 0)
          {
            dag::Vector<String> files;
            files.set_capacity(fileCount);

            for (int i = 0; i < fileCount; ++i)
            {
              const int lengthWithoutNullTerminator = DragQueryFileA(hdrop, 0, nullptr, 0);
              if (lengthWithoutNullTerminator > 0)
              {
                String path;
                path.resize(lengthWithoutNullTerminator + 1);
                if (DragQueryFileA(hdrop, 0, path.begin(), lengthWithoutNullTerminator + 1))
                  files.emplace_back(path);
              }
            }

            if (fileDropHandler(files))
              return IMMEDIATE_RETURN;
          }
        }
        break;
    }

    return PROCEED_OTHER_COMPONENTS;
  }

  EditorMainWindow::FileDropHandler fileDropHandler;

private:
  static ImGuiViewport *getMainImguiViewport()
  {
    ImGuiPlatformIO &platformIo = ImGui::GetPlatformIO();
    if (platformIo.Viewports.empty())
      return nullptr;

    G_ASSERT((platformIo.Viewports[0]->Flags & ImGuiViewportFlags_OwnedByApp) != 0);
    return platformIo.Viewports[0];
  }

  static void updateLastWindowPositionAndSize(void *hwnd)
  {
    RECT rect;
    if (GetWindowRect((HWND)hwnd, &rect) == FALSE)
    {
      last_windw_position_and_size.reset();
      return;
    }

    last_windw_position_and_size.rectangle = EcRect{.l = rect.left, .t = rect.top, .r = rect.right, .b = rect.bottom};
    last_windw_position_and_size.maximized = IsZoomed((HWND)hwnd) != 0;
  }
};

static EditorCoreWndProcComponent editor_core_wnd_proc_component;

class ImguiWndManagerWindows : public ImguiWndManagerBase
{
public:
  explicit ImguiWndManagerWindows(void *main_hwnd) : mainHwnd(main_hwnd) {}

  void close() override { quit_game(); }

  void loadMainWindowPositionAndSize(const DataBlock &blk) override
  {
    const IPoint2 position = blk.getIPoint2("position", IPoint2::ZERO);
    const IPoint2 size = blk.getIPoint2("size", IPoint2::ZERO);
    requestedMainWindowPositionAndSize.rectangle.l = position.x;
    requestedMainWindowPositionAndSize.rectangle.t = position.y;
    requestedMainWindowPositionAndSize.rectangle.r = position.x + size.x;
    requestedMainWindowPositionAndSize.rectangle.b = position.y + size.y;
    requestedMainWindowPositionAndSize.maximized = blk.getBool("maximized", true);
  }

  void saveMainWindowPositionAndSize(DataBlock &blk) override
  {
    if (!last_windw_position_and_size.isValid())
      return;

    blk.addIPoint2("position", IPoint2(last_windw_position_and_size.rectangle.l, last_windw_position_and_size.rectangle.t));
    blk.addIPoint2("size", IPoint2(last_windw_position_and_size.rectangle.width(), last_windw_position_and_size.rectangle.height()));
    blk.addBool("maximized", IsZoomed((HWND)mainHwnd) != 0);
  }

  void *getMainWindow() const override { return mainHwnd; }

  void setMainWindowCaption(const char *caption) override { win32_set_window_title(caption); }

  void getWindowClientSize(void *handle, unsigned &width, unsigned &height) override
  {
    width = 0;
    height = 0;

    if (handle == mainHwnd)
    {
      int w = 0;
      int h = 0;
      d3d::get_screen_size(w, h);
      width = w;
      height = h;
    }
  }

  bool init3d(const char *drv_name, const DataBlock *blkTexStreaming, const char *caption, const char *icon) override
  {
    void *hicon = (icon && *icon) ? LoadIcon((HINSTANCE)win32_get_instance(), icon) : nullptr;
    const bool succeeded = tools3d::init(drv_name, blkTexStreaming, caption, hicon);
    mainHwnd = win32_get_main_wnd();

    // At this point the window has been drawn, so we can show it.
    restoreWindowPostion(mainHwnd, requestedMainWindowPositionAndSize);
    requestedMainWindowPositionAndSize.reset();

    // Let work cycle process the window sizing. (Because the application might start loading without calling
    // dagor_work_cycle(), and the rendering would be blurry.)
    // Temporarily lift the FPS limit to prevent dagor_work_cycle() from returning because of not enough elapsed time.
    const bool oldLimitFps = dgs_limit_fps;
    dgs_limit_fps = false;
    dagor_work_cycle();
    dgs_limit_fps = oldLimitFps;

    if (editor_core_wnd_proc_component.fileDropHandler)
      DragAcceptFiles((HWND)mainHwnd, true);

    return succeeded;
  }

  void initCustomMouseCursors(const char *path) override
  {
    String cursorPath(128, "%scursor_additional_click.cur", path);
    cursorAdditionalClick = LoadImage(0, cursorPath, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!cursorAdditionalClick)
      logwarn("Failed to load cursor \"%s\".", cursorPath.c_str());
  }

  void updateImguiMouseCursor() override
  {
    if (!global_cls_drv_pnt)
      return;

    const ImGuiMouseCursor newCursor = ImGui::GetCurrentContext() ? ImGui::GetMouseCursor() : ImGuiMouseCursor_Arrow;
    const bool cursorHidden = !ec_is_cursor_visible();
    const CursorParams newCursorParams(newCursor, cursorHidden, ec_get_busy());
    if (newCursorParams == cursorParams && global_cls_drv_pnt->isMouseCursorHidden() == cursorParamsMakeHiddenCursor)
      return;

    cursorParams = newCursorParams;
    cursorParamsMakeHiddenCursor = updateOsMouseCursor(newCursor, cursorHidden, newCursorParams.busyCursor);
  }

private:
  bool updateOsMouseCursor(ImGuiMouseCursor imgui_mouse_cursor, bool hidden_cursor, bool busy_cursor) const
  {
    switch (imgui_mouse_cursor)
    {
      case ImGuiMouseCursor_Arrow: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_ARROW); break;
      case ImGuiMouseCursor_TextInput: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_IBEAM); break;
      case ImGuiMouseCursor_ResizeAll: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_SIZEALL); break;
      case ImGuiMouseCursor_ResizeEW: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_SIZEWE); break;
      case ImGuiMouseCursor_ResizeNS: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_SIZENS); break;
      case ImGuiMouseCursor_ResizeNESW: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_SIZENESW); break;
      case ImGuiMouseCursor_ResizeNWSE: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_SIZENWSE); break;
      case ImGuiMouseCursor_Hand: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_HAND); break;
      case ImGuiMouseCursor_NotAllowed: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_NO); break;
      case ImGuiMouseCursor_None: hidden_cursor = true; break;

      case EDITOR_CORE_CURSOR_ADDITIONAL_CLICK:
        win32_current_mouse_cursor = cursorAdditionalClick ? (HCURSOR)cursorAdditionalClick : LoadCursor(nullptr, IDC_ARROW);
        break;

      default: win32_current_mouse_cursor = LoadCursor(nullptr, IDC_ARROW); break;
    }

    if (hidden_cursor)
      win32_current_mouse_cursor = win32_empty_mouse_cursor;
    else if (busy_cursor)
      win32_current_mouse_cursor = LoadCursor(nullptr, IDC_WAIT);

    if (global_cls_drv_pnt->isMouseCursorHidden() != hidden_cursor)
      global_cls_drv_pnt->hideMouseCursor(hidden_cursor);

    // Make the change immediate instead of waiting to be updated at the next WM_SETCURSOR.
    SetCursor((HCURSOR)win32_current_mouse_cursor);

    return hidden_cursor;
  }

  void clipWindowPositionAndSizeToMonitor(WindowPositionAndSize &wps)
  {
    if (!wps.isValid())
      return;

    const RECT rc{.left = wps.rectangle.l, .top = wps.rectangle.t, .right = wps.rectangle.r, .bottom = wps.rectangle.b};
    HMONITOR monitor = MonitorFromRect(&rc, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFO info{};
    info.cbSize = sizeof(MONITORINFO);
    if (!monitor || GetMonitorInfo(monitor, &info) == FALSE)
    {
      wps.reset();
      return;
    }

    EcRect monitorRect{.l = info.rcMonitor.left, .t = info.rcMonitor.top, .r = info.rcMonitor.right, .b = info.rcMonitor.bottom};

    // GetWindowRect() returns a rectangle that includes the invisible border around the window.
    // Compensate for that by extending the monitor's rectangle.
    const UINT dpi = dpiHelper.getDpiForMonitor(monitor);
    const int frameX =
      dpiHelper.getSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + dpiHelper.getSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    const int frameY =
      dpiHelper.getSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + dpiHelper.getSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    monitorRect.l -= frameX;
    monitorRect.t -= frameY;
    monitorRect.r += frameX;
    monitorRect.b += frameY;

    wps.rectangle.l = clamp(wps.rectangle.l, (int)monitorRect.l, (int)monitorRect.r);
    wps.rectangle.t = clamp(wps.rectangle.t, (int)monitorRect.t, (int)monitorRect.b);
    wps.rectangle.r = clamp(wps.rectangle.r, (int)monitorRect.l, (int)monitorRect.r);
    wps.rectangle.b = clamp(wps.rectangle.b, (int)monitorRect.t, (int)monitorRect.b);
  }

  void restoreWindowPostion(void *hwnd, WindowPositionAndSize wps)
  {
    clipWindowPositionAndSizeToMonitor(wps);
    if (wps.isValid())
    {
      ShowWindow((HWND)hwnd, SW_SHOWNORMAL);
      SetWindowPos((HWND)hwnd, nullptr, wps.rectangle.l, wps.rectangle.t, wps.rectangle.width(), wps.rectangle.height(),
        SWP_NOZORDER | SWP_NOACTIVATE);
      if (wps.maximized)
        ShowWindow((HWND)hwnd, SW_SHOWMAXIMIZED);
    }
    else
    {
      ShowWindow((HWND)hwnd, SW_SHOWMAXIMIZED);
    }
  }

  struct CursorParams
  {
    explicit CursorParams(ImGuiMouseCursor imgui_mouse_cursor = ImGuiMouseCursor_None, bool hidden_cursor = false,
      bool busy_cursor = false) :
      imguiMouseCursor(imgui_mouse_cursor), hiddenCursor(hidden_cursor), busyCursor(busy_cursor)
    {}

    bool operator==(const CursorParams &other) const = default;

    ImGuiMouseCursor imguiMouseCursor;
    bool hiddenCursor;
    bool busyCursor;
  };

  void *mainHwnd;
  HANDLE cursorAdditionalClick = 0;
  CursorParams cursorParams;
  bool cursorParamsMakeHiddenCursor = false;
  WindowPositionAndSize requestedMainWindowPositionAndSize;
  WindowsDpiHelper dpiHelper;
};

static void post_shutdown_handler()
{
  del_wnd_proc_component(&editor_core_wnd_proc_component);

  if (editor_main_window_event_handler)
    editor_main_window_event_handler->onDestroy();

  ::shutdown_game(RESTART_ALL);

  // invoke shutdown handler installed by platform
  if (old_shutdown_handler)
    old_shutdown_handler();
}

IWndManager *EditorMainWindow::createWindowManager() { return new ImguiWndManagerWindows(mainHwnd); }

void EditorMainWindow::run(FileDropHandler file_drop_handler)
{
  editor_main_window_event_handler = &eventHandler;

  ::dagor_gui_manager = &general_gui_manager;

  old_shutdown_handler = ::dgs_post_shutdown_handler;
  ::dgs_post_shutdown_handler = post_shutdown_handler;

  editor_core_wnd_proc_component.fileDropHandler = file_drop_handler;
  add_wnd_proc_component(&editor_core_wnd_proc_component);

  onMainWindowCreated();

  ::dagor_reset_spent_work_time();
  for (;;) // infinite loop!
    if (d3d::is_inited())
      ::dagor_work_cycle();
    else
      ::dagor_idle_cycle();
}
