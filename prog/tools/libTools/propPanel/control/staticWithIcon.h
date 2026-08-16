// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/iconWithNameAndSize.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"


namespace PropPanel
{

class StaticWithIconPropertyControl : public PropertyControlBase
{
public:
  StaticWithIconPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], hdpi::Px h, bool word_wrap) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h), controlCaption(caption), wordWrap(word_wrap)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_BUTTON_PICTURES | CONTROL_CAPTION | CONTROL_DATA_TYPE_STRING; }
  unsigned getTypeMaskForGet() const override { return CONTROL_BUTTON_PICTURES | CONTROL_DATA_TYPE_STRING; }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setTextValue(const char value[]) override { controlCaption = value; }

  int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlCaption, buffer, buflen);
  }

  void setButtonPictureValues(const char *fname) override
  {
    // Currently this gets called before d3d::init in daEditorX, so the actual loading must be delayed.
    iconWithNameAndSize.setFileName(fname);
  }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    setFocusToNextImGuiControlIfRequested();

    // Use full width by default.
    ImGui::SetNextItemWidth(mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : -FLT_MIN);

    const int height = ImGui::GetTextLineHeight();
    const ImVec2 size = ImVec2(height, height);
    const IconId iconId = iconWithNameAndSize.getIconId(height);
    const ImTextureID icon = image_helper.getImTextureIdFromIconId(iconId);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    ImGui::Dummy(ImVec2(height + padding.x * 2, height + padding.y * 2));

    ImGui::SetCursorScreenPos(ImVec2(pos.x + padding.x, pos.y /* + padding.y */));

    ImGui::Image(icon, size);

    setPreviousImguiControlTooltip();

    ImGui::SameLine();

    if (wordWrap)
    {
      ImGui::SetNextItemWidth(-FLT_MIN);

      ImGui::PushTextWrapPos(0.0f);

      ImGui::TextEx(controlCaption.begin(), controlCaption.end(), ImGuiTextFlags_None);

      tooltip_helper.setPreviousImguiControlTooltip(this, controlTooltip.begin(), controlTooltip.end());

      ImGui::PopTextWrapPos();
    }
    else
    {
      labelWithTooltip(controlCaption.begin(), controlCaption.end(), true);
    }
  }

private:
  IconWithNameAndSize iconWithNameAndSize;
  String controlCaption;
  bool wordWrap = false;
  bool controlEnabled = true;
};

} // namespace PropPanel