//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/optional.h>
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/control/spinEditStandalone.h>
#include <propPanel/colors.h>
#include <propPanel/imguiHelper.h>
#include <propPanel/immediateFocusLossHandler.h>

namespace PropPanel
{

class SpinEditFloatPropertyControl : public PropertyControlBase, public IImmediateFocusLossHandler
{
public:
  // width_includes_label: if true then the width set to control includes both the label and the spin edit, and the
  // label is aligned to left and the spin edit to the right. If false then label is rendered as wide as required by its
  // contents, and the spin edit follows it directly.
  SpinEditFloatPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, const char caption[],
    int precision = 3, bool width_includes_label = true) :
    PropertyControlBase(id, event_handler, parent),
    controlCaption(caption),
    spinEdit(0.0f, 0.0f, 0.0f, precision),
    widthIncludesLabel(width_includes_label)
  {}

  ~SpinEditFloatPropertyControl() override
  {
    if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

  unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_FLOAT | CONTROL_DATA_MIN_MAX_STEP | CONTROL_CAPTION | CONTROL_DATA_PREC;
  }

  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_FLOAT; }

  float getFloatValue() const override { return spinEdit.getValue(); }
  void setFloatValue(float value) override { spinEdit.setValue(value); }
  void setMinMaxStepValue(float min, float max, float step) override { spinEdit.setMinMaxStepValue(min, max, step); }
  void setPrecValue(int prec) override { spinEdit.setPrecValue(prec); }
  void setCaptionValue(const char value[]) override { controlCaption = value; }
  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void reset() override
  {
    setFloatValue(spinEdit.getMinValue());

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == getFloatValue() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      setFloatValue(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<float>(); }

  void updateImgui() override;

private:
  void onImmediateFocusLoss() override { spinEdit.sendWcChangeIfVarChanged(*this); }

  String controlCaption;
  bool controlEnabled = true;
  const bool widthIncludesLabel;
  SpinEditControlStandalone spinEdit;
  eastl::optional<float> defaultValue;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel
