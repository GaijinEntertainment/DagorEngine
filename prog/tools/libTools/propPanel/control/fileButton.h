// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/constants.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"
#include <winGuiWrapper/wgw_dialogs.h>

namespace PropPanel
{

class FileButtonPropertyControl : public PropertyControlBase
{
public:
  FileButtonPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption), masks("All|*.*||")
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  void setTextValue(const char value[]) override { controlValue = value; }
  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setStringsValue(const Tab<String> &vals) override
  {
    masks.clear();

    for (int i = 0; i < vals.size(); ++i)
    {
      masks += vals[i];
      masks += "|";
    }

    masks += "|";
  }

  int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlValue, buffer, buflen);
  }

  void reset() override
  {
    controlValue.clear();

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == controlValue.str() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      setTextValue(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<SimpleString>(); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    separateLineLabelWithTooltip(controlCaption.begin(), controlCaption.end());
    setFocusToNextImGuiControlIfRequested();

    const float clearButtonWidth = ImGui::GetFrameHeight(); // Simply use a square button.
    const float spaceBetweenControls = hdpi::_pxS(Constants::SPACE_BETWEEN_BUTTONS_IN_COMBINED_CONTROL);
    const float pickButtonWidth = ImGui::GetContentRegionAvail().x - clearButtonWidth - spaceBetweenControls;

    const bool clickedOnPickButton =
      ImGui::Button(controlValue.empty() ? "none" : controlValue.c_str(), ImVec2(pickButtonWidth, 0.0f));
    setPreviousImguiControlTooltip();

    ImGui::SameLine(0.0f, spaceBetweenControls);
    const bool clickedOnClearButton = ImGui::Button("x", ImVec2(clearButtonWidth, 0.0f));
    setPreviousImguiControlTooltip();

    if (clickedOnPickButton)
    {
      const String result = wingw::file_open_dlg(getRootParent()->getWindowHandle(), "Select file...", masks, controlValue);
      if (!result.empty())
      {
        setTextValue(result.str());
        onWcChange(nullptr);
      }
    }
    else if (clickedOnClearButton)
    {
      controlValue.clear();
      onWcChange(nullptr);
    }
  }

private:
  String controlCaption;
  String controlValue;
  bool controlEnabled = true;
  String masks;
  eastl::optional<SimpleString> defaultValue;
};

} // namespace PropPanel