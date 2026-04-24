// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "controlType.h"
#include <propPanel/control/propertyControlBase.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace PropPanel
{

class ToolbarSeparatorPropertyControl : public PropertyControlBase
{
public:
  ToolbarSeparatorPropertyControl(int id, ControlEventHandler *event_handler, ContainerPropertyControl *parent, int x, int y,
    hdpi::Px w, hdpi::Px h) :
    PropertyControlBase(id, event_handler, parent, x, y, w, h)
  {}

  int getImguiControlType() const override { return (int)ControlType::ToolbarSeparator; }

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }
  unsigned getWidth() const override { return THICKNESS; }

  void updateImgui() override { ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, THICKNESS); }

private:
  static constexpr unsigned THICKNESS = 1;
};

} // namespace PropPanel