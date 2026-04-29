// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/3d/dag_info.h>
#include <generic/dag_initOnDemand.h>
#include <gui/dag_imgui.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <startup/dag_restart.h>
#include <startup/dag_inpDevClsDrv.h>

#include <EASTL/optional.h>
#include <imgui/imgui.h>

#if _TARGET_PC_WIN
#include <windows.h>
#elif _TARGET_PC_LINUX
#include <osApiWrappers/dag_linuxGUI.h>
#endif

extern IPoint2 ec_mouse_cursor_pos;

// NOTE: this is only used on Linux.

class EditorCoreImguiInputHandler : public HumanInput::IGenKeyboardClient, public HumanInput::IGenPointingClient
{
public:
  // IGenKeyboardClient

  void attached(HumanInput::IGenKeyboard *kbd) override {}
  void detached(HumanInput::IGenKeyboard *kbd) override {}
  void gkcMagicCtrlAltEnd() override {}
  void gkcMagicCtrlAltF() override {}

  void gkcButtonDown(HumanInput::IGenKeyboard *kbd, int btn_idx, bool repeat, wchar_t wchar) override
  {
    if (!kbd || !ImGui::GetCurrentContext())
      return;

    const unsigned modKey = kbd->getRawState().shifts;
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Alt, (modKey & HumanInput::KeyboardRawState::ALT_BITS) != 0);
    io.AddKeyEvent(ImGuiMod_Ctrl, (modKey & HumanInput::KeyboardRawState::CTRL_BITS) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (modKey & HumanInput::KeyboardRawState::SHIFT_BITS) != 0);

    if (wchar != 0)
      io.AddInputCharacterUTF16(wchar);

    const eastl::optional<ImGuiKey> key = map_dagor_key_to_imgui(btn_idx);
    if (key.has_value())
      io.AddKeyEvent(key.value(), true);
  }

  void gkcButtonUp(HumanInput::IGenKeyboard *kbd, int btn_idx) override
  {
    if (!kbd || !ImGui::GetCurrentContext())
      return;

    const unsigned modKey = kbd->getRawState().shifts;
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Alt, (modKey & HumanInput::KeyboardRawState::ALT_BITS) != 0);
    io.AddKeyEvent(ImGuiMod_Ctrl, (modKey & HumanInput::KeyboardRawState::CTRL_BITS) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (modKey & HumanInput::KeyboardRawState::SHIFT_BITS) != 0);

    const eastl::optional<ImGuiKey> key = map_dagor_key_to_imgui(btn_idx);
    if (key.has_value())
      io.AddKeyEvent(key.value(), false);
  }

  // IGenPointingClient

  void attached(HumanInput::IGenPointing *mouse) override {}
  void detached(HumanInput::IGenPointing *mouse) override {}

  void gmcMouseMove(HumanInput::IGenPointing *mouse, float dx, float dy) override
  {
    if (!ImGui::GetCurrentContext())
      return;

      // We cannot use mouse->getRawState().mouse.x and y because they are relative to the windows, and scaled by the window's visible
      // size. See WinMouseDevice::getCursorPos().

#if _TARGET_PC_WIN
    POINT cursorPos = {0, 0};
    GetCursorPos(&cursorPos);

    // ImGui expects absolute coordinates when viewports are enabled.
    G_ASSERT((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0);
    ImGui::GetIO().AddMousePosEvent(cursorPos.x, cursorPos.y);
#elif _TARGET_PC_LINUX
    linux_GUI::get_absolute_cursor_position(ec_mouse_cursor_pos.x, ec_mouse_cursor_pos.y);
    // ImGui expects absolute coordinates when viewports are enabled.
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
    {
      int wx, wy;
      linux_GUI::get_window_position(win32_get_main_wnd(), wx, wy);
      ImGui::GetIO().AddMousePosEvent(ec_mouse_cursor_pos.x - wx, ec_mouse_cursor_pos.y - wy);
    }
    else
    {
      ImGui::GetIO().AddMousePosEvent(ec_mouse_cursor_pos.x, ec_mouse_cursor_pos.y);
    }
#else
#error Unsupported platform!
#endif
  }

  void gmcMouseButtonDown(HumanInput::IGenPointing *mouse, int btn_idx) override
  {
    if (!ImGui::GetCurrentContext() || btn_idx >= ImGuiMouseButton_COUNT)
      return;

    ImGui::GetIO().AddMouseButtonEvent(btn_idx, true);
  }

  void gmcMouseButtonUp(HumanInput::IGenPointing *mouse, int btn_idx) override
  {
    if (!ImGui::GetCurrentContext() || btn_idx >= ImGuiMouseButton_COUNT)
      return;

    ImGui::GetIO().AddMouseButtonEvent(btn_idx, false);
  }

  void gmcMouseWheel(HumanInput::IGenPointing *mouse, int scroll) override
  {
    if (!ImGui::GetCurrentContext())
      return;

    ImGui::GetIO().AddMouseWheelEvent(0.0f, static_cast<float>(scroll) / EC_WHEEL_DELTA);
  }
};

static EditorCoreImguiInputHandler input_handler;

class EditorCoreInputRestartProc : public SRestartProc
{
public:
  // Using the same flags as KeybRestartProcWnd and MouseRestartProcWnd.
  EditorCoreInputRestartProc() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}

  const char *procname() override { return "EditorCoreInput"; }

  void startup() override
  {
    if (global_cls_drv_kbd)
      global_cls_drv_kbd->useDefClient(&input_handler);

    if (global_cls_drv_pnt)
    {
      global_cls_drv_pnt->useDefClient(&input_handler);
      global_cls_drv_pnt->hideMouseCursor(false);
    }
  }

  void shutdown() override
  {
    if (global_cls_drv_kbd)
      global_cls_drv_kbd->useDefClient(nullptr);
    if (global_cls_drv_pnt)
      global_cls_drv_pnt->useDefClient(nullptr);
  }
};

static InitOnDemand<EditorCoreInputRestartProc> restart_proc;

void editor_core_initialize_input_handler()
{
  if (d3d::is_stub_driver())
    return;
  restart_proc.demandInit();
  add_restart_proc(restart_proc);
}
