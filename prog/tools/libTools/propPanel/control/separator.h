// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <imgui/imgui.h>

namespace PropPanel
{

class SeparatorPropertyControl : public PropertyControlBase
{
public:
  SeparatorPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w) :
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0))
  {}

  virtual unsigned getTypeMaskForSet() const override { return 0; }
  virtual unsigned getTypeMaskForGet() const override { return 0; }

  virtual void updateImgui() override { ImGui::Separator(); }
};

} // namespace PropPanel