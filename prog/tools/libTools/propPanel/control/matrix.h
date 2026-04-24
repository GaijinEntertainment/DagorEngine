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

class MatrixPropertyControl : public PropertyControlBase, public IImmediateFocusLossHandler
{
public:
  MatrixPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], int prec) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)),
    controlCaption(caption),
    spinEdit{{{0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}},
      {{0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}},
      {{0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}, {0.0f, 0.0f, 0.0f, prec}}}
  {
    MatrixPropertyControl::setMatrixValue(TMatrix::IDENT);
  }

  ~MatrixPropertyControl() override
  {
    if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_MATRIX | CONTROL_CAPTION | CONTROL_DATA_PREC; }

  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_POINT3; }

  TMatrix getMatrixValue() const override { return controlValue; }

  void setMatrixValue(const TMatrix &value) override
  {
    controlValue = value;
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 4; ++col)
        spinEdit[row][col].setValue(value.m[col][row]);
  }

  void setPrecValue(int prec) override
  {
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 4; ++col)
        spinEdit[row][col].setPrecValue(prec);
  }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void reset() override
  {
    MatrixPropertyControl::setMatrixValue(TMatrix::IDENT);
    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  bool isDefaultValueSet() const override { return defaultValue ? *defaultValue == getMatrixValue() : true; }

  void applyDefaultValue() override
  {
    if (isDefaultValueSet())
    {
      return;
    }

    if (defaultValue)
    {
      setMatrixValue(*defaultValue);
      onWcChange(nullptr);
    }
  }

  void setDefaultValue(Variant var) override { defaultValue = var.convert<TMatrix>(); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    separateLineLabelWithTooltip(controlCaption.begin(), controlCaption.end());
    setFocusToNextImGuiControlIfRequested();

    // Override the background color of the edit box.
    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

    for (int row = 0; row < 3; ++row)
    {
      ImGui::PushMultiItemsWidths(4, ImGui::GetContentRegionAvail().x);
      for (int col = 0; col < 4; ++col)
      {
        if (col != 0)
          ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        spinEdit[row][col].updateImgui(*this, &controlTooltip, this);
        ImGui::PopItemWidth();
      }
    }

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    bool focused = false;
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 4; ++col)
        focused |= spinEdit[row][col].isTextInputFocused();

    if (focused)
      set_focused_immediate_focus_loss_handler(this);
    else if (get_focused_immediate_focus_loss_handler() == this)
      set_focused_immediate_focus_loss_handler(nullptr);
  }

private:
  void onWcChange(WindowBase *source) override
  {
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 4; ++col)
        controlValue.m[col][row] = spinEdit[row][col].getValue();

    PropertyControlBase::onWcChange(source);
  }

  void onImmediateFocusLoss() override
  {
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 4; ++col)
        spinEdit[row][col].sendWcChangeIfVarChanged(*this);
  }

  String controlCaption;
  TMatrix controlValue = TMatrix::IDENT;
  bool controlEnabled = true;
  SpinEditControlStandalone spinEdit[3][4];
  eastl::optional<TMatrix> defaultValue;
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel