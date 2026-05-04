// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/optional.h>
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/immediateFocusLossHandler.h>
#include "spinEditStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class SpinEditIntPropertyControl : public PropertyControlBase, public IImmediateFocusLossHandler
{
public:
  // width_includes_label: if true then the width set to control includes both the label and the spin edit, and the
  // label is aligned to left and the spin edit to the right. If false then label is rendered as wide as required by its
  // contents, and the spin edit follows it directly.
  SpinEditIntPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, const char caption[],
    bool width_includes_label = true) :
    PropertyControlBase(id, event_handler, parent),
    controlCaption(caption),
    widthIncludesLabel(width_includes_label),
    spinEdit(0, 0, 1, 1)
  {}

  ~SpinEditIntPropertyControl() override
  {
    if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT; }

  int getIntValue() const override { return floor(spinEdit.getValue()); }
  void setIntValue(int value) override { spinEdit.setValue(value); }

  void setMinMaxStepValue(float min, float max, float step) override
  {
    spinEdit.setMinMaxStepValue(floor(min), ceil(max), ceil(step));
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void reset() override
  {
    setIntValue(floor(spinEdit.getMinValue()));

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == spinEdit.getValue() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      spinEdit.setValue(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<int>(); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    float spinEditWidth = ImguiHelper::getDefaultRightSideEditWidth();

    if (widthIncludesLabel)
    {
      const float availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
      const float spaceBeforeSpinEdit = availableSpace - spinEditWidth;

      const float availableSpaceForLabel =
        controlCaption.size() > 0 ? (spaceBeforeSpinEdit - ImGui::GetStyle().ItemInnerSpacing.x) : 0.0f;
      if (availableSpaceForLabel > 0.0f)
      {
        ImGui::SetNextItemWidth(availableSpaceForLabel);
        labelWithTooltip(controlCaption.begin(), controlCaption.end());

        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      }
      else if (spaceBeforeSpinEdit > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBeforeSpinEdit);
      else
        spinEditWidth = availableSpace;
    }
    else
    {
      labelWithTooltip(controlCaption.begin(), controlCaption.end(), true);
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

      const float availableSpace = mW > 0 ? min((float)mW, ImGui::GetContentRegionAvail().x) : ImGui::GetContentRegionAvail().x;
      spinEditWidth = availableSpace;
    }

    ImGui::SetNextItemWidth(spinEditWidth);
    setFocusToNextImGuiControlIfRequested();

    // Override the background color of the edit box.
    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

    spinEdit.updateImgui(*this, &controlTooltip, this);

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    if (spinEdit.isTextInputFocused())
      set_focused_immediate_focus_loss_handler(this);
    else if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

private:
  void onImmediateFocusLoss() override { spinEdit.sendWcChangeIfVarChanged(*this); }

  String controlCaption;
  bool controlEnabled = true;
  const bool widthIncludesLabel;
  SpinEditControlStandalone spinEdit;
  eastl::optional<int> defaultValue;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel