// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_STRING | CONTROL_CAPTION; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_STRING; }

  virtual void setTextValue(const char value[]) override { controlValue = value; }
  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  virtual void setStringsValue(const Tab<String> &vals) override
  {
    masks.clear();

    for (int i = 0; i < vals.size(); ++i)
    {
      masks += vals[i];
      masks += "|";
    }

    masks += "|";
  }

  virtual int getTextValue(char *buffer, int buflen) const override
  {
    return ImguiHelper::getTextValueForString(controlValue, buffer, buflen);
  }

  virtual void reset() override
  {
    controlValue.clear();

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);
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
};

} // namespace PropPanel