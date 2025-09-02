// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/colors.h>
#include "controlType.h"
#include "../imageHelper.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class ToolbarToggleButtonGroupPropertyControl : public PropertyControlBase
{
public:
  ToolbarToggleButtonGroupPropertyControl(int id, ControlEventHandler *event_handler, PropPanel::ContainerPropertyControl *parent,
    int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h)
  {
    controlTooltip = caption;
  }

  int getImguiControlType() const override { return (int)ControlType::ToolbarToggleButtonGroup; }

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void setCaptionValue(const char value[]) override { controlTooltip = value; }
  void setBoolValue(bool value) override { controlValue = value; }
  bool getBoolValue() const override { return controlValue; }

  void reset() override
  {
    setBoolValue(false);

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setButtonPictureValues(const char *fname) override
  {
    // Currently this gets called before d3d::init in daEditorX, so the actual loading must be delayed.
    iconName = fname;
    iconId = IconId::Invalid;
  }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    if (!iconName.empty())
    {
      iconId = image_helper.loadIcon(iconName);
      iconName.clear();
    }

    if (controlValue)
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    else
      ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    ImGui::PushStyleColor(ImGuiCol_Border, getOverriddenColor(ColorOverride::TOGGLE_BUTTON_CHECKED_BORDER));

    const ImTextureID icon = image_helper.getImTextureIdFromIconId(iconId);
    const float size = ImGui::GetTextLineHeight();
    const bool clicked = ImGui::ImageButton("ib", icon, ImVec2(size, size)); // "ib" stands for ImageButton. It could be anything.

    ImGui::PopStyleColor(2);

    if (controlValue)
      ImGui::PopStyleColor();
    else
      ImGui::PopStyleVar();

    setPreviousImguiControlTooltip();

    if (clicked)
      handleButtonClick();
  }

private:
  // Both the start_index and end_index are inclusive.
  void getGroupRange(PropPanel::ContainerPropertyControl &parent, int &start_index, int &end_index) const
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

  void handleButtonClick()
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

  bool controlValue = false;
  bool controlEnabled = true;
  String iconName;
  IconId iconId = IconId::Invalid;
};

} // namespace PropPanel