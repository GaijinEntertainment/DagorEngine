// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <propPanel/control/toolbarToggleButtonGroup.h>

#include <propPanel/control/container.h>
#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>

#include "controlType.h"
#include "../imageHelper.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

ToolbarToggleButtonGroupPropertyControl::ToolbarToggleButtonGroupPropertyControl(int id, ControlEventHandler *event_handler,
  PropPanel::ContainerPropertyControl *parent, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]) :
  PropertyControlBase(id, event_handler, parent, x, y, w, h)
{
  controlTooltip = caption;
}

int ToolbarToggleButtonGroupPropertyControl::getImguiControlType() const { return (int)ControlType::ToolbarToggleButtonGroup; }

void ToolbarToggleButtonGroupPropertyControl::reset()
{
  setBoolValue(false);

  PropertyControlBase::reset();
}

void ToolbarToggleButtonGroupPropertyControl::setButtonPictureValues(const char *fname)
{
  // Currently this gets called before d3d::init in daEditorX, so the actual loading must be delayed.
  iconWithNameAndSize.setFileName(fname);
}

unsigned ToolbarToggleButtonGroupPropertyControl::getWidth() const
{
  const float size = ImGui::GetTextLineHeight();
  return ImguiHelper::getImageButtonSize(ImVec2(size, size)).x;
}

void ToolbarToggleButtonGroupPropertyControl::updateImgui()
{
  ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

  if (controlValue)
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
  else
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
  ImGui::PushStyleColor(ImGuiCol_Border, getOverriddenColor(ColorOverride::TOGGLE_BUTTON_CHECKED_BORDER));

  // "ib" stands for ImageButton. It could be anything.
  const int size = ImGui::GetTextLineHeight();
  const IconId iconId = iconWithNameAndSize.getIconId(size);
  const ImTextureID icon = image_helper.getImTextureIdFromIconId(iconId);
  const bool clicked = ImGui::ImageButtonEx(ImGui::GetCurrentWindow()->GetID("ib"), icon, ImVec2(size, size), ImVec2(0, 0),
    ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
  const bool rightClicked = clicked && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right, ImGui::GetItemID());

  ImGui::PopStyleColor(2);

  if (controlValue)
    ImGui::PopStyleColor();
  else
    ImGui::PopStyleVar();

  setPreviousImguiControlTooltip();

  if (clicked)
  {
    if (rightClicked)
      onWcRightClick(nullptr);
    else
      handleButtonClick();
  }
}

void ToolbarToggleButtonGroupPropertyControl::getGroupRange(PropPanel::ContainerPropertyControl &parent, int &start_index,
  int &end_index) const
{
  start_index = -1;
  end_index = -1;

  // We find the start and the end of the group this toggle button belongs too. A group is defined as a contiguous
  // collection of toggle buttons. (That is how the original worked, based on the Windows API.)
  const int controlCount = parent.getChildCount();
  for (int i = 0; i < controlCount; ++i)
  {
    const PropertyControlBase *control = parent.getByIndex(i);

    if (control->getImguiControlType() != (int)ControlType::ToolbarToggleButtonGroup)
    {
      start_index = -1;
      continue;
    }

    if (start_index < 0)
      start_index = i;

    if (control == this)
    {
      end_index = start_index;
      break;
    }
  }

  G_ASSERT(start_index >= 0);
  G_ASSERT(end_index >= 0);

  for (int i = end_index + 1; i < controlCount; ++i)
  {
    const PropertyControlBase *control = parent.getByIndex(i);

    if (control->getImguiControlType() != (int)ControlType::ToolbarToggleButtonGroup)
      break;

    end_index = i;
  }
}

void ToolbarToggleButtonGroupPropertyControl::handleButtonClick()
{
  PropPanel::ContainerPropertyControl *parent = getParent();
  G_ASSERT(parent);

  int startIndex;
  int endIndex;
  getGroupRange(*parent, startIndex, endIndex);

  for (int i = startIndex; i <= endIndex; ++i)
  {
    PropertyControlBase *control = parent->getByIndex(i);
    G_ASSERT(control->getImguiControlType() == (int)ControlType::ToolbarToggleButtonGroup);
    ToolbarToggleButtonGroupPropertyControl *toggleControl = static_cast<ToolbarToggleButtonGroupPropertyControl *>(control);
    toggleControl->controlValue = toggleControl == this;
  }

  onWcClick(nullptr);
}

} // namespace PropPanel