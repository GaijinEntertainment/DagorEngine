// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/colors.h>
#include "../imageHelper.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class ToolbarButtonPropertyControl : public PropertyControlBase
{
public:
  ToolbarButtonPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h)
  {
    controlTooltip = caption;
  }

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void setCaptionValue(const char value[]) override { controlTooltip = value; }

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

    setFocusToNextImGuiControlIfRequested();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); // To match the toggle buttons.
    ImGui::PushStyleColor(ImGuiCol_Border, getOverriddenColor(ColorOverride::TOGGLE_BUTTON_CHECKED_BORDER));

    const ImTextureID icon = image_helper.getImTextureIdFromIconId(iconId);
    const float size = ImGui::GetTextLineHeight();
    const bool clicked = ImGui::ImageButton("ib", icon, ImVec2(size, size)); // "ib" stands for ImageButton. It could be anything.

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    setPreviousImguiControlTooltip();

    if (clicked)
      message_queue.sendDelayedOnWcClick(*this);
  }

private:
  bool controlEnabled = true;
  String iconName;
  IconId iconId = IconId::Invalid;
};

} // namespace PropPanel