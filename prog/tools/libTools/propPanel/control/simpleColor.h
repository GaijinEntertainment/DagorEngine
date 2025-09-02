// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/commonWindow/colorDialog.h>
#include <propPanel/imguiHelper.h>
#include "../colorChangeBuffer.h"
#include "../messageQueueInternal.h"
#include "../scopedImguiBeginDisabled.h"
#include <math/dag_color.h>

namespace PropPanel
{

class SimpleColorPropertyControl : public PropertyControlBase, public IDelayedCallbackHandler
{
public:
  SimpleColorPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption)
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

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    const float defaultRightSideEditWidth = ImguiHelper::getDefaultRightSideEditWidth();

    if (!controlCaption.empty())
    {
      float availableWidthForLabel =
        ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemInnerSpacing.x - defaultRightSideEditWidth;
      if (availableWidthForLabel < 0.0f)
        availableWidthForLabel = 0.0f;

      ImGui::SetNextItemWidth(availableWidthForLabel);
      ImguiHelper::labelOnly(controlCaption);

      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
    }

    setFocusToNextImGuiControlIfRequested();

    // ImGui::SetNextItemWidth(defaultRightSideEditWidth); // TOOD: ImGui porting: ColorEdit4 is always rendered as a square, change it
    // to use our standard width

    G_STATIC_ASSERT(sizeof(Color4) == (sizeof(float) * 4));
    Color4 asColor4(controlValue);
    ImGui::ColorEdit4(controlCaption.c_str(), reinterpret_cast<float *>(&asColor4),
      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoPicker);

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
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
      else
      {
        message_queue.requestDelayedCallback(*this);
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

  String controlCaption;
  E3DCOLOR controlValue = E3DCOLOR(0, 0, 0, 255);
  bool controlEnabled = true;
};

} // namespace PropPanel