// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "paletteCellControl.h"
#include <propPanel/control/propertyControlBase.h>

#include <math/dag_color.h>
#include <imgui/imgui.h>

namespace PropPanel
{

PaletteCellControl::PaletteCellControl(PropertyControlBase &control_holder, int w, int h) :
  controlHolder(control_holder), width(w), height(h)
{}

void PaletteCellControl::updateImgui(E3DCOLOR color, const char *name, bool selected)
{
  ImGui::PushID(this);

  const ImVec2 deflateSize(3, 3);
  const ImVec2 buttonSize(width - (deflateSize.x * 2), height - (deflateSize.y * 2));
  ImGui::SetCursorPos(ImGui::GetCursorPos() + deflateSize);

  G_STATIC_ASSERT(sizeof(Color4) == (sizeof(ImVec4)));
  const Color4 asColor4(color);
  const bool clicked = ImGui::ColorButton(name, reinterpret_cast<const ImVec4 &>(asColor4), ImGuiColorEditFlags_None, buttonSize);

  if (selected)
  {
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    const ImU32 selectionColor = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(rectMin - deflateSize, rectMax + deflateSize, selectionColor);
    drawList->AddRect(rectMin - (deflateSize - ImVec2(1, 1)), rectMax + (deflateSize - ImVec2(1, 1)), selectionColor);
  }

  ImGui::SetCursorPos(ImGui::GetCursorPos() + deflateSize);
  ImGui::Dummy(ImVec2(0.0f, 0.0f)); // This is here to prevent assert in ImGui::ErrorCheckUsingSetCursorPosToExtendParentBoundaries().

  if (clicked)
    controlHolder.onWcChange(nullptr); // The palette cell control sends onWcChange instead of onWcClick.

  ImGui::PopID();
}

} // namespace PropPanel