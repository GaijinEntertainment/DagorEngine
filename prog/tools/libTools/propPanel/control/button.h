// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
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

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  virtual unsigned getTypeMaskForGet() const override { return 0; }

  virtual void setTextValue(const char value[]) override { controlCaption = value; }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
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