// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

bool ec_is_key_down(ImGuiKey key) { return ImGui::GetCurrentContext() && ImGui::IsKeyDown(key); }

bool ec_is_alt_key_down()
{
  const ImGuiContext *context = ImGui::GetCurrentContext();
  return context && (context->IO.KeyMods & ImGuiMod_Alt) != 0;
}

bool ec_is_ctrl_key_down()
{
  const ImGuiContext *context = ImGui::GetCurrentContext();
  return context && (context->IO.KeyMods & ImGuiMod_Ctrl) != 0;
}

bool ec_is_shift_key_down()
{
  const ImGuiContext *context = ImGui::GetCurrentContext();
  return context && (context->IO.KeyMods & ImGuiMod_Shift) != 0;
}
