// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_mainWindow.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderTarget.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_ViewportWindow.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/renderViewports/cachedViewports.h>
#include <osApiWrappers/dag_symHlp.h>
#include <sepGui/wndPublic.h>
#include <winGuiWrapper/wgw_input.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_workCycle.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

namespace tools3d
{
bool init(const char *drv_name, const DataBlock *blkTexStreaming);
}

namespace workcycle_internal
{
extern bool enable_idle_priority;
void set_priority(bool foreground);
} // namespace workcycle_internal

extern CachedRenderViewports *ec_cached_viewports;

// Taken from ImGui's Windows backend sample.
static ImGuiKey map_virtual_key_to_imgui_key(WPARAM wParam)
{
  switch (wParam)
  {
    case VK_TAB: return ImGuiKey_Tab;
    case VK_LEFT: return ImGuiKey_LeftArrow;
    case VK_RIGHT: return ImGuiKey_RightArrow;
    case VK_UP: return ImGuiKey_UpArrow;
    case VK_DOWN: return ImGuiKey_DownArrow;
    case VK_PRIOR: return ImGuiKey_PageUp;
    case VK_NEXT: return ImGuiKey_PageDown;
    case VK_HOME: return ImGuiKey_Home;
    case VK_END: return ImGuiKey_End;
    case VK_INSERT: return ImGuiKey_Insert;
    case VK_DELETE: return ImGuiKey_Delete;
    case VK_BACK: return ImGuiKey_Backspace;
    case VK_SPACE: return ImGuiKey_Space;
    case VK_RETURN: return ImGuiKey_Enter;
    case VK_ESCAPE: return ImGuiKey_Escape;
    case VK_OEM_7: return ImGuiKey_Apostrophe;
    case VK_OEM_COMMA: return ImGuiKey_Comma;
    case VK_OEM_MINUS: return ImGuiKey_Minus;
    case VK_OEM_PERIOD: return ImGuiKey_Period;
    case VK_OEM_2: return ImGuiKey_Slash;
    case VK_OEM_1: return ImGuiKey_Semicolon;
    case VK_OEM_PLUS: return ImGuiKey_Equal;
    case VK_OEM_4: return ImGuiKey_LeftBracket;
    case VK_OEM_5: return ImGuiKey_Backslash;
    case VK_OEM_6: return ImGuiKey_RightBracket;
    case VK_OEM_3: return ImGuiKey_GraveAccent;
    case VK_CAPITAL: return ImGuiKey_CapsLock;
    case VK_SCROLL: return ImGuiKey_ScrollLock;
    case VK_NUMLOCK: return ImGuiKey_NumLock;
    case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
    case VK_PAUSE: return ImGuiKey_Pause;
    case VK_NUMPAD0: return ImGuiKey_Keypad0;
    case VK_NUMPAD1: return ImGuiKey_Keypad1;
    case VK_NUMPAD2: return ImGuiKey_Keypad2;
    case VK_NUMPAD3: return ImGuiKey_Keypad3;
    case VK_NUMPAD4: return ImGuiKey_Keypad4;
    case VK_NUMPAD5: return ImGuiKey_Keypad5;
    case VK_NUMPAD6: return ImGuiKey_Keypad6;
    case VK_NUMPAD7: return ImGuiKey_Keypad7;
    case VK_NUMPAD8: return ImGuiKey_Keypad8;
    case VK_NUMPAD9: return ImGuiKey_Keypad9;
    case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
    case VK_DIVIDE: return ImGuiKey_KeypadDivide;
    case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case VK_ADD: return ImGuiKey_KeypadAdd;
    case VK_LSHIFT: return ImGuiKey_LeftShift;
    case VK_LCONTROL: return ImGuiKey_LeftCtrl;
    case VK_LMENU: return ImGuiKey_LeftAlt;
    case VK_LWIN: return ImGuiKey_LeftSuper;
    case VK_RSHIFT: return ImGuiKey_RightShift;
    case VK_RCONTROL: return ImGuiKey_RightCtrl;
    case VK_RMENU: return ImGuiKey_RightAlt;
    case VK_RWIN: return ImGuiKey_RightSuper;
    case VK_APPS: return ImGuiKey_Menu;
    case '0': return ImGuiKey_0;
    case '1': return ImGuiKey_1;
    case '2': return ImGuiKey_2;
    case '3': return ImGuiKey_3;
    case '4': return ImGuiKey_4;
    case '5': return ImGuiKey_5;
    case '6': return ImGuiKey_6;
    case '7': return ImGuiKey_7;
    case '8': return ImGuiKey_8;
    case '9': return ImGuiKey_9;
    case 'A': return ImGuiKey_A;
    case 'B': return ImGuiKey_B;
    case 'C': return ImGuiKey_C;
    case 'D': return ImGuiKey_D;
    case 'E': return ImGuiKey_E;
    case 'F': return ImGuiKey_F;
    case 'G': return ImGuiKey_G;
    case 'H': return ImGuiKey_H;
    case 'I': return ImGuiKey_I;
    case 'J': return ImGuiKey_J;
    case 'K': return ImGuiKey_K;
    case 'L': return ImGuiKey_L;
    case 'M': return ImGuiKey_M;
    case 'N': return ImGuiKey_N;
    case 'O': return ImGuiKey_O;
    case 'P': return ImGuiKey_P;
    case 'Q': return ImGuiKey_Q;
    case 'R': return ImGuiKey_R;
    case 'S': return ImGuiKey_S;
    case 'T': return ImGuiKey_T;
    case 'U': return ImGuiKey_U;
    case 'V': return ImGuiKey_V;
    case 'W': return ImGuiKey_W;
    case 'X': return ImGuiKey_X;
    case 'Y': return ImGuiKey_Y;
    case 'Z': return ImGuiKey_Z;
    case VK_F1: return ImGuiKey_F1;
    case VK_F2: return ImGuiKey_F2;
    case VK_F3: return ImGuiKey_F3;
    case VK_F4: return ImGuiKey_F4;
    case VK_F5: return ImGuiKey_F5;
    case VK_F6: return ImGuiKey_F6;
    case VK_F7: return ImGuiKey_F7;
    case VK_F8: return ImGuiKey_F8;
    case VK_F9: return ImGuiKey_F9;
    case VK_F10: return ImGuiKey_F10;
    case VK_F11: return ImGuiKey_F11;
    case VK_F12: return ImGuiKey_F12;
    case VK_F13: return ImGuiKey_F13;
    case VK_F14: return ImGuiKey_F14;
    case VK_F15: return ImGuiKey_F15;
    case VK_F16: return ImGuiKey_F16;
    case VK_F17: return ImGuiKey_F17;
    case VK_F18: return ImGuiKey_F18;
    case VK_F19: return ImGuiKey_F19;
    case VK_F20: return ImGuiKey_F20;
    case VK_F21: return ImGuiKey_F21;
    case VK_F22: return ImGuiKey_F22;
    case VK_F23: return ImGuiKey_F23;
    case VK_F24: return ImGuiKey_F24;
    case VK_BROWSER_BACK: return ImGuiKey_AppBack;
    case VK_BROWSER_FORWARD: return ImGuiKey_AppForward;
    default: return ImGuiKey_None;
  }
}

class ImguiWndManager : public IWndManager
{
public:
  explicit ImguiWndManager(void *main_hwnd) : mainHwnd(main_hwnd) {}

  virtual int run(int width, int height, const char *caption, const char *icon, WindowSizeInit size) override { return 0; }

  virtual void close() override
  {
    // Just forward it to EditorMainWindow::close().
    SendMessage((HWND)mainHwnd, WM_CLOSE, 0, 0);
  }

  virtual bool loadLayout(const char *filename) override { return false; }
  virtual void saveLayout(const char *filename) override {}

  virtual bool loadLayoutFromDataBlock(const DataBlock &data_block) override { return false; }
  virtual void saveLayoutToDataBlock(DataBlock &data_block) override {}

  virtual void setMainWindowCaption(const char *caption) override { SetWindowText((HWND)mainHwnd, caption); }

  virtual void registerWindowHandler(IWndManagerWindowHandler *handler) override
  {
    if (!handler)
      return;

    if (eastl::find(handlers.begin(), handlers.end(), handler) != handlers.end())
      return;

    handlers.push_back(handler);
  }

  virtual void unregisterWindowHandler(IWndManagerWindowHandler *handler) override
  {
    if (!handler)
      return;

    auto it = eastl::find(handlers.begin(), handlers.end(), handler);
    if (it == handlers.end())
      return;

    handlers.erase(it);
  }

  virtual void reset() override
  {
    while (!windows.empty())
    {
      G_VERIFY(removeWindow(windows[0]));
    }
  }

  virtual void show(WindowSizeInit size) override {}

  virtual void clientToScreen(int &x, int &y) override {}

  virtual void *getMainWindow() const override { return mainHwnd; }
  virtual void *getFirstWindow() const override { return nullptr; }

  virtual void *splitWindowF(void *old_handle, void *new_handle, float new_wsize, WindowAlign new_walign) override { return nullptr; }

  virtual void *splitNeighbourWindowF(void *old_handle, void *new_handle, float new_wsize, WindowAlign new_walign) override
  {
    return nullptr;
  }

  virtual bool resizeWindowF(void *handle, float new_wsize) override { return false; }

  virtual bool fixWindow(void *handle, bool fix) override { return false; }

  virtual bool removeWindow(void *handle) override
  {
    auto it = eastl::find(windows.begin(), windows.end(), handle);
    if (it == windows.end())
      return false;

    windows.erase(it);

    for (int i = 0; i < handlers.size(); ++i)
      if (handlers[i]->onWmDestroyWindow(handle))
        return true;

    return false;
  }

  virtual bool getMovableFlag(void *handle) override { return false; }

  virtual void setWindowType(void *handle, int type) override
  {
    G_ASSERT(!handle);

    for (int i = 0; i < handlers.size(); ++i)
    {
      handle = handlers[i]->onWmCreateWindow(type);
      if (handle)
      {
        windows.push_back(handle);
        return;
      }
    }
  }

  virtual int getWindowType(void *handle) override { return 0; }

  virtual void getWindowClientSize(void *handle, unsigned &width, unsigned &height) override
  {
    width = 0;
    height = 0;

    if (handle == mainHwnd)
    {
      RECT rc;
      if (GetClientRect((HWND)mainHwnd, &rc))
      {
        width = rc.right - rc.left;
        height = rc.bottom - rc.top;
      }
    }
  }

  virtual bool getWindowPosSize(void *handle, int &x, int &y, unsigned &width, unsigned &height) override { return false; }
  virtual int getSplitterControlSize() const override { return 0; }

  virtual void setHeader(void *handle, HeaderPos header_pos) override {}
  virtual void setCaption(void *handle, const char *caption) override {}
  virtual void setMinSize(void *handle, hdpi::Px width, hdpi::Px height) override {}
  virtual void setMenuArea(void *handle, hdpi::Px width, hdpi::Px height) override {}

  virtual IMenu *getMainMenu() override { return nullptr; }
  virtual IMenu *getMenu(void *handle) override { return nullptr; }

  virtual bool init3d(const char *drv_name, const DataBlock *blkTexStreaming) override
  {
    // tools3d::init creates an invisible window and passes that to d3d, which sets the focus to it in
    // set_render_window_params. Hence the need for re-focusing the main window.
    const bool result = tools3d::init(drv_name, blkTexStreaming);
    SetFocus((HWND)mainHwnd);
    return result;
  }

  virtual void addAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift) override
  {
    Accelerator accelerator(cmd_id, v_key, ctrl, alt, shift);
    accelerators.push_back(eastl::move(accelerator));
  }

  virtual void addAcceleratorUp(unsigned cmd_id, unsigned v_key, bool ctrl, bool alt, bool shift) override
  {
    G_ASSERT(!ctrl);
    G_ASSERT(!alt);
    G_ASSERT(!shift);

    Accelerator accelerator(cmd_id, v_key, ctrl, alt, shift);
    acceleratorUps.push_back(eastl::move(accelerator));
  }

  virtual void addViewportAccelerator(unsigned cmd_id, unsigned v_key, bool ctrl = false, bool alt = false, bool shift = false,
    bool allow_repeat = false) override
  {
    Accelerator accelerator(cmd_id, v_key, ctrl, alt, shift, allow_repeat);
    viewportAccelerators.push_back(eastl::move(accelerator));
  }

  virtual void clearAccelerators() override
  {
    accelerators.clear();
    acceleratorUps.clear();
    viewportAccelerators.clear();
  }

  virtual unsigned processImguiAccelerator() override
  {
    for (const Accelerator &accelerator : accelerators)
      if (ImGui::IsKeyChordPressed(accelerator.keyChord, accelerator.inputFlags, ImGuiKeyOwner_NoOwner))
        return accelerator.cmdId;

    // NOTE: ImGui porting: this should be ImGuiKeyOwner_NoOwner instead of ImGuiKeyOwner_Any but then CM_TAB_RELEASE
    // (currently the only one command that uses accelerator up) would not work.
    for (const Accelerator &accelerator : acceleratorUps)
      if (ImGui::IsKeyReleased((ImGuiKey)accelerator.keyChord, ImGuiKeyOwner_Any))
        return accelerator.cmdId;

    return 0;
  }

  virtual unsigned processImguiViewportAccelerator() override
  {
    for (const Accelerator &accelerator : viewportAccelerators)
      if (ImGui::IsKeyChordPressed(accelerator.keyChord, accelerator.inputFlags, ImGuiKeyOwner_NoOwner))
        return accelerator.cmdId;

    return 0;
  }

  virtual void initCustomMouseCursors(const char *path) override
  {
    String cursorPath(128, "%scursor_additional_click.cur", path);
    cursorAdditionalClick = LoadImage(0, cursorPath, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!cursorAdditionalClick)
      logwarn("Failed to load cursor \"%s\".", cursorPath.c_str());
  }

  virtual void updateImguiMouseCursor() override
  {
    ImGuiMouseCursor newCursor = ImGui::GetMouseCursor();
    if (newCursor == imguiMouseCursor)
      return;

    imguiMouseCursor = newCursor;
    updateOsMouseCursor();
  }

  bool updateOsMouseCursor()
  {
    if (!ImGui::GetCurrentContext())
      return false;

    LPTSTR windowsCursor;
    switch (imguiMouseCursor)
    {
      case ImGuiMouseCursor_Arrow: windowsCursor = IDC_ARROW; break;
      case ImGuiMouseCursor_TextInput: windowsCursor = IDC_IBEAM; break;
      case ImGuiMouseCursor_ResizeAll: windowsCursor = IDC_SIZEALL; break;
      case ImGuiMouseCursor_ResizeEW: windowsCursor = IDC_SIZEWE; break;
      case ImGuiMouseCursor_ResizeNS: windowsCursor = IDC_SIZENS; break;
      case ImGuiMouseCursor_ResizeNESW: windowsCursor = IDC_SIZENESW; break;
      case ImGuiMouseCursor_ResizeNWSE: windowsCursor = IDC_SIZENWSE; break;
      case ImGuiMouseCursor_Hand: windowsCursor = IDC_HAND; break;
      case ImGuiMouseCursor_NotAllowed: windowsCursor = IDC_NO; break;
      case ImGuiMouseCursor_None: ::SetCursor(nullptr); return true;

      case EDITOR_CORE_CURSOR_ADDITIONAL_CLICK:
        if (cursorAdditionalClick)
        {
          SetCursor((HCURSOR)cursorAdditionalClick);
          return true;
        }

        windowsCursor = IDC_ARROW;
        break;

      default: windowsCursor = IDC_ARROW; break;
    }

    SetCursor(LoadCursor(nullptr, windowsCursor));
    return true;
  }

private:
  struct Accelerator
  {
    Accelerator(unsigned cmd_id, unsigned in_key, bool in_ctrl, bool in_alt, bool in_shift, bool allow_repeat = false) : cmdId(cmd_id)
    {
      G_ASSERT(cmd_id != 0);

      const ImGuiKey key = map_virtual_key_to_imgui_key(in_key);
      G_ASSERT(key != ImGuiKey_None);

      keyChord = (ImGuiKeyChord)key;
      if (in_ctrl)
        keyChord |= ImGuiMod_Ctrl;
      if (in_alt)
        keyChord |= ImGuiMod_Alt;
      if (in_shift)
        keyChord |= ImGuiMod_Shift;

      inputFlags = ImGuiInputFlags_None;
      if (allow_repeat)
        inputFlags |= ImGuiInputFlags_Repeat;
    }

    unsigned cmdId;
    ImGuiKeyChord keyChord;
    ImGuiInputFlags inputFlags;
  };

  void *mainHwnd;
  dag::Vector<void *> windows;
  dag::Vector<IWndManagerWindowHandler *> handlers;
  dag::Vector<Accelerator> accelerators;
  dag::Vector<Accelerator> acceleratorUps;
  dag::Vector<Accelerator> viewportAccelerators;
  ImGuiMouseCursor imguiMouseCursor = ImGuiMouseCursor_None;
  HANDLE cursorAdditionalClick = 0;
};

static const char *WNDCLASS_EDITOR = "EDITOR_LAYOUT_DE_WINDOW";

static LRESULT CALLBACK main_window_proc(HWND h_wnd, unsigned msg, WPARAM w_param, LPARAM l_param)
{
  if (msg == WM_NCCREATE)
  {
    CREATESTRUCT *cs = (CREATESTRUCT *)l_param;
    SetWindowLongPtr(h_wnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);

    return DefWindowProc(h_wnd, msg, w_param, l_param);
  }

  EditorMainWindow *w = (EditorMainWindow *)GetWindowLongPtr(h_wnd, GWLP_USERDATA);
  if (w)
    return w->windowProc(h_wnd, msg, (void *)w_param, (void *)l_param);

  return DefWindowProc(h_wnd, msg, w_param, l_param);
}

EditorMainWindow::EditorMainWindow(IWndManagerEventHandler &event_handler) : eventHandler(event_handler) {}

void EditorMainWindow::run(const char *caption, const char *icon, FileDropHandler file_drop_handler)
{
  HINSTANCE hInstance = GetModuleHandle(nullptr);

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpszClassName = WNDCLASS_EDITOR;
  wc.hInstance = hInstance;
  wc.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
  wc.lpfnWndProc = (WNDPROC)main_window_proc;
  wc.hCursor = LoadCursor(0, IDC_ARROW);
  wc.hIcon = LoadIcon(hInstance, icon);
  RegisterClassEx(&wc);

  const int x = 0;
  const int y = 0;
  const int width = GetSystemMetrics(SM_CXFULLSCREEN);
  const int height = GetSystemMetrics(SM_CYFULLSCREEN) + GetSystemMetrics(SM_CYCAPTION);

  mainHwnd =
    CreateWindowEx(0, wc.lpszClassName, caption, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MAXIMIZE, x, y, width, height, 0, 0, 0, this);

  if (file_drop_handler)
  {
    fileDropHandler = file_drop_handler;
    DragAcceptFiles((HWND)mainHwnd, true);
  }

  onMainWindowCreated();

  ::dagor_reset_spent_work_time();
  for (;;) // infinite loop!
    if (d3d::is_inited())
      ::dagor_work_cycle();
    else
      ::dagor_idle_cycle();
}

void EditorMainWindow::onMainWindowCreated()
{
  G_ASSERT(!wndManager);
  wndManager.reset(new ImguiWndManager(mainHwnd));

  if (!::symhlp_load("daKernel-dev.dll"))
    DEBUG_CTX("can't load sym for: %s", "daKernel-dev.dll");

  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  eventHandler.onInit(wndManager.get());
}

void EditorMainWindow::close()
{
  if (eventHandler.onClose())
  {
    PostMessage((HWND)mainHwnd, WM_DESTROY, 0, 0);
  }
}

void EditorMainWindow::onClose()
{
  eventHandler.onDestroy();

  PostQuitMessage(0);
}

bool EditorMainWindow::onMouseButtonDown(int button)
{
  if (!ImGui::GetCurrentContext())
    return false;

  if (GetCapture() != (HWND)mainHwnd)
    SetCapture((HWND)mainHwnd);

  ImGui::GetIO().AddMouseButtonEvent(button, true);
  return ImGui::GetIO().WantCaptureMouse;
}

bool EditorMainWindow::onMouseButtonUp(int button)
{
  if (!ImGui::GetCurrentContext())
    return false;

  if (GetCapture() == (HWND)mainHwnd)
  {
    bool anyMouseButtonDown = false;
    for (int i = 0; i < ImGuiMouseButton_COUNT; ++i)
    {
      // Ignore the currently released button, it will be only processed in ImGui::NewFrame.
      if (i != button && ImGui::IsMouseDown(i))
      {
        anyMouseButtonDown = true;
        break;
      }
    }

    if (!anyMouseButtonDown)
      ReleaseCapture();
  }

  ImGui::GetIO().AddMouseButtonEvent(button, false);
  return ImGui::GetIO().WantCaptureMouse;
}

intptr_t EditorMainWindow::windowProc(void *h_wnd, unsigned msg, void *w_param, void *l_param)
{
  switch (msg)
  {
    case WM_CREATE:
    {
      G_ASSERT(!mainHwnd);
      mainHwnd = h_wnd;
      return 0;
    }

    case WM_DESTROY: onClose(); break;

    case WM_CLOSE: close(); return 0;

    // In some rare cases there was an issue on relying on WM_ACTIVATEAPP alone, so we use WM_ACTIVATE as a fallback for activation. We
    // do not use it for deactivation -- a missed deactivation is not really a big problem. By doing this way also has a benefit:
    // focusing the CoolConsole window is not handled as a deactivation.
    case WM_ACTIVATE:
    {
      using namespace workcycle_internal;

      const int fActive = IsIconic((HWND)h_wnd) ? WA_INACTIVE : LOWORD(w_param); // activation flag
      if (!::dgs_app_active && (fActive == WA_ACTIVE || fActive == WA_CLICKACTIVE))
      {
        set_priority(true);
        dgs_app_active = true;
        _fpreset();
      }
    }
    break;

    case WM_ACTIVATEAPP:
    {
      using namespace workcycle_internal;

      const bool n_app_active = w_param != 0;
      if (n_app_active && !::dgs_app_active)
      {
        set_priority(true);
        dgs_app_active = true;
        _fpreset();
      }
      else if (!n_app_active && ::dgs_app_active)
      {
        if (interlocked_relaxed_load(enable_idle_priority))
          set_priority(false);
        dgs_app_active = false;
      }
    }
    break;

    case WM_SETFOCUS:
      if (ImGui::GetCurrentContext())
        ImGui::GetIO().AddFocusEvent(true);
      return 0;

    case WM_KILLFOCUS:
      if (ImGui::GetCurrentContext())
        ImGui::GetIO().AddFocusEvent(false);
      return 0;

    case WM_PAINT:
      PAINTSTRUCT ps;
      BeginPaint((HWND)mainHwnd, &ps);
      EndPaint((HWND)mainHwnd, &ps);
      return 0;

    case WM_ERASEBKGND:
      if (dagor_get_current_game_scene())
        return 1;
      break;

    case WM_MOUSEMOVE:
      if (ImGui::GetCurrentContext())
        ImGui::GetIO().AddMousePosEvent(GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param));
      break;

    case WM_CHAR:
      if (ImGui::GetCurrentContext())
      {
        ImGui::GetIO().AddInputCharacter((unsigned)(uintptr_t)w_param);
        if (ImGui::GetIO().WantCaptureKeyboard)
          return 0;
      }
      break;

    case WM_KEYDOWN:
    case WM_KEYUP:
      if (ImGui::GetCurrentContext())
      {
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, wingw::is_key_pressed(wingw::V_ALT));
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, wingw::is_key_pressed(wingw::V_CONTROL));
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift, wingw::is_key_pressed(wingw::V_SHIFT));

        const ImGuiKey key = map_virtual_key_to_imgui_key((WPARAM)w_param);
        if (key != ImGuiKey_None)
        {
          const bool keyDown = msg == WM_KEYDOWN;
          ImGui::GetIO().AddKeyEvent(key, keyDown);
        }

        if (ImGui::GetIO().WantCaptureKeyboard)
          return 0;
      }
      break;

    case WM_LBUTTONDOWN:
      if (onMouseButtonDown(ImGuiMouseButton_Left))
        return 0;
      break;

    case WM_MBUTTONDOWN:
      if (onMouseButtonDown(ImGuiMouseButton_Middle))
        return 0;
      break;

    case WM_RBUTTONDOWN:
      if (onMouseButtonDown(ImGuiMouseButton_Right))
        return 0;
      break;

    case WM_LBUTTONUP:
      if (onMouseButtonUp(ImGuiMouseButton_Left))
        return 0;
      break;

    case WM_RBUTTONUP:
      if (onMouseButtonUp(ImGuiMouseButton_Right))
        return 0;
      break;

    case WM_MBUTTONUP:
      if (onMouseButtonUp(ImGuiMouseButton_Middle))
        return 0;
      break;

    case WM_MOUSEWHEEL:
      if (ImGui::GetCurrentContext())
      {
        ImGui::GetIO().AddMouseWheelEvent(0.0f, ((float)GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA);
        if (ImGui::GetIO().WantCaptureMouse)
          return 0;
      }
      break;

    case WM_MOUSEHWHEEL:
      if (ImGui::GetCurrentContext())
      {
        ImGui::GetIO().AddMouseWheelEvent(((float)GET_WHEEL_DELTA_WPARAM(w_param)) / WHEEL_DELTA, 0.0f);
        if (ImGui::GetIO().WantCaptureMouse)
          return 0;
      }
      break;

    case WM_SETCURSOR:
      // Let Windows do the cursor setting if the cursor is not in the client area to make the resizing cursors work.
      if (LOWORD(l_param) == HTCLIENT && wndManager.get() && static_cast<ImguiWndManager *>(wndManager.get())->updateOsMouseCursor())
        return 1;
      break;

    case WM_DROPFILES:
      if (fileDropHandler)
      {
        HDROP hdrop = (HDROP)w_param;
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
            return 0;
        }
      }
      break;
  }

  return DefWindowProc((HWND)mainHwnd, msg, (WPARAM)w_param, (LPARAM)l_param);
}
