// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../messageQueueInternal.h"
#include "../scopedImguiBeginDisabled.h"
#include "../c_constants.h"

namespace PropPanel
{

class ButtonPropertyControl : public PropertyControlBase
{
public:
  ButtonPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y, hdpi::Px w,
    hdpi::Px h, const char caption[], bool left_align_text = false) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h), controlCaption(caption), leftAlignText(left_align_text)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void setTextValue(const char value[]) override { controlCaption = value; }

  int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlCaption, buffer, buflen);
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  const char *getImguiTypeName() const override { return "Button"; }

  Point2 getPreferredSize(float max_width_px) const override
  {
    // Same as the item size in ImGui::Button().
    const ImGuiStyle &style = ImGui::GetStyle();
    const ImVec2 labelSize = ImGui::CalcTextSize(controlCaption.begin(), controlCaption.end(), true);
    const float width = mW > 0 ? (float)mW : (labelSize.x + style.FramePadding.x * 2.0f);
    return Point2(min(width, max_width_px), labelSize.y + style.FramePadding.y * 2.0f);
  }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    setFocusToNextImGuiControlIfRequested();

    // Use full width by default.
    const float width = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;

    float oldAlignX;
    if (leftAlignText)
    {
      oldAlignX = ImGui::GetStyle().ButtonTextAlign.x;
      ImGui::GetStyle().ButtonTextAlign.x = 0.0f;
    }

    const bool clicked = ImGui::Button(controlCaption, ImVec2(width, 0.0f));
    setImguiTestItemInfo();

    if (leftAlignText)
      ImGui::GetStyle().ButtonTextAlign.x = oldAlignX;

    setPreviousImguiControlTooltip();

    if (clicked)
      message_queue.sendDelayedOnWcClick(*this);
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  const bool leftAlignText;
};

} // namespace PropPanel