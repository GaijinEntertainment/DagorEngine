// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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

  virtual unsigned getTypeMaskForSet() const override { return 0; }
  virtual unsigned getTypeMaskForGet() const override { return 0; }

  virtual void updateImgui() override { ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, 1.0f); }
};

} // namespace PropPanel