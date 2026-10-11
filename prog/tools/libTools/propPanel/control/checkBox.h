// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class CheckBoxPropertyControl : public PropertyControlBase
{
public:
  CheckBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_BOOL | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_BOOL; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setBoolValue(bool value) override { controlValue = value; }
  bool getBoolValue() const override { return controlValue; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void reset() override
  {
    setBoolValue(false);

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == getBoolValue() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      setBoolValue(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<bool>(); }

  const char *getImguiTypeName() const override { return "CheckBox"; }

  Point2 getPreferredSize([[maybe_unused]] float max_width_px) const override
  {
    // Same as the item size in ImGui::Checkbox().
    const ImGuiStyle &style = ImGui::GetStyle();
    const ImVec2 labelSize = ImGui::CalcTextSize(controlCaption.begin(), controlCaption.end(), true);
    const float width = ImGui::GetFrameHeight() + (labelSize.x > 0.0f ? (style.ItemInnerSpacing.x + labelSize.x) : 0.0f);
    return Point2(width, labelSize.y + style.FramePadding.y * 2.0f);
  }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    setFocusToNextImGuiControlIfRequested();

    // Override the background color of the edit box.
    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

    const bool clicked = ImguiHelper::checkboxWithDragSelection(controlCaption, &controlValue);
    setImguiTestItemInfo();

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    setPreviousImguiControlTooltip();

    if (clicked)
    {
      onWcClick(nullptr);
      onWcChange(nullptr);
    }
  }

private:
  String controlCaption;
  bool controlValue = false;
  bool controlEnabled = true;
  eastl::optional<bool> defaultValue;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel