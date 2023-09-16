#include "imguiInputHandler.h"

#include "imgui/imguiInput.h"

#include <imgui/imgui.h>

#include <gui/dag_imgui.h>
#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiKeybData.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiXInputMappings.h>

DearImGuiInputHandler::DearImGuiInputHandler()
{
  ImGuiIO &io = ImGui::GetIO();

  enableMouseMode();

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
  io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
  io.BackendPlatformName = "imgui_impl_dagor";

  io.KeyMap[ImGuiKey_Tab] = HumanInput::DKEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = HumanInput::DKEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = HumanInput::DKEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = HumanInput::DKEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = HumanInput::DKEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = HumanInput::DKEY_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = HumanInput::DKEY_NEXT;
  io.KeyMap[ImGuiKey_Home] = HumanInput::DKEY_HOME;
  io.KeyMap[ImGuiKey_End] = HumanInput::DKEY_END;
  io.KeyMap[ImGuiKey_Insert] = HumanInput::DKEY_INSERT;
  io.KeyMap[ImGuiKey_Delete] = HumanInput::DKEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = HumanInput::DKEY_BACK;
  io.KeyMap[ImGuiKey_Space] = HumanInput::DKEY_SPACE;
  io.KeyMap[ImGuiKey_Enter] = HumanInput::DKEY_RETURN;
  io.KeyMap[ImGuiKey_Escape] = HumanInput::DKEY_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = HumanInput::DKEY_NUMPADENTER;
  io.KeyMap[ImGuiKey_A] = HumanInput::DKEY_A;
  io.KeyMap[ImGuiKey_C] = HumanInput::DKEY_C;
  io.KeyMap[ImGuiKey_V] = HumanInput::DKEY_V;
  io.KeyMap[ImGuiKey_X] = HumanInput::DKEY_X;
  io.KeyMap[ImGuiKey_Y] = HumanInput::DKEY_Y;
  io.KeyMap[ImGuiKey_Z] = HumanInput::DKEY_Z;
}

bool DearImGuiInputHandler::ehIsActive() const { return imgui_get_state() == ImGuiState::ACTIVE; }

bool DearImGuiInputHandler::gmehMove(const Context &ctx, float dx, float dy)
{
  enableMouseMode();
  ImGui::GetIO().MousePos = ImVec2(ctx.msX + viewPortOffsetX, ctx.msY + viewPortOffsetY);
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
  if (wchar != 0)
    io.AddInputCharacterUTF16(wchar);
  io.KeysDown[btn_idx] = 1;
  io.KeyCtrl = ctrl;
  io.KeyShift = shift;
  io.KeyAlt = alt;
  return !hybridInput || ImGui::GetIO().WantCaptureKeyboard;
}

bool DearImGuiInputHandler::gkehButtonUp(const Context &ctx, int btn_idx)
{
  bool ctrl, shift, alt;
  getModifiers(ctx, ctrl, shift, alt);
  if (imgui_handle_special_keys_up(ctrl, shift, alt, btn_idx, ctx.keyModif))
    return true;
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[btn_idx] = 0;
  io.KeyCtrl = ctrl;
  io.KeyShift = shift;
  io.KeyAlt = alt;
  return !hybridInput || ImGui::GetIO().WantCaptureKeyboard;
}

bool DearImGuiInputHandler::gjehStateChanged(const Context &ctx, HumanInput::IGenJoystick *joy, int joy_ord_id)
{
  if (imgui_handle_special_controller_combinations(joy))
    return true;

  ImGuiIO &io = ImGui::GetIO();

  auto mapButton = [&](ImGuiNavInput_ nav, int btn_id) { io.NavInputs[nav] = joy->getButtonState(btn_id) ? 1.f : 0.f; };
  mapButton(ImGuiNavInput_Activate, HumanInput::JOY_XINPUT_REAL_BTN_A);
  mapButton(ImGuiNavInput_Cancel, HumanInput::JOY_XINPUT_REAL_BTN_B);
  mapButton(ImGuiNavInput_Menu, HumanInput::JOY_XINPUT_REAL_BTN_X);
  mapButton(ImGuiNavInput_Input, HumanInput::JOY_XINPUT_REAL_BTN_Y);
  mapButton(ImGuiNavInput_DpadLeft, HumanInput::JOY_XINPUT_REAL_BTN_D_LEFT);
  mapButton(ImGuiNavInput_DpadRight, HumanInput::JOY_XINPUT_REAL_BTN_D_RIGHT);
  mapButton(ImGuiNavInput_DpadUp, HumanInput::JOY_XINPUT_REAL_BTN_D_UP);
  mapButton(ImGuiNavInput_DpadDown, HumanInput::JOY_XINPUT_REAL_BTN_D_DOWN);
  mapButton(ImGuiNavInput_FocusPrev, HumanInput::JOY_XINPUT_REAL_BTN_L_SHOULDER);
  mapButton(ImGuiNavInput_FocusNext, HumanInput::JOY_XINPUT_REAL_BTN_R_SHOULDER);
  mapButton(ImGuiNavInput_TweakSlow, HumanInput::JOY_XINPUT_REAL_BTN_L_SHOULDER);
  mapButton(ImGuiNavInput_TweakFast, HumanInput::JOY_XINPUT_REAL_BTN_R_SHOULDER);

  const float lAxisH = joy->getAxisPosRaw(HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_H) / float(HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  const float lAxisV = joy->getAxisPosRaw(HumanInput::JOY_XINPUT_REAL_AXIS_L_THUMB_V) / float(HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
  io.NavInputs[ImGuiNavInput_LStickLeft] = -lAxisH;
  io.NavInputs[ImGuiNavInput_LStickRight] = +lAxisH;
  io.NavInputs[ImGuiNavInput_LStickUp] = +lAxisV;
  io.NavInputs[ImGuiNavInput_LStickDown] = -lAxisV;

  return true;
}

bool DearImGuiInputHandler::gtehTouchBegan(const Context &, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch)
{
  if (activeTouchIdx >= 0)
    return false;
  enableTouchMode();
  activeTouchIdx = touch_idx;
  ImGui::GetIO().MousePos = ImVec2(touch.x0 + viewPortOffsetX, touch.y0 + viewPortOffsetY);
  ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = true;
  return true;
}

bool DearImGuiInputHandler::gtehTouchMoved(const Context &, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch)
{
  if (activeTouchIdx != touch_idx)
    return false;
  enableTouchMode();
  ImGui::GetIO().MousePos = ImVec2(touch.x + viewPortOffsetX, touch.y + viewPortOffsetY);
  return true;
}

bool DearImGuiInputHandler::gtehTouchEnded(const Context &, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch)
{
  if (activeTouchIdx != touch_idx)
    return false;
  enableTouchMode();
  ImGui::GetIO().MousePos = ImVec2(touch.x + viewPortOffsetX, touch.y + viewPortOffsetY);
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
