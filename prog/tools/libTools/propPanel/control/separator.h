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