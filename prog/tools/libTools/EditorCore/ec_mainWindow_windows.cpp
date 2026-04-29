// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_mainWindow.h>

#include "ec_imguiWndManagerBase.h"
#include "ec_init3d.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/hid/dag_hiPointing.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
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

namespace workcycle_internal
{
extern bool window_initing;
} // namespace workcycle_internal

static IWndManagerEventHandler *editor_main_window_event_handler = nullptr;
static void (*old_shutdown_handler)() = nullptr;

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
        break;

      case WM_SIZE:
        if (ImGui::GetCurrentContext())
          if (ImGuiViewport *imguiViewport = getMainImguiViewport())
            imguiViewport->PlatformRequestResize = true;
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
};

static EditorCoreWndProcComponent editor_core_wnd_proc_component;

class ImguiWndManagerWindows : public ImguiWndManagerBase
{
public:
  explicit ImguiWndManagerWindows(void *main_hwnd) : mainHwnd(main_hwnd) {}

  void close() override { quit_game(); }

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

    // At this point the window has been drawn, so we can show it maximized.
    ShowWindow((HWND)win32_get_main_wnd(), SW_SHOWMAXIMIZED);

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
