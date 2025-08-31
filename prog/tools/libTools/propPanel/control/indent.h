// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class IndentPropertyControl : public PropertyControlBase
{
public:
  IndentPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0))
  {}

  unsigned getTypeMaskForSet() const override { return 0; }
  unsigned getTypeMaskForGet() const override { return 0; }

  void updateImgui() override { ImGui::Dummy(ImVec2(mW, mH)); }
};

} // namespace PropPanel