// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiPointing.h>
#include <generic/dag_initOnDemand.h>
#include <gui/dag_imgui.h>
#include <startup/dag_restart.h>
#include <startup/dag_inpDevClsDrv.h>

#include <EASTL/optional.h>
#include <imgui/imgui.h>

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
    if (!mouse || !ImGui::GetCurrentContext())
      return;

    // TODO: tools Linux porting: multi-viewport support
    // // ImGui expects absolute coordinates when viewports are enabled.
    // if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
    //   ::ClientToScreen((HWND)h_wnd, &pt);

    ImGui::GetIO().AddMousePosEvent(mouse->getRawState().mouse.x, mouse->getRawState().mouse.y);
  }

  void gmcMouseButtonDown(HumanInput::IGenPointing *mouse, int btn_idx) override
  {
    if (!ImGui::GetCurrentContext())
      return;

    ImGui::GetIO().AddMouseButtonEvent(btn_idx, true);
  }

  void gmcMouseButtonUp(HumanInput::IGenPointing *mouse, int btn_idx) override
  {
    if (!ImGui::GetCurrentContext())
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
  restart_proc.demandInit();
  add_restart_proc(restart_proc);
}
