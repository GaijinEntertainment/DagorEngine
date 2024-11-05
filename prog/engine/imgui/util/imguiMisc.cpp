// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gui/dag_imguiUtil.h>

// Helper to display a little (?) mark which shows a tooltip when hovered.
// Shamelessly stolen from imgui_demo.cpp
void ImGuiDagor::HelpMarker(const char *desc, float text_wrap_pos_in_pixels)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(text_wrap_pos_in_pixels > 0.0 ? text_wrap_pos_in_pixels : (ImGui::GetFontSize() * 35.0f));
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}