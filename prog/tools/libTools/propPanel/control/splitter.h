// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/container.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class SplitterPropertyControl : public ContainerPropertyControl
{
public:
  SplitterPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y,
    bool horizontal) :
    ContainerPropertyControl(id, event_handler, parent, x, y, hdpi::Px(0), hdpi::Px(0)), horizontalSplitter(horizontal)
  {}

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  bool isRealContainer() override { return false; }

  void setSplitRatios(dag::ConstSpan<float> ratios) override { splitRatios = ratios; }

  dag::ConstSpan<float> getSplitRatios() const override { return splitRatios; }

  void updateImgui() override
  {
    if (mControlArray.size() < 2)
    {
      ContainerPropertyControl::updateImgui();
      return;
    }

    if (splitRatios.size() != (mControlArray.size() - 1))
    {
      splitRatios.resize_noinit(mControlArray.size() - 1);
      for (int i = 0; i < splitRatios.size(); ++i)
        splitRatios[i] = 1.0f / mControlArray.size();
    }

    const ImGuiAxis sizeAxis = horizontalSplitter ? ImGuiAxis_Y : ImGuiAxis_X;
    ImVec2 contentRegionAvailableSpacingRemoved = ImGui::GetContentRegionAvail();
    contentRegionAvailableSpacingRemoved[sizeAxis] -= ImGui::GetStyle().ItemSpacing[sizeAxis] * splitRatios.size();

    for (int i = 0; i < mControlArray.size(); ++i)
    {
      PropertyControlBase *control = mControlArray[i];

      if (!horizontalSplitter && i != 0)
        ImGui::SameLine();

      const bool lastBlock = i == splitRatios.size();
      ImVec2 childSize;

      if (lastBlock)
        childSize[sizeAxis] = ImGui::GetContentRegionAvail()[sizeAxis];
      else
        childSize[sizeAxis] = contentRegionAvailableSpacingRemoved[sizeAxis] * splitRatios[i];

      ImGui::PushID(control);

      if (ImGui::BeginChild("c", childSize, ImGuiChildFlags_None, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground))
        control->updateImgui();
      ImGui::EndChild();

      ImGui::PopID();

      // Just a safety check after updateImgui().
      G_ASSERT(mControlArray.size() >= 2);
      G_ASSERT(splitRatios.size() == (mControlArray.size() - 1));

      if (!lastBlock && drawSplitter(i, ImGui::GetCurrentContext()->LastItemData.Rect))
        handleSplitterDrag(i, childSize[sizeAxis], contentRegionAvailableSpacingRemoved[sizeAxis]);
    }
  }

private:
  bool drawSplitterInternal(int splitter_index, const ImRect &small_grip_rect, const ImRect &full_grip_rect)
  {
    const ImGuiID splitterId = ImGui::GetIDWithSeed("#splitter", nullptr, splitter_index);
    ImGui::ItemAdd(full_grip_rect, splitterId, NULL, ImGuiItemFlags_NoNav);

    bool hovered = false;
    bool held = false;
    ImGui::ButtonBehavior(full_grip_rect, splitterId, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

    const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f; // from imgui.cpp
    if (hovered && ImGui::GetCurrentContext()->HoveredIdTimer <= WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER)
      hovered = false;

    if (held || hovered)
    {
      ImGui::SetMouseCursor(horizontalSplitter ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);

      const ImU32 color = held ? ImGui::GetColorU32(ImGuiCol_ResizeGripActive) : ImGui::GetColorU32(ImGuiCol_ResizeGripHovered);
      ImGui::GetWindowDrawList()->AddRectFilled(full_grip_rect.Min, full_grip_rect.Max, color);
    }
    else
    {
      const ImU32 color = ImGui::GetColorU32(ImGuiCol_ResizeGrip);
      ImGui::GetWindowDrawList()->AddRectFilled(small_grip_rect.Min, small_grip_rect.Max, color);
    }

    return held;
  }

  bool drawSplitter(int splitter_index, ImRect block_rect)
  {
    if (horizontalSplitter)
    {
      const float halfSplitLength = block_rect.GetWidth() * 0.5f;
      const float halfGripLength = floorf(min((float)hdpi::_pxScaled(Constants::SPLITTER_GRIP_LENGTH), halfSplitLength) * 0.5f);
      const float gripCenterPosX = block_rect.Min.x + halfSplitLength;
      const float itemSpacing = ImGui::GetStyle().ItemSpacing.y;
      const ImRect gripRect(gripCenterPosX - halfGripLength, block_rect.Max.y, gripCenterPosX + halfGripLength,
        block_rect.Max.y + itemSpacing);
      const ImRect splitRect(block_rect.Min.x, block_rect.Max.y, block_rect.Max.x, block_rect.Max.y + itemSpacing);
      return drawSplitterInternal(splitter_index, gripRect, splitRect);
    }
    else
    {
      const float halfSplitLength = block_rect.GetHeight() * 0.5f;
      const float halfGripLength = floorf(min((float)hdpi::_pxScaled(Constants::SPLITTER_GRIP_LENGTH), halfSplitLength) * 0.5f);
      const float gripCenterPosY = block_rect.Min.y + halfSplitLength;
      const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
      const ImRect gripRect(block_rect.Max.x, gripCenterPosY - halfGripLength, block_rect.Max.x + itemSpacing,
        gripCenterPosY + halfGripLength);
      const ImRect splitRect(block_rect.Max.x, block_rect.Min.y, block_rect.Max.x + itemSpacing, block_rect.Max.y);
      return drawSplitterInternal(splitter_index, gripRect, splitRect);
    }
  }

  void handleSplitterDrag(int splitter_index, float child_size, float content_region_available_spacing_removed)
  {
    const ImGuiAxis sizeAxis = horizontalSplitter ? ImGuiAxis_Y : ImGuiAxis_X;
    const float dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left)[sizeAxis];
    if (fabsf(dragDelta) < 1.0f || content_region_available_spacing_removed <= 0.0f)
      return;

    const float newRatio = (child_size + dragDelta) / content_region_available_spacing_removed;
    if (newRatio < 0.1f || newRatio > 0.9f) // Do not let entirely hide the sides.
      return;

    const float ratioDifference = newRatio - splitRatios[splitter_index];
    splitRatios[splitter_index] += ratioDifference;
    if ((splitter_index + 1) < splitRatios.size())
      splitRatios[splitter_index + 1] -= ratioDifference;

    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
  }

  dag::Vector<float> splitRatios; // size = mControlArray.size() - 1
  bool horizontalSplitter;
};

} // namespace PropPanel