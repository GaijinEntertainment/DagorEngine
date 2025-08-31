// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_mainWindow.h>

#include "ec_imguiWndManagerBase.h"
#include "ec_init3d.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/hid/dag_hiPointing.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_input.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <workCycle/dag_genGuiMgr.h>
#include <workCycle/dag_workCycle.h>

#include <X11/Xcursor/Xcursor.h>

namespace workcycle_internal
{
extern Display *get_root_display();
extern bool window_initing;
} // namespace workcycle_internal

extern bool ec_mouse_cursor_hidden;

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

class ImguiWndManagerLinux : public ImguiWndManagerBase
{
public:
  explicit ImguiWndManagerLinux(void *main_hwnd) : mainHwnd(main_hwnd) {}

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

  bool init3d(const char *drv_name, const DataBlock *blkTexStreaming) override
  {
    workcycle_internal::window_initing = true;

    const bool succeeded = tools3d::init(drv_name, blkTexStreaming);

    // Mark window_initing complete because while it is true dgs_app_active is not updated in prog/engine/workCycle/mainWndProc.cpp,
    // and handle_app_activity_change() in imguiImp.cpp uses dgs_app_active to send focus events.
    workcycle_internal::window_initing = false;

    return succeeded;
  }

  void initCustomMouseCursors(const char *path) override
  {
    LOGERR_CTX("TODO: tools Linux porting: ImguiWndManagerLinux::initCustomMouseCursors");
  }

  void updateImguiMouseCursor() override
  {
    if (!global_cls_drv_pnt)
      return;

    const ImGuiMouseCursor newCursor = ImGui::GetCurrentContext() ? ImGui::GetMouseCursor() : ImGuiMouseCursor_Arrow;
    const CursorParams newCursorParams(newCursor, ec_mouse_cursor_hidden, ec_get_busy());
    if (newCursorParams == cursorParams && global_cls_drv_pnt->isMouseCursorHidden() == cursorParamsMakeHiddenCursor)
      return;

    cursorParams = newCursorParams;
    cursorParamsMakeHiddenCursor = updateOsMouseCursor(newCursor, ec_mouse_cursor_hidden, newCursorParams.busyCursor);
  }

private:
  static bool updateOsMouseCursor(ImGuiMouseCursor imgui_mouse_cursor, bool hidden_cursor, bool busy_cursor)
  {
    static constexpr const char *arrowCursor = "default";

    const char *cursorName;
    switch (imgui_mouse_cursor)
    {
      case ImGuiMouseCursor_Arrow: cursorName = arrowCursor; break;
      case ImGuiMouseCursor_TextInput: cursorName = "text"; break;
      case ImGuiMouseCursor_ResizeAll: cursorName = "move"; break;
      case ImGuiMouseCursor_ResizeEW: cursorName = "ew-resize"; break;
      case ImGuiMouseCursor_ResizeNS: cursorName = "ns-resize"; break;
      case ImGuiMouseCursor_ResizeNESW: cursorName = "nesw-resize"; break;
      case ImGuiMouseCursor_ResizeNWSE: cursorName = "nwse-resize"; break;
      case ImGuiMouseCursor_Hand: cursorName = "pointer"; break;
      case ImGuiMouseCursor_NotAllowed: cursorName = "not-allowed"; break;
      case ImGuiMouseCursor_None: cursorName = nullptr; break;

      case EDITOR_CORE_CURSOR_ADDITIONAL_CLICK: cursorName = "text"; break;

      default: cursorName = arrowCursor; break;
    }

    if (hidden_cursor)
      cursorName = nullptr;
    else if (busy_cursor)
      cursorName = "wait";

    if (cursorName)
    {
      if (global_cls_drv_pnt->isMouseCursorHidden())
        global_cls_drv_pnt->hideMouseCursor(false);

      // Set the cursor after hideMouseCursor() because that might have also changed the cursor.
      Display *display = workcycle_internal::get_root_display();
      Window window = (intptr_t)win32_get_main_wnd(); // see getWindowFromPtrHandle()
      Cursor cursor = XcursorLibraryLoadCursor(display, cursorName);
      if (cursor == None)
        cursor = XcursorLibraryLoadCursor(display, arrowCursor);
      XDefineCursor(display, window, cursor);

      return false;
    }
    else
    {
      if (!global_cls_drv_pnt->isMouseCursorHidden())
        global_cls_drv_pnt->hideMouseCursor(true);

      return true;
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
  CursorParams cursorParams;
  bool cursorParamsMakeHiddenCursor = false;
};

// Used in the Linux version, not used in the Windows version.
static void post_shutdown_handler()
{
  if (editor_main_window_event_handler)
    editor_main_window_event_handler->onDestroy();

  ::shutdown_game(RESTART_ALL);

  d3d::release_driver();

  // invoke shutdown handler installed by platform
  if (old_shutdown_handler)
    old_shutdown_handler();
}

IWndManager *EditorMainWindow::createWindowManager() { return new ImguiWndManagerLinux(mainHwnd); }

void EditorMainWindow::run(const char *caption, const char *icon, FileDropHandler file_drop_handler, E3DCOLOR bg_color)
{
  editor_main_window_event_handler = &eventHandler;

  ::dagor_gui_manager = &general_gui_manager;

  old_shutdown_handler = ::dgs_post_shutdown_handler;
  ::dgs_post_shutdown_handler = post_shutdown_handler;

  onMainWindowCreated();

  ::dagor_reset_spent_work_time();
  for (;;) // infinite loop!
    if (d3d::is_inited())
      ::dagor_work_cycle();
    else
      ::dagor_idle_cycle();
}

void EditorMainWindow::close() {}

void EditorMainWindow::onClose() {}

// TODO: tools Linux porting: missing messages: WM_MOVE, WM_SIZE -- only needed for multi view
// TODO: tools Linux porting: missing messages: WM_DROPFILES -- only used for ScreenshotMetaInfoLoader
