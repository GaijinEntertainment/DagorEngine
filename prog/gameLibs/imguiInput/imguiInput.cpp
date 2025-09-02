// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "imgui/imguiInput.h"

#include "imguiInputHandler.h"

#include <EASTL/unique_ptr.h>
#include <imgui/imgui.h>

#include <gui/dag_imgui.h>
#include <util/dag_console.h>
#include <math/integer/dag_IPoint2.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <daInput/input_api.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiXInputMappings.h>

static eastl::unique_ptr<DearImGuiInputHandler> imgui_input_handler;
static GlobalInputHandler imgui_global_input_handler;
static IPoint2 saved_mouse_pos = IPoint2(0, 0);
static bool saved_draw_cursor = true;
static bool hybrid_input_mode = false;

static void on_imgui_state_change(ImGuiState old_state, ImGuiState new_state)
{
  if (!imgui_input_handler)
  {
    imgui_input_handler = eastl::make_unique<DearImGuiInputHandler>();
    imgui_input_handler->hybridInput = hybrid_input_mode;
    d3d::get_screen_size(saved_mouse_pos.x, saved_mouse_pos.y);
    saved_mouse_pos /= 2;
  }

  if (new_state == ImGuiState::ACTIVE)
  {
    register_hid_event_handler(imgui_input_handler.get(), 200);
    ImGui::GetIO().MouseDrawCursor = saved_draw_cursor;
    ImGui::GetIO().MousePos = ImVec2(saved_mouse_pos);
  }
  else if (old_state == ImGuiState::ACTIVE)
  {
    saved_draw_cursor = ImGui::GetIO().MouseDrawCursor;
    ImGui::GetIO().MouseDrawCursor = false;
    saved_mouse_pos = ipoint2(ImGui::GetMousePos());
    unregister_hid_event_handler(imgui_input_handler.get());
  }
}

static void request_imgui_state(ImGuiState new_state)
{
  static bool registered = false;
  if (!registered)
  {
    imgui_register_on_state_change_handler(on_imgui_state_change);
    registered = true;
  }
  imgui_request_state_change(new_state);
}

void imgui_switch_state() { request_imgui_state(imgui_get_state() != ImGuiState::ACTIVE ? ImGuiState::ACTIVE : ImGuiState::OFF); }

void imgui_switch_overlay() { request_imgui_state(imgui_get_state() != ImGuiState::OVERLAY ? ImGuiState::OVERLAY : ImGuiState::OFF); }

const IPoint2 &imgui_get_saved_mouse_pos() { return saved_mouse_pos; }

bool imgui_handle_special_keys_down(bool ctrl, bool shift, bool alt, int btn_idx, unsigned int key_modif)
{
  switch (btn_idx)
  {
    case HumanInput::DKEY_F2:
      if (!(ctrl || shift || alt))
      {
        imgui_switch_state();
        return true;
      }
      if (shift && !(ctrl || alt))
      {
        imgui_switch_overlay();
        return true;
      }
  }
  if (imgui_get_state() == ImGuiState::ACTIVE && imgui_global_input_handler != nullptr)
  {
    if (imgui_global_input_handler(/*key_down*/ true, btn_idx, key_modif))
      return true;
  }
  return false;
}

bool imgui_handle_special_keys_up(bool, bool, bool, int btn_idx, unsigned key_modif)
{
  if (imgui_get_state() == ImGuiState::ACTIVE && imgui_global_input_handler != nullptr)
  {
    if (imgui_global_input_handler(/*key_down*/ false, btn_idx, key_modif))
      return true;
  }
  return false;
}

void imgui_register_global_input_handler(GlobalInputHandler handler) { imgui_global_input_handler = eastl::move(handler); }

bool imgui_handle_special_controller_combinations(const HumanInput::IGenJoystick *joy)
{
  if (joy->getButtonState(HumanInput::JOY_XINPUT_REAL_BTN_L_THUMB) && joy->getButtonState(HumanInput::JOY_XINPUT_REAL_BTN_D_DOWN))
  {
    imgui_switch_state();
    return true;
  }
  return false;
}

bool imgui_in_hybrid_input_mode() { return hybrid_input_mode; }

void imgui_use_hybrid_input_mode(bool value)
{
  hybrid_input_mode = value;
  if (imgui_input_handler)
    imgui_input_handler->hybridInput = value;
}

void imgui_draw_mouse_cursor(bool draw_mouse_cursor)
{
  if (imgui_input_handler)
  {
    imgui_input_handler->drawMouseCursor = draw_mouse_cursor;
  }
  else
  {
    logerr("imgui_input_handler == null, call imgui initialization");
  }
}
static bool imgui_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("imgui", "toggle", 1, 1) { imgui_switch_state(); }
  CONSOLE_CHECK_NAME("imgui", "off", 1, 1) { request_imgui_state(ImGuiState::OFF); }
  CONSOLE_CHECK_NAME("imgui", "activate", 1, 1) { request_imgui_state(ImGuiState::ACTIVE); }
  CONSOLE_CHECK_NAME("imgui", "overlay", 1, 1) { request_imgui_state(ImGuiState::OVERLAY); }
  return found;
}

REGISTER_CONSOLE_HANDLER(imgui_console_handler);

REGISTER_IMGUI_FUNCTION_EX("ImGui", "Toggle ImGui", "F2", 100, [] { imgui_switch_state(); });
REGISTER_IMGUI_FUNCTION_EX("ImGui", "Toggle overaly ImGui", "Shift+F2", 100, [] { imgui_switch_overlay(); });
REGISTER_IMGUI_FUNCTION_EX("ImGui", "Switch ImGui to Overlay mode", nullptr, 110, [] { request_imgui_state(ImGuiState::OVERLAY); });
REGISTER_IMGUI_FUNCTION_EX("ImGui", "Switch ImGui off", nullptr, 120, [] { request_imgui_state(ImGuiState::OFF); });
REGISTER_IMGUI_FUNCTION_EX("ImGui", "Cascade windows", nullptr, 130, [] { imgui_cascade_windows(); });
