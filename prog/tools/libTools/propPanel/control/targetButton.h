// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../messageQueueInternal.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class TargetButtonPropertyControl : public PropertyControlBase
{
public:
  TargetButtonPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption)
  {}

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  virtual void setTextValue(const char value[]) override { controlValue = value; }
  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlValue, buffer, buflen);
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void reset() override { controlValue.clear(); }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    // Use half of the space for label, and the other half for the button.
    const float availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
    const float buttonWidth = availableSpace / 2.0f;
    const float spaceBeforeButton = availableSpace - buttonWidth;

    const float availableSpaceForLabel = controlCaption.size() > 0 ? (spaceBeforeButton - ImGui::GetStyle().ItemInnerSpacing.x) : 0.0f;
    if (availableSpaceForLabel > 0.0f)
    {
      ImGui::SetNextItemWidth(availableSpaceForLabel);
      ImguiHelper::labelOnly(controlCaption);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    }
    else if (spaceBeforeButton > 0.0f)
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBeforeButton);

    setFocusToNextImGuiControlIfRequested();

    const char *buttonTitle = controlValue.empty() ? "<none>" : controlValue.c_str();
    const bool clicked = ImGui::Button(buttonTitle, ImVec2(buttonWidth, 0.0f));

    setPreviousImguiControlTooltip();

    // Intentionally using onWcChange here, just like the original control did.
    if (clicked)
      message_queue.sendDelayedOnWcChange(*this);
  }

private:
  String controlCaption;
  String controlValue;
  bool controlEnabled = true;
};

} // namespace PropPanel