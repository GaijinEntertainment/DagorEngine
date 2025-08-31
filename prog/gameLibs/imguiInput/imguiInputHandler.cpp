// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "imguiInputHandler.h"

#include "imgui/imguiInput.h"

#include <imgui/imgui.h>

#include <gui/dag_imgui.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiKeybData.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <drv/hid/dag_hiPointingData.h>

DearImGuiInputHandler::DearImGuiInputHandler()
{
  ImGuiIO &io = ImGui::GetIO();

  enableMouseMode();

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  io.BackendPlatformName = "imgui_impl_dagor";
}

bool DearImGuiInputHandler::ehIsActive() const { return imgui_get_state() == ImGuiState::ACTIVE; }

static ImVec2 get_imgui_view_pos()
{
  if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    return ImVec2(0, 0);
  return ImGui::GetMainViewport()->Pos;
}

bool DearImGuiInputHandler::gmehMove(const Context &ctx, float dx, float dy)
{
  enableMouseMode();
  ImVec2 viewPos = get_imgui_view_pos();
  ImGui::GetIO().MousePos = ImVec2(ctx.msX + viewPos.x, ctx.msY + viewPos.y);
  return !hybridInput || ImGui::GetIO().WantCaptureMouse;
}

bool DearImGuiInputHandler::gmehButtonDown(const Context &ctx, int btn_idx)
{
  enableMouseMode();
  ImGui::GetIO().MouseDown[btn_idx] = true;
  return !hybridInput || ImGui::GetIO().WantCaptureMouse;
}

bool DearImGuiInputHandler::gmehButtonUp(const Context &ctx, int btn_idx)
{
  enableMouseMode();
  ImGui::GetIO().MouseDown[btn_idx] = false;
  return !hybridInput || ImGui::GetIO().WantCaptureMouse;
}

bool DearImGuiInputHandler::gmehWheel(const Context &ctx, int dz)
{
  enableMouseMode();
  const float mouseWheelSensitivity = 1.f / 120.f; // This is based on WHEEL_DELTA defined in WinUser.h
  ImGui::GetIO().MouseWheel += dz * mouseWheelSensitivity;
  return !hybridInput || ImGui::GetIO().WantCaptureMouse;
}

bool DearImGuiInputHandler::gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wchar)
{
  bool ctrl, shift, alt;
  getModifiers(ctx, ctrl, shift, alt);
  if (imgui_handle_special_keys_down(ctrl, shift, alt, btn_idx, ctx.keyModif))
    return true;
  ImGuiIO &io = ImGui::GetIO();

  io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
  io.AddKeyEvent(ImGuiMod_Shift, shift);
  io.AddKeyEvent(ImGuiMod_Alt, alt);

  if (wchar != 0)
    io.AddInputCharacterUTF16(wchar);

  eastl::optional<ImGuiKey> imgui_key = map_dagor_key_to_imgui(btn_idx);
  if (imgui_key.has_value())
    io.AddKeyEvent(imgui_key.value(), true);

  return !hybridInput || ImGui::GetIO().WantCaptureKeyboard;
}

bool DearImGuiInputHandler::gkehButtonUp(const Context &ctx, int btn_idx)
{
  bool ctrl, shift, alt;
  getModifiers(ctx, ctrl, shift, alt);
  if (imgui_handle_special_keys_up(ctrl, shift, alt, btn_idx, ctx.keyModif))
    return true;
  ImGuiIO &io = ImGui::GetIO();

  io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
  io.AddKeyEvent(ImGuiMod_Shift, shift);
  io.AddKeyEvent(ImGuiMod_Alt, alt);

  eastl::optional<ImGuiKey> imgui_key = map_dagor_key_to_imgui(btn_idx);
  if (imgui_key.has_value())
    io.AddKeyEvent(imgui_key.value(), false);

  return !hybridInput || ImGui::GetIO().WantCaptureKeyboard;
}

bool DearImGuiInputHandler::gjehStateChanged(const Context &ctx, HumanInput::IGenJoystick *joy, int joy_ord_id)
{
  if (imgui_handle_special_controller_combinations(joy))
    return true;

  ImGuiIO &io = ImGui::GetIO();

  auto mapButton = [&](ImGuiKey nav, int btn_id) { io.AddKeyEvent(nav, joy->getButtonState(btn_id)); };
  mapButton(ImGuiKey_GamepadFaceDown, HumanInput::JOY_XINPUT_REAL_BTN_A);
  mapButton(ImGuiKey_GamepadFaceRight, HumanInput::JOY_XINPUT_REAL_BTN_B);
  mapButton(ImGuiKey_GamepadFaceLeft, HumanInput::JOY_XINPUT_REAL_BTN_X);
  mapButton(ImGuiKey_GamepadFaceUp, HumanInput::JOY_XINPUT_REAL_BTN_Y);
  mapButton(ImGuiKey_GamepadDpadLeft, HumanInput::JOY_XINPUT_REAL_BTN_D_LEFT);
  mapButton(ImGuiKey_GamepadDpadRight, HumanInput::JOY_XINPUT_REAL_BTN_D_RIGHT);
  mapButton(ImGuiKey_GamepadDpadUp, HumanInput::JOY_XINPUT_REAL_BTN_D_UP);
  mapButton(ImGuiKey_GamepadDpadDown, HumanInput::JOY_XINPUT_REAL_BTN_D_DOWN);
  mapButton(ImGuiKey_GamepadL1, HumanInput::JOY_XINPUT_REAL_BTN_L_SHOULDER);
  mapButton(ImGuiKey_GamepadR1, HumanInput::JOY_XINPUT_REAL_BTN_R_SHOULDER);

  const float lAxisH = joy->getAxisPosRaw(HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_H) / float(HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  const float lAxisV = joy->getAxisPosRaw(HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_V) / float(HumanInput::JOY_XINPUT_MAX_AXIS_VAL);

  io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickLeft, lAxisH < 0.f, -lAxisH);
  io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickRight, lAxisH > 0.f, +lAxisH);
  io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickUp, lAxisV < 0.f, -lAxisV);
  io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickDown, lAxisV > 0.f, +lAxisV);

  return true;
}

bool DearImGuiInputHandler::gtehTouchBegan(const Context &, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch)
{
  if (activeTouchIdx >= 0)
    return false;
  if (touch.touchSrc != HumanInput::PointingRawState::Touch::TS_emuTouchScreen)
    enableTouchMode();
  activeTouchIdx = touch_idx;
  ImVec2 viewPos = get_imgui_view_pos();
  ImGui::GetIO().MousePos = ImVec2(touch.x0 + viewPos.x, touch.y0 + viewPos.y);
  ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = true;
  return true;
}

bool DearImGuiInputHandler::gtehTouchMoved(const Context &, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch)
{
  if (activeTouchIdx != touch_idx)
    return false;
  if (touch.touchSrc != HumanInput::PointingRawState::Touch::TS_emuTouchScreen)
    enableTouchMode();
  ImVec2 viewPos = get_imgui_view_pos();
  ImGui::GetIO().MousePos = ImVec2(touch.x + viewPos.x, touch.y + viewPos.y);
  return true;
}

bool DearImGuiInputHandler::gtehTouchEnded(const Context &, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch)
{
  if (activeTouchIdx != touch_idx)
    return false;
  if (touch.touchSrc != HumanInput::PointingRawState::Touch::TS_emuTouchScreen)
    enableTouchMode();
  ImVec2 viewPos = get_imgui_view_pos();
  ImGui::GetIO().MousePos = ImVec2(touch.x + viewPos.x, touch.y + viewPos.y);
  ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = false;
  activeTouchIdx = -1;
  return true;
}

void DearImGuiInputHandler::getModifiers(const Context &ctx, bool &ctrl, bool &shift, bool &alt)
{
  ctrl = ctx.keyModif & HumanInput::KeyboardRawState::CTRL_BITS;
  shift = ctx.keyModif & HumanInput::KeyboardRawState::SHIFT_BITS;
  alt = ctx.keyModif & HumanInput::KeyboardRawState::ALT_BITS;
}

void DearImGuiInputHandler::enableTouchMode()
{
  if (!ImGui::GetIO().MouseDrawCursor)
    return;
  ImGui::GetIO().MouseDrawCursor = false;
  ImGui::GetStyle().TouchExtraPadding = ImVec2(10, 10);
}

void DearImGuiInputHandler::enableMouseMode()
{
  if (ImGui::GetIO().MouseDrawCursor && drawMouseCursor)
    return;
  ImGui::GetIO().MouseDrawCursor = drawMouseCursor;
  ImGui::GetStyle().TouchExtraPadding = ImVec2(0, 0);
}
