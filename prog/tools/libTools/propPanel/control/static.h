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

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setTextValue(const char value[]) override { controlCaption = value; }

  virtual int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlCaption, buffer, buflen);
  }

  virtual void setBoolValue(bool value) override { bold = value; }

  virtual void updateImgui() override
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