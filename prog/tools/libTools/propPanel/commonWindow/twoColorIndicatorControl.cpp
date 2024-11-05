// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "twoColorIndicatorControl.h"
#include <propPanel/control/propertyControlBase.h>

#include <imgui/imgui.h>

namespace PropPanel
{

TwoColorIndicatorControl::TwoColorIndicatorControl(PropertyControlBase &control_holder) :
  controlHolder(control_holder), colorOld(E3DCOLOR(0, 0, 0)), colorNew(E3DCOLOR(0, 0, 0)), viewOffset(0.0f, 0.0f)
{}

void TwoColorIndicatorControl::draw(int width, int height)
{
  const ImU32 imColorOld = IM_COL32(colorOld.r, colorOld.g, colorOld.b, 255);
  const ImU32 imColorNew = IM_COL32(colorNew.r, colorNew.g, colorNew.b, 255);
  const ImU32 imColorBorder = ImGui::GetColorU32(ImGuiCol_FrameBg);
  ImDrawList *drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(ImVec2(1, 1) + viewOffset, ImVec2(width / 2, height) + viewOffset, imColorOld);
  drawList->AddRectFilled(ImVec2(width / 2, 1) + viewOffset, ImVec2(width - 1, height) + viewOffset, imColorNew);
  drawList->AddRect(ImVec2(0, 0) + viewOffset, ImVec2(width, height) + viewOffset, imColorBorder);
}

void TwoColorIndicatorControl::updateImgui(int width, int height)
{
  ImGui::PushID(this);

  viewOffset = ImGui::GetCursorScreenPos();

  // This will catch the interactions.
  ImGui::InvisibleButton("canvas", ImVec2(width, height));

  if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
  {
    const ImVec2 mousePosInCanvas = ImGui::GetIO().MousePos - viewOffset;
    if (mousePosInCanvas.x < (width / 2.0f))
      controlHolder.onWcClick(nullptr);
  }

  draw(width, height);

  ImGui::PopID();
}

} // namespace PropPanel