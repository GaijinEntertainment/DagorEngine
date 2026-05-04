// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/toolbarButton.h>

#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>

#include "../imageHelper.h"
#include "../messageQueueInternal.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

ToolbarButtonPropertyControl::ToolbarButtonPropertyControl(int id, ControlEventHandler *event_handler,
  ContainerPropertyControl *parent, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]) :
  PropertyControlBase(id, event_handler, parent, x, y, w, h)
{
  controlTooltip = caption;
}

void ToolbarButtonPropertyControl::setButtonPictureValues(const char *fname)
{
  // Currently this gets called before d3d::init in daEditorX, so the actual loading must be delayed.
  iconWithNameAndSize.setFileName(fname);
}

unsigned ToolbarButtonPropertyControl::getWidth() const
{
  const float size = ImGui::GetTextLineHeight();
  return ImguiHelper::getImageButtonSize(ImVec2(size, size)).x;
}

void ToolbarButtonPropertyControl::updateImgui()
{
  ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

  setFocusToNextImGuiControlIfRequested();

  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); // To match the toggle buttons.
  ImGui::PushStyleColor(ImGuiCol_Border, getOverriddenColor(ColorOverride::TOGGLE_BUTTON_CHECKED_BORDER));

  // "ib" stands for ImageButton. It could be anything.
  const int size = ImGui::GetTextLineHeight();
  const IconId iconId = iconWithNameAndSize.getIconId(size);
  const ImTextureID icon = image_helper.getImTextureIdFromIconId(iconId);
  const bool clicked = ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("ib"), icon, ImVec2(size, size), ImVec2(0, 0),
    ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
  const bool rightClicked = clicked && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right, ImGui::GetItemID());

  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar();

  setPreviousImguiControlTooltip();

  if (clicked)
  {
    if (rightClicked)
      onWcRightClick(nullptr);
    else
      message_queue.sendDelayedOnWcClick(*this);
  }
}

} // namespace PropPanel