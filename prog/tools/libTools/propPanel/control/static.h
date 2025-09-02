// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class StaticPropertyControl : public PropertyControlBase
{
public:
  StaticPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], hdpi::Px h) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h), controlCaption(caption)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setTextValue(const char value[]) override { controlCaption = value; }

  int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlCaption, buffer, buflen);
  }

  void setBoolValue(bool value) override { bold = value; }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    // Use full width by default.
    ImGui::SetNextItemWidth(mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN);

    if (bold)
      ImGui::PushFont(imgui_get_bold_font());

    ImguiHelper::labelOnly(controlCaption);

    if (bold)
      ImGui::PopFont();
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  bool bold = false;
};

} // namespace PropPanel