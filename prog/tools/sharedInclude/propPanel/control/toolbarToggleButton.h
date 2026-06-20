//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/iconWithNameAndSize.h>

namespace PropPanel
{

class ToolbarToggleButtonPropertyControl : public PropertyControlBase
{
public:
  ToolbarToggleButtonPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y,
    hdpi::Px w, hdpi::Px h, const char caption[]);

  int getImguiControlType() const override;

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }
  unsigned getWidth() const override;

  void setCaptionValue(const char value[]) override { controlTooltip = value; }
  void setBoolValue(bool value) override { controlValue = value; }
  bool getBoolValue() const override { return controlValue; }

  void reset() override;

  void setEnabled(bool enabled) override { controlEnabled = enabled; }
  bool isEnabled() const { return controlEnabled; }

  void setButtonPictureValues(const char *fname) override;
  IconWithNameAndSize &getIcon() { return iconWithNameAndSize; }

  void sendOnClickNotification();

  void updateImgui() override { toolbarToggleButtonUpdateImgui(ImDrawFlags_RoundCornersAll); }

  // frame_draw_flags: allows specifying which side of the button should be rounded
  // button_id: optional output parameter, contains the ID of the button (what ImGui::GetItemID() would return)
  // button_active: optional output parameter, contains whether the button is active or not (ImGui::IsItemActive())
  // button_rect: optional output parameter, contains the button's rectangle (ImGui::GetCurrentContext()->LastItemData.Rect)
  void toolbarToggleButtonUpdateImgui(ImDrawFlags frame_draw_flags, ImGuiID *button_id = nullptr, bool *button_active = nullptr,
    ImRect *button_rect = nullptr);

private:
  bool controlValue = false;
  bool controlEnabled = true;
  IconWithNameAndSize iconWithNameAndSize;
};

} // namespace PropPanel