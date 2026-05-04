// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/colorDialog.h>
#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "../colorChangeBuffer.h"
#include "../messageQueueInternal.h"
#include "../scopedImguiBeginDisabled.h"
#include "../tooltipHelper.h"
#include <math/dag_color.h>

namespace PropPanel
{

class ColorBoxPropertyControl : public PropertyControlBase, public IDelayedCallbackHandler
{
public:
  ColorBoxPropertyControl(ControlEventHandler *event_handler, PropPanel::ContainerPropertyControl *parent, int id, int x, int y,
    hdpi::Px w, const char caption[], bool use_modal_color_selector) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)),
    controlCaption(caption),
    useModalColorSelector(use_modal_color_selector)
  {}

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_COLOR | CONTROL_CAPTION; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_COLOR; }

  E3DCOLOR getColorValue() const override { return controlValue; }
  void setColorValue(E3DCOLOR value) override { controlValue = value; }

  void setCaptionValue(const char value[]) override { controlCaption = value; }

  void reset() override
  {
    controlValue = E3DCOLOR(0);

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { controlEnabled = enabled; }

  void setValueHighlight(ColorOverride::ColorIndex color) override { valueHighlightColor = color; }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);
    bool showTooltip = false;

    if (!controlCaption.empty())
    {
      float availableWidthForLabel =
        ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemInnerSpacing.x - ImguiHelper::getDefaultRightSideEditWidth();
      if (availableWidthForLabel < 0.0f)
        availableWidthForLabel = 0.0f;

      ImGui::SetNextItemWidth(availableWidthForLabel);
      if (ImguiHelper::labelOnly(controlCaption.begin(), controlCaption.end()))
        showTooltip = tooltip_helper.isImguiControlHovered(this);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    }

    setFocusToNextImGuiControlIfRequested();

    G_STATIC_ASSERT(sizeof(Color4) == (sizeof(float) * 4));
    Color4 asColor4(controlValue);

    const ImVec2 previewSize(ImGui::GetContentRegionAvail().x, 0.0f);
    const bool pressed = ImGui::ColorButton("##preview", ImVec4(asColor4.r, asColor4.g, asColor4.b, asColor4.a),
      ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip, previewSize);

    if (pressed)
    {
      if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
      {
        ColorChangeBuffer::set(controlValue);
      }
      else if (ImGui::IsKeyDown(ImGuiMod_Shift))
      {
        setColorValue(ColorChangeBuffer::get());
        onWcChange(nullptr);
      }
      else if (useModalColorSelector)
      {
        message_queue.requestDelayedCallback(*this);
      }
      else if (modelessSelectorState == ModelessSelectorState::Hidden)
      {
        modelessSelectorState = ModelessSelectorState::RequestedShowing;
        modelessOriginalControlValue = controlValue;
      }
    }
    else
    {
      showTooltip = showTooltip || tooltip_helper.isImguiControlHovered(this);
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    // Override the background color of the edit box.
    const bool valueHighlightColorSet = valueHighlightColor != ColorOverride::NONE;
    if (valueHighlightColorSet)
      ImGui::PushStyleColor(ImGuiCol_FrameBg, getOverriddenColor(valueHighlightColor));

    const bool changed = ImGui::ColorEdit4(controlCaption.c_str(), reinterpret_cast<float *>(&asColor4),
      ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSmallPreview);

    if (valueHighlightColorSet)
      ImGui::PopStyleColor();

    if (changed)
    {
      controlValue = e3dcolor(asColor4);
      onWcChange(nullptr);
    }
    else
    {
      showTooltip = showTooltip || tooltip_helper.isImguiControlHovered(this);
    }

    if (modelessSelectorState == ModelessSelectorState::Hidden)
    {
      if (showTooltip)
        ImGui::ColorTooltip(controlTooltip.empty() ? nullptr : controlTooltip.c_str(), reinterpret_cast<const float *>(&asColor4),
          ImGuiColorEditFlags_InputRGB);
    }
    else
    {
      const char *popupId = "p"; // "p" stands for pop-up. It could be anything.

      if (modelessSelectorState == ModelessSelectorState::RequestedShowing)
      {
        modelessSelectorState = ModelessSelectorState::Showing;

        const ImVec2 lastItemBottomLeft = ImGui::GetCurrentContext()->LastItemData.Rect.GetBL();
        ImGui::SetNextWindowPos(ImVec2(lastItemBottomLeft.x, lastItemBottomLeft.y + ImGui::GetStyle().ItemSpacing.y));
        ImGui::OpenPopup(popupId);
      }

      if (ImGui::BeginPopup(popupId))
      {
        G_STATIC_ASSERT(sizeof(Color4) == (sizeof(float) * 4));
        const Color4 originalColor4(modelessOriginalControlValue);
        const bool changed = ImGui::ColorPicker4("##picker", reinterpret_cast<float *>(&asColor4),
          ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoLabel,
          reinterpret_cast<const float *>(&originalColor4));
        if (changed)
        {
          controlValue = e3dcolor(asColor4);
          onWcChange(nullptr);
        }

        ImGui::EndPopup();
      }
      else
      {
        modelessSelectorState = ModelessSelectorState::Hidden;
      }
    }
  }

private:
  void onImguiDelayedCallback(void *data) override
  {
    String dialogCaption;
    if (controlCaption.empty())
      dialogCaption = "Select color";
    else
      dialogCaption.printf(128, "Select color for \"%s\"", controlCaption.c_str());

    ColorDialog dialog(nullptr, dialogCaption, controlValue);
    if (dialog.showDialog() == DIALOG_ID_OK)
    {
      controlValue = dialog.getColor();
      onWcChange(nullptr);
    }
  }

  enum class ModelessSelectorState
  {
    Hidden,
    RequestedShowing,
    Showing,
  };

  String controlCaption;
  E3DCOLOR controlValue = E3DCOLOR(0, 0, 0, 255);
  bool controlEnabled = true;
  bool useModalColorSelector;
  ModelessSelectorState modelessSelectorState = ModelessSelectorState::Hidden;
  E3DCOLOR modelessOriginalControlValue = E3DCOLOR(0, 0, 0, 255);
  ColorOverride::ColorIndex valueHighlightColor = ColorOverride::NONE;
};

} // namespace PropPanel