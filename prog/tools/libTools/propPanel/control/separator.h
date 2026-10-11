// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <util/dag_simpleString.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class SeparatorPropertyControl : public PropertyControlBase
{
public:
  SeparatorPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[] = "") :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption)
  {}

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  const char *getImguiTypeName() const override { return "Separator"; }

  Point2 getPreferredSize(float max_width_px) const override
  {
    // ImGui::Separator() is one pixel thick.
    if (controlCaption.empty())
      return Point2(0.0f, 1.0f);

    // Same as the item size in ImGui::SeparatorText().
    const ImGuiStyle &style = ImGui::GetStyle();
    const ImVec2 labelSize = ImGui::CalcTextSize(controlCaption.c_str());
    return Point2(min(labelSize.x + style.SeparatorTextPadding.x * 2.0f, max_width_px),
      max(labelSize.y + style.SeparatorTextPadding.y * 2.0f, style.SeparatorTextBorderSize));
  }

  void updateImgui() override
  {
    if (controlCaption.empty())
    {
      ImGui::Separator();
    }
    else
    {
      ImGui::SeparatorText(controlCaption);
    }
  }

protected:
  SimpleString controlCaption;
};

} // namespace PropPanel